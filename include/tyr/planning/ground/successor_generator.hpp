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

#ifndef TYR_PLANNING_GROUND_SUCCESSOR_GENERATOR_HPP_
#define TYR_PLANNING_GROUND_SUCCESSOR_GENERATOR_HPP_

#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/state_repository.hpp"
#include "tyr/planning/successor_generator.hpp"

#include <functional>
#include <memory>
#include <type_traits>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <vector>

namespace tyr::planning
{

template<>
class SuccessorGenerator<GroundTag>
{
private:
    struct Impl;
    using Source = SourceNode<GroundTag>;
    using BuilderNode = Node<GroundTag, BuilderStateView<GroundTag>>;
    using Builder = urd::builder<State<GroundTag>>;
    using Bindings = urd::Store<urd::sequence<formalism::planning::ActionRowView>>;

    explicit SuccessorGenerator(std::unique_ptr<Impl> impl) noexcept;

public:
    /// Ground successors are generated without Datalog, so the execution context is not used.
    SuccessorGenerator(TaskPtr<GroundTag> task, const ygg::ExecutionContextPtr& execution_context);
    ~SuccessorGenerator();

    SuccessorGenerator(const SuccessorGenerator&) = delete;
    SuccessorGenerator& operator=(const SuccessorGenerator&) = delete;
    SuccessorGenerator(SuccessorGenerator&&) noexcept;
    SuccessorGenerator& operator=(SuccessorGenerator&&) noexcept;

    Node<GroundTag> get_initial_node(StateRepository<GroundTag>& state_repository, AxiomEvaluator<GroundTag>& axiom_evaluator);

    /// Checks full applicability without interning the offered binding. The borrowed object row must resolve to this task's canonical owners.
    /// Foreign states, schemas or objects and wrong arity throw invalid_argument. May be called from a binding callback.
    /// Ground tuples absent from the compiled action set are INAPPLICABLE; lifted domain failures are OUTSIDE_PARAMETER_DOMAIN.
    /// Accepts repository-backed and data-backed action bindings without publishing them.
    template<ygg::RowViewConcept<formalism::planning::Action<LiftedTag>> Binding>
    ActionBindingStatus check_action_binding(const Source& node, Binding binding);

    /// Retains a borrowed binding after validating schema, object ownership and arity, without checking applicability.
    formalism::planning::ActionRowView materialize_action_binding(BorrowedActionRowView<GroundTag> binding);

    /// Checks the same contract as check_action_binding and publishes only an applicable binding.
    /// The returned binding borrows the task repository. May be called from a binding callback.
    template<ygg::RowViewConcept<formalism::planning::Action<LiftedTag>> Binding>
    ActionBindingResult try_get_applicable_action_binding(const Source& node, Binding binding);

    // Successors are generated into builders: caller-owned builders give borrowed nodes; a state repository gives each
    // successor a fresh builder of its own and registers it. Sources are registered or builder nodes alike.
    // List storage appends builders and never clears them, preserving earlier returned views.
    // Borrowed outputs remain valid while their storage and task live and the builders are unchanged.
    // A single output builder must be distinct from the source builder.
    // Unlabeled successor API.
    NodeList<GroundTag> get_successor_nodes(const Source& node, StateRepository<GroundTag>& storage, AxiomEvaluator<GroundTag>& axiom_evaluator);
    NodeList<GroundTag, BuilderStateView<GroundTag>>
    get_successor_nodes(const Source& node, SuccessorBuilders<GroundTag>& storage, AxiomEvaluator<GroundTag>& axiom_evaluator);
    void
    get_successor_nodes(const Source& node, StateRepository<GroundTag>& storage, AxiomEvaluator<GroundTag>& axiom_evaluator, NodeList<GroundTag>& out_nodes);
    void get_successor_nodes(const Source& node,
                             SuccessorBuilders<GroundTag>& storage,
                             AxiomEvaluator<GroundTag>& axiom_evaluator,
                             NodeList<GroundTag, BuilderStateView<GroundTag>>& out_nodes);
    NodeList<GroundTag> get_successor_nodes(const Source& node,
                                            formalism::planning::ActionView<LiftedTag> action,
                                            StateRepository<GroundTag>& storage,
                                            AxiomEvaluator<GroundTag>& axiom_evaluator);
    NodeList<GroundTag, BuilderStateView<GroundTag>> get_successor_nodes(const Source& node,
                                                                         formalism::planning::ActionView<LiftedTag> action,
                                                                         SuccessorBuilders<GroundTag>& storage,
                                                                         AxiomEvaluator<GroundTag>& axiom_evaluator);
    void get_successor_nodes(const Source& node,
                             formalism::planning::ActionView<LiftedTag> action,
                             StateRepository<GroundTag>& storage,
                             AxiomEvaluator<GroundTag>& axiom_evaluator,
                             NodeList<GroundTag>& out_nodes);
    void get_successor_nodes(const Source& node,
                             formalism::planning::ActionView<LiftedTag> action,
                             SuccessorBuilders<GroundTag>& storage,
                             AxiomEvaluator<GroundTag>& axiom_evaluator,
                             NodeList<GroundTag, BuilderStateView<GroundTag>>& out_nodes);

