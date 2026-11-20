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
        result += urd::memory_usage(m_next[i]) + urd::memory_usage(m_snapshot[i]);
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
    const auto add = [&](Table& table, std::span<const ygg::uint_t> row) { table.insert(Version::CURRENT, row); };
    const auto absent = [&](const Table& table, const Rows&, std::span<const ygg::uint_t> row) { return !table.contains(Version::CURRENT, row); };
    merge(program, add);
    iterate(program, executor, Version::CURRENT, absent, add);
    for (const auto table : info().tables)
        program.tables[table]->clear(Version::FRONTIER);
}

template<typename Add>
bool RecursiveStratum::merge(CompiledProgram& program, const Add& add)
{
    bool any = false;
    for (size_t i = 0; i < info().tables.size(); ++i)
    {
        auto& table = *program.tables[info().tables[i]];
        auto& next = m_next[i];
        table.clear(Version::FRONTIER);
        for (const auto handle : next)
        {
            const auto row = next.at(handle);
            add(table, row);
            table.insert(Version::FRONTIER, row);
        }
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
                if (literal.kind != Literal::Kind::POSITIVE || !info().contains(literal.table) || program.tables[literal.table]->empty(Version::FRONTIER))
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
        deleted += program.tables[t]->size(Version::REMOVED);
        rows += program.tables[t]->size(Version::CURRENT);
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
                if (literal.kind != Literal::Kind::POSITIVE || !info().contains(literal.table) || program.tables[literal.table]->empty(Version::FRONTIER))
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
        table.clear(Version::REMOVED);
        snapshot.clear();
        table.for_each(Version::CURRENT, [&](std::span<const ygg::uint_t> row) { snapshot.insert(row); });
        table.clear(Version::CURRENT);
        table.clear(Version::FRONTIER);
        m_next[i].clear();
    }
    derive(program, executor);
    for (size_t i = 0; i < info().tables.size(); ++i)
    {
        auto& table = *program.tables[info().tables[i]];
        auto& snapshot = m_snapshot[i];
        table.for_each(Version::CURRENT,
                       [&](std::span<const ygg::uint_t> row)
                       {
                           if (!snapshot.contains(row))
                               table.insert(Version::ADDED, row);
                       });
        for (const auto handle : snapshot)
            if (!table.contains(Version::CURRENT, snapshot.at(handle)))
                table.insert(Version::REMOVED, snapshot.at(handle));
        snapshot.clear();
    }
}

void RecursiveStratum::update(CompiledProgram& program, Executor& executor)
{
    // 1. Over-delete: rows with a derivation through a removed row, with the old relations; the stratum's own
    //    relations are still the old ones.
    const auto deletable = [&](const Table& table, const Rows&, std::span<const ygg::uint_t> row)
    { return table.contains(Version::CURRENT, row) && !table.contains(Version::REMOVED, row); };
    const auto deleted = [&](Table& table, std::span<const ygg::uint_t> row) { table.insert(Version::REMOVED, row); };
    seed(program, executor, Version::OLD, [](Literal::Kind kind) { return kind == Literal::Kind::NEGATED ? Version::ADDED : Version::REMOVED; }, deletable);
    merge(program, deleted);
    if (overdeleted_too_many(program) || !iterate_deletions(program, executor, deletable, deleted))
        return recompute(program, executor);
    // 2. Remove them.
    for (const auto t : info().tables)
    {
        auto& table = *program.tables[t];
        table.for_each(Version::REMOVED, [&](std::span<const ygg::uint_t> row) { table.erase(Version::CURRENT, row); });
        table.clear(Version::FRONTIER);
    }
    // 3. Rederive the deleted rows that a rule still derives from the remaining rows.
    for (const auto r : info().rules)
    {
        const auto& rule = program.rules[r];
        auto& head = *program.tables[rule.head];
        auto& next = m_next[info().position(rule.head)];
        const auto& read = reads(rule, Version::CURRENT);
        head.for_each(Version::REMOVED,
                      [&](std::span<const ygg::uint_t> row)
                      {
                          if (!next.contains(row) && executor.derives(rule, rule.rederive, read, row))
                              next.insert(row);
                      });
    }
    // 4. Insert semi-naively from the rederived rows and the additions.
    const auto inserted = [&](Table& table, std::span<const ygg::uint_t> row)
    {
        table.insert(Version::CURRENT, row);
        if (!table.erase(Version::REMOVED, row))
            table.insert(Version::ADDED, row);
    };
    const auto absent = [&](const Table& table, const Rows& next, std::span<const ygg::uint_t> row)
    { return !table.contains(Version::CURRENT, row) && !next.contains(row); };
    for (size_t i = 0; i < info().tables.size(); ++i)
        for (const auto handle : m_next[i])
            program.tables[info().tables[i]]->insert(Version::CURRENT, m_next[i].at(handle));
    for (size_t i = 0; i < info().tables.size(); ++i)
    {
        auto& table = *program.tables[info().tables[i]];
        for (const auto handle : m_next[i])
        {
            const auto row = m_next[i].at(handle);
            table.erase(Version::REMOVED, row);
            table.insert(Version::FRONTIER, row);
        }
        m_next[i].clear();
    }
    seed(program, executor, Version::CURRENT, [](Literal::Kind kind) { return kind == Literal::Kind::NEGATED ? Version::REMOVED : Version::ADDED; }, absent);
    for (size_t i = 0; i < info().tables.size(); ++i)
    {
        auto& table = *program.tables[info().tables[i]];
        for (const auto handle : m_next[i])
        {
            const auto row = m_next[i].at(handle);
            inserted(table, row);
            table.insert(Version::FRONTIER, row);
        }
        m_next[i].clear();
    }
    iterate(program, executor, Version::CURRENT, absent, inserted);
    for (const auto t : info().tables)
        program.tables[t]->clear(Version::FRONTIER);
}
}  // namespace tyr::datalog::database::datalog::detail
