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

#ifndef TYR_PLANNING_LIFTED_SUCCESSOR_GENERATOR_HPP_
#define TYR_PLANNING_LIFTED_SUCCESSOR_GENERATOR_HPP_

#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/state_repository.hpp"
#include "tyr/planning/successor_builder.hpp"
#include "tyr/planning/successor_generator.hpp"
#include "tyr/planning/successor_view.hpp"

#include <functional>
#include <memory>
#include <type_traits>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <vector>

namespace tyr::planning
{

class ApplicableActionProgram;

template<>
class SuccessorGenerator<LiftedTag>
{
private:
    struct Impl;
    using Source = SourceNode<LiftedTag>;
    using BuilderNode = Node<LiftedTag, StateBuilderView<LiftedTag>>;
    using Builder = urd::builder<State<LiftedTag>, StateRepository<LiftedTag>>;
    using Bindings = urd::Store<urd::sequence<formalism::ActionRowView>>;
    using BindingData = urd::builder<ygg::Row<formalism::Action<LiftedTag>>, formalism::Repository>;

    explicit SuccessorGenerator(std::unique_ptr<Impl> impl) noexcept;

public:
    SuccessorGenerator(TaskPtr<LiftedTag> task, ygg::ExecutionContextPtr execution_context);
    ~SuccessorGenerator();

    SuccessorGenerator(const SuccessorGenerator&) = delete;
    SuccessorGenerator& operator=(const SuccessorGenerator&) = delete;
    SuccessorGenerator(SuccessorGenerator&&) noexcept;
    SuccessorGenerator& operator=(SuccessorGenerator&&) noexcept;

    Node<LiftedTag> get_initial_node(StateRepository<LiftedTag>& state_repository, AxiomEvaluator<LiftedTag>& axiom_evaluator);

    /// Checks full applicability without interning the offered binding. The borrowed object row must resolve to this task's canonical owners.
    /// Foreign states, schemas or objects and wrong arity throw invalid_argument. May be called from a binding callback for the
    /// callback's state; for another state it throws logic_error.
    /// Ground tuples absent from the compiled action set are INAPPLICABLE; lifted domain failures are OUTSIDE_PARAMETER_DOMAIN.
    /// Accepts repository-backed and data-backed action bindings without publishing them.
    template<ygg::RowViewConcept<formalism::Action<LiftedTag>> Binding>
    ActionBindingStatus check_action_binding(const Source& node, Binding binding);

    /// Retains a borrowed binding after validating schema, object ownership and arity, without checking applicability.
    formalism::ActionRowView materialize_action_binding(ActionBindingView<LiftedTag> binding);

    /// Checks the same contract as check_action_binding and publishes only an applicable binding.
    /// The returned binding borrows the task repository. May be called from a binding callback.
    template<ygg::RowViewConcept<formalism::Action<LiftedTag>> Binding>
    ActionBindingResult try_get_applicable_action_binding(const Source& node, Binding binding);

