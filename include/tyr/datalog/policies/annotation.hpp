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

template<TaskKind Kind, typename Binding>
Cost fetch_annotation_cost(Binding binding, const SelectedPredicateAnnotations<Kind>& annotations)
{
    if (const auto* annotation = annotations.find(binding))
        return get_cost(*annotation);
    return std::numeric_limits<Cost>::max();
}

template<TaskKind Kind>
bool witness_wins_tie(const WitnessAnnotation<Kind>& witness, const Annotation<Kind>* incumbent)
{
    if (!incumbent)
        return true;
    const auto* incumbent_witness = std::get_if<WitnessAnnotation<Kind>>(incumbent);
    return incumbent_witness && witness < *incumbent_witness;
}

template<TaskKind Kind, typename Binding>
const Annotation<Kind>* select_incumbent(Binding program_head,
                                         Binding delta_head,
                                         Cost best_global_cost,
                                         Cost best_local_cost,
                                         const SelectedPredicateAnnotations<Kind>& program_and_annot,
                                         const SelectedPredicateAnnotations<Kind>& delta_and_annot)
{
    return best_local_cost <= best_global_cost ? delta_and_annot.find(delta_head) : program_and_annot.find(program_head);
}

template<TaskKind Kind>
class NoOrAnnotationPolicy
{
public:
    using PredicateHead = PredicateAnnotationHeadT<Kind>;
    using FunctionHead = FunctionAnnotationHeadT<Kind>;

    void initialize_annotation(PredicateHead, SelectedPredicateAnnotations<Kind>&) const noexcept {}
    void initialize_annotation(FunctionHead, ygg::ClosedInterval<ygg::float_t>, SelectedFunctionAnnotations<Kind>&) const noexcept {}

    CostUpdate<Kind>
    update_annotation(PredicateHead, PredicateHead, const SelectedPredicateAnnotations<Kind>&, SelectedPredicateAnnotations<Kind>&) const noexcept
    {
        return {};
    }
};

template<TaskKind Kind>
class NoAndAnnotationPolicy
{
public:
    using PredicateHead = PredicateAnnotationHeadT<Kind>;
    using FunctionHead = FunctionAnnotationHeadT<Kind>;

    void clear_achievers() noexcept {}

    void record_achiever(PredicateHead, const AndAnnotationContext<Kind>&) const noexcept {}

    void update_annotation(PredicateHead, PredicateHead, const AndAnnotationContext<Kind>&, SelectedPredicateAnnotations<Kind>&) const noexcept {}

    void update_annotation(FunctionHead,
                           FunctionHead,
                           ygg::ClosedInterval<ygg::float_t>,
                           const AndAnnotationContext<Kind>&,
                           SelectedFunctionAnnotations<Kind>&) const noexcept
    {
    }
};

}

#endif
