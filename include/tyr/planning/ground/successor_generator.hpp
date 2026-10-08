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
#include "tyr/planning/state_view.hpp"
#include "tyr/planning/successor_generator.hpp"

#include <atomic>
#include <functional>
#include <memory>
#include <type_traits>
#include <vector>

namespace tyr::planning
{

template<>
class SuccessorGenerator<GroundTag>
{
    friend class SuccessorGeneratorFactory<GroundTag>;

private:
    struct Impl;

    SuccessorGenerator(ygg::uint_t index,
                       TaskPtr<GroundTag> task,
                       ygg::ExecutionContextPtr execution_context,
                       std::shared_ptr<std::atomic<ygg::uint_t>> next_index);

    explicit SuccessorGenerator(std::unique_ptr<Impl> impl) noexcept;

public:
    ~SuccessorGenerator();

    SuccessorGenerator(const SuccessorGenerator&) = delete;
    SuccessorGenerator& operator=(const SuccessorGenerator&) = delete;
    SuccessorGenerator(SuccessorGenerator&&) noexcept;
    SuccessorGenerator& operator=(SuccessorGenerator&&) noexcept;

    Node<GroundTag> get_initial_node(StateRepository<GroundTag>& state_repository, AxiomEvaluator<GroundTag>& axiom_evaluator);

    /// Checks full applicability without interning the offered binding. The borrowed object row must resolve to this task's canonical owners.
    /// Foreign states, schemas or objects and wrong arity throw invalid_argument. May be called from a binding callback.
    /// Ground tuples absent from the compiled action set are INAPPLICABLE; lifted domain failures are OUTSIDE_PARAMETER_DOMAIN.
    template<StateViewConcept<GroundTag> S>
    ActionBindingStatus
    check_action_binding(const Node<GroundTag, S>& node, formalism::planning::ActionView<LiftedTag> action, formalism::planning::ObjectSpanView objects);

    /// Accepts repository-backed and data-backed action bindings without publishing them.
    template<StateViewConcept<GroundTag> S,
             ygg::formalism::RelationBindingViewConcept<formalism::planning::Action<LiftedTag>, formalism::ObjectTag> Binding>
    ActionBindingStatus check_action_binding(const Node<GroundTag, S>& node, Binding binding);

    /// Retains a borrowed binding after validating schema, object ownership and arity, without checking applicability.
    formalism::planning::ActionBindingView materialize_action_binding(BorrowedActionBindingView<GroundTag> binding);

    /// Checks the same contract as check_action_binding and publishes only an applicable binding.
    /// The returned binding borrows the task repository. May be called from a binding callback.
    template<StateViewConcept<GroundTag> S>
    ActionBindingResult try_get_applicable_action_binding(const Node<GroundTag, S>& node,
                                                          formalism::planning::ActionView<LiftedTag> action,
                                                          formalism::planning::ObjectSpanView objects);

    // Indexed inputs intern successors; builder inputs write caller-owned storage.
    // List storage appends builders and never clears them, preserving earlier returned views.
    // Borrowed outputs remain valid while their storage and task live and the builders are unchanged.
    // A single output builder must be distinct from the source builder.
    // Unlabeled successor API.
    template<StateViewConcept<GroundTag> S>
    NodeList<GroundTag, S>
    get_successor_nodes(const Node<GroundTag, S>& node, SuccessorListStorage<GroundTag, S>& storage, AxiomEvaluator<GroundTag>& axiom_evaluator);
    template<StateViewConcept<GroundTag> S>
    void get_successor_nodes(const Node<GroundTag, S>& node,
                             SuccessorListStorage<GroundTag, S>& storage,
                             AxiomEvaluator<GroundTag>& axiom_evaluator,
                             NodeList<GroundTag, S>& out_nodes);
    template<StateViewConcept<GroundTag> S>
    NodeList<GroundTag, S> get_successor_nodes(const Node<GroundTag, S>& node,
                                               formalism::planning::ActionView<LiftedTag> action,
                                               SuccessorListStorage<GroundTag, S>& storage,
                                               AxiomEvaluator<GroundTag>& axiom_evaluator);
    template<StateViewConcept<GroundTag> S>
    void get_successor_nodes(const Node<GroundTag, S>& node,
                             formalism::planning::ActionView<LiftedTag> action,
                             SuccessorListStorage<GroundTag, S>& storage,
                             AxiomEvaluator<GroundTag>& axiom_evaluator,
                             NodeList<GroundTag, S>& out_nodes);

