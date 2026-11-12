/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "tyr/datalog/lifted/database/datalog/detail/executor.hpp"
#include "tyr/datalog/lifted/database/datalog/detail/strata.hpp"

#include <stdexcept>

namespace tyr::datalog::database::datalog::detail
{
PlainStratum::PlainStratum(StratumInfo stratum, const CompiledProgram& /*program*/) : Stratum(std::move(stratum)) { m_counts.resize(info().tables.size()); }

void PlainStratum::evaluate(CompiledProgram& program, Executor& executor) { derive(program, executor, false); }

void PlainStratum::initialize(CompiledProgram& program, Executor& executor) { derive(program, executor, true); }

void PlainStratum::derive(CompiledProgram& program, Executor& executor, bool counting)
{
    for (size_t i = 0; i < info().tables.size(); ++i)
    {
        program.tables[info().tables[i]]->clear();
        m_counts[i].rows.clear();
        m_counts[i].counts.clear();
    }
    for (const auto r : info().rules)
    {
        const auto& rule = program.rules[r];
        auto& head = *program.tables[rule.head];
        executor.run(rule,
                     rule.plain,
                     reads(rule, Version::CURRENT),
                     [&](std::span<const ygg::uint_t> row)
                     {
                         if (counting)
                             count(rule.head, row, 1);
                         else
                             head.insert(Version::CURRENT, row);
                     });
        if (counting)
            apply_counts(head, m_counts[info().position(rule.head)], false);
    }
}

/// The rule once per changed literal, which reads its added (or removed) rows, earlier literals the new, later ones the
/// old relations; a negated literal's additions remove derivations.
void PlainStratum::update(CompiledProgram& program, Executor& executor)
{
    for (const auto r : info().rules)
    {
        const auto& rule = program.rules[r];
        for (size_t j = 0; j < rule.literals.size(); ++j)
        {
            const auto& literal = rule.literals[j];
            if (literal.table == npos || !program.tables[literal.table]->changed())
                continue;
            for (const auto change : { Version::ADDED, Version::REMOVED })
            {
                const bool positive = literal.kind != Literal::Kind::NEGATED;
                const int64_t sign = (change == Version::ADDED) == positive ? 1 : -1;
                auto& read = reads(rule, Version::CURRENT);
                for (size_t i = j + 1; i < rule.literals.size(); ++i)
                    read[i] = Version::OLD;
                read[j] = change;
                executor.run(rule, rule.by_source[j], read, [&](std::span<const ygg::uint_t> row) { count(rule.head, row, sign); });
            }
        }
    }
    for (size_t i = 0; i < info().tables.size(); ++i)
        apply_counts(*program.tables[info().tables[i]], m_counts[i], true);
}

size_t PlainStratum::state_memory_usage() const noexcept
{
    size_t result = 0;
    for (const auto& counts : m_counts)
        result +=
            urd::memory_usage(counts.rows) + urd::memory_usage(counts.pending_rows) + urd::memory_usage(counts.counts) + urd::memory_usage(counts.pending);
    return result;
}

void PlainStratum::count(size_t table, std::span<const ygg::uint_t> row, int64_t change)
{
    auto& counts = m_counts[info().position(table)];
    const auto [handle, inserted] = counts.pending_rows.insert(row);
    if (inserted)
        counts.pending.push_back(0);
    counts.pending[handle] += change;
}

void PlainStratum::apply_counts(Table& table, Counts& counts, bool record)
{
    for (const auto pending : counts.pending_rows)
    {
        const auto change = counts.pending[pending];
        if (change == 0)
            continue;
        const auto row = counts.pending_rows.at(pending);
        const auto handle = counts.rows.find(row);
        const auto before = handle.is_none() ? 0 : counts.counts[handle];
        const auto after = before + change;
        if (after < 0)
            throw std::logic_error("Datalog evaluator: a derivation count became negative.");
        if (!handle.is_none() && after == 0)
        {
            // The last row moves into the erased row's handle, and so does its count.
            if (const auto moved = counts.rows.erase(handle); !moved.is_none())
                counts.counts[handle] = counts.counts[moved];
            counts.counts.pop_back();
        }
        else if (!handle.is_none())
            counts.counts[handle] = after;
        else
        {
            counts.rows.insert(row);
            counts.counts.push_back(after);
        }
        if (before == 0 && after > 0)
        {
            table.insert(Version::CURRENT, row);
            if (record)
                table.insert(Version::ADDED, row);
        }
        else if (before > 0 && after == 0)
        {
            table.erase(Version::CURRENT, row);
            if (record)
                table.insert(Version::REMOVED, row);
        }
    }
    counts.pending_rows.clear();
    counts.pending.clear();
}
}  // namespace tyr::datalog::database::datalog::detail
