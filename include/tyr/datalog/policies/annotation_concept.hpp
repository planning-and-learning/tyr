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
                                                PredicateAnnotationHeadT<Kind> program_head,
                                                PredicateAnnotationHeadT<Kind> delta_head,
                                                FunctionAnnotationHeadT<Kind> function_head,
                                                ygg::ClosedInterval<ygg::float_t> interval,
                                                const DeltaPredicateAnnotations<Kind>& delta_and_annot,
                                                SelectedPredicateAnnotations<Kind>& program_and_annot,
                                                SelectedFunctionAnnotations<Kind>& program_numeric_and_annot) {
                                           { policy.initialize_annotation(program_head, program_and_annot) } -> std::same_as<void>;
                                           { policy.initialize_annotation(function_head, interval, program_numeric_and_annot) } -> std::same_as<void>;
                                           {
                                               policy.update_annotation(program_head, delta_head, delta_and_annot, program_and_annot)
                                           } -> std::same_as<CostUpdate<Kind>>;
                                       };

template<typename T, typename Kind>
concept AndAnnotationPolicyConcept = TaskKind<Kind>
                                     && requires(T& policy,
                                                 const T& const_policy,
                                                 PredicateAnnotationHeadT<Kind> program_head,
                                                 PredicateAnnotationHeadT<Kind> delta_head,
                                                 FunctionAnnotationHeadT<Kind> function_head,
                                                 ygg::ClosedInterval<ygg::float_t> interval,
                                                 const AndAnnotationContext<Kind>& context,
                                                 DeltaPredicateAnnotations<Kind>& delta_and_annot,
                                                 DeltaFunctionAnnotations<Kind>& delta_numeric_and_annot) {
                                            { policy.clear_achievers() } -> std::same_as<void>;
                                            { const_policy.record_achiever(program_head, context) } -> std::same_as<void>;
                                            { const_policy.update_annotation(program_head, delta_head, context, delta_and_annot) } -> std::same_as<void>;
                                            {
                                                const_policy.update_annotation(function_head, function_head, interval, context, delta_numeric_and_annot)
                                            } -> std::same_as<void>;
                                        };

}

#endif
