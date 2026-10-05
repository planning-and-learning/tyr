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
#include "tyr/planning/lifted/state_view.hpp"
#include "tyr/planning/programs/action.hpp"
#include "tyr/planning/successor_generator.hpp"

#include <atomic>
#include <functional>
#include <memory>
#include <type_traits>
#include <vector>

namespace tyr::planning
{

template<>
class SuccessorGenerator<LiftedTag>
{
    friend class SuccessorGeneratorFactory<LiftedTag>;

private:
    struct Impl;

    SuccessorGenerator(ygg::uint_t index,
                       TaskPtr<LiftedTag> task,
                       ygg::ExecutionContextPtr execution_context,
                       std::shared_ptr<std::atomic<ygg::uint_t>> next_index);

    explicit SuccessorGenerator(std::unique_ptr<Impl> impl) noexcept;

public:
    ~SuccessorGenerator();

    SuccessorGenerator(const SuccessorGenerator&) = delete;
    SuccessorGenerator& operator=(const SuccessorGenerator&) = delete;
    SuccessorGenerator(SuccessorGenerator&&) noexcept;
    SuccessorGenerator& operator=(SuccessorGenerator&&) noexcept;

    Node<StateView<LiftedTag>> get_initial_node(StateRepository<LiftedTag>& state_repository, AxiomEvaluator<LiftedTag>& axiom_evaluator);

    /// Checks full applicability without interning the offered binding. The borrowed object row must resolve to this task's canonical owners.
    /// Foreign states, schemas or objects and wrong arity throw invalid_argument. May be called from a binding callback.
    /// Ground tuples absent from the compiled action set are INAPPLICABLE; lifted domain failures are OUTSIDE_PARAMETER_DOMAIN.
    template<StateViewConcept<LiftedTag> S>
    ActionBindingStatus
    check_action_binding(const Node<S>& node, formalism::planning::ActionView<LiftedTag> action, formalism::planning::ObjectSpanView objects);

    /// Checks the same contract as check_action_binding and publishes only an applicable binding.
    /// The returned binding borrows the task repository. May be called from a binding callback.
    template<StateViewConcept<LiftedTag> S>
    ActionBindingResult
    try_get_applicable_action_binding(const Node<S>& node, formalism::planning::ActionView<LiftedTag> action, formalism::planning::ObjectSpanView objects);

    // Indexed inputs intern successors; builder inputs write caller-owned storage.
    // List storage appends builders and never clears them, preserving earlier returned views.
    // Borrowed outputs remain valid while their storage and task live and the builders are unchanged.
    // A single output builder must be distinct from the source builder.
    // Unlabeled successor API. Does not intern action bindings.
    template<StateViewConcept<LiftedTag> S>
    NodeList<S> get_successor_nodes(const Node<S>& node, SuccessorListStorage<S>& storage, AxiomEvaluator<LiftedTag>& axiom_evaluator);
    template<StateViewConcept<LiftedTag> S>
    void get_successor_nodes(const Node<S>& node, SuccessorListStorage<S>& storage, AxiomEvaluator<LiftedTag>& axiom_evaluator, NodeList<S>& out_nodes);
    template<StateViewConcept<LiftedTag> S>
    NodeList<S> get_successor_nodes(const Node<S>& node,
                                    formalism::planning::ActionView<LiftedTag> action,
                                    SuccessorListStorage<S>& storage,
                                    AxiomEvaluator<LiftedTag>& axiom_evaluator);
    template<StateViewConcept<LiftedTag> S>
    void get_successor_nodes(const Node<S>& node,
                             formalism::planning::ActionView<LiftedTag> action,
                             SuccessorListStorage<S>& storage,
                             AxiomEvaluator<LiftedTag>& axiom_evaluator,
                             NodeList<S>& out_nodes);

    // Labeled successor API. Interns action bindings.
    template<StateViewConcept<LiftedTag> S>
    LabeledNodeList<S> get_labeled_successor_nodes(const Node<S>& node, SuccessorListStorage<S>& storage, AxiomEvaluator<LiftedTag>& axiom_evaluator);
    template<StateViewConcept<LiftedTag> S>
    void get_labeled_successor_nodes(const Node<S>& node,
                                     SuccessorListStorage<S>& storage,
                                     AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                     LabeledNodeList<S>& out_nodes);
    template<StateViewConcept<LiftedTag> S>
    LabeledNodeList<S> get_labeled_successor_nodes(const Node<S>& node,
                                                   formalism::planning::ActionView<LiftedTag> action,
                                                   SuccessorListStorage<S>& storage,
                                                   AxiomEvaluator<LiftedTag>& axiom_evaluator);
    template<StateViewConcept<LiftedTag> S>
    void get_labeled_successor_nodes(const Node<S>& node,
                                     formalism::planning::ActionView<LiftedTag> action,
                                     SuccessorListStorage<S>& storage,
                                     AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                     LabeledNodeList<S>& out_nodes);

