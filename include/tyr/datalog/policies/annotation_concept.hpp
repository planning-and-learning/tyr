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

namespace tyr::datalog
{

template<typename T, typename Kind>
concept OrAnnotationPolicyConcept = TaskKind<Kind>
                                    && requires(const T& policy,
                                                PredicateAnnotationHead<Kind> head,
                                                FunctionAnnotationHead<Kind> function_head,
                                                ygg::ClosedInterval<ygg::float_t> interval,
                                                const DeltaPredicateAnnotations<Kind>& delta_and_annot,
                                                PredicateAnnotations<Kind>& and_annot,
                                                FunctionAnnotations<Kind>& numeric_and_annot) {
                                           { policy.initialize_annotation(head, and_annot) } -> std::same_as<void>;
                                           { policy.initialize_annotation(function_head, interval, numeric_and_annot) } -> std::same_as<void>;
                                           { policy.update_annotation(head, delta_and_annot, and_annot) } -> std::same_as<CostUpdate<Kind>>;
                                       };

template<typename T, typename Kind>
concept AndAnnotationPolicyConcept = TaskKind<Kind>
                                     && requires(T& policy,
                                                 const T& const_policy,
                                                 PredicateAnnotationHead<Kind> head,
                                                 FunctionAnnotationHead<Kind> function_head,
                                                 ygg::ClosedInterval<ygg::float_t> interval,
                                                 const AndAnnotationContext<Kind, ::tyr::formalism::PredicateTag>& predicate_context,
                                                 const AndAnnotationContext<Kind, ::tyr::formalism::FunctionTag>& function_context,
                                                 DeltaPredicateAnnotations<Kind>& delta_and_annot,
                                                 DeltaFunctionAnnotations<Kind>& delta_numeric_and_annot) {
                                            { policy.clear_achievers() } -> std::same_as<void>;
                                            { policy.record_achiever(head, predicate_context) } -> std::same_as<void>;
                                            { const_policy.update_annotation(head, predicate_context, delta_and_annot) } -> std::same_as<void>;
                                            {
                                                const_policy.update_annotation(function_head, interval, function_context, delta_numeric_and_annot)
                                            } -> std::same_as<void>;
                                        };

}

#endif
