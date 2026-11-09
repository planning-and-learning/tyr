/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef TYR_DATALOG_DATABASE_SEMANTICS_BOUND_CONJUNCTIVE_PLAN_HPP_
#define TYR_DATALOG_DATABASE_SEMANTICS_BOUND_CONJUNCTIVE_PLAN_HPP_

#include "tyr/datalog/lifted/database/optimization/statistics.hpp"
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include "tyr/datalog/lifted/database/semantics/bound/union_normal_form.hpp"

#include <algorithm>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <utility>
#include <vector>

namespace tyr::datalog::database::bound
{

/// Filters of a branch that become checkable at one point of a plan.
struct ConjunctiveFilters
{
    urd::Store<urd::sequence<std::pair<uint32_t, uint32_t>>> equal;                ///< Variables that must hold equal values.
    urd::Store<urd::sequence<std::pair<uint32_t, ygg::Value>>> value;  ///< Variable, the constant it must hold.
    urd::Store<urd::sequence<size_t>> negations;                                   ///< Indices into the plan's negation_plans.
};

/// One atom of a conjunctive plan: its input slot, the columns bound on arrival (its lookup key), the variables it
/// binds, and the filters that become checkable after it.
struct ConjunctiveStep
{
    static constexpr size_t unresolved = std::numeric_limits<size_t>::max();

