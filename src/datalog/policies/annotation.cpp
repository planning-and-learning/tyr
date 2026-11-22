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
#include <utility>

namespace tyr::datalog
{

template<TaskKind Kind, typename AggregationFunction>
void MinCostAnnotationWithAchieversPolicy<Kind, AggregationFunction>::clear_achievers() noexcept
{ m_achievers.clear(); }

template<TaskKind Kind, typename AggregationFunction>
template<formalism::FactKind F>
const typename MinCostAnnotationWithAchieversPolicy<Kind, AggregationFunction>::Achievers*
MinCostAnnotationWithAchieversPolicy<Kind, AggregationFunction>::find_achievers(formalism::PredicateRowKey<F> head) const noexcept
{
    const auto* entry = m_achievers.template get<F>().find(head);
    return entry ? &entry->achievers : nullptr;
}

template<TaskKind Kind, typename AggregationFunction>
template<formalism::FactKind F>
void MinCostAnnotationWithAchieversPolicy<Kind, AggregationFunction>::record_achiever(formalism::PredicateRowKey<F> head, PredicateWitness witness)
{
    m_achievers.template get<F>().update(head,
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
static_assert(AnnotationPolicyConcept<MinCostAnnotationWithAchieversPolicy<GroundTag, MaxAggregation>>);
static_assert(AnnotationPolicyConcept<MinCostAnnotationWithAchieversPolicy<LiftedTag, MaxAggregation>>);

template class MinCostAnnotationPolicy<SumAggregation>;
template class MinCostAnnotationPolicy<MaxAggregation>;
template class MinCostAnnotationWithAchieversPolicy<GroundTag, MaxAggregation>;
template class MinCostAnnotationWithAchieversPolicy<LiftedTag, MaxAggregation>;
template const MinCostAnnotationWithAchieversPolicy<GroundTag, MaxAggregation>::Achievers*
    MinCostAnnotationWithAchieversPolicy<GroundTag, MaxAggregation>::find_achievers(formalism::PredicateRowKey<formalism::FluentTag>) const noexcept;
template const MinCostAnnotationWithAchieversPolicy<GroundTag, MaxAggregation>::Achievers*
    MinCostAnnotationWithAchieversPolicy<GroundTag, MaxAggregation>::find_achievers(formalism::PredicateRowKey<formalism::DerivedTag>) const noexcept;
template const MinCostAnnotationWithAchieversPolicy<LiftedTag, MaxAggregation>::Achievers*
    MinCostAnnotationWithAchieversPolicy<LiftedTag, MaxAggregation>::find_achievers(formalism::PredicateRowKey<formalism::FluentTag>) const noexcept;
template const MinCostAnnotationWithAchieversPolicy<LiftedTag, MaxAggregation>::Achievers*
    MinCostAnnotationWithAchieversPolicy<LiftedTag, MaxAggregation>::find_achievers(formalism::PredicateRowKey<formalism::DerivedTag>) const noexcept;
template void MinCostAnnotationWithAchieversPolicy<GroundTag, MaxAggregation>::record_achiever(formalism::PredicateRowKey<formalism::FluentTag>,
                                                                                               WitnessAnnotation<GroundTag>);
template void MinCostAnnotationWithAchieversPolicy<GroundTag, MaxAggregation>::record_achiever(formalism::PredicateRowKey<formalism::DerivedTag>,
                                                                                               WitnessAnnotation<GroundTag>);
template void MinCostAnnotationWithAchieversPolicy<LiftedTag, MaxAggregation>::record_achiever(formalism::PredicateRowKey<formalism::FluentTag>,
                                                                                               WitnessAnnotation<LiftedTag>);
template void MinCostAnnotationWithAchieversPolicy<LiftedTag, MaxAggregation>::record_achiever(formalism::PredicateRowKey<formalism::DerivedTag>,
                                                                                               WitnessAnnotation<LiftedTag>);

}
