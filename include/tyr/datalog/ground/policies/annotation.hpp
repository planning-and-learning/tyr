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

#include "tyr/datalog/declarations.hpp"
#include "tyr/datalog/ground/policies/annotation_types.hpp"
#include "tyr/datalog/policies/aggregation.hpp"
#include "tyr/datalog/policies/annotation.hpp"
#include "tyr/datalog/policies/annotation_concept.hpp"

#include <vector>

namespace tyr::datalog
{

template<>
class OrAnnotationPolicy<GroundTag>
{
public:
    using PredicateHead = PredicateAnnotationHead<GroundTag>;
    using FunctionHead = FunctionAnnotationHead<GroundTag>;

    void initialize_annotation(PredicateHead head, PredicateAnnotations<GroundTag>& and_annot) const;

    void initialize_annotation(FunctionHead head, ygg::ClosedInterval<ygg::float_t> interval, FunctionAnnotations<GroundTag>& numeric_and_annot) const;
    void initialize_annotation(::tyr::formalism::datalog::FunctionBindingView<::tyr::formalism::FluentTag> head,
                               ygg::ClosedInterval<ygg::float_t> interval,
                               FunctionAnnotations<GroundTag>& numeric_and_annot) const;

    CostUpdate<GroundTag>
    update_annotation(PredicateHead head, const DeltaPredicateAnnotations<GroundTag>& delta_and_annot, PredicateAnnotations<GroundTag>& and_annot) const;
};

template<typename AggregationFunction>
class AndAnnotationPolicy<GroundTag, AggregationFunction>
{
public:
    using PredicateHead = PredicateAnnotationHead<GroundTag>;
    using FunctionHead = FunctionAnnotationHead<GroundTag>;

    static constexpr AggregationFunction agg = AggregationFunction {};
    static constexpr bool records_propositional_achievers = false;

    void clear_achievers() noexcept;

    void record_achiever(PredicateHead, const AndAnnotationContext<GroundTag, ::tyr::formalism::PredicateTag>&) const noexcept;

    void update_annotation(PredicateHead head,
                           const AndAnnotationContext<GroundTag, ::tyr::formalism::PredicateTag>& context,
                           DeltaPredicateAnnotations<GroundTag>& delta_and_annot) const;

    void update_annotation(FunctionHead head,
                           ygg::ClosedInterval<ygg::float_t> interval,
                           const AndAnnotationContext<GroundTag, ::tyr::formalism::FunctionTag>& context,
                           DeltaFunctionAnnotations<GroundTag>& delta_numeric_and_annot) const;
};

template<typename AggregationFunction>
class AchieverAndAnnotationPolicy<GroundTag, AggregationFunction> : public AndAnnotationPolicy<GroundTag, AggregationFunction>
{
public:
    using PredicateHead = PredicateAnnotationHead<GroundTag>;
    using Achievers = std::vector<WitnessAnnotation<GroundTag, ::tyr::formalism::PredicateTag>>;

    static constexpr bool records_propositional_achievers = true;

    void initialize(size_t num_predicates) { m_achievers.initialize(num_predicates); }

    void clear_achievers() noexcept;

    const Achievers* find_achievers(PredicateHead head) const noexcept;

    void record_achiever(PredicateHead head, const AndAnnotationContext<GroundTag, ::tyr::formalism::PredicateTag>& context);

private:
    DenseRelationMap<::tyr::formalism::PredicateTag, Achievers> m_achievers;
};

static_assert(OrAnnotationPolicyConcept<NoOrAnnotationPolicy<GroundTag>, GroundTag>);
static_assert(AndAnnotationPolicyConcept<NoAndAnnotationPolicy<GroundTag>, GroundTag>);
static_assert(OrAnnotationPolicyConcept<OrAnnotationPolicy<GroundTag>, GroundTag>);
static_assert(AndAnnotationPolicyConcept<AndAnnotationPolicy<GroundTag, SumAggregation>, GroundTag>);
static_assert(AndAnnotationPolicyConcept<AchieverAndAnnotationPolicy<GroundTag, MaxAggregation>, GroundTag>);

}

#endif