    template<StateViewConcept<LiftedTag> S>
    Node<S> get_successor_node(const Node<S>& node,
                               formalism::planning::ActionView<GroundTag> action,
                               SuccessorStorage<S>& storage,
                               AxiomEvaluator<LiftedTag>& axiom_evaluator);
    formalism::planning::ActionView<GroundTag> ground_action(formalism::planning::ActionBindingView binding);

    // Interned action-binding input; state storage follows S.
    template<StateViewConcept<LiftedTag> S>
    Node<S> get_successor_node(const Node<S>& node,
                               formalism::planning::ActionBindingView binding,
                               SuccessorStorage<S>& storage,
                               AxiomEvaluator<LiftedTag>& axiom_evaluator);

    template<StateViewConcept<LiftedTag> S>
    std::vector<formalism::planning::ActionBindingView> get_applicable_action_bindings(const Node<S>& node);

    template<StateViewConcept<LiftedTag> S>
    void get_applicable_action_bindings(const Node<S>& node, std::vector<formalism::planning::ActionBindingView>& out_bindings);
    template<StateViewConcept<LiftedTag> S>
    std::vector<formalism::planning::ActionBindingView> get_applicable_action_bindings(const Node<S>& node, formalism::planning::ActionView<LiftedTag> action);
    template<StateViewConcept<LiftedTag> S>
    void get_applicable_action_bindings(const Node<S>& node,
                                        formalism::planning::ActionView<LiftedTag> action,
                                        std::vector<formalism::planning::ActionBindingView>& out_bindings);

    /// Callbacks return true to continue, false to stop. The result is true iff enumeration was exhausted.
    /// Enumeration must not reenter this generator's enumeration APIs: they share scratch storage.
    /// Generating a single successor inside an action-binding callback is supported.
    /// Builder successor callbacks reuse the supplied builder; copy results before the next callback.
    template<StateViewConcept<LiftedTag> S>
    bool for_each_applicable_action_binding(const Node<S>& node, const std::function<bool(formalism::planning::ActionBindingView)>& callback);
    template<StateViewConcept<LiftedTag> S>
    bool for_each_successor_node(const Node<S>& node,
                                 SuccessorStorage<S>& storage,
                                 AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                 const std::type_identity_t<std::function<bool(Node<S>)>>& callback);
    template<StateViewConcept<LiftedTag> S>
    bool for_each_labeled_successor_node(const Node<S>& node,
                                         SuccessorStorage<S>& storage,
                                         AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                         const std::type_identity_t<std::function<bool(LabeledNode<S>)>>& callback);
    template<StateViewConcept<LiftedTag> S>
    bool for_each_applicable_action_binding(const Node<S>& node,
                                            formalism::planning::ActionView<LiftedTag> action,
                                            const std::function<bool(formalism::planning::ActionBindingView)>& callback);
    template<StateViewConcept<LiftedTag> S>
    bool for_each_successor_node(const Node<S>& node,
                                 formalism::planning::ActionView<LiftedTag> action,
                                 SuccessorStorage<S>& storage,
                                 AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                 const std::type_identity_t<std::function<bool(Node<S>)>>& callback);
    template<StateViewConcept<LiftedTag> S>
    bool for_each_labeled_successor_node(const Node<S>& node,
                                         formalism::planning::ActionView<LiftedTag> action,
                                         SuccessorStorage<S>& storage,
                                         AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                         const std::type_identity_t<std::function<bool(LabeledNode<S>)>>& callback);

