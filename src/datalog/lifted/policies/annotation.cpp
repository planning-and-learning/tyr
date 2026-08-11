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

#include "tyr/datalog/lifted/policies/annotation.hpp"

#include "tyr/datalog/policies/aggregation.hpp"
#include "tyr/datalog/policies/annotation.hpp"
#include "tyr/datalog/policies/annotation_concept.hpp"

#include <algorithm>
#include <limits>
#include <variant>

namespace tyr::datalog
{
/**
 * MinCostAnnotationPolicy
 */

template<typename AggregationFunction>
void MinCostAnnotationPolicy<LiftedTag, AggregationFunction>::initialize_annotation(
    ::tyr::formalism::datalog::PredicateBindingView<::tyr::formalism::FluentTag> head,
    PredicateAnnotations<LiftedTag>& annotations) const
{
    annotations.insert_or_assign(head, BaseAnnotation<LiftedTag>(Cost(0)));
}

template<typename AggregationFunction>
void MinCostAnnotationPolicy<LiftedTag, AggregationFunction>::initialize_annotation(
    ::tyr::formalism::datalog::FunctionBindingView<::tyr::formalism::FluentTag> head,
    ygg::ClosedInterval<ygg::float_t> interval,
    FunctionAnnotations<LiftedTag>& numeric_annotations) const
{
    numeric_annotations.insert(head, interval, BaseAnnotation<LiftedTag>(Cost(0)));
}

template<typename AggregationFunction>
CostUpdate<LiftedTag>
MinCostAnnotationPolicy<LiftedTag, AggregationFunction>::commit_annotation(::tyr::formalism::datalog::PredicateBindingView<::tyr::formalism::FluentTag> head,
                                                                           const ConcurrentPredicateAnnotations& delta_annotations,
                                                                           PredicateAnnotations<LiftedTag>& annotations) const
{
    const auto* old_annotation = annotations.find(head);
    const auto old_cost = old_annotation ? get_cost(*old_annotation) : std::numeric_limits<Cost>::max();
    if (old_cost == Cost(0))
        return CostUpdate<LiftedTag>(old_cost, old_cost);

    const auto* delta_annotation = delta_annotations.find(head);
    const auto* witness = delta_annotation ? std::get_if<WitnessAnnotation<LiftedTag>>(delta_annotation) : nullptr;
    if (!witness)
        return CostUpdate<LiftedTag>(old_cost, old_cost);

    const auto new_cost = witness->get_cost();
    if (new_cost < old_cost)
    {
        annotations.insert_or_assign(head, *delta_annotation);
        return CostUpdate<LiftedTag>(old_cost, new_cost);
    }
    return CostUpdate<LiftedTag>(old_cost, old_cost);
}

template<typename AggregationFunction>
bool MinCostAnnotationPolicy<LiftedTag, AggregationFunction>::can_update(PredicateHead head,
                                                                         Cost cost,
                                                                         const PredicateAnnotations<LiftedTag>& annotations,
                                                                         const ConcurrentPredicateAnnotations& delta_annotations) const noexcept
{
    return cost < std::min(fetch_annotation_cost<LiftedTag>(head, annotations), delta_annotations.fetch_cost(head));
}

template<typename AggregationFunction>
bool MinCostAnnotationPolicy<LiftedTag, AggregationFunction>::can_update(FunctionHead head,
                                                                         ygg::ClosedInterval<ygg::float_t> interval,
                                                                         Cost cost,
                                                                         const FunctionAnnotations<LiftedTag>& numeric_annotations,
                                                                         const ConcurrentFunctionAnnotations& delta_numeric_annotations) const noexcept
{
    const auto* annotation = numeric_annotations.find(head, interval);
    const auto global_cost = annotation ? get_cost(*annotation) : std::numeric_limits<Cost>::max();
    return cost < std::min(global_cost, delta_numeric_annotations.fetch_cost(head, interval));
}

template<typename AggregationFunction>
bool MinCostAnnotationPolicy<LiftedTag, AggregationFunction>::try_update_candidate(PredicateHead head,
                                                                                   WitnessAnnotation<LiftedTag, ::tyr::formalism::PredicateTag>&& witness,
                                                                                   ConcurrentPredicateAnnotations& delta_annotations) const
{
    return delta_annotations.insert_if_better(head, Annotation<LiftedTag, ::tyr::formalism::PredicateTag>(std::move(witness)));
}

template<typename AggregationFunction>
bool MinCostAnnotationPolicy<LiftedTag, AggregationFunction>::try_update_candidate(FunctionHead head,
                                                                                   ygg::ClosedInterval<ygg::float_t> interval,
                                                                                   WitnessAnnotation<LiftedTag, ::tyr::formalism::FunctionTag>&& witness,
                                                                                   ConcurrentFunctionAnnotations& delta_numeric_annotations) const
{
    return delta_numeric_annotations.insert(head, interval, Annotation<LiftedTag, ::tyr::formalism::FunctionTag>(std::move(witness)));
}

/**
 * MinCostAnnotationWithAchieversPolicy
 */

template<typename AggregationFunction>
void MinCostAnnotationWithAchieversPolicy<LiftedTag, AggregationFunction>::clear_achievers() noexcept
{
    m_achievers.clear();
}

template<typename AggregationFunction>
const typename MinCostAnnotationWithAchieversPolicy<LiftedTag, AggregationFunction>::Achievers*
MinCostAnnotationWithAchieversPolicy<LiftedTag, AggregationFunction>::find_achievers(PredicateBinding head) const noexcept
{
    return m_achievers.find(head);
}

template<typename AggregationFunction>
void MinCostAnnotationWithAchieversPolicy<LiftedTag, AggregationFunction>::record_achiever(
    PredicateBinding head,
    const WitnessAnnotation<LiftedTag, ::tyr::formalism::PredicateTag>& witness)
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

static_assert(AnnotationPolicyConcept<NoAnnotationPolicy<LiftedTag>, LiftedTag>);
static_assert(AnnotationPolicyConcept<MinCostAnnotationPolicy<LiftedTag, SumAggregation>, LiftedTag>);
static_assert(AnnotationPolicyConcept<MinCostAnnotationPolicy<LiftedTag, MaxAggregation>, LiftedTag>);
static_assert(AnnotationPolicyConcept<MinCostAnnotationWithAchieversPolicy<LiftedTag, MaxAggregation>, LiftedTag>);

template class MinCostAnnotationPolicy<LiftedTag, SumAggregation>;
template class MinCostAnnotationPolicy<LiftedTag, MaxAggregation>;
template class MinCostAnnotationWithAchieversPolicy<LiftedTag, MaxAggregation>;

}
