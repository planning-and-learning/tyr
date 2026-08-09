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

template<>
class OrAnnotationPolicy<LiftedTag>
{
public:
    using PredicateHead = PredicateAnnotationHead<LiftedTag>;
    using FunctionHead = FunctionAnnotationHead<LiftedTag>;

    void initialize_annotation(PredicateHead head, PredicateAnnotations<LiftedTag>& and_annot) const;

    void initialize_annotation(FunctionHead head, ygg::ClosedInterval<ygg::float_t> interval, FunctionAnnotations<LiftedTag>& numeric_and_annot) const;

    CostUpdate<LiftedTag>
    update_annotation(PredicateHead head, const DeltaPredicateAnnotations<LiftedTag>& delta_and_annot, PredicateAnnotations<LiftedTag>& and_annot) const;
};

template<typename AggregationFunction>
class AndAnnotationPolicy<LiftedTag, AggregationFunction>
{
public:
    using PredicateHead = PredicateAnnotationHead<LiftedTag>;
    using FunctionHead = FunctionAnnotationHead<LiftedTag>;

    static constexpr AggregationFunction agg = AggregationFunction {};
    static constexpr bool records_propositional_achievers = false;

    void clear_achievers() noexcept {}

    void record_achiever(PredicateHead, const AndAnnotationContext<LiftedTag, ::tyr::formalism::PredicateTag>&) const noexcept {}

    void update_annotation(PredicateHead head,
                           const AndAnnotationContext<LiftedTag, ::tyr::formalism::PredicateTag>& context,
                           DeltaPredicateAnnotations<LiftedTag>& delta_and_annot) const;

    void update_annotation(FunctionHead head,
                           ygg::ClosedInterval<ygg::float_t> interval,
                           const AndAnnotationContext<LiftedTag, ::tyr::formalism::FunctionTag>& context,
                           DeltaFunctionAnnotations<LiftedTag>& delta_numeric_and_annot) const;
};

template<typename AggregationFunction>
class AchieverAndAnnotationPolicy<LiftedTag, AggregationFunction> : public AndAnnotationPolicy<LiftedTag, AggregationFunction>
{
public:
    using PredicateBinding = PredicateAnnotationHead<LiftedTag>;
    using Achievers = std::vector<WitnessAnnotation<LiftedTag, ::tyr::formalism::PredicateTag>>;

    static constexpr bool records_propositional_achievers = true;

    void initialize(size_t num_predicates) { m_achievers.initialize(num_predicates); }

    void clear_achievers() noexcept;

    const Achievers* find_achievers(PredicateBinding head) const noexcept;

    void record_achiever(PredicateBinding head, const AndAnnotationContext<LiftedTag, ::tyr::formalism::PredicateTag>& context);

private:
    ConcurrentRelationMap<::tyr::formalism::PredicateTag, Achievers> m_achievers;
};

static_assert(OrAnnotationPolicyConcept<NoOrAnnotationPolicy<LiftedTag>, LiftedTag>);
static_assert(AndAnnotationPolicyConcept<NoAndAnnotationPolicy<LiftedTag>, LiftedTag>);
static_assert(OrAnnotationPolicyConcept<OrAnnotationPolicy<LiftedTag>, LiftedTag>);
static_assert(AndAnnotationPolicyConcept<AndAnnotationPolicy<LiftedTag, SumAggregation>, LiftedTag>);
static_assert(AndAnnotationPolicyConcept<AchieverAndAnnotationPolicy<LiftedTag, MaxAggregation>, LiftedTag>);

}

#endif
