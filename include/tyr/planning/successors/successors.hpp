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
#include "tyr/planning/successors/successor_builder.hpp"
#include "tyr/planning/successors/successor_generator.hpp"
#include "tyr/planning/successors/successor_generator_concepts.hpp"
#include "tyr/planning/successors/successor_view.hpp"

#include <concepts>
#include <cstddef>
#include <functional>
#include <optional>
#include <span>
#include <type_traits>
#include <urd/elements/builder.hpp>
#include <urd/elements/index.hpp>
#include <vector>
#include <yggdrasil/core/config.hpp>

// The successors of a node, in steps that stay apart. A successor generator, one per task and kind, generates: it writes a
// transition's successor, its states neither closed under the axioms nor registered, and interns nothing. With
// interning, three steps follow, each its own function: the lookup (find_successor, find_outcomes) gives a state that is
// registered already its index and its derived atoms; the axiom step (extend_successor, extend_outcomes), the only one
// that takes an axiom evaluator, closes a state that has no index yet; registering (register_successor,
// register_outcomes) stores what is new, and interning a label keeps its binding (intern_label, intern_action_binding).
// Without interning, the axiom step alone follows. The generating functions name the environment they work in as their
// first template argument (Environment: what nature does once a transition is chosen) and give a transition's successor
// in that environment's shape (SuccessorOf). One function enumerates, for_each_successor: its options prune, reorder,
// pick and stop, and without options it gives every successor.