    // Labeled successor API.
    LabeledNodeList<GroundTag> get_labeled_successor_nodes(const Source& node, StateRepository<GroundTag>& storage, AxiomEvaluator<GroundTag>& axiom_evaluator);
    LabeledNodeList<GroundTag, BuilderStateView<GroundTag>>
    get_labeled_successor_nodes(const Source& node, SuccessorBuilders<GroundTag>& storage, AxiomEvaluator<GroundTag>& axiom_evaluator);
    void get_labeled_successor_nodes(const Source& node,
                                     StateRepository<GroundTag>& storage,
                                     AxiomEvaluator<GroundTag>& axiom_evaluator,
                                     LabeledNodeList<GroundTag>& out_nodes);
    void get_labeled_successor_nodes(const Source& node,
                                     SuccessorBuilders<GroundTag>& storage,
                                     AxiomEvaluator<GroundTag>& axiom_evaluator,
                                     LabeledNodeList<GroundTag, BuilderStateView<GroundTag>>& out_nodes);
    LabeledNodeList<GroundTag> get_labeled_successor_nodes(const Source& node,
                                                           formalism::planning::ActionView<LiftedTag> action,
                                                           StateRepository<GroundTag>& storage,
                                                           AxiomEvaluator<GroundTag>& axiom_evaluator);
    LabeledNodeList<GroundTag, BuilderStateView<GroundTag>> get_labeled_successor_nodes(const Source& node,
                                                                                        formalism::planning::ActionView<LiftedTag> action,
                                                                                        SuccessorBuilders<GroundTag>& storage,
                                                                                        AxiomEvaluator<GroundTag>& axiom_evaluator);
    void get_labeled_successor_nodes(const Source& node,
                                     formalism::planning::ActionView<LiftedTag> action,
                                     StateRepository<GroundTag>& storage,
                                     AxiomEvaluator<GroundTag>& axiom_evaluator,
                                     LabeledNodeList<GroundTag>& out_nodes);
    void get_labeled_successor_nodes(const Source& node,
                                     formalism::planning::ActionView<LiftedTag> action,
                                     SuccessorBuilders<GroundTag>& storage,
                                     AxiomEvaluator<GroundTag>& axiom_evaluator,
                                     LabeledNodeList<GroundTag, BuilderStateView<GroundTag>>& out_nodes);

    Node<GroundTag> get_successor_node(const Source& node,
                                       formalism::planning::ActionRowView binding,
                                       StateRepository<GroundTag>& storage,
                                       AxiomEvaluator<GroundTag>& axiom_evaluator);
    BuilderNode
    get_successor_node(const Source& node, formalism::planning::ActionRowView binding, Builder& storage, AxiomEvaluator<GroundTag>& axiom_evaluator);
    Node<GroundTag> get_successor_node(const Source& node,
                                       formalism::planning::ActionView<GroundTag> action,
                                       StateRepository<GroundTag>& storage,
                                       AxiomEvaluator<GroundTag>& axiom_evaluator);
    BuilderNode
    get_successor_node(const Source& node, formalism::planning::ActionView<GroundTag> action, Builder& storage, AxiomEvaluator<GroundTag>& axiom_evaluator);
    [[nodiscard]] formalism::planning::ActionView<GroundTag> ground_action(formalism::planning::ActionRowView binding) const;

    Bindings get_applicable_action_bindings(const Source& node);
    void get_applicable_action_bindings(const Source& node, Bindings& out_bindings);
    Bindings get_applicable_action_bindings(const Source& node, formalism::planning::ActionView<LiftedTag> action);
    void get_applicable_action_bindings(const Source& node, formalism::planning::ActionView<LiftedTag> action, Bindings& out_bindings);

