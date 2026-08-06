/*
 * Copyright (C) 2026 Dominik Drexler
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

#ifndef TYR_SRC_PLANNING_ALGORITHMS_SEARCH_ENGINE_CONCEPTS_HPP_
#define TYR_SRC_PLANNING_ALGORITHMS_SEARCH_ENGINE_CONCEPTS_HPP_

#include "tyr/planning/algorithms/utils.hpp"
#include "tyr/planning/heuristic.hpp"
#include "tyr/planning/search_space/search_node.hpp"
#include "tyr/planning/successor_generator.hpp"
#include "tyr/planning/worker_state_index.hpp"

#include <chrono>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <optional>
#include <span>
#include <type_traits>
#include <utility>
#include <vector>
#include <yggdrasil/containers/segmented_vector.hpp>

namespace tyr::planning::detail
{

enum class AcceptanceResult : uint8_t
{
    QUEUED,
    DISCARDED,
    TERMINAL,
};

template<typename T, typename Kind>
concept SearchPolicyConcept =
    TaskKind<Kind>
    && requires {
           typename T::SearchTag;
           typename T::TaskTag;
           typename T::Options;
           typename T::EventHandlerPtr;
           typename T::WorkerEventHandlerPtr;
           typename T::SearchNode;
           typename T::SuccessorMetadata;
           typename T::PoppedEntry;
           typename std::bool_constant<T::supports_priority_layer_synchronization>;
           requires SearchKind<typename T::SearchTag>;
           requires std::same_as<typename T::TaskTag, Kind>;
       } && std::constructible_from<T, Heuristic<Kind>&, const typename T::Options&> && requires(T& policy, const T& const_policy, ygg::Index<Worker> worker, ygg::Index<State<Kind>> state, ygg::float_t value, SearchNodeStatus status, bool preferred, const typename T::SuccessorMetadata& metadata, typename T::SearchNode& search_node, const typename T::SearchNode& const_search_node, const typename T::PoppedEntry& entry, ::tyr::formalism::planning::ActionBindingView action, const typename T::Options& options, const typename T::EventHandlerPtr& event_handler) {
           { T::terminate_on_goal } -> std::convertible_to<bool>;
           { T::supports_priority_layer_synchronization } -> std::convertible_to<bool>;
           { T::check_timeout_after_generation } -> std::convertible_to<bool>;
           { T::check_timeout_per_successor } -> std::convertible_to<bool>;
           { policy.initialize_start(state, value, value) } -> std::same_as<typename T::SearchNode&>;
           { const_policy.get_start_priority() } -> std::same_as<ygg::float_t>;
           { policy.open_start(state, const_search_node) } -> std::same_as<void>;
           { const_policy.empty() } -> std::convertible_to<bool>;
           { policy.pop() } -> std::same_as<typename T::PoppedEntry>;
           { const_policy.should_discard(entry, value) } -> std::convertible_to<bool>;
           { policy.get_search_node(state) } -> std::same_as<typename T::SearchNode&>;
           { const_policy.make_successor_metadata(worker, state, const_search_node, action) } -> std::same_as<typename T::SuccessorMetadata>;
           { T::set_parent(search_node, metadata.parent) } -> std::same_as<void>;
           { policy.open_successor(state, value, value, status, preferred) } -> std::same_as<void>;
           { T::make_worker_event_handler(event_handler, worker) } -> std::same_as<typename T::WorkerEventHandlerPtr>;
           { const_policy.get_search_nodes() } -> std::same_as<const ygg::SegmentedVector<typename T::SearchNode>&>;
       } && (!T::supports_priority_layer_synchronization || requires(const T& policy, const typename T::Options& options) {
           { T::synchronize_priority_layers(options) } -> std::convertible_to<bool>;
           { policy.get_min_priority() } -> std::same_as<ygg::float_t>;
           { policy.get_num_open_entries() } -> std::same_as<size_t>;
       });

template<typename T, typename Kind>
concept StateRoutingPolicyConcept = TaskKind<Kind> && std::constructible_from<T, uint64_t> && requires {
    typename T::TaskTag;
    typename T::PreparedTarget;
    requires std::same_as<typename T::TaskTag, Kind>;
} && requires(SuccessorGenerator<Kind>& successor_generator, std::span<const ygg::ExecutionContextPtr> execution_contexts, ygg::Index<State<Kind>> state, ygg::Index<Worker> worker, size_t num_workers, typename T::PreparedTarget prepared) {
    { T::make_successor_generators(successor_generator, execution_contexts) } -> std::same_as<std::vector<SuccessorGeneratorPtr<Kind>>>;
    { T::search_node_divisor(num_workers) } -> std::same_as<size_t>;
    { T::search_node_index(state, worker, num_workers) } -> std::same_as<ygg::Index<State<Kind>>>;
    { prepared.owner } -> std::same_as<ygg::Index<Worker>&>;
};

template<typename T, typename Kind, typename SearchPolicy>
concept ExecutionPolicyConcept = TaskKind<Kind> && SearchPolicyConcept<SearchPolicy, Kind> && std::constructible_from<T, uint64_t> && requires {
    typename T::SearchTag;
    typename T::TaskTag;
    typename T::WorkerState;
    requires std::same_as<typename T::SearchTag, typename SearchPolicy::SearchTag>;
    requires std::same_as<typename T::TaskTag, Kind>;
} && std::constructible_from<typename T::WorkerState, uint64_t> && requires(T& policy, const T& const_policy, const typename SearchPolicy::Options& options, ygg::Index<State<Kind>> state, ygg::Index<Worker> worker, size_t num_workers, ygg::float_t value, std::optional<std::chrono::steady_clock::duration> max_time, ygg::uint_t max_num_states) {
    { T::validate(options) } -> std::same_as<void>;
    { T::num_workers(options) } -> std::same_as<size_t>;
    { T::search_node_index(state, worker, num_workers) } -> std::same_as<ygg::Index<State<Kind>>>;
    { T::has_start_state_capacity(max_num_states) } -> std::convertible_to<bool>;
    { policy.initialize_best_h(value) } -> std::same_as<void>;
    {
        policy.improve_best_h(value, [] {})
    } -> std::convertible_to<bool>;
    { policy.start(max_time, value, options) } -> std::same_as<void>;
    { const_policy.running() } -> std::convertible_to<bool>;
    { const_policy.timed_out() } -> std::convertible_to<bool>;
    { const_policy.incumbent_cost() } -> std::same_as<ygg::float_t>;
    { const_policy.status() } -> std::same_as<SearchStatus>;
    { const_policy.goal() } -> std::same_as<std::optional<WorkerStateIndex<Kind>>>;
    { const_policy.exception() } -> std::same_as<std::exception_ptr>;
    { policy.reserve_state(max_num_states) } -> std::convertible_to<bool>;
};

template<typename T, typename WorkerData, typename Kind, typename SearchPolicy>
concept WorkerPolicyConcept = TaskKind<Kind> && SearchPolicyConcept<SearchPolicy, Kind>
                              && requires(T& policy,
                                          const T& const_policy,
                                          ygg::Index<Worker> worker,
                                          WorkerStateIndex<Kind> goal,
                                          SuccessorGenerator<Kind>& successor_generator,
                                          const typename SearchPolicy::Options& options) {
                                     { const_policy.size() } -> std::same_as<size_t>;
                                     { policy.get(worker) } -> std::same_as<WorkerData&>;
                                     { const_policy.get(worker) } -> std::same_as<const WorkerData&>;
                                     {
                                         policy.for_each([](WorkerData&) {})
                                     } -> std::same_as<void>;
                                     {
                                         const_policy.for_each([](const WorkerData&) {})
                                     } -> std::same_as<void>;
                                     { policy.reconstruct_solution(goal, successor_generator, options) } -> std::same_as<std::pair<Plan<Kind>, Node<Kind>>>;
                                 };

template<SearchKind Search,
         TaskKind Kind,
         SearchPolicyConcept<Kind> SearchPolicy,
         ExecutionPolicyConcept<Kind, SearchPolicy> ExecutionPolicy,
         typename WorkerData>
class WorkerPolicy;

}

#endif