    // Successors are generated into builders: caller-owned builders give borrowed nodes; a state repository gives each
    // successor a fresh builder of its own and registers it. Sources are registered or builder nodes alike.
    // List storage appends builders and never clears them, preserving earlier returned views.
    // Borrowed outputs remain valid while their storage and task live and the builders are unchanged.
    // A single output builder must be distinct from the source builder.
    // Unlabeled successor API. Does not intern action bindings.
    NodeList<LiftedTag> get_successor_nodes(const Source& node, StateRepository<LiftedTag>& storage, AxiomEvaluator<LiftedTag>& axiom_evaluator);
    void
    get_successor_nodes(const Source& node, StateRepository<LiftedTag>& storage, AxiomEvaluator<LiftedTag>& axiom_evaluator, NodeList<LiftedTag>& out_nodes);
    NodeList<LiftedTag, StateBuilderView<LiftedTag>>
    get_successor_nodes(const Source& node, SuccessorBuilders<LiftedTag>& storage, AxiomEvaluator<LiftedTag>& axiom_evaluator);
    void get_successor_nodes(const Source& node,
                             SuccessorBuilders<LiftedTag>& storage,
                             AxiomEvaluator<LiftedTag>& axiom_evaluator,
                             NodeList<LiftedTag, StateBuilderView<LiftedTag>>& out_nodes);
    NodeList<LiftedTag> get_successor_nodes(const Source& node,
                                            formalism::ActionView<LiftedTag> action,
                                            StateRepository<LiftedTag>& storage,
                                            AxiomEvaluator<LiftedTag>& axiom_evaluator);
    void get_successor_nodes(const Source& node,
                             formalism::ActionView<LiftedTag> action,
                             StateRepository<LiftedTag>& storage,
                             AxiomEvaluator<LiftedTag>& axiom_evaluator,
                             NodeList<LiftedTag>& out_nodes);
    NodeList<LiftedTag, StateBuilderView<LiftedTag>> get_successor_nodes(const Source& node,
                                                                         formalism::ActionView<LiftedTag> action,
                                                                         SuccessorBuilders<LiftedTag>& storage,
                                                                         AxiomEvaluator<LiftedTag>& axiom_evaluator);
    void get_successor_nodes(const Source& node,
                             formalism::ActionView<LiftedTag> action,
                             SuccessorBuilders<LiftedTag>& storage,
                             AxiomEvaluator<LiftedTag>& axiom_evaluator,
                             NodeList<LiftedTag, StateBuilderView<LiftedTag>>& out_nodes);

    // Labeled successor API. Interns action bindings.
    LabeledNodeList<LiftedTag> get_labeled_successor_nodes(const Source& node, StateRepository<LiftedTag>& storage, AxiomEvaluator<LiftedTag>& axiom_evaluator);
    void get_labeled_successor_nodes(const Source& node,
                                     StateRepository<LiftedTag>& storage,
                                     AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                     LabeledNodeList<LiftedTag>& out_nodes);
    LabeledNodeList<LiftedTag, StateBuilderView<LiftedTag>>
    get_labeled_successor_nodes(const Source& node, SuccessorBuilders<LiftedTag>& storage, AxiomEvaluator<LiftedTag>& axiom_evaluator);
    void get_labeled_successor_nodes(const Source& node,
                                     SuccessorBuilders<LiftedTag>& storage,
                                     AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                     LabeledNodeList<LiftedTag, StateBuilderView<LiftedTag>>& out_nodes);
    LabeledNodeList<LiftedTag> get_labeled_successor_nodes(const Source& node,
                                                           formalism::ActionView<LiftedTag> action,
                                                           StateRepository<LiftedTag>& storage,
                                                           AxiomEvaluator<LiftedTag>& axiom_evaluator);
    void get_labeled_successor_nodes(const Source& node,
                                     formalism::ActionView<LiftedTag> action,
                                     StateRepository<LiftedTag>& storage,
                                     AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                     LabeledNodeList<LiftedTag>& out_nodes);
    LabeledNodeList<LiftedTag, StateBuilderView<LiftedTag>> get_labeled_successor_nodes(const Source& node,
                                                                                        formalism::ActionView<LiftedTag> action,
                                                                                        SuccessorBuilders<LiftedTag>& storage,
                                                                                        AxiomEvaluator<LiftedTag>& axiom_evaluator);
    void get_labeled_successor_nodes(const Source& node,
                                     formalism::ActionView<LiftedTag> action,
                                     SuccessorBuilders<LiftedTag>& storage,
                                     AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                     LabeledNodeList<LiftedTag, StateBuilderView<LiftedTag>>& out_nodes);

