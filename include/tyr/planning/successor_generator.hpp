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

#ifndef TYR_PLANNING_SUCCESSOR_GENERATOR_HPP_
#define TYR_PLANNING_SUCCESSOR_GENERATOR_HPP_

#include "tyr/planning/declarations.hpp"
#include "tyr/planning/node.hpp"
#include "tyr/planning/state_index.hpp"

#include <concepts>
#include <deque>
#include <functional>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>
#include <yggdrasil/containers/shared_object_pool.hpp>
#include <yggdrasil/containers/span.hpp>
#include <yggdrasil/core/concepts.hpp>
#include <yggdrasil/core/config.hpp>

namespace tyr::planning
{

/// Result of checking an offered tuple without publishing its action binding.
enum class ActionBindingStatus
{
    APPLICABLE,
    OUTSIDE_PARAMETER_DOMAIN,
    INAPPLICABLE
};

/// The binding is present exactly when status is APPLICABLE and borrows the task repository.
struct ActionBindingResult
{
    ActionBindingStatus status;
    std::optional<formalism::planning::ActionBindingView> binding;
};

/// Ground enumeration borrows compiled bindings; lifted enumeration borrows reusable binding data.
template<TaskKind Kind>
using BorrowedActionBindingView =
    std::conditional_t<std::same_as<Kind, GroundTag>, formalism::planning::ActionBindingView, formalism::planning::ActionBindingDataView>;

/// Indexed outputs live in a repository; borrowed single/callback outputs use caller-owned scratch.
template<StateViewConcept S>
using SuccessorStorage =
    std::conditional_t<std::same_as<S, StateView<typename S::KindType>>, StateRepository<typename S::KindType>, ygg::Builder<State<typename S::KindType>>>;

/// Borrowed lists append distinct builders without invalidating previous outputs or the source.
/// Callers keep those builders alive and unchanged for as long as their views are used.
template<StateViewConcept S>
using SuccessorListStorage = std::conditional_t<std::same_as<S, StateView<typename S::KindType>>,
                                                StateRepository<typename S::KindType>,
                                                std::deque<ygg::Builder<State<typename S::KindType>>>>;

template<typename T, typename Kind, typename S = StateView<Kind>>
concept SuccessorGeneratorConcept = requires(T& r,
                                             const T& const_r,
                                             ygg::Index<State<Kind>> state_index,
                                             const Node<S>& node,
                                             NodeList<S>& successor_nodes,
                                             LabeledNodeList<S>& labeled_successor_nodes,
                                             PackedNodeList<Kind>& packed_successor_nodes,
                                             PackedLabeledNodeList<Kind>& packed_labeled_successor_nodes,
                                             const std::function<bool(Node<S>)>& node_callback,
                                             const std::function<bool(LabeledNode<S>)>& labeled_node_callback,
                                             const std::function<bool(formalism::planning::ActionBindingView)>& binding_callback,
                                             std::vector<formalism::planning::ActionBindingView>& action_bindings,
                                             formalism::planning::ActionBindingView binding,
                                             BorrowedActionBindingView<Kind> borrowed_binding,
                                             const std::function<bool(BorrowedActionBindingView<Kind>)>& borrowed_binding_callback,
                                             formalism::planning::ActionView<LiftedTag> action,
                                             formalism::planning::ObjectSpanView objects,
                                             StateRepository<Kind>& state_repository,
                                             SuccessorStorage<S>& successor_storage,
                                             SuccessorListStorage<S>& successor_list_storage,
                                             AxiomEvaluator<Kind>& axiom_evaluator,
                                             ygg::Builder<State<Kind>>& state_builder,
                                             ygg::SharedObjectPoolPtr<ygg::Builder<State<Kind>>, true> state_builder_ptr,
                                             ygg::float_t auxiliary_value,
                                             ygg::ExecutionContextPtr execution_context) {
    requires TaskKind<Kind>;
    requires StateViewConcept<S, Kind>;
    { r.check_action_binding(node, action, objects) } -> std::same_as<ActionBindingStatus>;
    { r.check_action_binding(node, binding) } -> std::same_as<ActionBindingStatus>;
    { r.materialize_action_binding(borrowed_binding) } -> std::same_as<formalism::planning::ActionBindingView>;
    { r.get_successor_node(node, borrowed_binding, successor_storage, axiom_evaluator) } -> std::same_as<Node<S>>;
    { r.try_get_applicable_action_binding(node, action, objects) } -> std::same_as<ActionBindingResult>;
    { r.get_initial_node(state_repository, axiom_evaluator) } -> std::same_as<Node<StateView<Kind>>>;
    { r.get_successor_nodes(node, successor_list_storage, axiom_evaluator) } -> std::same_as<NodeList<S>>;
    { r.get_successor_nodes(node, successor_list_storage, axiom_evaluator, successor_nodes) } -> std::same_as<void>;
    { r.get_labeled_successor_nodes(node, successor_list_storage, axiom_evaluator) } -> std::same_as<LabeledNodeList<S>>;
    { r.get_labeled_successor_nodes(node, successor_list_storage, axiom_evaluator, labeled_successor_nodes) } -> std::same_as<void>;
    { r.get_applicable_action_bindings(node) } -> std::same_as<std::vector<formalism::planning::ActionBindingView>>;
    { r.get_applicable_action_bindings(node, action_bindings) } -> std::same_as<void>;
    { r.get_successor_node(node, binding, successor_storage, axiom_evaluator) } -> std::same_as<Node<S>>;
    { r.generate_successor_state(node, binding, state_builder) } -> std::same_as<ygg::float_t>;
    { r.finalize_successor_state(state_repository, axiom_evaluator, std::move(state_builder_ptr), auxiliary_value) } -> std::same_as<Node<StateView<Kind>>>;
    { r.get_node(state_repository, state_index) } -> std::same_as<Node<StateView<Kind>>>;
    { r.get_packed_initial_node(state_repository, axiom_evaluator) } -> std::same_as<PackedNode<Kind>>;
    { r.get_packed_node(state_repository, state_index) } -> std::same_as<PackedNode<Kind>>;
    { r.get_packed_successor_node(node, binding, state_repository, axiom_evaluator) } -> std::same_as<PackedNode<Kind>>;
    { r.get_packed_successor_nodes(node, state_repository, axiom_evaluator) } -> std::same_as<PackedNodeList<Kind>>;
    { r.get_packed_successor_nodes(node, state_repository, axiom_evaluator, packed_successor_nodes) } -> std::same_as<void>;
    { r.get_packed_labeled_successor_nodes(node, state_repository, axiom_evaluator) } -> std::same_as<PackedLabeledNodeList<Kind>>;
    { r.get_packed_labeled_successor_nodes(node, state_repository, axiom_evaluator, packed_labeled_successor_nodes) } -> std::same_as<void>;
    { r.for_each_successor_node(node, successor_storage, axiom_evaluator, node_callback) } -> std::same_as<bool>;
    { r.for_each_labeled_successor_node(node, successor_storage, axiom_evaluator, labeled_node_callback) } -> std::same_as<bool>;
    { r.for_each_applicable_action_binding(node, binding_callback) } -> std::same_as<bool>;
    { r.for_each_borrowed_applicable_action_binding(node, borrowed_binding_callback) } -> std::same_as<bool>;
    { r.for_each_borrowed_applicable_action_binding(node, action, borrowed_binding_callback) } -> std::same_as<bool>;
    { const_r.make_worker(execution_context) } -> std::same_as<SuccessorGeneratorPtr<Kind>>;
    { const_r.get_task() } -> std::same_as<const TaskPtr<Kind>&>;
    { r.get_index() } -> std::same_as<ygg::uint_t>;
};
}

#endif