    /// Callbacks return true to continue, false to stop. The result is true iff enumeration was exhausted.
    /// Enumeration must not reenter this generator's enumeration APIs: they share scratch storage.
    /// Generating a single successor inside an action-binding callback is supported.
    /// Builder successor callbacks reuse the supplied builder; copy results before the next callback.
    bool for_each_applicable_action_binding(const Source& node, const std::function<bool(formalism::planning::ActionRowView)>& callback);
    bool for_each_successor_node(const Source& node,
                                 StateRepository<GroundTag>& storage,
                                 AxiomEvaluator<GroundTag>& axiom_evaluator,
                                 const std::function<bool(Node<GroundTag>)>& callback);
    bool
    for_each_successor_node(const Source& node, Builder& storage, AxiomEvaluator<GroundTag>& axiom_evaluator, const std::function<bool(BuilderNode)>& callback);
    bool for_each_labeled_successor_node(const Source& node,
                                         StateRepository<GroundTag>& storage,
                                         AxiomEvaluator<GroundTag>& axiom_evaluator,
                                         const std::function<bool(LabeledNode<GroundTag>)>& callback);
    bool for_each_labeled_successor_node(const Source& node,
                                         Builder& storage,
                                         AxiomEvaluator<GroundTag>& axiom_evaluator,
                                         const std::function<bool(LabeledNode<GroundTag, BuilderStateView<GroundTag>>)>& callback);
    bool for_each_applicable_action_binding(const Source& node,
                                            formalism::planning::ActionView<LiftedTag> action,
                                            const std::function<bool(formalism::planning::ActionRowView)>& callback);
    bool for_each_successor_node(const Source& node,
                                 formalism::planning::ActionView<LiftedTag> action,
                                 StateRepository<GroundTag>& storage,
                                 AxiomEvaluator<GroundTag>& axiom_evaluator,
                                 const std::function<bool(Node<GroundTag>)>& callback);
    bool for_each_successor_node(const Source& node,
                                 formalism::planning::ActionView<LiftedTag> action,
                                 Builder& storage,
                                 AxiomEvaluator<GroundTag>& axiom_evaluator,
                                 const std::function<bool(BuilderNode)>& callback);
    bool for_each_labeled_successor_node(const Source& node,
                                         formalism::planning::ActionView<LiftedTag> action,
                                         StateRepository<GroundTag>& storage,
                                         AxiomEvaluator<GroundTag>& axiom_evaluator,
                                         const std::function<bool(LabeledNode<GroundTag>)>& callback);
    bool for_each_labeled_successor_node(const Source& node,
                                         formalism::planning::ActionView<LiftedTag> action,
                                         Builder& storage,
                                         AxiomEvaluator<GroundTag>& axiom_evaluator,
                                         const std::function<bool(LabeledNode<GroundTag, BuilderStateView<GroundTag>>)>& callback);

    /// Does not publish bindings. A lifted binding and its object row are valid only during the callback.
    /// Checks, materialization and single-successor generation preserve that borrowed binding.
    /// The same continuation and non-reentrancy rules as the indexed binding callbacks apply.
    bool for_each_borrowed_applicable_action_binding(const Source& node, const std::function<bool(BorrowedActionRowView<GroundTag>)>& callback);
    bool for_each_borrowed_applicable_action_binding(const Source& node,
                                                     formalism::planning::ActionView<LiftedTag> action,
                                                     const std::function<bool(BorrowedActionRowView<GroundTag>)>& callback);

    /// The predicate inspects condition-matching candidates before effect compatibility is checked.
    /// Rejected candidates do not reach callback. stop is polled before enumeration and each candidate.
    /// Predicate bindings are borrowed for that call only; successor generation belongs in callback.
    bool for_each_borrowed_applicable_action_binding(const Source& node,
                                                     formalism::planning::ActionView<LiftedTag> action,
                                                     const std::function<bool(BorrowedActionRowView<GroundTag>)>& accept,
                                                     const std::function<bool(BorrowedActionRowView<GroundTag>)>& callback,
                                                     const std::function<bool()>& stop);

