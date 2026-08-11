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

#include "tyr/datalog/policies/annotation.hpp"

#include "tyr/datalog/policies/annotation_concept.hpp"

#include <algorithm>

namespace tyr::datalog
{

template<TaskKind Kind, typename AggregationFunction>
void MinCostAnnotationPolicy<Kind, AggregationFunction>::initialize_annotation(PredicateHead head, PredicateAnnotations<Kind>& annotations) const
{
    annotations.insert_or_assign(head, BaseAnnotation<Kind>(Cost(0)));
}

template<TaskKind Kind, typename AggregationFunction>
void MinCostAnnotationPolicy<Kind, AggregationFunction>::initialize_annotation(FunctionBinding head,
                                                                               ygg::ClosedInterval<ygg::float_t> interval,
                                                                               FunctionAnnotations<Kind>& numeric_annotations) const
{
    numeric_annotations.insert(head, interval, BaseAnnotation<Kind>(Cost(0)));
}

template<TaskKind Kind, typename AggregationFunction>
CostUpdate<Kind> MinCostAnnotationPolicy<Kind, AggregationFunction>::commit_annotation(PredicateHead head,
                                                                                       const PredicateAnnotations<Kind, true>& delta_annotations,
                                                                                       PredicateAnnotations<Kind>& annotations) const
{
    const auto old_cost = annotations.fetch_cost(head);
    const auto* delta_annotation = delta_annotations.find(head);
    if (delta_annotation)
        if (auto update = annotations.insert_if_better(head, *delta_annotation))
            return *update;
    return CostUpdate<Kind>(old_cost, old_cost);
}

template<TaskKind Kind, typename AggregationFunction>
bool MinCostAnnotationPolicy<Kind, AggregationFunction>::can_update(PredicateHead head,
                                                                    Cost cost,
                                                                    const PredicateAnnotations<Kind>& annotations,
                                                                    const PredicateAnnotations<Kind, true>& delta_annotations) const noexcept
{
    return cost < std::min(annotations.fetch_cost(head), delta_annotations.fetch_cost(head));
}

template<TaskKind Kind, typename AggregationFunction>
bool MinCostAnnotationPolicy<Kind, AggregationFunction>::can_update(FunctionBinding head,
                                                                    ygg::ClosedInterval<ygg::float_t> interval,
                                                                    Cost cost,
                                                                    const FunctionAnnotations<Kind>& numeric_annotations,
                                                                    const FunctionAnnotations<Kind, true>& delta_numeric_annotations) const noexcept
{
    return cost < std::min(numeric_annotations.fetch_cost(head, interval), delta_numeric_annotations.fetch_cost(head, interval));
}

template<TaskKind Kind, typename AggregationFunction>
void MinCostAnnotationWithAchieversPolicy<Kind, AggregationFunction>::clear_achievers() noexcept
{
    m_achievers.clear();
}

template<TaskKind Kind, typename AggregationFunction>
const typename MinCostAnnotationWithAchieversPolicy<Kind, AggregationFunction>::Achievers*
MinCostAnnotationWithAchieversPolicy<Kind, AggregationFunction>::find_achievers(PredicateHead head) const noexcept
{
    return m_achievers.find(head);
}

template<TaskKind Kind, typename AggregationFunction>
void MinCostAnnotationWithAchieversPolicy<Kind, AggregationFunction>::record_achiever(PredicateHead head, const PredicateWitness& witness)
{
    m_achievers.update(head,
                       [&](auto& achievers, bool initialized)
                       {
                           if (!initialized)
                               achievers.clear();
                           if (std::find(achievers.begin(), achievers.end(), witness) == achievers.end())
                               achievers.push_back(witness);
                       });
}

static_assert(AnnotationPolicyConcept<NoAnnotationPolicy<GroundTag>, GroundTag>);
static_assert(AnnotationPolicyConcept<NoAnnotationPolicy<LiftedTag>, LiftedTag>);
static_assert(AnnotationPolicyConcept<MinCostAnnotationPolicy<GroundTag, SumAggregation>, GroundTag>);
static_assert(AnnotationPolicyConcept<MinCostAnnotationPolicy<GroundTag, MaxAggregation>, GroundTag>);
static_assert(AnnotationPolicyConcept<MinCostAnnotationPolicy<LiftedTag, SumAggregation>, LiftedTag>);
static_assert(AnnotationPolicyConcept<MinCostAnnotationPolicy<LiftedTag, MaxAggregation>, LiftedTag>);
static_assert(AnnotationPolicyConcept<MinCostAnnotationWithAchieversPolicy<GroundTag, MaxAggregation>, GroundTag>);
static_assert(AnnotationPolicyConcept<MinCostAnnotationWithAchieversPolicy<LiftedTag, MaxAggregation>, LiftedTag>);

template class MinCostAnnotationPolicy<GroundTag, SumAggregation>;
template class MinCostAnnotationPolicy<GroundTag, MaxAggregation>;
template class MinCostAnnotationPolicy<LiftedTag, SumAggregation>;
template class MinCostAnnotationPolicy<LiftedTag, MaxAggregation>;
template class MinCostAnnotationWithAchieversPolicy<GroundTag, MaxAggregation>;
template class MinCostAnnotationWithAchieversPolicy<LiftedTag, MaxAggregation>;

}
