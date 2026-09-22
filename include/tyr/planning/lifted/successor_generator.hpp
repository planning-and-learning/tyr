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

    Node<LiftedTag> get_initial_node(StateRepository<LiftedTag>& state_repository, AxiomEvaluator<LiftedTag>& axiom_evaluator);

    // Unlabeled successor API. Does not intern action bindings.
    NodeList<LiftedTag>
    get_successor_nodes(const Node<LiftedTag>& node, StateRepository<LiftedTag>& state_repository, AxiomEvaluator<LiftedTag>& axiom_evaluator);
    void get_successor_nodes(const Node<LiftedTag>& node,
                             StateRepository<LiftedTag>& state_repository,
                             AxiomEvaluator<LiftedTag>& axiom_evaluator,
                             NodeList<LiftedTag>& out_nodes);
    NodeList<LiftedTag> get_successor_nodes(const Node<LiftedTag>& node,
                                            formalism::planning::ActionView<LiftedTag> action,
                                            StateRepository<LiftedTag>& state_repository,
                                            AxiomEvaluator<LiftedTag>& axiom_evaluator);
    void get_successor_nodes(const Node<LiftedTag>& node,
                             formalism::planning::ActionView<LiftedTag> action,
                             StateRepository<LiftedTag>& state_repository,
                             AxiomEvaluator<LiftedTag>& axiom_evaluator,
                             NodeList<LiftedTag>& out_nodes);

    // Labeled successor API. Interns action bindings.
    LabeledNodeList<LiftedTag>
    get_labeled_successor_nodes(const Node<LiftedTag>& node, StateRepository<LiftedTag>& state_repository, AxiomEvaluator<LiftedTag>& axiom_evaluator);
    void get_labeled_successor_nodes(const Node<LiftedTag>& node,
                                     StateRepository<LiftedTag>& state_repository,
                                     AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                     LabeledNodeList<LiftedTag>& out_nodes);
    LabeledNodeList<LiftedTag> get_labeled_successor_nodes(const Node<LiftedTag>& node,
                                                           formalism::planning::ActionView<LiftedTag> action,
                                                           StateRepository<LiftedTag>& state_repository,
                                                           AxiomEvaluator<LiftedTag>& axiom_evaluator);
    void get_labeled_successor_nodes(const Node<LiftedTag>& node,
                                     formalism::planning::ActionView<LiftedTag> action,
                                     StateRepository<LiftedTag>& state_repository,
                                     AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                     LabeledNodeList<LiftedTag>& out_nodes);

    Node<LiftedTag> get_successor_node(const Node<LiftedTag>& node,
                                       formalism::planning::ActionView<GroundTag> action,
                                       StateRepository<LiftedTag>& state_repository,
                                       AxiomEvaluator<LiftedTag>& axiom_evaluator);
    formalism::planning::ActionView<GroundTag> ground_action(formalism::planning::ActionBindingView binding);

    // Action binding API (interning)
    Node<LiftedTag> get_successor_node(const Node<LiftedTag>& node,
                                       formalism::planning::ActionBindingView binding,
                                       StateRepository<LiftedTag>& state_repository,
                                       AxiomEvaluator<LiftedTag>& axiom_evaluator);

    std::vector<formalism::planning::ActionBindingView> get_applicable_action_bindings(const Node<LiftedTag>& node);

    void get_applicable_action_bindings(const Node<LiftedTag>& node, std::vector<formalism::planning::ActionBindingView>& out_bindings);
    std::vector<formalism::planning::ActionBindingView> get_applicable_action_bindings(const Node<LiftedTag>& node,
                                                                                       formalism::planning::ActionView<LiftedTag> action);
    void get_applicable_action_bindings(const Node<LiftedTag>& node,
                                        formalism::planning::ActionView<LiftedTag> action,
                                        std::vector<formalism::planning::ActionBindingView>& out_bindings);

    /// Callbacks return true to continue, false to stop. The result is true iff enumeration was exhausted.
    /// Enumeration must not reenter this generator's enumeration APIs: they share scratch storage.
    /// Generating a single successor inside an action-binding callback is supported.
    bool for_each_applicable_action_binding(const Node<LiftedTag>& node, const std::function<bool(formalism::planning::ActionBindingView)>& callback);
    bool for_each_successor_node(const Node<LiftedTag>& node,
                                 StateRepository<LiftedTag>& state_repository,
                                 AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                 const std::function<bool(Node<LiftedTag>)>& callback);
    bool for_each_labeled_successor_node(const Node<LiftedTag>& node,
                                         StateRepository<LiftedTag>& state_repository,
                                         AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                         const std::function<bool(LabeledNode<LiftedTag>)>& callback);
    bool for_each_applicable_action_binding(const Node<LiftedTag>& node,
                                            formalism::planning::ActionView<LiftedTag> action,
                                            const std::function<bool(formalism::planning::ActionBindingView)>& callback);
    bool for_each_successor_node(const Node<LiftedTag>& node,
                                 formalism::planning::ActionView<LiftedTag> action,
                                 StateRepository<LiftedTag>& state_repository,
                                 AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                 const std::function<bool(Node<LiftedTag>)>& callback);
    bool for_each_labeled_successor_node(const Node<LiftedTag>& node,
                                         formalism::planning::ActionView<LiftedTag> action,
                                         StateRepository<LiftedTag>& state_repository,
                                         AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                         const std::function<bool(LabeledNode<LiftedTag>)>& callback);

    // Packed output retains registered state handles without retaining unpacked builders.
    PackedNode<LiftedTag> get_packed_initial_node(StateRepository<LiftedTag>& state_repository, AxiomEvaluator<LiftedTag>& axiom_evaluator);
    PackedNode<LiftedTag> get_packed_node(StateRepository<LiftedTag>& state_repository, ygg::Index<State<LiftedTag>> state_index);
    PackedNode<LiftedTag> get_packed_successor_node(const Node<LiftedTag>& node,
                                                    formalism::planning::ActionBindingView binding,
                                                    StateRepository<LiftedTag>& state_repository,
                                                    AxiomEvaluator<LiftedTag>& axiom_evaluator);
    PackedNode<LiftedTag> get_packed_successor_node(const Node<LiftedTag>& node,
                                                    formalism::planning::ActionView<GroundTag> action,
                                                    StateRepository<LiftedTag>& state_repository,
                                                    AxiomEvaluator<LiftedTag>& axiom_evaluator);
    PackedNodeList<LiftedTag>
    get_packed_successor_nodes(const Node<LiftedTag>& node, StateRepository<LiftedTag>& state_repository, AxiomEvaluator<LiftedTag>& axiom_evaluator);
    void get_packed_successor_nodes(const Node<LiftedTag>& node,
                                    StateRepository<LiftedTag>& state_repository,
                                    AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                    PackedNodeList<LiftedTag>& out_nodes);
    PackedNodeList<LiftedTag> get_packed_successor_nodes(const Node<LiftedTag>& node,
                                                         formalism::planning::ActionView<LiftedTag> action,
                                                         StateRepository<LiftedTag>& state_repository,
                                                         AxiomEvaluator<LiftedTag>& axiom_evaluator);
    void get_packed_successor_nodes(const Node<LiftedTag>& node,
                                    formalism::planning::ActionView<LiftedTag> action,
                                    StateRepository<LiftedTag>& state_repository,
                                    AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                    PackedNodeList<LiftedTag>& out_nodes);
    PackedLabeledNodeList<LiftedTag>
    get_packed_labeled_successor_nodes(const Node<LiftedTag>& node, StateRepository<LiftedTag>& state_repository, AxiomEvaluator<LiftedTag>& axiom_evaluator);
    void get_packed_labeled_successor_nodes(const Node<LiftedTag>& node,
                                            StateRepository<LiftedTag>& state_repository,
                                            AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                            PackedLabeledNodeList<LiftedTag>& out_nodes);
    PackedLabeledNodeList<LiftedTag> get_packed_labeled_successor_nodes(const Node<LiftedTag>& node,
                                                                        formalism::planning::ActionView<LiftedTag> action,
                                                                        StateRepository<LiftedTag>& state_repository,
                                                                        AxiomEvaluator<LiftedTag>& axiom_evaluator);
    void get_packed_labeled_successor_nodes(const Node<LiftedTag>& node,
                                            formalism::planning::ActionView<LiftedTag> action,
                                            StateRepository<LiftedTag>& state_repository,
                                            AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                            PackedLabeledNodeList<LiftedTag>& out_nodes);

    PackedNode<LiftedTag> get_packed_successor_node(const Node<LiftedTag>& node,
                                                    const ygg::Data<formalism::RelationBinding<formalism::planning::Action<LiftedTag>>>& binding,
                                                    StateRepository<LiftedTag>& state_repository,
                                                    AxiomEvaluator<LiftedTag>& axiom_evaluator);

    /// Writes an unregistered successor. Pass the same pooled builder and auxiliary value to finalize_successor_state().
    ygg::float_t
    generate_successor_state(const Node<LiftedTag>& node, formalism::planning::ActionBindingView binding, ygg::Builder<State<LiftedTag>>& out_state);
    /// Computes axiom closure and the final metric, then interns the completed state.
    Node<LiftedTag> finalize_successor_state(StateRepository<LiftedTag>& state_repository,
                                             AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                             ygg::SharedObjectPoolPtr<ygg::Builder<State<LiftedTag>>, true> state,
                                             ygg::float_t auxiliary_value);

    // Action binding API (no interning)
    Node<LiftedTag> get_successor_node(const Node<LiftedTag>& node,
                                       const ygg::Data<formalism::RelationBinding<formalism::planning::Action<LiftedTag>>>& binding,
                                       StateRepository<LiftedTag>& state_repository,
                                       AxiomEvaluator<LiftedTag>& axiom_evaluator);

    // Lookup
    Node<LiftedTag> get_node(StateRepository<LiftedTag>& state_repository, ygg::Index<State<LiftedTag>> state_index);
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
