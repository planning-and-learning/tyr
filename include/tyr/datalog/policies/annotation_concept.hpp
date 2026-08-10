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

#ifndef TYR_DATALOG_POLICIES_ANNOTATION_CONCEPT_HPP_
#define TYR_DATALOG_POLICIES_ANNOTATION_CONCEPT_HPP_

#include "tyr/datalog/policies/annotation_types.hpp"

#include <concepts>
#include <type_traits>

namespace tyr::datalog
{

template<typename T, typename Kind>
concept AnnotationPolicyConcept = TaskKind<Kind>
                                  && requires(T& policy,
                                              const T& const_policy,
                                              PredicateAnnotationHead<Kind> head,
                                              FunctionAnnotationHead<Kind> function_head,
                                              ::tyr::formalism::datalog::FunctionBindingView<::tyr::formalism::FluentTag> function_binding,
                                              ygg::ClosedInterval<ygg::float_t> interval,
                                              const AnnotationContext<Kind, ::tyr::formalism::PredicateTag>& predicate_context,
                                              const AnnotationContext<Kind, ::tyr::formalism::FunctionTag>& function_context,
                                              PredicateAnnotations<Kind>& annotations,
                                              FunctionAnnotations<Kind>& numeric_annotations,
                                              DeltaPredicateAnnotations<Kind>& delta_annotations,
                                              const DeltaPredicateAnnotations<Kind>& const_delta_annotations,
                                              DeltaFunctionAnnotations<Kind>& delta_numeric_annotations) {
                                         typename std::bool_constant<T::stores_annotations>;
                                         typename std::bool_constant<T::records_propositional_achievers>;
                                         { const_policy.is_widening_label_preserving(Cost {}, Cost {}) } -> std::same_as<bool>;
                                         { const_policy.initialize_annotation(head, annotations) } -> std::same_as<void>;
                                         { const_policy.initialize_annotation(function_binding, interval, numeric_annotations) } -> std::same_as<void>;
                                         { policy.clear_achievers() } -> std::same_as<void>;
                                         { policy.record_achiever(head, predicate_context) } -> std::same_as<void>;
                                         { const_policy.try_update_candidate(head, predicate_context, delta_annotations) } -> std::same_as<bool>;
                                         {
                                             const_policy.try_update_candidate(function_head, interval, function_context, delta_numeric_annotations)
                                         } -> std::same_as<bool>;
                                         { const_policy.commit_annotation(head, const_delta_annotations, annotations) } -> std::same_as<CostUpdate<Kind>>;
                                     };

}

#endif
