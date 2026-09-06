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
#include <utility>
#include <vector>
#include <yggdrasil/containers/shared_object_pool.hpp>
#include <yggdrasil/core/concepts.hpp>
#include <yggdrasil/core/config.hpp>

namespace tyr::planning
{

template<TaskKind Kind>
class SuccessorGenerator;

struct PendingActionResult
{
    ygg::float_t auxiliary_value;
};

template<typename T, typename Kind>
concept SuccessorGeneratorConcept = requires(T& r,
                                             const T& const_r,
                                             ygg::Index<State<Kind>> state_index,
                                             const Node<Kind>& node,
                                             NodeList<Kind>& successor_nodes,
                                             LabeledNodeList<Kind>& labeled_successor_nodes,
                                             std::vector<formalism::planning::ActionBindingView>& action_bindings,
                                             formalism::planning::ActionBindingView binding,
                                             StateRepository<Kind>& state_repository,
                                             AxiomEvaluator<Kind>& axiom_evaluator,
                                             ygg::Builder<State<Kind>>& state_builder,
                                             ygg::SharedObjectPoolPtr<ygg::Builder<State<Kind>>, true> state_builder_ptr,
                                             PendingActionResult pending_result,
                                             ygg::ExecutionContextPtr execution_context) {
    requires TaskKind<Kind>;
    { r.get_initial_node(state_repository, axiom_evaluator) } -> std::same_as<Node<Kind>>;
    { r.get_successor_nodes(node, state_repository, axiom_evaluator) } -> std::same_as<NodeList<Kind>>;
    { r.get_successor_nodes(node, state_repository, axiom_evaluator, successor_nodes) } -> std::same_as<void>;
    { r.get_labeled_successor_nodes(node, state_repository, axiom_evaluator) } -> std::same_as<LabeledNodeList<Kind>>;
    { r.get_labeled_successor_nodes(node, state_repository, axiom_evaluator, labeled_successor_nodes) } -> std::same_as<void>;
    { r.get_applicable_action_bindings(node) } -> std::same_as<std::vector<formalism::planning::ActionBindingView>>;
    { r.get_applicable_action_bindings(node, action_bindings) } -> std::same_as<void>;
    { r.get_successor_node(node, binding, state_repository, axiom_evaluator) } -> std::same_as<Node<Kind>>;
    { r.generate_successor_state(node, binding, state_builder) } -> std::same_as<PendingActionResult>;
    { r.finalize_successor_state(state_repository, axiom_evaluator, std::move(state_builder_ptr), pending_result) } -> std::same_as<Node<Kind>>;
    { r.get_node(state_repository, state_index) } -> std::same_as<Node<Kind>>;
    { const_r.make_worker(execution_context) } -> std::same_as<SuccessorGeneratorPtr<Kind>>;
    { const_r.get_task() } -> std::same_as<const TaskPtr<Kind>&>;
    { r.get_index() } -> std::same_as<ygg::uint_t>;
};

}

#endif