    // Single successors of an action, an interned binding, a borrowed binding (validated) or raw binding data (not interned).
    Node<LiftedTag> get_successor_node(const Source& node,
                                       formalism::ActionView<GroundTag> action,
                                       StateRepository<LiftedTag>& storage,
                                       AxiomEvaluator<LiftedTag>& axiom_evaluator);
    BuilderNode get_successor_node(const Source& node, formalism::ActionView<GroundTag> action, Builder& storage, AxiomEvaluator<LiftedTag>& axiom_evaluator);
    Node<LiftedTag>
    get_successor_node(const Source& node, formalism::ActionRowView binding, StateRepository<LiftedTag>& storage, AxiomEvaluator<LiftedTag>& axiom_evaluator);
    BuilderNode get_successor_node(const Source& node, formalism::ActionRowView binding, Builder& storage, AxiomEvaluator<LiftedTag>& axiom_evaluator);
    Node<LiftedTag> get_successor_node(const Source& node,
                                       formalism::ActionRowBuilderView binding,
                                       StateRepository<LiftedTag>& storage,
                                       AxiomEvaluator<LiftedTag>& axiom_evaluator);
    BuilderNode get_successor_node(const Source& node, formalism::ActionRowBuilderView binding, Builder& storage, AxiomEvaluator<LiftedTag>& axiom_evaluator);
    Node<LiftedTag>
    get_successor_node(const Source& node, const BindingData& binding, StateRepository<LiftedTag>& storage, AxiomEvaluator<LiftedTag>& axiom_evaluator);
    BuilderNode get_successor_node(const Source& node, const BindingData& binding, Builder& storage, AxiomEvaluator<LiftedTag>& axiom_evaluator);
    formalism::ActionView<GroundTag> ground_action(formalism::ActionRowView binding);

    // Successors with outcomes, over happenings: applying an instantaneous action binding, starting a durative one, or
    // ending an open one; their times stay flexible within the state's temporal network; no action is grounded. An action with
    // oneof or probabilistic effects has several outcomes, a deterministic one a distribution of one. Outcome states are
    // registered in `storage`. A successor is a builder checked out of this generator's pools, held by its handle, which
    // gives it back when it goes: it must not outlive the generator. A transition whose effects conflict in some joint outcome, or whose outcomes all violate a running
    // over-all condition, has no distributions (it is not applicable).
    SuccessorHandle<LiftedTag>
    get_successor(const Source& node, formalism::ActionRowView binding, StateRepository<LiftedTag>& storage, AxiomEvaluator<LiftedTag>& axiom_evaluator);
    /// The successor of ending the open durative action at position `open` among the node's (none if its end condition
    /// does not hold, or it cannot be scheduled).
    SuccessorHandle<LiftedTag>
    get_end_successor(const Source& node, std::uint32_t open, StateRepository<LiftedTag>& storage, AxiomEvaluator<LiftedTag>& axiom_evaluator);
    /// The successor of the next timed initial literals (none once all happened, or if they cannot be scheduled).
    SuccessorHandle<LiftedTag> get_timed_literal_successor(const Source& node, StateRepository<LiftedTag>& storage, AxiomEvaluator<LiftedTag>& axiom_evaluator);
    /// Calls `callback` with the successor of each applicable happening (the action bindings', the open actions' ends, then the next timed initial literals);
    /// true iff enumeration was exhausted.
    bool for_each_successor(const Source& node,
                            StateRepository<LiftedTag>& storage,
                            AxiomEvaluator<LiftedTag>& axiom_evaluator,
                            const std::function<bool(const SuccessorView<LiftedTag>&)>& callback);

    // Like the above, with the outcome states written into state builders checked out of this generator's pools and closed
    // under the axioms, not registered: each held by the successor (its outcome's state a built child).
    SuccessorHandle<LiftedTag>
    get_successor(const Source& node, formalism::ActionRowView binding, AxiomEvaluator<LiftedTag>& axiom_evaluator);
    SuccessorHandle<LiftedTag> get_end_successor(const Source& node, std::uint32_t open, AxiomEvaluator<LiftedTag>& axiom_evaluator);
    SuccessorHandle<LiftedTag> get_timed_literal_successor(const Source& node, AxiomEvaluator<LiftedTag>& axiom_evaluator);
    bool for_each_successor(const Source& node,
                            AxiomEvaluator<LiftedTag>& axiom_evaluator,
                            const std::function<bool(const SuccessorView<LiftedTag>&)>& callback);

    Bindings get_applicable_action_bindings(const Source& node);
    void get_applicable_action_bindings(const Source& node, Bindings& out_bindings);
    Bindings get_applicable_action_bindings(const Source& node, formalism::ActionView<LiftedTag> action);
    void get_applicable_action_bindings(const Source& node, formalism::ActionView<LiftedTag> action, Bindings& out_bindings);

