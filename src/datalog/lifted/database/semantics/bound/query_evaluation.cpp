/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "tyr/datalog/lifted/database/semantics/bound/query_evaluation.hpp"
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>

namespace tyr::datalog::database::bound
{
urd::Store<urd::sequence<ConjunctivePlan>> QueryEvaluator::plans(const VariableSet& bound)
{
    auto measured = urd::Store<urd::sequence<std::optional<RelationStatistics>>>(input_count());
    const auto statistics = [&](size_t slot) -> const RelationStatistics&
    {
        if (!measured[slot])
            measured[slot] = measure_relation(input(slot));
        return *measured[slot];
    };
    auto compiled = urd::Store<urd::sequence<ConjunctivePlan>>();
    for (const auto& branch : m_query.branches)
    {
        auto& plan = compiled.emplace_back(plan_branch(m_query, branch, bound, statistics));
        resolve(plan);
    }
    return compiled;
}
}

namespace tyr::datalog::database::bound
{
bool QueryEvaluator::bind_atom(const ConjunctiveAtom& atom, const ygg::RowPool::row_type& row)
{
    for (size_t c = 0; c < atom.vars.size(); ++c)
    {
        const auto value = row.get(c);
        if (static_cast<size_t>(std::ranges::find(atom.vars, atom.vars[c]) - atom.vars.begin()) == c)
            m_frame[atom.vars[c]] = value;
        else if (value != m_frame[atom.vars[c]])
            return false;
    }
    return true;
}
}

namespace tyr::datalog::database::bound
{
size_t QueryEvaluator::index_id(size_t slot, std::span<const std::pair<size_t, uint32_t>> key)
{
    auto positions = urd::Store<urd::sequence<size_t>>();
    for (const auto& [c, var] : key)
        positions.push_back(c);
    for (size_t id = 0; id < m_indexes.size(); ++id)
        if (m_indexes[id].slot == slot && m_indexes[id].positions == positions)
            return id;
    auto& entry = m_indexes.emplace_back();
    entry.slot = slot;
    for (const auto position : positions)
        entry.keys.push_back(ColumnSlice { position });
    entry.positions = std::move(positions);
    return m_indexes.size() - 1;
}
}

namespace tyr::datalog::database::bound
{
void QueryEvaluator::resolve(ConjunctivePlan& plan)
{
    for (auto& step : plan.steps)
    {
        auto& lookup = m_lookups.emplace_back();
        lookup.index = index_id(step.slot, step.key);
        for (const auto& [c, var] : step.key)
            lookup.probe.push_back(ColumnSlice { var });  // the variable's place in the frame
        step.index = m_lookups.size() - 1;
    }
    for (auto& plans : plan.negation_plans)
        for (auto& inner : plans)
            resolve(inner);
}
}

namespace tyr::datalog::database::bound
{
bool QueryEvaluator::accepts(const ConjunctivePlan& plan, const ConjunctiveFilters& filters)
{
    for (const auto& [lhs, rhs] : filters.equal)
        if (m_frame[lhs] != m_frame[rhs])
            return false;
    for (const auto& [var, constant] : filters.value)
        if (m_frame[var] != constant)
            return false;
    // A negation rejects the row if any of its branches matches (an anti-join, its outer variables bound).
    for (const auto i : filters.negations)
        for (const auto& inner : plan.negation_plans[i])
            if (accepts(inner, inner.filters) && !run(inner, 0, [] { return false; }))
                return false;
    return true;
}
}
