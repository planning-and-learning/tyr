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

#include "tyr/datalog/policies/annotation_concepts.hpp"

#include <algorithm>

namespace tyr::datalog
{

template<typename AggregationFunction>
void MinCostAnnotationPolicy<AggregationFunction>::initialize_annotation(PredicateHead head, PredicateAnnotations<>& annotations) const
{ annotations.insert_or_assign(head, BaseAnnotation(Cost(0))); }

template<typename AggregationFunction>
void MinCostAnnotationPolicy<AggregationFunction>::initialize_annotation(FunctionBinding head,
                                                                         ygg::ClosedInterval<ygg::float_t> interval,
                                                                         FunctionAnnotations<>& numeric_annotations) const
{ numeric_annotations.insert(head, interval, BaseAnnotation(Cost(0))); }

template<typename AggregationFunction>
void MinCostAnnotationWithAchieversPolicy<AggregationFunction>::clear_achievers() noexcept
{ m_achievers.clear(); }

template<typename AggregationFunction>
const typename MinCostAnnotationWithAchieversPolicy<AggregationFunction>::Achievers*
MinCostAnnotationWithAchieversPolicy<AggregationFunction>::find_achievers(PredicateHead head) const noexcept
{
    const auto* entry = m_achievers.find(head);
    return entry ? &entry->achievers : nullptr;
}

template<typename AggregationFunction>
void MinCostAnnotationWithAchieversPolicy<AggregationFunction>::record_achiever(PredicateHead head, PredicateWitness witness)
{
    m_achievers.update(head,
                       [&](auto& entry, bool initialized)
                       {
                           if (!initialized)
                           {
                               entry.achievers.clear();
                               entry.indices.clear();
                           }

                           const auto [it, inserted] = entry.indices.try_emplace(witness.get_rule_key(), entry.achievers.size());
                           if (inserted)
                               entry.achievers.push_back(std::move(witness));
                           else if (witness.get_cost() < entry.achievers[it->second].get_cost())
                               entry.achievers[it->second] = std::move(witness);
                       });
}

static_assert(AnnotationPolicyConcept<NoAnnotationPolicy>);
static_assert(AnnotationPolicyConcept<MinCostAnnotationPolicy<SumAggregation>>);
static_assert(AnnotationPolicyConcept<MinCostAnnotationPolicy<MaxAggregation>>);
static_assert(AnnotationPolicyConcept<MinCostAnnotationWithAchieversPolicy<MaxAggregation>>);

template class MinCostAnnotationPolicy<SumAggregation>;
template class MinCostAnnotationPolicy<MaxAggregation>;
template class MinCostAnnotationWithAchieversPolicy<MaxAggregation>;

}
