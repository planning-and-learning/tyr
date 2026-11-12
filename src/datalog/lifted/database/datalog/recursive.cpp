/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "tyr/datalog/lifted/database/datalog/detail/executor.hpp"
#include "tyr/datalog/lifted/database/datalog/detail/strata.hpp"

#include <algorithm>

namespace tyr::datalog::database::datalog::detail
{
namespace
{
/// Past this fraction of the stratum's rows over-deleted, rederiving costs more than evaluating the stratum anew.
constexpr double recompute_fraction = 0.1;
}  // namespace

RecursiveStratum::RecursiveStratum(StratumInfo stratum, const CompiledProgram& /*program*/) : Stratum(std::move(stratum))
{
    m_next.resize(info().tables.size());
    m_snapshot.resize(info().tables.size());
}

void RecursiveStratum::evaluate(CompiledProgram& program, Executor& executor)
{
    for (const auto table : info().tables)
        program.tables[table]->clear();
    derive(program, executor);
}

size_t RecursiveStratum::state_memory_usage() const noexcept
{
    size_t result = 0;
    for (size_t i = 0; i < m_next.size(); ++i)
        result += m_next[i].memory_usage() + urd::memory_usage(m_snapshot[i]);
    return result;
}

/// The rules that read no relation of the stratum, then semi-naively.
void RecursiveStratum::derive(CompiledProgram& program, Executor& executor)
{
    for (const auto r : info().rules)
    {
        const auto& rule = program.rules[r];
        auto& next = m_next[info().position(rule.head)];
        if (std::ranges::any_of(rule.literals, [&](const Literal& l) { return l.kind == Literal::Kind::POSITIVE && info().contains(l.table); }))
            continue;
        executor.run(rule, rule.plain, reads(rule, Version::CURRENT), [&](std::span<const ygg::uint_t> row) { next.insert(row); });
    }
    const auto add = [&](Table& table, std::span<const ygg::uint_t> row) { table.current.insert(row); };
    const auto absent = [&](const Table& table, const IndexedTuples&, std::span<const ygg::uint_t> row) { return !table.current.contains(row); };
    merge(program, add);
    iterate(program, executor, Version::CURRENT, absent, add);
    for (const auto table : info().tables)
        program.tables[table]->frontier.clear();
}

template<typename Add>
bool RecursiveStratum::merge(CompiledProgram& program, const Add& add)
{
    bool any = false;
    for (size_t i = 0; i < info().tables.size(); ++i)
    {
        auto& table = *program.tables[info().tables[i]];
        auto& next = m_next[i];
        table.frontier.clear();
        next.for_each(
            [&](std::span<const ygg::uint_t> row)
            {
                add(table, row);
                table.frontier.insert(row);
            });
        any = any || !next.empty();
        next.clear();
    }
    return any;
}

template<typename Keep, typename Add>
void RecursiveStratum::iterate(CompiledProgram& program, Executor& executor, Version others, const Keep& keep, const Add& add)
{
    while (true)
    {
        for (const auto r : info().rules)
        {
            const auto& rule = program.rules[r];
            auto& head = *program.tables[rule.head];
            auto& next = m_next[info().position(rule.head)];
            for (size_t j = 0; j < rule.literals.size(); ++j)
            {
                const auto& literal = rule.literals[j];
                if (literal.kind != Literal::Kind::POSITIVE || !info().contains(literal.table) || program.tables[literal.table]->frontier.empty())
                    continue;
                auto& read = reads(rule, others);
                read[j] = Version::FRONTIER;
                executor.run(rule,
                             rule.by_source[j],
                             read,
                             [&](std::span<const ygg::uint_t> row)
                             {
                                 if (keep(head, next, row))
                                     next.insert(row);
                             });
            }
        }
        if (!merge(program, add))
            return;
    }
}

template<typename Change, typename Keep>
void RecursiveStratum::seed(CompiledProgram& program, Executor& executor, Version others, const Change& change, const Keep& keep)
{
    for (const auto r : info().rules)
    {
        const auto& rule = program.rules[r];
        auto& head = *program.tables[rule.head];
        auto& next = m_next[info().position(rule.head)];
        for (size_t j = 0; j < rule.literals.size(); ++j)
        {
            const auto& literal = rule.literals[j];
            if (literal.table == npos || info().contains(literal.table) || !program.tables[literal.table]->changed())
                continue;
            auto& read = reads(rule, others);
            for (size_t i = 0; i < rule.literals.size(); ++i)
                if (rule.literals[i].table != npos && info().contains(rule.literals[i].table))
                    read[i] = Version::CURRENT;
            read[j] = change(literal.kind);
            executor.run(rule,
                         rule.by_source[j],
                         read,
                         [&](std::span<const ygg::uint_t> row)
                         {
                             if (keep(head, next, row))
                                 next.insert(row);
                         });
        }
    }
}

bool RecursiveStratum::overdeleted_too_many(const CompiledProgram& program) const
{
    size_t deleted = 0, rows = 0;
    for (const auto t : info().tables)
    {
        deleted += program.tables[t]->removed.size();
        rows += program.tables[t]->current.size();
    }
    return static_cast<double>(deleted) > recompute_fraction * static_cast<double>(rows);
}

template<typename Keep, typename Add>
bool RecursiveStratum::iterate_deletions(CompiledProgram& program, Executor& executor, const Keep& keep, const Add& add)
{
    while (true)
    {
        for (const auto r : info().rules)
        {
            const auto& rule = program.rules[r];
            auto& head = *program.tables[rule.head];
            auto& next = m_next[info().position(rule.head)];
            for (size_t j = 0; j < rule.literals.size(); ++j)
            {
                const auto& literal = rule.literals[j];
                if (literal.kind != Literal::Kind::POSITIVE || !info().contains(literal.table) || program.tables[literal.table]->frontier.empty())
                    continue;
                auto& read = reads(rule, Version::OLD);
                for (size_t i = 0; i < rule.literals.size(); ++i)
                    if (rule.literals[i].table != npos && info().contains(rule.literals[i].table))
                        read[i] = Version::CURRENT;
                read[j] = Version::FRONTIER;
                executor.run(rule,
                             rule.by_source[j],
                             read,
                             [&](std::span<const ygg::uint_t> row)
                             {
                                 if (keep(head, next, row))
                                     next.insert(row);
                             });
            }
        }
        if (!merge(program, add))
            return true;
        if (overdeleted_too_many(program))
            return false;
    }
}

void RecursiveStratum::recompute(CompiledProgram& program, Executor& executor)
{
    for (size_t i = 0; i < info().tables.size(); ++i)
    {
        auto& table = *program.tables[info().tables[i]];
        auto& snapshot = m_snapshot[i];
        table.removed.clear();
        snapshot.clear();
        table.current.for_each([&](std::span<const ygg::uint_t> row) { snapshot.insert(row); });
        table.current.clear();
        table.frontier.clear();
        m_next[i].clear();
    }
    derive(program, executor);
    for (size_t i = 0; i < info().tables.size(); ++i)
    {
        auto& table = *program.tables[info().tables[i]];
        auto& snapshot = m_snapshot[i];
        table.current.for_each(
            [&](std::span<const ygg::uint_t> row)
            {
                if (!snapshot.contains(row))
                    table.added.insert(row);
            });
        for (const auto handle : row_handles(snapshot))
            if (!table.current.contains(snapshot.at(handle)))
                table.removed.insert(snapshot.at(handle));
        snapshot.clear();
    }
}

void RecursiveStratum::update(CompiledProgram& program, Executor& executor)
{
    // 1. Over-delete: rows with a derivation through a removed row, with the old relations; the stratum's own
    //    relations are still the old ones.
    const auto deletable = [&](const Table& table, const IndexedTuples&, std::span<const ygg::uint_t> row)
    { return table.current.contains(row) && !table.removed.contains(row); };
    const auto deleted = [&](Table& table, std::span<const ygg::uint_t> row) { table.removed.insert(row); };
    seed(program, executor, Version::OLD, [](Literal::Kind kind) { return kind == Literal::Kind::NEGATED ? Version::ADDED : Version::REMOVED; }, deletable);
    merge(program, deleted);
    if (overdeleted_too_many(program) || !iterate_deletions(program, executor, deletable, deleted))
        return recompute(program, executor);
    // 2. Remove them.
    for (const auto t : info().tables)
    {
        auto& table = *program.tables[t];
        table.removed.for_each([&](std::span<const ygg::uint_t> row) { table.current.erase(row); });
        table.frontier.clear();
    }
    // 3. Rederive the deleted rows that a rule still derives from the remaining rows.
    for (const auto r : info().rules)
    {
        const auto& rule = program.rules[r];
        auto& head = *program.tables[rule.head];
        auto& next = m_next[info().position(rule.head)];
        const auto& read = reads(rule, Version::CURRENT);
        head.removed.for_each(
            [&](std::span<const ygg::uint_t> row)
            {
                if (!next.contains(row) && executor.derives(rule, rule.rederive, read, row))
                    next.insert(row);
            });
    }
    // 4. Insert semi-naively from the rederived rows and the additions.
    const auto inserted = [&](Table& table, std::span<const ygg::uint_t> row)
    {
        table.current.insert(row);
        if (!table.removed.erase(row))
            table.added.insert(row);
    };
    const auto absent = [&](const Table& table, const IndexedTuples& next, std::span<const ygg::uint_t> row)
    { return !table.current.contains(row) && !next.contains(row); };
    for (size_t i = 0; i < info().tables.size(); ++i)
        m_next[i].for_each([&](std::span<const ygg::uint_t> row) { program.tables[info().tables[i]]->current.insert(row); });
    for (size_t i = 0; i < info().tables.size(); ++i)
    {
        auto& table = *program.tables[info().tables[i]];
        m_next[i].for_each(
            [&](std::span<const ygg::uint_t> row)
            {
                table.removed.erase(row);
                table.frontier.insert(row);
            });
        m_next[i].clear();
    }
    seed(program, executor, Version::CURRENT, [](Literal::Kind kind) { return kind == Literal::Kind::NEGATED ? Version::REMOVED : Version::ADDED; }, absent);
    for (size_t i = 0; i < info().tables.size(); ++i)
    {
        auto& table = *program.tables[info().tables[i]];
        m_next[i].for_each(
            [&](std::span<const ygg::uint_t> row)
            {
                inserted(table, row);
                table.frontier.insert(row);
            });
        m_next[i].clear();
    }
    iterate(program, executor, Version::CURRENT, absent, inserted);
    for (const auto t : info().tables)
        program.tables[t]->frontier.clear();
}
}  // namespace tyr::datalog::database::datalog::detail
