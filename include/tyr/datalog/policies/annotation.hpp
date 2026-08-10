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

#ifndef TYR_DATALOG_POLICIES_ANNOTATION_HPP_
#define TYR_DATALOG_POLICIES_ANNOTATION_HPP_

#include "tyr/datalog/policies/annotation_types.hpp"

#include <limits>
#include <variant>
#include <yggdrasil/semantics/comparison.hpp>

namespace tyr::datalog
{

template<TaskKind Kind>
Cost fetch_annotation_cost(typename PredicateAnnotations<Kind>::Key key, const PredicateAnnotations<Kind>& annotations)
{
    if (const auto* annotation = annotations.find(key))
        return get_cost(*annotation);
    return std::numeric_limits<Cost>::max();
}

template<TaskKind Kind, ::tyr::formalism::RelationKind R, typename Less>
bool witness_wins_tie(const WitnessAnnotation<Kind, R>& witness, const Annotation<Kind, R>* incumbent, Less less)
{
    if (!incumbent)
        return true;
    const auto* incumbent_witness = std::get_if<WitnessAnnotation<Kind, R>>(incumbent);
    return incumbent_witness && less(witness, *incumbent_witness);
}

template<TaskKind Kind, ::tyr::formalism::RelationKind R>
bool witness_wins_tie(const WitnessAnnotation<Kind, R>& witness, const Annotation<Kind, R>* incumbent)
{
    return witness_wins_tie(witness, incumbent, ygg::Less<> {});
}

template<TaskKind Kind>
class NoAnnotationPolicy
{
public:
    using PredicateHead = PredicateAnnotationHead<Kind>;
    using FunctionHead = FunctionAnnotationHead<Kind>;
    using FunctionBinding = ::tyr::formalism::datalog::FunctionBindingView<::tyr::formalism::FluentTag>;

    static constexpr bool stores_annotations = false;
    static constexpr bool records_propositional_achievers = false;

    void initialize_annotation(PredicateHead, PredicateAnnotations<Kind>&) const noexcept {}
    void initialize_annotation(FunctionBinding, ygg::ClosedInterval<ygg::float_t>, FunctionAnnotations<Kind>&) const noexcept {}

    void clear_achievers() noexcept {}

    void record_achiever(PredicateHead, const AnnotationContext<Kind, ::tyr::formalism::PredicateTag>&) const noexcept {}

    bool try_update_candidate(PredicateHead, const AnnotationContext<Kind, ::tyr::formalism::PredicateTag>&, DeltaPredicateAnnotations<Kind>&) const noexcept
    {
        return false;
    }

    bool try_update_candidate(FunctionHead,
                              ygg::ClosedInterval<ygg::float_t>,
                              const AnnotationContext<Kind, ::tyr::formalism::FunctionTag>&,
                              DeltaFunctionAnnotations<Kind>&) const noexcept
    {
        return false;
    }

    CostUpdate<Kind> commit_annotation(PredicateHead, const DeltaPredicateAnnotations<Kind>&, PredicateAnnotations<Kind>&) const noexcept { return {}; }
};

}

#endif
