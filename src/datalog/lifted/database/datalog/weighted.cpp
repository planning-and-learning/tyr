/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "tyr/datalog/lifted/database/datalog/detail/executor.hpp"
#include "tyr/datalog/lifted/database/datalog/detail/goal_search.hpp"
#include "tyr/datalog/lifted/database/datalog/detail/strata.hpp"

#include <algorithm>
#include <functional>
#include <urd/memory.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>

namespace tyr::datalog::database::datalog::detail
{
WeightedStratum::WeightedStratum(StratumInfo stratum,
                                 const CompiledProgram& program,
                                 urd::Store<urd::sequence<size_t>> goals,
                                 std::unique_ptr<GoalHeuristic> heuristic,
                                 std::unique_ptr<GoalFrontier> frontier,
                                 std::unique_ptr<UnitSearch> unit,
                                 urd::Store<urd::sequence<size_t>> target_tables) :
    Stratum(std::move(stratum)),
    m_goals(std::move(goals)),
    m_holds(m_goals.size()),
    m_heuristic(std::move(heuristic)),
    m_frontier(std::move(frontier)),
    m_unit(std::move(unit)),
    m_target_tables(std::move(target_tables))
{
    for (const auto table : info().tables)
    {
        const auto arity = program.tables[table]->arity;
        m_settled.emplace_back(arity - 1);
        m_queued.emplace_back(arity - 1);
        m_best.emplace_back();
        m_snapshot.emplace_back(arity);
    }
    for (const auto g : m_goals)
    {
        const auto& rule = program.rules[g];
        const auto& read =
            *std::ranges::find_if(rule.literals, [&](const Literal& l) { return l.kind == Literal::Kind::POSITIVE && info().contains(l.table); });
        auto columns = urd::Store<urd::sequence<size_t>>();
        for (size_t i = 0; i + 1 < rule.head_terms.size(); ++i)
            columns.push_back(static_cast<size_t>(std::ranges::find(read.terms, rule.head_terms[i]) - read.terms.begin()));
        const auto key_arity = std::max<size_t>(columns.size(), 1);
        m_goal_keys.push_back(GoalKeys { info().position(read.table), std::move(columns), RowSet(key_arity) });
    }
}

WeightedStratum::~WeightedStratum() = default;

void WeightedStratum::offer(size_t position, std::span<const ygg::uint_t> row)
{
    const auto key = row.first(row.size() - 1);
    const auto weight = row.back();
    if (m_settled[position].contains(key) || finished(position, row) || (m_frontier && m_frontier->excluded(key)))
        return;
    const auto estimate = m_heuristic ? (*m_heuristic)(key) : 0;
    if (estimate == Arithmetic::infinity)
        return;
    auto& queued = m_queued[position];
    auto& best = m_best[position];
    const auto size = queued.size();
    const auto index = queued.insert(key);
    if (index == size)
        best.push_back(weight);
    else if (weight >= best[index])
        return;
    else
        best[index] = weight;
    m_queue.push_back(Candidate { Arithmetic { Arithmetic::Operation::ADD, {}, {}, 0 }.apply(weight, estimate),
                                  position,
                                  urd::Store<urd::sequence<ygg::uint_t>>(row.begin(), row.end()) });
    std::ranges::push_heap(m_queue, std::greater {});
}

void WeightedStratum::offer_derived()
{
    for (const auto& [position, row] : m_derived)
        offer(position, row);
    m_derived.clear();
}

bool WeightedStratum::goals_hold(CompiledProgram& program, Executor& executor, size_t table)
{
    for (size_t g = 0; g < m_goals.size(); ++g)
    {
        if (m_holds[g])
            continue;
        auto& keys = m_goal_keys[g];
        const auto& rule = program.rules[m_goals[g]];
        for (size_t j = 0; j < rule.literals.size() && !m_holds[g]; ++j)
        {
            if (rule.literals[j].table != table)
                continue;
            auto& read = reads(rule, Version::CURRENT);
            read[j] = Version::FRONTIER;
            executor.run(rule,
                         rule.by_source[j],
                         read,
                         [&](std::span<const ygg::uint_t> row)
                         {
                             if (keys.columns.empty())
                                 m_holds[g] = true;
                             else
                                 keys.done.insert(row.first(row.size() - 1));
                         });
        }
    }
    return std::ranges::all_of(m_goal_keys, [](const GoalKeys& keys) { return keys.columns.empty(); })
           && std::ranges::all_of(m_holds, [](bool holds) { return holds; });
}

bool WeightedStratum::finished(size_t position, std::span<const ygg::uint_t> row)
{
    auto read = false;
    for (const auto& keys : m_goal_keys)
    {
        if (keys.position != position)
            continue;
        if (keys.columns.empty())
            return false;
        m_key.clear();
        for (const auto column : keys.columns)
            m_key.push_back(row[column]);
        if (!keys.done.contains(m_key))
            return false;
        read = true;
    }
    return read;
}

void WeightedStratum::derive(CompiledProgram& program, Executor& executor, bool fresh)
{
    const auto targets_changed = fresh || std::ranges::any_of(m_target_tables, [&](size_t t) { return program.tables[t]->changed(); });
    m_holds.assign(m_goals.size(), false);
    for (auto& keys : m_goal_keys)
        keys.done.clear();
    for (size_t i = 0; i < info().tables.size(); ++i)
    {
        program.tables[info().tables[i]]->clear();
        m_settled[i].clear();
        m_queued[i].clear();
        m_best[i].clear();
    }
    m_queue.clear();
    if (m_heuristic)
        m_heuristic->read_targets(program, executor, targets_changed);
    if (m_unit)
    {
        // The base rules' rows are the searches' sources.
        auto sources = urd::Store<urd::sequence<urd::Store<urd::sequence<ygg::uint_t>>>>();
        for (const auto r : info().rules)
        {
            const auto& rule = program.rules[r];
            if (std::ranges::any_of(rule.literals, [&](const Literal& l) { return l.kind == Literal::Kind::POSITIVE && info().contains(l.table); }))
                continue;
            executor.run(rule,
                         rule.plain,
                         reads(rule, Version::CURRENT),
                         [&](std::span<const ygg::uint_t> row) { sources.emplace_back(row.begin(), row.end()); });
        }
        m_unit->run(program, executor, sources, m_heuristic.get(), targets_changed);
        return;
    }
    if (m_frontier)
        m_frontier->read_targets(executor);
    // Rules that read no relation of the stratum start the queue.
    for (const auto r : info().rules)
    {
        const auto& rule = program.rules[r];
        if (std::ranges::any_of(rule.literals, [&](const Literal& l) { return l.kind == Literal::Kind::POSITIVE && info().contains(l.table); }))
            continue;
        const auto position = info().position(rule.head);
        executor.run(rule,
                     rule.plain,
                     reads(rule, Version::CURRENT),
                     [&](std::span<const ygg::uint_t> row)
                     { m_derived.emplace_back(position, urd::Store<urd::sequence<ygg::uint_t>>(row.begin(), row.end())); });
        if (m_frontier)
            for (const auto& [_, row] : m_derived)
                m_frontier->add_source(std::span<const ygg::uint_t>(row).first(row.size() - 1));
        offer_derived();
    }
    // Settle rows in order of weight; a settled row is expanded with the rows settled before it.
    while (!m_queue.empty())
    {
        std::ranges::pop_heap(m_queue, std::greater {});
        auto candidate = std::move(m_queue.back());
        m_queue.pop_back();
        const auto row = std::span<const ygg::uint_t>(candidate.row);
        const auto key = row.first(row.size() - 1);
        auto& settled = m_settled[candidate.position];
        // A lighter derivation of the row was queued after this one, or settled. A queued row has a best weight.
        const auto queued = m_queued[candidate.position].find(key);
        if (settled.contains(key) || !queued || row.back() != m_best[candidate.position][*queued] || finished(candidate.position, row)
            || (m_frontier && m_frontier->excluded(key)))
            continue;
        settled.insert(key);
        const auto table_index = info().tables[candidate.position];
        auto& table = *program.tables[table_index];
        table.current.insert(row);
        table.frontier.clear();
        table.frontier.insert(row);
        for (const auto r : info().rules)
        {
            const auto& rule = program.rules[r];
            const auto position = info().position(rule.head);
            for (size_t j = 0; j < rule.literals.size(); ++j)
            {
                if (rule.literals[j].kind != Literal::Kind::POSITIVE || rule.literals[j].table != table_index)
                    continue;
                auto& read = reads(rule, Version::CURRENT);
                read[j] = Version::FRONTIER;
                executor.run(rule,
                             rule.by_source[j],
                             read,
                             [&](std::span<const ygg::uint_t> derived)
                             { m_derived.emplace_back(position, urd::Store<urd::sequence<ygg::uint_t>>(derived.begin(), derived.end())); });
            }
        }
        offer_derived();
        const bool done = !m_goals.empty() && goals_hold(program, executor, table_index);
        table.frontier.clear();
        if (done)
            break;
        // The other side advances as this one does, so that the smaller is exhausted first.
        if (m_frontier)
            m_frontier->advance(executor, key);
    }
    m_queue.clear();
}

void WeightedStratum::evaluate(CompiledProgram& program, Executor& executor)
{
    if (m_unit)
        m_unit->reset();
    derive(program, executor, true);
}

void WeightedStratum::update(CompiledProgram& program, Executor& executor)
{
    if (m_unit)
        m_unit->changed(program, executor);
    for (size_t i = 0; i < info().tables.size(); ++i)
    {
        auto& snapshot = m_snapshot[i];
        snapshot.clear();
        program.tables[info().tables[i]]->current.for_each([&](std::span<const ygg::uint_t> row) { snapshot.insert(row); });
    }
    derive(program, executor, false);
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
        for (ygg::uint_t r = 0; r < snapshot.size(); ++r)
            if (!table.current.contains(snapshot[r]))
                table.removed.insert(snapshot[r]);
        snapshot.clear();
    }
}

size_t WeightedStratum::state_memory_usage() const noexcept
{
    size_t result = urd::memory_usage(m_queue) + (m_heuristic ? m_heuristic->memory_usage() : 0) + (m_frontier ? m_frontier->memory_usage() : 0)
                    + (m_unit ? m_unit->memory_usage() : 0);
    for (size_t i = 0; i < m_settled.size(); ++i)
        result += m_settled[i].memory_usage() + m_queued[i].memory_usage() + urd::memory_usage(m_best[i]) + m_snapshot[i].memory_usage();
    for (const auto& keys : m_goal_keys)
        result += keys.done.memory_usage();
    return result;
}
}  // namespace tyr::datalog::database::datalog::detail
