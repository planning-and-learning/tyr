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

#include "tyr/datalog/ground/policies/annotation.hpp"

#include "tyr/datalog/policies/annotation_concept.hpp"

#include <optional>
#include <utility>

namespace tyr::datalog
{

template<typename AggregationFunction>
void MinCostAnnotationPolicy<GroundTag, AggregationFunction>::initialize_annotation(
    ::tyr::formalism::datalog::PredicateBindingView<::tyr::formalism::FluentTag> head,
    PredicateAnnotations<GroundTag>& annotations) const
{
    annotations.insert_or_assign(head, BaseAnnotation<GroundTag>(Cost(0)));
}

template<typename AggregationFunction>
void MinCostAnnotationPolicy<GroundTag, AggregationFunction>::initialize_annotation(
    ::tyr::formalism::datalog::FunctionBindingView<::tyr::formalism::FluentTag> head,
    ygg::ClosedInterval<ygg::float_t> interval,
    FunctionAnnotations<GroundTag>& numeric_annotations) const
{
    numeric_annotations.insert(head.get_relation().get_index(), head.get_index().row, interval, BaseAnnotation<GroundTag>(Cost(0)));
}

template<typename AggregationFunction>
std::optional<CostUpdate<GroundTag>>
MinCostAnnotationPolicy<GroundTag, AggregationFunction>::publish_annotation(PredicateHead head,
                                                                            PredicateWitness witness,
                                                                            PredicateAnnotations<GroundTag>& annotations) const
{
    const auto new_cost = witness.get_cost();
    if (const auto* old_annotation = annotations.find(head))
    {
        const auto old_cost = get_cost(*old_annotation);
        if (new_cost < old_cost)
        {
            annotations.insert_or_assign(head, Annotation<GroundTag>(std::move(witness)));
            return CostUpdate<GroundTag>(old_cost, new_cost);
        }
        if (new_cost == old_cost && witness_wins_tie<GroundTag>(witness, old_annotation))
        {
            annotations.insert_or_assign(head, Annotation<GroundTag>(std::move(witness)));
            return CostUpdate<GroundTag>(old_cost, old_cost);
        }
        return std::nullopt;
    }

    annotations.insert_or_assign(head, Annotation<GroundTag>(std::move(witness)));
    return CostUpdate<GroundTag>(std::nullopt, new_cost);
}

template<typename AggregationFunction>
void MinCostAnnotationWithAchieversPolicy<GroundTag, AggregationFunction>::clear_achievers() noexcept
{
    m_achievers.clear();
}

template<typename AggregationFunction>
const typename MinCostAnnotationWithAchieversPolicy<GroundTag, AggregationFunction>::Achievers*
MinCostAnnotationWithAchieversPolicy<GroundTag, AggregationFunction>::find_achievers(PredicateHead head) const noexcept
{
    return m_achievers.find(head);
}

template<typename AggregationFunction>
void MinCostAnnotationWithAchieversPolicy<GroundTag, AggregationFunction>::record_achiever(PredicateHead head, const PredicateWitness& witness)
{
    m_achievers.update(head,
                       [&](auto& achievers, bool initialized)
                       {
                           if (!initialized)
                               achievers.clear();
                           achievers.push_back(witness);
                       });
}

static_assert(AnnotationPolicyConcept<NoAnnotationPolicy<GroundTag>, GroundTag>);
static_assert(AnnotationPolicyConcept<MinCostAnnotationPolicy<GroundTag, SumAggregation>, GroundTag>);
static_assert(AnnotationPolicyConcept<MinCostAnnotationPolicy<GroundTag, MaxAggregation>, GroundTag>);
static_assert(AnnotationPolicyConcept<MinCostAnnotationWithAchieversPolicy<GroundTag, MaxAggregation>, GroundTag>);

template class MinCostAnnotationPolicy<GroundTag, SumAggregation>;
template class MinCostAnnotationPolicy<GroundTag, MaxAggregation>;
template class MinCostAnnotationWithAchieversPolicy<GroundTag, MaxAggregation>;

}
