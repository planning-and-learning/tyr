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

#ifndef TYR_DATALOG_POLICIES_ANNOTATION_CONCEPTS_HPP_
#define TYR_DATALOG_POLICIES_ANNOTATION_CONCEPTS_HPP_

#include "tyr/datalog/policies/annotation_types.hpp"

#include <boost/mp11.hpp>
#include <concepts>
#include <optional>
#include <type_traits>
#include <utility>

namespace tyr::datalog
{

/// A policy that stores the annotations of the predicate facts of kind F.
template<typename T, typename Kind, typename F>
concept AnnotationStoragePolicyFor = requires(T& policy,
                                              const T& const_policy,
                                              formalism::PredicateRowKey<F> head,
                                              WitnessAnnotation<Kind> predicate_witness,
                                              PredicateAnnotations<Kind>& annotations,
                                              PredicateAnnotations<Kind, true>& delta_annotations,
                                              const PredicateAnnotations<Kind, true>& const_delta_annotations,
                                              StagedPredicateAnnotation<Kind>& staged_annotation) {
    { policy.record_achiever(head, std::move(predicate_witness)) } -> std::same_as<void>;
    { const_policy.publish_annotation(head, std::move(predicate_witness), annotations) } -> std::same_as<std::optional<CostUpdate>>;
    { const_policy.can_update(head, Cost {}, annotations, const_delta_annotations) } -> std::same_as<bool>;
    { const_policy.try_update_candidate(head, std::move(predicate_witness), delta_annotations) } -> std::same_as<bool>;
    { const_policy.commit_annotation(head, const_delta_annotations, annotations) } -> std::same_as<CostUpdate>;
    { const_policy.can_update(head, Cost {}, annotations, staged_annotation) } -> std::same_as<bool>;
    { const_policy.try_update_candidate(head, std::move(predicate_witness), staged_annotation) } -> std::same_as<bool>;
    { const_policy.commit_annotation(head, staged_annotation, annotations) } -> std::same_as<CostUpdate>;
};

template<typename T, typename Kind, typename F>
concept AnnotationInitializingPolicyFor = requires(const T& const_policy, formalism::PredicateRowKey<F> head, PredicateAnnotations<Kind>& annotations) {
    { const_policy.initialize_annotation(head, annotations) } -> std::same_as<void>;
};

template<typename T, typename Kind, typename Tags>
inline constexpr bool is_annotation_storage_policy_for_all = false;

template<typename T, typename Kind, typename... Fs>
inline constexpr bool is_annotation_storage_policy_for_all<T, Kind, boost::mp11::mp_list<Fs...>> = (AnnotationStoragePolicyFor<T, Kind, Fs> && ...);

template<typename T, typename Kind, typename Tags>
inline constexpr bool is_annotation_initializing_policy_for_all = false;

template<typename T, typename Kind, typename... Fs>
inline constexpr bool is_annotation_initializing_policy_for_all<T, Kind, boost::mp11::mp_list<Fs...>> = (AnnotationInitializingPolicyFor<T, Kind, Fs> && ...);

/// A policy that stores the annotations of the predicate facts of each mutable fact kind.
template<typename T, typename Kind>
concept AnnotationStoragePolicy = is_annotation_storage_policy_for_all<T, Kind, MutableTags>;

template<typename T, typename Kind>
concept AnnotationPolicyFor =
    requires(T& policy) {
        typename std::bool_constant<T::stores_annotations>;
        typename std::bool_constant<T::records_propositional_achievers>;
        { policy.clear_achievers() } -> std::same_as<void>;
    } && is_annotation_initializing_policy_for_all<T, Kind, MutableTags>
    && (!T::stores_annotations || (requires { typename T::Aggregation; } && AnnotationStoragePolicy<T, Kind>) );

/// A policy of either engine: the witnesses name rule instances by RuleKey<Kind>, so a policy that stores them serves
/// one engine.
template<typename T>
concept AnnotationPolicyConcept = AnnotationPolicyFor<T, GroundTag> || AnnotationPolicyFor<T, LiftedTag>;

}

#endif