    // Labeled successor API.
    template<StateViewConcept<GroundTag> S>
    LabeledNodeList<GroundTag, S>
    get_labeled_successor_nodes(const Node<GroundTag, S>& node, SuccessorListStorage<GroundTag, S>& storage, AxiomEvaluator<GroundTag>& axiom_evaluator);
    template<StateViewConcept<GroundTag> S>
    void get_labeled_successor_nodes(const Node<GroundTag, S>& node,
                                     SuccessorListStorage<GroundTag, S>& storage,
                                     AxiomEvaluator<GroundTag>& axiom_evaluator,
                                     LabeledNodeList<GroundTag, S>& out_nodes);
    template<StateViewConcept<GroundTag> S>
    LabeledNodeList<GroundTag, S> get_labeled_successor_nodes(const Node<GroundTag, S>& node,
                                                              formalism::planning::ActionView<LiftedTag> action,
                                                              SuccessorListStorage<GroundTag, S>& storage,
                                                              AxiomEvaluator<GroundTag>& axiom_evaluator);
    template<StateViewConcept<GroundTag> S>
    void get_labeled_successor_nodes(const Node<GroundTag, S>& node,
                                     formalism::planning::ActionView<LiftedTag> action,
                                     SuccessorListStorage<GroundTag, S>& storage,
                                     AxiomEvaluator<GroundTag>& axiom_evaluator,
                                     LabeledNodeList<GroundTag, S>& out_nodes);

    template<StateViewConcept<GroundTag> S>
    Node<GroundTag, S> get_successor_node(const Node<GroundTag, S>& node,
                                          formalism::planning::ActionBindingView binding,
                                          SuccessorStorage<GroundTag, S>& storage,
                                          AxiomEvaluator<GroundTag>& axiom_evaluator);
    template<StateViewConcept<GroundTag> S>
    Node<GroundTag, S> get_successor_node(const Node<GroundTag, S>& node,
                                          formalism::planning::ActionView<GroundTag> action,
                                          SuccessorStorage<GroundTag, S>& storage,
                                          AxiomEvaluator<GroundTag>& axiom_evaluator);
    formalism::planning::ActionView<GroundTag> ground_action(formalism::planning::ActionBindingView binding) const;

    template<StateViewConcept<GroundTag> S>
    std::vector<formalism::planning::ActionBindingView> get_applicable_action_bindings(const Node<GroundTag, S>& node);
    template<StateViewConcept<GroundTag> S>
    void get_applicable_action_bindings(const Node<GroundTag, S>& node, std::vector<formalism::planning::ActionBindingView>& out_bindings);
    template<StateViewConcept<GroundTag> S>
    std::vector<formalism::planning::ActionBindingView> get_applicable_action_bindings(const Node<GroundTag, S>& node,
                                                                                       formalism::planning::ActionView<LiftedTag> action);
    template<StateViewConcept<GroundTag> S>
    void get_applicable_action_bindings(const Node<GroundTag, S>& node,
                                        formalism::planning::ActionView<LiftedTag> action,
                                        std::vector<formalism::planning::ActionBindingView>& out_bindings);

