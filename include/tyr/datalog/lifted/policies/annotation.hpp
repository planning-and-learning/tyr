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

#ifndef TYR_SOLVER_POLICIES_ANNOTATION_HPP_
#define TYR_SOLVER_POLICIES_ANNOTATION_HPP_

#include "tyr/datalog/declarations.hpp"
#include "tyr/datalog/lifted/policies/annotation_types.hpp"
#include "tyr/datalog/policies/aggregation.hpp"
#include "tyr/datalog/policies/annotation.hpp"
#include "tyr/datalog/policies/annotation_concept.hpp"
#include "tyr/formalism/datalog/declarations.hpp"

#include <algorithm>
#include <cassert>
#include <concepts>
#include <limits>
#include <optional>
#include <tuple>
#include <vector>
#include <yggdrasil/containers/vector.hpp>
#include <yggdrasil/core/config.hpp>

namespace tyr::datalog
{

template<typename AggregationFunction>
class MinCostAnnotationPolicy<LiftedTag, AggregationFunction>
{
public:
    using PredicateHead = PredicateAnnotationHead<LiftedTag>;
    using FunctionHead = FunctionAnnotationHead<LiftedTag>;
    using Aggregation = AggregationFunction;

    static constexpr bool records_propositional_achievers = false;

    void initialize_annotation(PredicateHead head, PredicateAnnotations<LiftedTag>& annotations) const;

    void initialize_annotation(FunctionHead head, ygg::ClosedInterval<ygg::float_t> interval, FunctionAnnotations<LiftedTag>& numeric_annotations) const;

    CostUpdate<LiftedTag>
    update_annotation(PredicateHead head, const DeltaPredicateAnnotations<LiftedTag>& delta_annotations, PredicateAnnotations<LiftedTag>& annotations) const;

    void clear_achievers() noexcept {}

    void record_achiever(PredicateHead, const AnnotationContext<LiftedTag, ::tyr::formalism::PredicateTag>&) const noexcept {}

    bool update_annotation(PredicateHead head,
                           const AnnotationContext<LiftedTag, ::tyr::formalism::PredicateTag>& context,
                           DeltaPredicateAnnotations<LiftedTag>& delta_annotations) const;

    bool update_annotation(FunctionHead head,
                           ygg::ClosedInterval<ygg::float_t> interval,
                           const AnnotationContext<LiftedTag, ::tyr::formalism::FunctionTag>& context,
                           DeltaFunctionAnnotations<LiftedTag>& delta_numeric_annotations) const;
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

    void record_achiever(PredicateBinding head, const AnnotationContext<LiftedTag, ::tyr::formalism::PredicateTag>& context);

private:
    ConcurrentRelationMap<::tyr::formalism::PredicateTag, Achievers> m_achievers;
};

static_assert(AnnotationPolicyConcept<NoAnnotationPolicy<LiftedTag>, LiftedTag>);
static_assert(AnnotationPolicyConcept<MinCostAnnotationPolicy<LiftedTag, SumAggregation>, LiftedTag>);
static_assert(AnnotationPolicyConcept<MinCostAnnotationPolicy<LiftedTag, MaxAggregation>, LiftedTag>);
static_assert(AnnotationPolicyConcept<MinCostAnnotationWithAchieversPolicy<LiftedTag, MaxAggregation>, LiftedTag>);

}

#endif
