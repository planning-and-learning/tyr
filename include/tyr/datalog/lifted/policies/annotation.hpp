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

#ifndef TYR_DATALOG_LIFTED_POLICIES_ANNOTATION_HPP_
#define TYR_DATALOG_LIFTED_POLICIES_ANNOTATION_HPP_

#include "tyr/datalog/declarations.hpp"
#include "tyr/datalog/lifted/policies/annotation_types.hpp"
#include "tyr/datalog/policies/aggregation.hpp"
#include "tyr/datalog/policies/annotation.hpp"

#include <concepts>
#include <vector>

namespace tyr::datalog
{

template<typename AggregationFunction>
class MinCostAnnotationPolicy<LiftedTag, AggregationFunction>
{
public:
    using Aggregation = AggregationFunction;
    using PredicateHead = PredicateAnnotationHead<LiftedTag>;
    using FunctionHead = FunctionAnnotationHead<LiftedTag>;
    using FunctionBinding = ::tyr::formalism::datalog::FunctionBindingView<::tyr::formalism::FluentTag>;
    static constexpr bool stores_annotations = true;
    static constexpr bool records_propositional_achievers = false;

    void initialize_annotation(PredicateHead head, PredicateAnnotations<LiftedTag>& annotations) const;

    void initialize_annotation(FunctionBinding head, ygg::ClosedInterval<ygg::float_t> interval, FunctionAnnotations<LiftedTag>& numeric_annotations) const;

    CostUpdate<LiftedTag>
    commit_annotation(PredicateHead head, const ConcurrentPredicateAnnotations& delta_annotations, PredicateAnnotations<LiftedTag>& annotations) const;

    void clear_achievers() noexcept {}

    void record_achiever(PredicateHead, const WitnessAnnotation<LiftedTag, ::tyr::formalism::PredicateTag>&) const noexcept {}

    bool can_update(PredicateHead head,
                    Cost cost,
                    const PredicateAnnotations<LiftedTag>& annotations,
                    const ConcurrentPredicateAnnotations& delta_annotations) const noexcept;

    bool can_update(FunctionHead head,
                    ygg::ClosedInterval<ygg::float_t> interval,
                    Cost cost,
                    const FunctionAnnotations<LiftedTag>& numeric_annotations,
                    const ConcurrentFunctionAnnotations& delta_numeric_annotations) const noexcept;

    bool try_update_candidate(PredicateHead head,
                              WitnessAnnotation<LiftedTag, ::tyr::formalism::PredicateTag>&& witness,
                              ConcurrentPredicateAnnotations& delta_annotations) const;

    bool try_update_candidate(FunctionHead head,
                              ygg::ClosedInterval<ygg::float_t> interval,
                              WitnessAnnotation<LiftedTag, ::tyr::formalism::FunctionTag>&& witness,
                              ConcurrentFunctionAnnotations& delta_numeric_annotations) const;

    bool is_widening_label_preserving(Cost candidate_label, Cost current_target_label) const noexcept
    {
        if constexpr (std::same_as<AggregationFunction, MaxAggregation>)
            return true;
        else
            return candidate_label == current_target_label;
    }
};

template<typename AggregationFunction>
class MinCostAnnotationWithAchieversPolicy<LiftedTag, AggregationFunction> : public MinCostAnnotationPolicy<LiftedTag, AggregationFunction>
{
public:
    using PredicateBinding = PredicateAnnotationHead<LiftedTag>;
    using Achievers = std::vector<WitnessAnnotation<LiftedTag, ::tyr::formalism::PredicateTag>>;

    static constexpr bool records_propositional_achievers = true;

    void initialize(size_t num_predicates) { m_achievers.initialize(num_predicates); }

    void clear_achievers() noexcept;

    const Achievers* find_achievers(PredicateBinding head) const noexcept;

    void record_achiever(PredicateBinding head, const WitnessAnnotation<LiftedTag, ::tyr::formalism::PredicateTag>& witness);

private:
    ConcurrentRelationMap<::tyr::formalism::PredicateTag, Achievers> m_achievers;
};

}

#endif