    /// Callbacks return true to continue, false to stop. The result is true iff enumeration was exhausted.
    /// Enumeration must not reenter this generator's enumeration APIs: they share scratch storage.
    /// Generating a single successor inside an action-binding callback is supported.
    /// Builder successor callbacks reuse the supplied builder; copy results before the next callback.
    template<StateViewConcept<GroundTag> S>
    bool for_each_applicable_action_binding(const Node<GroundTag, S>& node, const std::function<bool(formalism::planning::ActionBindingView)>& callback);
    template<StateViewConcept<GroundTag> S>
    bool for_each_successor_node(const Node<GroundTag, S>& node,
                                 SuccessorStorage<GroundTag, S>& storage,
                                 AxiomEvaluator<GroundTag>& axiom_evaluator,
                                 const std::type_identity_t<std::function<bool(Node<GroundTag, S>)>>& callback);
    template<StateViewConcept<GroundTag> S>
    bool for_each_labeled_successor_node(const Node<GroundTag, S>& node,
                                         SuccessorStorage<GroundTag, S>& storage,
                                         AxiomEvaluator<GroundTag>& axiom_evaluator,
                                         const std::type_identity_t<std::function<bool(LabeledNode<GroundTag, S>)>>& callback);
    template<StateViewConcept<GroundTag> S>
    bool for_each_applicable_action_binding(const Node<GroundTag, S>& node,
                                            formalism::planning::ActionView<LiftedTag> action,
                                            const std::function<bool(formalism::planning::ActionBindingView)>& callback);
    template<StateViewConcept<GroundTag> S>
    bool for_each_successor_node(const Node<GroundTag, S>& node,
                                 formalism::planning::ActionView<LiftedTag> action,
                                 SuccessorStorage<GroundTag, S>& storage,
                                 AxiomEvaluator<GroundTag>& axiom_evaluator,
                                 const std::type_identity_t<std::function<bool(Node<GroundTag, S>)>>& callback);
    template<StateViewConcept<GroundTag> S>
    bool for_each_labeled_successor_node(const Node<GroundTag, S>& node,
                                         formalism::planning::ActionView<LiftedTag> action,
                                         SuccessorStorage<GroundTag, S>& storage,
                                         AxiomEvaluator<GroundTag>& axiom_evaluator,
                                         const std::type_identity_t<std::function<bool(LabeledNode<GroundTag, S>)>>& callback);

    /// Does not publish bindings. A lifted binding and its object row are valid only during the callback.
    /// Checks, materialization and single-successor generation preserve that borrowed binding.
    /// The same continuation and non-reentrancy rules as the indexed binding callbacks apply.
    template<StateViewConcept<GroundTag> S>
    bool for_each_borrowed_applicable_action_binding(const Node<GroundTag, S>& node, const std::function<bool(BorrowedActionBindingView<GroundTag>)>& callback);
    template<StateViewConcept<GroundTag> S>
    bool for_each_borrowed_applicable_action_binding(const Node<GroundTag, S>& node,
                                                     formalism::planning::ActionView<LiftedTag> action,
                                                     const std::function<bool(BorrowedActionBindingView<GroundTag>)>& callback);

    /// The predicate inspects condition-matching candidates before effect compatibility is checked.
    /// Rejected candidates do not reach callback. stop is polled before enumeration and each candidate.
    /// Predicate bindings are borrowed for that call only; successor generation belongs in callback.
    template<StateViewConcept<GroundTag> S>
    bool for_each_borrowed_applicable_action_binding(const Node<GroundTag, S>& node,
                                                     formalism::planning::ActionView<LiftedTag> action,
                                                     const std::function<bool(BorrowedActionBindingView<GroundTag>)>& accept,
                                                     const std::function<bool(BorrowedActionBindingView<GroundTag>)>& callback,
                                                     const std::function<bool()>& stop);