    // Packed output retains registered state handles without retaining unpacked builders.
    PackedNode<LiftedTag> get_packed_initial_node(StateRepository<LiftedTag>& state_repository, AxiomEvaluator<LiftedTag>& axiom_evaluator);
    PackedNode<LiftedTag> get_packed_node(StateRepository<LiftedTag>& state_repository, ygg::Index<State<LiftedTag>> state_index);
    template<StateViewConcept<LiftedTag> S>
    PackedNode<LiftedTag> get_packed_successor_node(const Node<S>& node,
                                                    formalism::planning::ActionBindingView binding,
                                                    StateRepository<LiftedTag>& state_repository,
                                                    AxiomEvaluator<LiftedTag>& axiom_evaluator);
    template<StateViewConcept<LiftedTag> S>
    PackedNode<LiftedTag> get_packed_successor_node(const Node<S>& node,
                                                    formalism::planning::ActionView<GroundTag> action,
                                                    StateRepository<LiftedTag>& state_repository,
                                                    AxiomEvaluator<LiftedTag>& axiom_evaluator);
    template<StateViewConcept<LiftedTag> S>
    PackedNodeList<LiftedTag>
    get_packed_successor_nodes(const Node<S>& node, StateRepository<LiftedTag>& state_repository, AxiomEvaluator<LiftedTag>& axiom_evaluator);
    template<StateViewConcept<LiftedTag> S>
    void get_packed_successor_nodes(const Node<S>& node,
                                    StateRepository<LiftedTag>& state_repository,
                                    AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                    PackedNodeList<LiftedTag>& out_nodes);
    template<StateViewConcept<LiftedTag> S>
    PackedNodeList<LiftedTag> get_packed_successor_nodes(const Node<S>& node,
                                                         formalism::planning::ActionView<LiftedTag> action,
                                                         StateRepository<LiftedTag>& state_repository,
                                                         AxiomEvaluator<LiftedTag>& axiom_evaluator);
    template<StateViewConcept<LiftedTag> S>
    void get_packed_successor_nodes(const Node<S>& node,
                                    formalism::planning::ActionView<LiftedTag> action,
                                    StateRepository<LiftedTag>& state_repository,
                                    AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                    PackedNodeList<LiftedTag>& out_nodes);
    template<StateViewConcept<LiftedTag> S>
    PackedLabeledNodeList<LiftedTag>
    get_packed_labeled_successor_nodes(const Node<S>& node, StateRepository<LiftedTag>& state_repository, AxiomEvaluator<LiftedTag>& axiom_evaluator);
    template<StateViewConcept<LiftedTag> S>
    void get_packed_labeled_successor_nodes(const Node<S>& node,
                                            StateRepository<LiftedTag>& state_repository,
                                            AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                            PackedLabeledNodeList<LiftedTag>& out_nodes);
    template<StateViewConcept<LiftedTag> S>
    PackedLabeledNodeList<LiftedTag> get_packed_labeled_successor_nodes(const Node<S>& node,
                                                                        formalism::planning::ActionView<LiftedTag> action,
                                                                        StateRepository<LiftedTag>& state_repository,
                                                                        AxiomEvaluator<LiftedTag>& axiom_evaluator);
    template<StateViewConcept<LiftedTag> S>
    void get_packed_labeled_successor_nodes(const Node<S>& node,
                                            formalism::planning::ActionView<LiftedTag> action,
                                            StateRepository<LiftedTag>& state_repository,
                                            AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                            PackedLabeledNodeList<LiftedTag>& out_nodes);

    template<StateViewConcept<LiftedTag> S>
    PackedNode<LiftedTag> get_packed_successor_node(const Node<S>& node,
                                                    const ygg::Data<formalism::RelationBinding<formalism::planning::Action<LiftedTag>>>& binding,
                                                    StateRepository<LiftedTag>& state_repository,
                                                    AxiomEvaluator<LiftedTag>& axiom_evaluator);

    /// Writes an unregistered successor without axiom closure. Use get_successor_node() for a completed borrowed successor.
    /// Pass the same pooled builder and auxiliary value to finalize_successor_state() to intern it.
    template<StateViewConcept<LiftedTag> S>
    ygg::float_t generate_successor_state(const Node<S>& node, formalism::planning::ActionBindingView binding, ygg::Builder<State<LiftedTag>>& out_state);
    /// Computes axiom closure and the final metric, then interns the completed state.
    Node<StateView<LiftedTag>> finalize_successor_state(StateRepository<LiftedTag>& state_repository,
                                                        AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                                        ygg::SharedObjectPoolPtr<ygg::Builder<State<LiftedTag>>, true> state,
                                                        ygg::float_t auxiliary_value);

    // Raw action-binding input; bindings are not interned and state storage follows S.
    template<StateViewConcept<LiftedTag> S>
    Node<S> get_successor_node(const Node<S>& node,
                               const ygg::Data<formalism::RelationBinding<formalism::planning::Action<LiftedTag>>>& binding,
                               SuccessorStorage<S>& storage,
                               AxiomEvaluator<LiftedTag>& axiom_evaluator);

    // Lookup
    Node<StateView<LiftedTag>> get_node(StateRepository<LiftedTag>& state_repository, ygg::Index<State<LiftedTag>> state_index);
    [[nodiscard]] SuccessorGeneratorPtr<LiftedTag> make_worker(ygg::ExecutionContextPtr execution_context) const;

    // Diagnostics
    void print_summary(size_t verbosity) const;

    // Getters
    const ApplicableActionProgram<LiftedTag>& get_action_program() const noexcept;
    const TaskPtr<LiftedTag>& get_task() const noexcept;
    ygg::uint_t get_index() const noexcept;

private:
    std::unique_ptr<Impl> m_impl;
};

static_assert(SuccessorGeneratorConcept<SuccessorGenerator<LiftedTag>, LiftedTag>);
}

#endif