    /// Callbacks return true to continue, false to stop. The result is true iff enumeration was exhausted.
    /// Enumeration must not reenter this generator's enumeration APIs: they share scratch storage, and reentering throws logic_error.
    /// Generating a single successor of the callback's state inside an action-binding callback is supported; another state throws
    /// logic_error.
    /// Builder successor callbacks reuse the supplied builder; copy results before the next callback.
    bool for_each_applicable_action_binding(const Source& node, const std::function<bool(formalism::ActionRowView)>& callback);
    bool for_each_applicable_action_binding(const Source& node,
                                            formalism::ActionView<LiftedTag> action,
                                            const std::function<bool(formalism::ActionRowView)>& callback);
    bool for_each_successor_node(const Source& node,
                                 StateRepository<LiftedTag>& storage,
                                 AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                 const std::function<bool(Node<LiftedTag>)>& callback);
    bool
    for_each_successor_node(const Source& node, Builder& storage, AxiomEvaluator<LiftedTag>& axiom_evaluator, const std::function<bool(BuilderNode)>& callback);
    bool for_each_successor_node(const Source& node,
                                 formalism::ActionView<LiftedTag> action,
                                 StateRepository<LiftedTag>& storage,
                                 AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                 const std::function<bool(Node<LiftedTag>)>& callback);
    bool for_each_successor_node(const Source& node,
                                 formalism::ActionView<LiftedTag> action,
                                 Builder& storage,
                                 AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                 const std::function<bool(BuilderNode)>& callback);
    bool for_each_labeled_successor_node(const Source& node,
                                         StateRepository<LiftedTag>& storage,
                                         AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                         const std::function<bool(LabeledNode<LiftedTag>)>& callback);
    bool for_each_labeled_successor_node(const Source& node,
                                         Builder& storage,
                                         AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                         const std::function<bool(LabeledNode<LiftedTag, StateBuilderView<LiftedTag>>)>& callback);
    bool for_each_labeled_successor_node(const Source& node,
                                         formalism::ActionView<LiftedTag> action,
                                         StateRepository<LiftedTag>& storage,
                                         AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                         const std::function<bool(LabeledNode<LiftedTag>)>& callback);
    bool for_each_labeled_successor_node(const Source& node,
                                         formalism::ActionView<LiftedTag> action,
                                         Builder& storage,
                                         AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                         const std::function<bool(LabeledNode<LiftedTag, StateBuilderView<LiftedTag>>)>& callback);

    /// Does not publish bindings. A lifted binding and its object row are valid only during the callback.
    /// Checks, materialization and single-successor generation preserve that borrowed binding.
    /// The same continuation and non-reentrancy rules as the indexed binding callbacks apply.
    bool for_each_borrowed_applicable_action_binding(const Source& node, const std::function<bool(ActionBindingView<LiftedTag>)>& callback);
    bool for_each_borrowed_applicable_action_binding(const Source& node,
                                                     formalism::ActionView<LiftedTag> action,
                                                     const std::function<bool(ActionBindingView<LiftedTag>)>& callback);

    /// The predicate inspects condition-matching candidates before effect compatibility is checked.
    /// Rejected candidates do not reach callback. stop is polled before enumeration and each candidate.
    /// Predicate bindings are borrowed for that call only; successor generation belongs in callback.
    bool for_each_borrowed_applicable_action_binding(const Source& node,
                                                     formalism::ActionView<LiftedTag> action,
                                                     const std::function<bool(ActionBindingView<LiftedTag>)>& accept,
                                                     const std::function<bool(ActionBindingView<LiftedTag>)>& callback,
                                                     const std::function<bool()>& stop);

