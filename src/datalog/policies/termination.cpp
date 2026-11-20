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

#include <boost/mp11.hpp>
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
    boost::mp11::mp_for_each<MutableTags>(
        [&]<typename F>(F)
        {
            for (const auto literal : goals_.template get_literals<F>())
                if (!literal.get_polarity())
                    throw std::invalid_argument("TerminationPolicy requires positive fluent and derived goals");
        });

    clear();
    goals = goals_;
}

template<typename AggregationFunction>
bool TerminationPolicy<AggregationFunction>::check(const FactSets& fact_sets) const
{
    if (!goals)
        return false;

    return is_applicable(*goals, fact_sets);
}

template<typename AggregationFunction>
bool TerminationPolicy<AggregationFunction>::should_terminate(const FactSets& fact_sets) const
{ return early_termination && check(fact_sets); }

template<typename AggregationFunction>
template<TaskKind Kind>
Cost TerminationPolicy<AggregationFunction>::get_total_cost(const FactSets&, const PredicateAnnotations<Kind>& annotations) const
{
    if (!goals)
        return AggregationFunction::identity();

    auto total = AggregationFunction::identity();
    auto reachable = true;
    boost::mp11::mp_for_each<MutableTags>(
        [&]<typename F>(F)
        {
            for (const auto literal : goals->template get_literals<F>())
            {
                if (!reachable)
                    return;
                const auto* annotation = annotations.find(literal.get_atom().get_row().index());
                assert(annotation);
                if (!annotation || get_cost(*annotation) == std::numeric_limits<Cost>::max())
                {
                    reachable = false;
                    return;
                }
                total = agg(total, get_cost(*annotation));
            }
        });

    return reachable ? total : std::numeric_limits<Cost>::max();
}

template<typename AggregationFunction>
void TerminationPolicy<AggregationFunction>::reset() noexcept
{
}

template<typename AggregationFunction>
void TerminationPolicy<AggregationFunction>::clear() noexcept
{
    goals = std::nullopt;
    reset();
}

template class TerminationPolicy<SumAggregation>;
template class TerminationPolicy<MaxAggregation>;
template Cost TerminationPolicy<SumAggregation>::get_total_cost(const FactSets&, const PredicateAnnotations<GroundTag>&) const;
template Cost TerminationPolicy<SumAggregation>::get_total_cost(const FactSets&, const PredicateAnnotations<LiftedTag>&) const;
template Cost TerminationPolicy<MaxAggregation>::get_total_cost(const FactSets&, const PredicateAnnotations<GroundTag>&) const;
template Cost TerminationPolicy<MaxAggregation>::get_total_cost(const FactSets&, const PredicateAnnotations<LiftedTag>&) const;

}
