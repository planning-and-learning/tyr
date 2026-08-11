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

#ifndef TYR_DATALOG_GROUND_POLICIES_ANNOTATION_HPP_
#define TYR_DATALOG_GROUND_POLICIES_ANNOTATION_HPP_

#include "tyr/datalog/ground/policies/annotation_types.hpp"
#include "tyr/datalog/ground/rule_instance.hpp"
#include "tyr/datalog/policies/aggregation.hpp"
#include "tyr/datalog/policies/annotation.hpp"
#include "tyr/datalog/rule_evaluation.hpp"

#include <optional>
#include <utility>
#include <vector>

namespace tyr::datalog
{

template<typename AggregationFunction>
class MinCostAnnotationPolicy<GroundTag, AggregationFunction>
{
public:
    using Aggregation = AggregationFunction;
    using PredicateHead = PredicateAnnotationHead<GroundTag>;
    using FunctionBinding = ::tyr::formalism::datalog::FunctionBindingView<::tyr::formalism::FluentTag>;
    using PredicateWitness = WitnessAnnotation<GroundTag, ::tyr::formalism::PredicateTag>;

    static constexpr bool stores_annotations = true;
    static constexpr bool records_propositional_achievers = false;

    bool is_widening_label_preserving(Cost candidate_label, Cost current_target_label) const noexcept
    {
        if constexpr (std::same_as<AggregationFunction, MaxAggregation>)
            return true;
        return candidate_label == current_target_label;
    }

    void initialize_annotation(PredicateHead head, PredicateAnnotations<GroundTag>& annotations) const;

    void initialize_annotation(FunctionBinding head, ygg::ClosedInterval<ygg::float_t> interval, FunctionAnnotations<GroundTag>& numeric_annotations) const;

    void clear_achievers() noexcept {}

    void record_achiever(PredicateHead, const PredicateWitness&) const noexcept {}

    std::optional<CostUpdate<GroundTag>> publish_annotation(PredicateHead head, PredicateWitness witness, PredicateAnnotations<GroundTag>& annotations) const;
};

template<typename AggregationFunction>
class MinCostAnnotationWithAchieversPolicy<GroundTag, AggregationFunction> : public MinCostAnnotationPolicy<GroundTag, AggregationFunction>
{
public:
    using PredicateHead = PredicateAnnotationHead<GroundTag>;
    using PredicateWitness = WitnessAnnotation<GroundTag, ::tyr::formalism::PredicateTag>;
    using Achievers = std::vector<PredicateWitness>;

    static constexpr bool records_propositional_achievers = true;

    void initialize(size_t num_predicates) { m_achievers.initialize(num_predicates); }

    void clear_achievers() noexcept;

    const Achievers* find_achievers(PredicateHead head) const noexcept;

    void record_achiever(PredicateHead head, const PredicateWitness& witness);

private:
    DenseRelationMap<::tyr::formalism::PredicateTag, Achievers> m_achievers;
};

template<typename AP>
    requires(AP::stores_annotations)
std::optional<CostUpdate<GroundTag>> publish_candidate(AP& policy,
                                                       RuleInstance<GroundTag, ::tyr::formalism::PredicateTag>& instance,
                                                       const PredicateCandidate<GroundTag>& candidate,
                                                       PredicateAnnotations<GroundTag>& annotations)
{
    auto witness = materialize_witness(instance, candidate);
    policy.record_achiever(candidate.head, witness);
    return policy.publish_annotation(candidate.head, std::move(witness), annotations);
}

template<typename AP>
    requires(AP::stores_annotations)
bool publish_candidate(AP&,
                       RuleInstance<GroundTag, ::tyr::formalism::FunctionTag>& instance,
                       const FunctionCandidate<GroundTag>& candidate,
                       FunctionAnnotations<GroundTag>& annotations)
{
    return annotations.insert(candidate.annotation_head, candidate.interval, materialize_witness(instance, candidate));
}

}

#endif