namespace tyr::planning
{

/// A transition a node offers, borrowed (valid during the callback that receives it): its kind, the action binding it
/// applies or starts (none for an end and for timed initial literals) and, for an end, the position of the open action
/// among its state's.
template<TaskKind Kind>
struct TransitionLabel
{
    TransitionKind kind = TransitionKind::APPLY;
    std::optional<ActionBindingView<Kind>> binding;
    std::uint32_t open = 0;
};

/// A deterministic transition's successor as the generator wrote it from a node: its state, a builder of the node's state
/// repository, neither closed under the axioms nor registered (find_successor, extend_successor and register_successor
/// do that, each its own step); its auxiliary value, the cost term the metric reads; and its exact changes to the node's
/// state.
template<TaskKind Kind>
struct SuccessorState
{
    StateBuilderHandle<Kind> state;
    ygg::float_t auxiliary_value = 0;
    StateDelta delta;
};

/// The successor of a transition in environment E: a deterministic one its successor state (SuccessorState); one where
/// nature acts a successor builder of that environment (SuccessorBuilder<E, Kind>: its outcomes, or in the general
/// environment its distributions of them). Either way its states are neither closed under the axioms nor registered.
template<typename E, typename Kind>
struct successor_of;

template<TaskKind Kind>
struct successor_of<DeterministicTag, Kind>
{
    using type = SuccessorState<Kind>;
};

template<NatureEnvironment E, TaskKind Kind>
struct successor_of<E, Kind>
{
    using type = SuccessorBuilder<E, Kind>;
};

template<Environment E, TaskKind Kind>
using SuccessorOf = typename successor_of<E, Kind>::type;

/// What an enumeration of a node's successors does beyond visiting each (for_each_successor), every member optional:
/// - `action`: only the bindings of this action (and then no end of an open action, no timed initial literals);
/// - `accept`: each candidate binding is inspected first, before its effects are checked and its successor generated; a
///   rejected one is not reported (pruning by label);
/// - `arrange`: called once every transition is known, with their labels (borrowed for the call) and `order` holding
///   their positions as enumerated; it rewrites `order` to the positions to generate, in the order to generate them (a
///   shuffle, one picked by a policy or at random, none). Without it nothing is buffered: each successor is generated as
///   its transition is found;
/// - `stop`: polled before the enumeration and before each candidate; true ends it.
template<TaskKind Kind>
struct SuccessorOptions : ActionBindingFilter<Kind>
{
    std::function<void(std::span<const TransitionLabel<Kind>> labels, std::vector<std::size_t>& order)> arrange;
};

/// Throws std::invalid_argument unless environment E gives the generator's task its exact successors (supports<E>): an
/// algorithm calls it once, when it is set up.
template<Environment E, TaskKind Kind>
void require_environment(const SuccessorGenerator<Kind>& generator);

/// Calls `callback` with each transition of `node` that `options` let through, and its successor in environment E: its
/// applicable action bindings (an instantaneous action applied, a durative one started), then, in a temporal task, the end
/// of each open action and the next timed initial literals, where they can happen. Nothing is closed under the axioms,
/// registered or interned (the steps that follow do that). The label is borrowed, valid during
/// the callback (intern_action_binding keeps its binding); the successor is written into one builder reused across the
/// transitions, which the callback may take (a deterministic one's state by moving its handle; another by urd::take).
/// False from the callback stops; whether every transition was visited. It must not be reentered for the same
/// generator.
template<Environment E, TaskKind Kind>
bool for_each_successor(SuccessorGenerator<Kind>& generator,
                        const std::type_identity_t<NodeBuilderView<Kind>>& node,
                        const std::type_identity_t<std::function<bool(const TransitionLabel<Kind>&, SuccessorOf<E, Kind>&)>>& callback,
                        const std::type_identity_t<SuccessorOptions<Kind>>& options = {});

/// Writes into `out` the successor of `node` by one known `transition` in environment E, without enumerating the node
/// (a plan replayed, a chosen action applied); whether the transition can happen (false: `out` holds nothing to read). A
/// deterministic `out` has its state, a builder of the node's repository (get_state_builder), written; any other is a
/// successor builder of that repository. Nothing is closed under the axioms or registered.
template<Environment E, TaskKind Kind>
bool generate_successor(SuccessorGenerator<Kind>& generator,
                        const std::type_identity_t<NodeBuilderView<Kind>>& node,
                        const std::type_identity_t<TransitionLabel<Kind>>& transition,
                        std::type_identity_t<SuccessorOf<E, Kind>>& out);

/// The lookup for a successor where nature acts: each outcome whose state is registered in `repository` already is given
/// that state's index and its derived atoms (find_state), so its axioms are not evaluated again.
template<NatureEnvironment E, TaskKind Kind>
void find_outcomes(StateRepository<Kind>& repository, SuccessorBuilder<E, Kind>& successor);

/// The axiom step for a successor where nature acts: closes under the axioms, in place, the state of each outcome that
/// has no index yet (builders of `repository`; one the lookup found is closed already), and marks as failed an outcome
/// in which an open durative action's over-all condition does not hold (a plan reaching it is invalid). Whether the
/// transition is applicable: if every outcome failed it is not, and the successor is left with nothing to pick from.
template<NatureEnvironment E, TaskKind Kind>
bool extend_outcomes(SuccessorGenerator<Kind>& generator,
                     const StateRepository<Kind>& repository,
                     AxiomEvaluator<Kind>& axiom_evaluator,
                     SuccessorBuilder<E, Kind>& successor);

/// Registering for a successor where nature acts: registers in `repository`, in place, the state of each outcome that
/// did not fail and has no index yet. The states hold their axiom closure (extend_outcomes): no axiom is evaluated here.
template<NatureEnvironment E, TaskKind Kind>
void register_outcomes(StateRepository<Kind>& repository, SuccessorBuilder<E, Kind>& successor);

/// Every step at once: the applicable successors of `node` (of `repository`) in an environment where nature acts, kept,
/// each a transition with its outcomes (in the general environment its distributions of them), their states closed
/// under the axioms. With `intern` (the default) the states are registered and each label's action binding is interned
/// (find_outcomes, extend_outcomes, register_outcomes, intern_label); without, nothing is: the states stay builders and
/// a borrowed binding a row of the label's own (extend_outcomes).
template<NatureEnvironment E, TaskKind Kind>
std::vector<OwnedSuccessorBuilderView<E, Kind>> get_successors(SuccessorGenerator<Kind>& generator,
                                                               StateRepository<Kind>& repository,
                                                               AxiomEvaluator<Kind>& axiom_evaluator,
                                                               const std::type_identity_t<NodeBuilderView<Kind>>& node,
                                                               bool intern = true);

/// Every step at once in the deterministic environment: the successors of `node` (of `repository`), kept, each a labeled
/// node, its state closed under the axioms. With `intern` (the default) the state is registered and the label's action
/// binding interned (find_successor, extend_successor, register_successor, intern_action_binding); without, nothing is:
/// the state stays a builder and a borrowed binding a row of the label's own (extend_successor, successor_node).
template<std::same_as<DeterministicTag> E, TaskKind Kind>
std::vector<OwnedLabeledNodeBuilderView<Kind>> get_successors(SuccessorGenerator<Kind>& generator,
                                                              StateRepository<Kind>& repository,
                                                              AxiomEvaluator<Kind>& axiom_evaluator,
                                                              const std::type_identity_t<NodeBuilderView<Kind>>& node,
                                                              bool intern = true);

/// Interns the action binding of the successor's label into the task's repository where it holds one of its own
/// (a borrowed binding copied, through a scratch row of `builder`); its outcome states stay as they are.
template<NatureEnvironment E, TaskKind Kind>
void intern_label(StateRepository<Kind>& repository, formalism::Builder& builder, SuccessorBuilder<E, Kind>& successor);

/// The lookup for a deterministic successor state of `node` in `repository` (the node's, or another of its task, whose
/// builder the successor state then is): whether a state of its identity is registered already. If so, the successor
/// state is given that state's index and its derived atoms (find_state), so its axioms are not evaluated again.
template<TaskKind Kind>
bool find_successor(StateRepository<Kind>& repository, const std::type_identity_t<NodeBuilderView<Kind>>& node, SuccessorState<Kind>& successor_state);

/// The axiom step for a deterministic successor state of `node`: closes it under the axioms in place, unless it has its
/// index already (the lookup found it registered: it holds its derived atoms).
template<TaskKind Kind>
void extend_successor(AxiomEvaluator<Kind>& axiom_evaluator, const std::type_identity_t<NodeBuilderView<Kind>>& node, SuccessorState<Kind>& successor_state);

/// Registering for a deterministic successor state of `node`, which holds its axiom closure (the lookup found it, or
/// extend_successor closed it): its node, its state registered in `repository` unless it is already (stored as its
/// changes to the state of `node` where that is registered in the same storage), and its metric. The successor state
/// is consumed: its state becomes the node's. No axiom is evaluated here.
template<TaskKind Kind>
NodeBuilder<Kind>
register_successor(StateRepository<Kind>& repository, const std::type_identity_t<NodeBuilderView<Kind>>& node, SuccessorState<Kind>& successor_state);

/// Without interning: the node of a deterministic successor state of `node`, its state as it is (closed under the
/// axioms by extend_successor, registered nowhere) and its metric. The successor state is consumed.
template<TaskKind Kind>
NodeBuilder<Kind> successor_node(const std::type_identity_t<NodeBuilderView<Kind>>& node, SuccessorState<Kind>& successor_state);

/// `binding` interned into the task's repository, after validating its schema, objects and arity (invalid_argument
/// otherwise): a label a caller keeps (a ground binding is its ground action's row, interned already).
template<TaskKind Kind>
formalism::ActionRowView intern_action_binding(SuccessorGenerator<Kind>& generator, formalism::Builder& builder, ActionBindingView<Kind> binding);

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
PlanBuilder<DeterministicTag, Kind> replay_plan(const urd::data<Node<Kind>>& initial,
                                                std::span<const formalism::ActionRowView> actions,
                                                SuccessorGenerator<Kind>& generator,
                                                StateRepository<Kind>& states,
                                                AxiomEvaluator<Kind>& axioms);

}

#endif