    // Packed output retains registered state handles without retaining unpacked builders.
    PackedNode<LiftedTag> get_packed_initial_node(StateRepository<LiftedTag>& state_repository, AxiomEvaluator<LiftedTag>& axiom_evaluator);
    PackedNode<LiftedTag> get_packed_node(StateRepository<LiftedTag>& state_repository, urd::index<State<LiftedTag>> state_index);
    PackedNode<LiftedTag> get_packed_successor_node(const Source& node,
                                                    formalism::ActionRowView binding,
                                                    StateRepository<LiftedTag>& state_repository,
                                                    AxiomEvaluator<LiftedTag>& axiom_evaluator);
    PackedNode<LiftedTag> get_packed_successor_node(const Source& node,
                                                    formalism::ActionView<GroundTag> action,
                                                    StateRepository<LiftedTag>& state_repository,
                                                    AxiomEvaluator<LiftedTag>& axiom_evaluator);
    PackedNode<LiftedTag> get_packed_successor_node(const Source& node,
                                                    const BindingData& binding,
                                                    StateRepository<LiftedTag>& state_repository,
                                                    AxiomEvaluator<LiftedTag>& axiom_evaluator);
    PackedNodeList<LiftedTag>
    get_packed_successor_nodes(const Source& node, StateRepository<LiftedTag>& state_repository, AxiomEvaluator<LiftedTag>& axiom_evaluator);
    void get_packed_successor_nodes(const Source& node,
                                    StateRepository<LiftedTag>& state_repository,
                                    AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                    PackedNodeList<LiftedTag>& out_nodes);
    PackedNodeList<LiftedTag> get_packed_successor_nodes(const Source& node,
                                                         formalism::ActionView<LiftedTag> action,
                                                         StateRepository<LiftedTag>& state_repository,
                                                         AxiomEvaluator<LiftedTag>& axiom_evaluator);
    void get_packed_successor_nodes(const Source& node,
                                    formalism::ActionView<LiftedTag> action,
                                    StateRepository<LiftedTag>& state_repository,
                                    AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                    PackedNodeList<LiftedTag>& out_nodes);
    PackedLabeledNodeList<LiftedTag>
    get_packed_labeled_successor_nodes(const Source& node, StateRepository<LiftedTag>& state_repository, AxiomEvaluator<LiftedTag>& axiom_evaluator);
    void get_packed_labeled_successor_nodes(const Source& node,
                                            StateRepository<LiftedTag>& state_repository,
                                            AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                            PackedLabeledNodeList<LiftedTag>& out_nodes);
    PackedLabeledNodeList<LiftedTag> get_packed_labeled_successor_nodes(const Source& node,
                                                                        formalism::ActionView<LiftedTag> action,
                                                                        StateRepository<LiftedTag>& state_repository,
                                                                        AxiomEvaluator<LiftedTag>& axiom_evaluator);
    void get_packed_labeled_successor_nodes(const Source& node,
                                            formalism::ActionView<LiftedTag> action,
                                            StateRepository<LiftedTag>& state_repository,
                                            AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                            PackedLabeledNodeList<LiftedTag>& out_nodes);

    /// Writes an unregistered successor without axiom closure, and its exact changes relative to `node` into `out_delta`.
    /// Use get_successor_node() for a completed borrowed successor.
    /// Pass the same pooled builder and auxiliary value to finalize_successor_state() to intern it.
    ygg::float_t generate_successor_state(const Source& node, formalism::ActionRowView binding, Builder& out_state, StateDelta& out_delta);
    /// Computes axiom closure and the final metric, then interns the completed state.
    Node<LiftedTag> finalize_successor_state(StateRepository<LiftedTag>& state_repository,
                                             AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                             std::shared_ptr<Builder> state,
                                             ygg::float_t auxiliary_value);
    /// Like finalize_successor_state(), but `state` was generated from `parent` with exact changes `delta`.
    Node<LiftedTag> finalize_successor_state(StateRepository<LiftedTag>& state_repository,
                                             AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                             std::shared_ptr<Builder> state,
                                             ygg::float_t auxiliary_value,
                                             const StateView<LiftedTag>& parent,
                                             const StateDelta& delta);

    // Lookup
    Node<LiftedTag> get_node(StateRepository<LiftedTag>& state_repository, urd::index<State<LiftedTag>> state_index);
    [[nodiscard]] SuccessorGeneratorPtr<LiftedTag> make_worker(ygg::ExecutionContextPtr execution_context) const;

    // Diagnostics
    void print_summary(size_t verbosity) const;

    // Getters
    [[nodiscard]] const ApplicableActionProgram& get_action_program() const noexcept;
    [[nodiscard]] const TaskPtr<LiftedTag>& get_task() const noexcept;

private:
    std::unique_ptr<Impl> m_impl;
};

}

#endif