    // Packed output retains registered state handles without retaining unpacked builders.
    PackedNode<GroundTag> get_packed_initial_node(StateRepository<GroundTag>& state_repository, AxiomEvaluator<GroundTag>& axiom_evaluator);
    PackedNode<GroundTag> get_packed_node(StateRepository<GroundTag>& state_repository, ygg::Index<State<GroundTag>> state_index);
    template<StateViewConcept<GroundTag> S>
    PackedNode<GroundTag> get_packed_successor_node(const Node<GroundTag, S>& node,
                                                    formalism::planning::ActionBindingView binding,
                                                    StateRepository<GroundTag>& state_repository,
                                                    AxiomEvaluator<GroundTag>& axiom_evaluator);
    template<StateViewConcept<GroundTag> S>
    PackedNode<GroundTag> get_packed_successor_node(const Node<GroundTag, S>& node,
                                                    formalism::planning::ActionView<GroundTag> action,
                                                    StateRepository<GroundTag>& state_repository,
                                                    AxiomEvaluator<GroundTag>& axiom_evaluator);
    template<StateViewConcept<GroundTag> S>
    PackedNodeList<GroundTag>
    get_packed_successor_nodes(const Node<GroundTag, S>& node, StateRepository<GroundTag>& state_repository, AxiomEvaluator<GroundTag>& axiom_evaluator);
    template<StateViewConcept<GroundTag> S>
    void get_packed_successor_nodes(const Node<GroundTag, S>& node,
                                    StateRepository<GroundTag>& state_repository,
                                    AxiomEvaluator<GroundTag>& axiom_evaluator,
                                    PackedNodeList<GroundTag>& out_nodes);
    template<StateViewConcept<GroundTag> S>
    PackedNodeList<GroundTag> get_packed_successor_nodes(const Node<GroundTag, S>& node,
                                                         formalism::planning::ActionView<LiftedTag> action,
                                                         StateRepository<GroundTag>& state_repository,
                                                         AxiomEvaluator<GroundTag>& axiom_evaluator);
    template<StateViewConcept<GroundTag> S>
    void get_packed_successor_nodes(const Node<GroundTag, S>& node,
                                    formalism::planning::ActionView<LiftedTag> action,
                                    StateRepository<GroundTag>& state_repository,
                                    AxiomEvaluator<GroundTag>& axiom_evaluator,
                                    PackedNodeList<GroundTag>& out_nodes);
    template<StateViewConcept<GroundTag> S>
    PackedLabeledNodeList<GroundTag> get_packed_labeled_successor_nodes(const Node<GroundTag, S>& node,
                                                                        StateRepository<GroundTag>& state_repository,
                                                                        AxiomEvaluator<GroundTag>& axiom_evaluator);
    template<StateViewConcept<GroundTag> S>
    void get_packed_labeled_successor_nodes(const Node<GroundTag, S>& node,
                                            StateRepository<GroundTag>& state_repository,
                                            AxiomEvaluator<GroundTag>& axiom_evaluator,
                                            PackedLabeledNodeList<GroundTag>& out_nodes);
    template<StateViewConcept<GroundTag> S>
    PackedLabeledNodeList<GroundTag> get_packed_labeled_successor_nodes(const Node<GroundTag, S>& node,
                                                                        formalism::planning::ActionView<LiftedTag> action,
                                                                        StateRepository<GroundTag>& state_repository,
                                                                        AxiomEvaluator<GroundTag>& axiom_evaluator);
    template<StateViewConcept<GroundTag> S>
    void get_packed_labeled_successor_nodes(const Node<GroundTag, S>& node,
                                            formalism::planning::ActionView<LiftedTag> action,
                                            StateRepository<GroundTag>& state_repository,
                                            AxiomEvaluator<GroundTag>& axiom_evaluator,
                                            PackedLabeledNodeList<GroundTag>& out_nodes);

    /// Writes an unregistered successor without axiom closure. Use get_successor_node() for a completed borrowed successor.
    /// Pass the same pooled builder and auxiliary value to finalize_successor_state() to intern it.
    template<StateViewConcept<GroundTag> S>
    ygg::float_t
    generate_successor_state(const Node<GroundTag, S>& node, formalism::planning::ActionBindingView binding, ygg::Builder<State<GroundTag>>& out_state);
    /// Computes axiom closure and the final metric, then interns the completed state.
    Node<GroundTag> finalize_successor_state(StateRepository<GroundTag>& state_repository,
                                             AxiomEvaluator<GroundTag>& axiom_evaluator,
                                             ygg::SharedObjectPoolPtr<ygg::Builder<State<GroundTag>>, true> state,
                                             ygg::float_t auxiliary_value);

    Node<GroundTag> get_node(StateRepository<GroundTag>& state_repository, ygg::Index<State<GroundTag>> state_index);
    [[nodiscard]] SuccessorGeneratorPtr<GroundTag> make_worker(ygg::ExecutionContextPtr execution_context) const;

    const TaskPtr<GroundTag>& get_task() const noexcept;
    ygg::uint_t get_index() const noexcept;

private:
    std::unique_ptr<Impl> m_impl;
};

static_assert(SuccessorGeneratorConcept<SuccessorGenerator<GroundTag>, GroundTag>);

}

#endif