    // Packed output retains registered state handles without retaining unpacked builders.
    PackedNode<GroundTag> get_packed_initial_node(StateRepository<GroundTag>& state_repository, AxiomEvaluator<GroundTag>& axiom_evaluator);
    PackedNode<GroundTag> get_packed_node(StateRepository<GroundTag>& state_repository, urd::index<State<GroundTag>> state_index);
    PackedNode<GroundTag> get_packed_successor_node(const Source& node,
                                                    formalism::planning::ActionRowView binding,
                                                    StateRepository<GroundTag>& state_repository,
                                                    AxiomEvaluator<GroundTag>& axiom_evaluator);
    PackedNode<GroundTag> get_packed_successor_node(const Source& node,
                                                    formalism::planning::ActionView<GroundTag> action,
                                                    StateRepository<GroundTag>& state_repository,
                                                    AxiomEvaluator<GroundTag>& axiom_evaluator);
    PackedNodeList<GroundTag>
    get_packed_successor_nodes(const Source& node, StateRepository<GroundTag>& state_repository, AxiomEvaluator<GroundTag>& axiom_evaluator);
    void get_packed_successor_nodes(const Source& node,
                                    StateRepository<GroundTag>& state_repository,
                                    AxiomEvaluator<GroundTag>& axiom_evaluator,
                                    PackedNodeList<GroundTag>& out_nodes);
    PackedNodeList<GroundTag> get_packed_successor_nodes(const Source& node,
                                                         formalism::planning::ActionView<LiftedTag> action,
                                                         StateRepository<GroundTag>& state_repository,
                                                         AxiomEvaluator<GroundTag>& axiom_evaluator);
    void get_packed_successor_nodes(const Source& node,
                                    formalism::planning::ActionView<LiftedTag> action,
                                    StateRepository<GroundTag>& state_repository,
                                    AxiomEvaluator<GroundTag>& axiom_evaluator,
                                    PackedNodeList<GroundTag>& out_nodes);
    PackedLabeledNodeList<GroundTag>
    get_packed_labeled_successor_nodes(const Source& node, StateRepository<GroundTag>& state_repository, AxiomEvaluator<GroundTag>& axiom_evaluator);
    void get_packed_labeled_successor_nodes(const Source& node,
                                            StateRepository<GroundTag>& state_repository,
                                            AxiomEvaluator<GroundTag>& axiom_evaluator,
                                            PackedLabeledNodeList<GroundTag>& out_nodes);
    PackedLabeledNodeList<GroundTag> get_packed_labeled_successor_nodes(const Source& node,
                                                                        formalism::planning::ActionView<LiftedTag> action,
                                                                        StateRepository<GroundTag>& state_repository,
                                                                        AxiomEvaluator<GroundTag>& axiom_evaluator);
    void get_packed_labeled_successor_nodes(const Source& node,
                                            formalism::planning::ActionView<LiftedTag> action,
                                            StateRepository<GroundTag>& state_repository,
                                            AxiomEvaluator<GroundTag>& axiom_evaluator,
                                            PackedLabeledNodeList<GroundTag>& out_nodes);

    /// Writes an unregistered successor without axiom closure, and its exact changes relative to `node` into `out_delta`.
    /// Use get_successor_node() for a completed borrowed successor.
    /// Pass the same pooled builder and auxiliary value to finalize_successor_state() to intern it.
    ygg::float_t generate_successor_state(const Source& node, formalism::planning::ActionRowView binding, Builder& out_state, StateDelta& out_delta);
    /// Computes axiom closure and the final metric, then interns the completed state.
    Node<GroundTag> finalize_successor_state(StateRepository<GroundTag>& state_repository,
                                             AxiomEvaluator<GroundTag>& axiom_evaluator,
                                             std::shared_ptr<Builder> state,
                                             ygg::float_t auxiliary_value);
    /// Like finalize_successor_state(), but `state` was generated from `parent` with exact changes `delta`.
    Node<GroundTag> finalize_successor_state(StateRepository<GroundTag>& state_repository,
                                             AxiomEvaluator<GroundTag>& axiom_evaluator,
                                             std::shared_ptr<Builder> state,
                                             ygg::float_t auxiliary_value,
                                             const StateView<GroundTag>& parent,
                                             const StateDelta& delta);

    Node<GroundTag> get_node(StateRepository<GroundTag>& state_repository, urd::index<State<GroundTag>> state_index);
    [[nodiscard]] SuccessorGeneratorPtr<GroundTag> make_worker(const ygg::ExecutionContextPtr& execution_context) const;

    [[nodiscard]] const TaskPtr<GroundTag>& get_task() const noexcept;

private:
    std::unique_ptr<Impl> m_impl;
};

}

#endif
