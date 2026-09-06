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

#include "tyr/datalog/applicability.hpp"

#include <cassert>
#include <limits>
#include <stdexcept>

namespace tyr::datalog
{

static_assert(TerminationPolicyConcept<NoTerminationPolicy>);
static_assert(TerminationPolicyConcept<TerminationPolicy<SumAggregation>>);
static_assert(TerminationPolicyConcept<TerminationPolicy<MaxAggregation>>);

template<typename AggregationFunction>
void TerminationPolicy<AggregationFunction>::set_goals(formalism::datalog::ConjunctiveConditionView<GroundTag> goals_)
{
    for (const auto literal : goals_.template get_literals<formalism::FluentTag>())
        if (!literal.get_polarity())
            throw std::invalid_argument("TerminationPolicy requires positive fluent goals");

    clear();
    goals = goals_;
}

template<typename AggregationFunction>
bool TerminationPolicy<AggregationFunction>::check(const FactSets& fact_sets) const noexcept
{
    if (!goals)
        return false;

    return is_applicable(*goals, fact_sets);
}

template<typename AggregationFunction>
bool TerminationPolicy<AggregationFunction>::should_terminate(const FactSets& fact_sets) const noexcept
{
    return early_termination && check(fact_sets);
}

template<typename AggregationFunction>
Cost TerminationPolicy<AggregationFunction>::get_total_cost(const FactSets&,
                                                            const PredicateAnnotations<>& annotations,
                                                            const FunctionAnnotations<>&,
                                                            const NumericSupportSelector& numeric_support_selector) const noexcept
{
    if (!goals)
        return AggregationFunction::identity();

    auto total = AggregationFunction::identity();
    for (const auto literal : goals->template get_literals<formalism::FluentTag>())
    {
        const auto* annotation = annotations.find(literal.get_atom().get_row());
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
        const auto constraint_cost = numeric_support_selector.get_constraint_cost(numeric_constraint, numeric_support_selector_workspace.selection, agg);
        if (constraint_cost == std::numeric_limits<Cost>::max())
            return std::numeric_limits<Cost>::max();
        total = agg(total, constraint_cost);
    }

    return total;
}

template<typename AggregationFunction>
void TerminationPolicy<AggregationFunction>::reset() noexcept
{
    numeric_support_selector_workspace.clear();
}

template<typename AggregationFunction>
void TerminationPolicy<AggregationFunction>::clear() noexcept
{
    goals = std::nullopt;
    reset();
}

template class TerminationPolicy<SumAggregation>;
template class TerminationPolicy<MaxAggregation>;

}
