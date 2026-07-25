/*
 * Copyright (C) 2025-2026 Dominik Drexler
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include "tyr/datalog/policies/termination.hpp"

#include "tyr/datalog/ground/policies/numeric_support.hpp"
#include "tyr/datalog/lifted/applicability.hpp"
#include "tyr/datalog/lifted/policies/numeric_support.hpp"

#include <cassert>
#include <limits>

namespace tyr::datalog
{

static_assert(TerminationPolicyConcept<NoTerminationPolicy<GroundTag>, GroundTag>);
static_assert(TerminationPolicyConcept<NoTerminationPolicy<LiftedTag>, LiftedTag>);
static_assert(TerminationPolicyConcept<TerminationPolicy<GroundTag, SumAggregation>, GroundTag>);
static_assert(TerminationPolicyConcept<TerminationPolicy<GroundTag, MaxAggregation>, GroundTag>);
static_assert(TerminationPolicyConcept<TerminationPolicy<LiftedTag, SumAggregation>, LiftedTag>);
static_assert(TerminationPolicyConcept<TerminationPolicy<LiftedTag, MaxAggregation>, LiftedTag>);

template<TaskKind Kind, typename AggregationFunction>
void TerminationPolicy<Kind, AggregationFunction>::set_goals(::tyr::formalism::datalog::GroundConjunctiveConditionView goals_)
{
    clear();
    goals = goals_;
}

template<TaskKind Kind, typename AggregationFunction>
bool TerminationPolicy<Kind, AggregationFunction>::check(const FactSets& fact_sets) const noexcept
{
    if (!goals)
        return false;

    return is_applicable(*goals, fact_sets);
}

template<TaskKind Kind, typename AggregationFunction>
Cost TerminationPolicy<Kind, AggregationFunction>::get_total_cost(const FactSets&,
                                                                  const SelectedPredicateAnnotations<Kind>& and_annot,
                                                                  const SelectedFunctionAnnotations<Kind>&,
                                                                  const NumericSupportSelector<Kind>& numeric_support_selector) const noexcept
{
    if (!goals)
        return AggregationFunction::identity();

    auto total = AggregationFunction::identity();
    for (const auto literal : goals->template get_literals<::tyr::formalism::FluentTag>())
    {
        if (!literal.get_polarity())
            continue;

        const auto* annotation = and_annot.find(literal.get_atom().get_row());
        assert(annotation);
        if (!annotation)
            return std::numeric_limits<Cost>::max();

        const auto atom_cost = get_cost(*annotation);
        if (atom_cost == std::numeric_limits<Cost>::max())
            return std::numeric_limits<Cost>::max();
        total = agg(total, atom_cost);
    }

    for (const auto numeric_constraint : goals->get_numeric_constraints())
    {
        const auto constraint_cost = numeric_support_selector.get_constraint_cost(numeric_constraint, numeric_support_selector_workspace, agg);
        if (constraint_cost == std::numeric_limits<Cost>::max())
            return std::numeric_limits<Cost>::max();
        total = agg(total, constraint_cost);
    }

    return total;
}

template<TaskKind Kind, typename AggregationFunction>
void TerminationPolicy<Kind, AggregationFunction>::reset() noexcept
{
}

template<TaskKind Kind, typename AggregationFunction>
void TerminationPolicy<Kind, AggregationFunction>::clear() noexcept
{
    goals = std::nullopt;
    numeric_support_selector_workspace.clear();
}

template class TerminationPolicy<GroundTag, SumAggregation>;
template class TerminationPolicy<GroundTag, MaxAggregation>;
template class TerminationPolicy<LiftedTag, SumAggregation>;
template class TerminationPolicy<LiftedTag, MaxAggregation>;

}
