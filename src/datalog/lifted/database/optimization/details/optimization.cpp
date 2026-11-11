/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "tyr/datalog/lifted/database/optimization/details/optimization.hpp"

#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>

namespace tyr::datalog::database::detail
{
void validate(std::span<const QueryView<>> roots, const Statistics& statistics)
{
    for (const auto query : reachable(roots))
    {
        if (is<QueryInputTag>(query))
            if (const auto it = statistics.inputs.find(as<QueryInputTag>(query).get_input_slot()); it != statistics.inputs.end())
                validate_statistics(it->second, query.columns());
        if (const auto it = statistics.expressions.find(query.index()); it != statistics.expressions.end())
            validate_statistics(it->second, query.columns());
    }
}
}

namespace tyr::datalog::database::detail
{
QueryPlan optimize(std::span<const QueryView<>> roots, std::span<const Evaluation> evaluations, const Statistics& statistics)
{
    if (evaluations.size() != roots.size())
        throw std::invalid_argument("optimize: one evaluation per root is required.");
    validate(roots, statistics);
    if (roots.empty())
        return {};
    QueryBuilder builder;
    QueryRepository normalized;
    Normalizer normalize(normalized, builder);
    // The equivalent caller query of each normalized query; the first one wins.
    urd::Store<urd::map<urd::index<Query<>>, urd::index<Query<>>>, urd::dense> sources;
    for (const auto query : reachable(roots))
        sources.try_emplace(normalize(query).index(), query.index());
    QueryRepository plans;
    Planner planner(statistics, sources, plans, builder);
    urd::Store<urd::sequence<QueryView<>>> outputs;
    for (size_t i = 0; i < roots.size(); ++i)
    {
        const auto root = roots[i];
        // The bound evaluator orders each binding's joins itself, so a bound root keeps its normalized join structure.
        outputs.push_back(
            evaluations[i] == Evaluation::bound ? copy(normalize(root), [](size_t slot) { return slot; }, plans, builder) : planner.plan(normalize(root)));
        assert(std::ranges::equal(outputs.back().columns(), root.columns()) && "Plans preserve ordered schemas.");
    }
    return compile(std::span<const QueryView<>>(outputs));
}
}

namespace tyr::datalog::database::detail
{
bool Planner::known(QueryView<> plan)
{
    if (const auto it = m_known.find(plan.index()); it != m_known.end())
        return it->second;
    bool result = false;
    if (const auto caller = source(plan); caller && m_statistics->expressions.contains(*caller))
        result = true;
    else if (is<QueryDistanceTag>(plan))
        result = false;
    else if (is<QueryInputTag>(plan))
        result = m_statistics->inputs.contains(as<QueryInputTag>(plan).get_input_slot());
    else
    {
        result = true;
        for_each_child(plan, [&](QueryView<> child) { result = result && known(child); });
    }
    m_known.insert_or_assign(plan.index(), result);
    return result;
}
}

namespace tyr::datalog::database::detail
{
QueryView<> Planner::plan_block(QueryView<> join, std::span<const ygg::ColumnLayout> output)
{
    auto labels = urd::Store<urd::sequence<urd::index<ygg::Column>>>();
    for (const auto& column : output)
        labels.push_back(column.label);
    JoinBlock block { {}, ygg::canonicalized(labels) };
    urd::Store<urd::set<QueryIndex>> seen;
    collect(join, block.atoms, seen);
    if (block.atoms.empty())  // The empty join is the nullary relation with one row.
        return m_build.generic_join(block.atoms, {});
    const bool measured = block.atoms.size() <= max_cost_based_atoms && std::ranges::all_of(block.atoms, [&](auto atom) { return known(atom); });
    const auto result = block.atoms.size() == 1 ? block.atoms.front() :
                        measured                ? CostBasedPlanner(block, m_build, [&](QueryView<> atom) { return estimate(atom); }).plan() :
                                                  structural_plan(block, m_build);
    return m_build.project(result, labels);
}
}

namespace tyr::datalog::database::detail
{
QueryView<> Planner::plan(QueryView<> normalized)
{
    if (const auto it = m_planned.find(normalized.index()); it != m_planned.end())
        return it->second;
    QueryView<> result = [&]
    {
        if (joins(normalized))
            return plan_block(normalized, normalized.columns());
        if (is<QueryProjectTag>(normalized))
        {
            if (const auto projection = as<QueryProjectTag>(normalized); joins(projection.get_arg()))
                return plan_block(projection.get_arg(), normalized.columns());
        }
        urd::Store<urd::sequence<QueryView<>>> children;
        for_each_child(normalized, [&](QueryView<> child) { children.push_back(plan(child)); });
        return m_build.clone(normalized, children);
    }();
    if (const auto caller = m_normalized_sources->find(normalized.index()); caller != m_normalized_sources->end())
        m_sources.try_emplace(result.index(), caller->second);
    m_planned.insert_or_assign(normalized.index(), result);
    return result;
}
}
