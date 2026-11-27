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

#ifndef TYR_PLANNING_SUCCESSORS_SUCCESSORS_HPP_
#define TYR_PLANNING_SUCCESSORS_SUCCESSORS_HPP_

#include "tyr/declarations.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/nodes/labeled_node_view.hpp"
#include "tyr/planning/nodes/node_view.hpp"
#include "tyr/planning/plans/plan_builder.hpp"
#include "tyr/planning/states/state_builder.hpp"
#include "tyr/planning/states/state_delta.hpp"
#include "tyr/planning/successors/successor_generator.hpp"
#include "tyr/planning/successors/successor_builder.hpp"
#include "tyr/planning/successors/successor_view.hpp"
#include "tyr/planning/successors/successor_generator_concepts.hpp"

#include <functional>
#include <span>
#include <type_traits>
#include <vector>
#include <urd/elements/builder.hpp>
#include <urd/elements/index.hpp>
#include <yggdrasil/core/config.hpp>

// What a successor generator's results become. A generator never interns: it enumerates applicable bindings borrowed
// (for_each_applicable_action_binding) and writes a successor state into a builder (generate_successor_state), neither
// closed under the axioms nor registered. These make them lasting or complete; everything else (a single successor,
// successors enumerated, labeled or listed, packed nodes) is composed from them and the generator.