    size_t slot = 0;
    urd::Store<urd::sequence<std::pair<size_t, uint32_t>>> key;      ///< Column, variable bound on arrival, in column order.
    urd::Store<urd::sequence<std::pair<size_t, uint32_t>>> assign;   ///< Column, variable bound here.
    urd::Store<urd::sequence<std::pair<size_t, uint32_t>>> compare;  ///< Column, variable bound earlier in this row.
    ConjunctiveFilters filters;
    bool exists = false;        ///< No later step, filter or output reads its variables: one matching row suffices.
    size_t index = unresolved;  ///< The executor's lookup structure for (slot, key columns); set by the executor.
};

/// A branch's atoms in evaluation order for one set of initially bound variables. Filters whose variables are bound
/// from the start are checked before the first step; negations carry plans of their own, their outer variables bound.
/// A plan is self-contained: executing it needs no branch, only the evaluator that resolved it.
struct ConjunctivePlan
{
    ConjunctiveFilters filters;  ///< Checkable before the first step.
    urd::Store<urd::sequence<ConjunctiveStep>> steps;
    std::vector<std::vector<ConjunctivePlan>> negation_plans;  ///< Per negation, per branch. std::vector: of an incomplete type (a Store resolves its element type at once).
};

/// A greedy left-deep order for index nested-loop evaluation: next the atom with the smallest expected fanout, using
/// System R's uniform estimate that divides an input's rows by the distinct values of each bound column (Selinger et
/// al., "Access Path Selection in a Relational Database Management System", SIGMOD 1979). Greedy instead of System R's
/// dynamic programming over left-deep orders: branches are small and planned once per set of bound columns. Each
/// equality, constant and negation attaches to the first step after which its variables are bound. A step whose
/// variables nothing after it reads is an existence test, so the rest of the plan runs for its first matching row
/// only; projected-away subqueries thus cost one match instead of all of them. This is the semijoin form of a join
/// whose other columns are projected away, π_X(R ⋈ S) = R ⋉ S (Bernstein & Chiu, "Using Semi-Joins to Solve
/// Relational Queries", JACM 1981), decided per step as the existence checks of compiled Datalog rules are (Scholz,
/// Jordan, Subotić & Westmann, "On Fast Large-Scale Program Analysis in Datalog", CC 2016). A step that also binds a
/// variable read later is not one, so it may still emit duplicates; binding the output variables before the
/// existential ones avoids them (Bagan, Durand & Grandjean, "On Acyclic Conjunctive Queries and Constant Delay
/// Enumeration", CSL 2007). `statistics(slot)` measures an input.
template<typename Statistics>
    requires std::invocable<Statistics&, size_t> && std::convertible_to<std::invoke_result_t<Statistics&, size_t>, const RelationStatistics&>
ConjunctivePlan plan_branch(const UnionNormalForm& query, const ConjunctiveBranch& branch, std::vector<bool> bound, Statistics statistics)
{
    const auto fanout = [&](const ConjunctiveAtom& atom)
    {
        const RelationStatistics& measured = statistics(atom.slot);
        auto estimate = measured.rows;
        for (size_t c = 0; c < atom.vars.size(); ++c)
            if (bound[atom.vars[c]] && estimate > 0)
                estimate /= measured.distinct.at(query.input_columns[atom.slot][c].label);
        return estimate;
    };
    auto plan = ConjunctivePlan {};
    auto pending_equal = std::vector<bool>(branch.equal.size(), true);
    auto pending_value = std::vector<bool>(branch.value.size(), true);
    auto pending_negation = std::vector<bool>(branch.negations.size(), true);
    const auto attach = [&](ConjunctiveFilters& filters)
    {
        for (size_t i = 0; i < branch.equal.size(); ++i)
            if (pending_equal[i] && bound[branch.equal[i].first] && bound[branch.equal[i].second])
            {
                pending_equal[i] = false;
                filters.equal.push_back(branch.equal[i]);
            }
        for (size_t i = 0; i < branch.value.size(); ++i)
            if (pending_value[i] && bound[branch.value[i].first])
            {
                pending_value[i] = false;
                filters.value.push_back(branch.value[i]);
            }
        for (size_t i = 0; i < branch.negations.size(); ++i)
            if (pending_negation[i] && std::ranges::all_of(branch.negations[i].outer, [&](uint32_t var) { return static_cast<bool>(bound[var]); }))
            {
                pending_negation[i] = false;
                filters.negations.push_back(i);
            }
    };
    attach(plan.filters);
    auto remaining = urd::Store<urd::sequence<size_t>>(branch.atoms.size());
    for (size_t i = 0; i < remaining.size(); ++i)
        remaining[i] = i;
    while (!remaining.empty())
    {
        const auto best = std::ranges::min_element(remaining, {}, [&](size_t atom) { return fanout(branch.atoms[atom]); });
        const auto& atom = branch.atoms[*best];
        auto& step = plan.steps.emplace_back();
        step.slot = atom.slot;
        auto assigned = urd::Store<urd::sequence<uint32_t>>();
        for (size_t c = 0; c < atom.vars.size(); ++c)
        {
            const auto var = atom.vars[c];
            if (bound[var])
                step.key.emplace_back(c, var);
            else if (std::ranges::find(assigned, var) != assigned.end())
                step.compare.emplace_back(c, var);
            else
            {
                step.assign.emplace_back(c, var);
                assigned.push_back(var);
            }
        }
        for (const auto var : assigned)
            bound[var] = true;
        remaining.erase(best);
        attach(step.filters);
    }
    // Every variable of a filter or a negation's outer variable is bound by some atom of the branch (safe queries).
    assert(std::ranges::find(pending_equal, true) == pending_equal.end() && std::ranges::find(pending_value, true) == pending_value.end()
           && std::ranges::find(pending_negation, true) == pending_negation.end());
    auto read = std::vector<bool>(query.variables.size(), false);
    for (const auto var : query.output)
        read[var] = true;
    for (auto s = plan.steps.size(); s-- > 0;)
    {
        auto& step = plan.steps[s];
        step.exists =
            std::ranges::none_of(step.assign, [&](const std::pair<size_t, uint32_t>& assignment) { return static_cast<bool>(read[assignment.second]); });
        for (const auto& pairs : { &step.key, &step.assign, &step.compare })
            for (const auto& [c, var] : *pairs)
                read[var] = true;
        for (const auto& [lhs, rhs] : step.filters.equal)
            read[lhs] = read[rhs] = true;
        for (const auto& [var, constant] : step.filters.value)
            read[var] = true;
        for (const auto i : step.filters.negations)
            for (const auto var : branch.negations[i].outer)
                read[var] = true;
    }
    for (const auto& negation : branch.negations)
    {
        auto outer = std::vector<bool>(query.variables.size(), false);
        for (const auto var : negation.outer)
            outer[var] = true;
        auto& plans = plan.negation_plans.emplace_back();
        for (const auto& inner : negation.branches)
            plans.push_back(plan_branch(query, inner, outer, statistics));
    }
    return plan;
}

}  // namespace tyr::datalog::database::bound

#endif
