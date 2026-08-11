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

#include <variant>
#include <yggdrasil/semantics/comparison.hpp>

namespace tyr::datalog
{

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

/// Zero-cost conservative reachability: solvers ignore annotation-only metric effects and cost credits.
template<TaskKind Kind>
class NoAnnotationPolicy
{
public:
    using PredicateHead = ::tyr::formalism::datalog::PredicateBindingView<::tyr::formalism::FluentTag>;
    using FunctionBinding = ::tyr::formalism::datalog::FunctionBindingView<::tyr::formalism::FluentTag>;

    static constexpr bool stores_annotations = false;
    static constexpr bool records_propositional_achievers = false;

    bool is_widening_label_preserving(Cost, Cost) const noexcept { return true; }

    void initialize_annotation(PredicateHead, PredicateAnnotations<Kind>&) const noexcept {}
    void initialize_annotation(FunctionBinding, ygg::ClosedInterval<ygg::float_t>, FunctionAnnotations<Kind>&) const noexcept {}

    void clear_achievers() noexcept {}
};

}

#endif