namespace tyr::planning
{

/// A successor state the generator wrote from a node (generate_successor_state): its state, a builder of the state
/// repository it was taken from; its auxiliary value, the cost term the metric reads; and its exact changes to the
/// node's state.
template<TaskKind Kind>
struct SuccessorState
{
    StateBuilderHandle<Kind> state;
    ygg::float_t auxiliary_value = 0;
    StateDelta delta;
};

/// The successors of `node` (of `repository`), kept: each a transition with its distributions of outcomes, its outcome
/// states registered and its label's action binding interned (register_outcomes, intern_label).
template<TaskKind Kind>
std::vector<OwnedSuccessorBuilderView<Kind>> get_successors(SuccessorGenerator<Kind>& generator,
                                                            StateRepository<Kind>& repository,
                                                            AxiomEvaluator<Kind>& axiom_evaluator,
                                                            const std::type_identity_t<NodeBuilderView<Kind>>& node);

/// Calls `callback` with each successor of `node`, written into a builder the callback may take (move), nothing
/// interned: its outcome states builders of the node's repository, its label's binding a row of its own where it was
/// borrowed. False from the callback stops; whether every successor was visited.
template<TaskKind Kind>
bool for_each_successor(SuccessorGenerator<Kind>& generator,
                        const std::type_identity_t<NodeBuilderView<Kind>>& node,
                        AxiomEvaluator<Kind>& axiom_evaluator,
                        const std::type_identity_t<std::function<bool(SuccessorBuilder<Kind>&)>>& callback);

/// Interns the action binding of the successor's label into the task's repository where it holds one of its own
/// (a borrowed binding copied, through a scratch row of `builder`); its outcome states stay as they are.
template<TaskKind Kind>
void intern_label(StateRepository<Kind>& repository, formalism::Builder& builder, SuccessorBuilder<Kind>& successor);

/// Calls `callback` with each applicable action binding of `node` that `filter` reports, borrowed (valid during the
/// callback; intern_action_binding keeps one). False from the callback stops; whether every binding was visited.
template<TaskKind Kind>
bool for_each_applicable_action_binding(SuccessorGenerator<Kind>& generator,
                                        const std::type_identity_t<NodeBuilderView<Kind>>& node,
                                        const std::type_identity_t<std::function<bool(ActionBindingView<Kind>)>>& callback,
                                        const std::type_identity_t<ActionBindingFilter<Kind>>& filter = {});

/// The successor state of `node` (of `repository`) by `binding` (interned or borrowed), in a builder taken from
/// `repository`, not closed under the axioms nor registered (register_successor).
template<TaskKind Kind, formalism::ActionRowViewConcept Binding>
SuccessorState<Kind> generate_successor_state(SuccessorGenerator<Kind>& generator,
                                              const StateRepository<Kind>& repository,
                                              const std::type_identity_t<NodeBuilderView<Kind>>& node,
                                              Binding binding);

/// The node of `successor_state`, generated from `node`, registered in `repository` as register_successor below.
template<TaskKind Kind>
NodeBuilder<Kind> register_successor(StateRepository<Kind>& repository,
                                     AxiomEvaluator<Kind>& axiom_evaluator,
                                     const std::type_identity_t<NodeBuilderView<Kind>>& node,
                                     SuccessorState<Kind> successor_state);

/// `binding` interned into the task's repository, after validating its schema, objects and arity (invalid_argument
/// otherwise): a label a caller keeps (a ground binding is its ground action's row, interned already).
template<TaskKind Kind>
formalism::ActionRowView intern_action_binding(SuccessorGenerator<Kind>& generator, formalism::Builder& builder, ActionBindingView<Kind> binding);

/// The node of the successor state `state` (a builder of `repository`) the generator wrote from `node` with auxiliary
/// value `auxiliary_value` and exact changes `delta`: its metric, and the state registered, closed under the axioms only
/// if it is new and stored as its changes to the state of `node` where that is registered.
template<TaskKind Kind>
NodeBuilder<Kind> register_successor(StateRepository<Kind>& repository,
                                     AxiomEvaluator<Kind>& axiom_evaluator,
                                     StateBuilderHandle<Kind> state,
                                     ygg::float_t auxiliary_value,
                                     const std::type_identity_t<NodeBuilderView<Kind>>& node,
                                     const StateDelta& delta);

/// The node of the successor state `state` (a builder of `node`'s repository) the generator wrote from `node` with
/// auxiliary value `auxiliary_value`: its metric, and the state closed under the axioms in place, not registered.
template<TaskKind Kind>
NodeBuilder<Kind> extend_successor(StateBuilderHandle<Kind> state,
                                   ygg::float_t auxiliary_value,
                                   const std::type_identity_t<NodeBuilderView<Kind>>& node,
                                   AxiomEvaluator<Kind>& axiom_evaluator);

/// Registers in `repository` the outcome states of `successor` (built from a node of it by generate_successor or
/// for_each_successor, closed under the axioms already) in place; a failed outcome's state stays unregistered.
template<TaskKind Kind>
void register_outcomes(StateRepository<Kind>& repository, SuccessorBuilder<Kind>& successor);

/// The initial node: the task's initial state, registered, and its metric, read through `repository` (its builder taken
/// out with release()).
template<TaskKind Kind>
OwnedNodeBuilderView<Kind> get_initial_node(StateRepository<Kind>& repository, AxiomEvaluator<Kind>& axiom_evaluator);

/// The node of a registered state: the state unpacked, and its metric.
template<TaskKind Kind>
NodeBuilder<Kind> get_node(const StateRepository<Kind>& repository, urd::index<State<Kind>> state_index);

/// A packed node unpacked: its state unpacked from `repository`, which registered it, and its metric.
template<TaskKind Kind>
NodeBuilder<Kind> get_node(const StateRepository<Kind>& repository, const urd::data<Node<Kind>>& node);

/// A packed labeled node unpacked: its label, and its node unpacked from `repository`.
template<TaskKind Kind>
LabeledNodeBuilder<Kind> get_labeled_node(const StateRepository<Kind>& repository, const urd::data<LabeledNode<Kind>>& node);

/// The plan from `initial` (a packed node of `states`) through a known applicable action sequence, each successor
/// registered.
template<TaskKind Kind>
PlanBuilder<Kind> replay_plan(const urd::data<Node<Kind>>& initial,
                              std::span<const formalism::ActionRowView> actions,
                              SuccessorGenerator<Kind>& generator,
                              StateRepository<Kind>& states,
                              AxiomEvaluator<Kind>& axioms);

}

#endif
