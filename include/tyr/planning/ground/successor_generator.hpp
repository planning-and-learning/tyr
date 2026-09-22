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
#include "tyr/planning/ground/state_view.hpp"
#include "tyr/planning/successor_generator.hpp"

#include <atomic>
#include <functional>
#include <memory>
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

    // Unlabeled successor API.
    NodeList<GroundTag>
    get_successor_nodes(const Node<GroundTag>& node, StateRepository<GroundTag>& state_repository, AxiomEvaluator<GroundTag>& axiom_evaluator);
    void get_successor_nodes(const Node<GroundTag>& node,
                             StateRepository<GroundTag>& state_repository,
                             AxiomEvaluator<GroundTag>& axiom_evaluator,
                             NodeList<GroundTag>& out_nodes);
    NodeList<GroundTag> get_successor_nodes(const Node<GroundTag>& node,
                                            formalism::planning::ActionView<LiftedTag> action,
                                            StateRepository<GroundTag>& state_repository,
                                            AxiomEvaluator<GroundTag>& axiom_evaluator);
    void get_successor_nodes(const Node<GroundTag>& node,
                             formalism::planning::ActionView<LiftedTag> action,
                             StateRepository<GroundTag>& state_repository,
                             AxiomEvaluator<GroundTag>& axiom_evaluator,
                             NodeList<GroundTag>& out_nodes);

    // Labeled successor API.
    LabeledNodeList<GroundTag>
    get_labeled_successor_nodes(const Node<GroundTag>& node, StateRepository<GroundTag>& state_repository, AxiomEvaluator<GroundTag>& axiom_evaluator);
    void get_labeled_successor_nodes(const Node<GroundTag>& node,
                                     StateRepository<GroundTag>& state_repository,
                                     AxiomEvaluator<GroundTag>& axiom_evaluator,
                                     LabeledNodeList<GroundTag>& out_nodes);
    LabeledNodeList<GroundTag> get_labeled_successor_nodes(const Node<GroundTag>& node,
                                                           formalism::planning::ActionView<LiftedTag> action,
                                                           StateRepository<GroundTag>& state_repository,
                                                           AxiomEvaluator<GroundTag>& axiom_evaluator);
    void get_labeled_successor_nodes(const Node<GroundTag>& node,
                                     formalism::planning::ActionView<LiftedTag> action,
                                     StateRepository<GroundTag>& state_repository,
                                     AxiomEvaluator<GroundTag>& axiom_evaluator,
                                     LabeledNodeList<GroundTag>& out_nodes);

    Node<GroundTag> get_successor_node(const Node<GroundTag>& node,
                                       formalism::planning::ActionBindingView binding,
                                       StateRepository<GroundTag>& state_repository,
                                       AxiomEvaluator<GroundTag>& axiom_evaluator);
    Node<GroundTag> get_successor_node(const Node<GroundTag>& node,
                                       formalism::planning::ActionView<GroundTag> action,
                                       StateRepository<GroundTag>& state_repository,
                                       AxiomEvaluator<GroundTag>& axiom_evaluator);
    formalism::planning::ActionView<GroundTag> ground_action(formalism::planning::ActionBindingView binding) const;

    std::vector<formalism::planning::ActionBindingView> get_applicable_action_bindings(const Node<GroundTag>& node);
    void get_applicable_action_bindings(const Node<GroundTag>& node, std::vector<formalism::planning::ActionBindingView>& out_bindings);
    std::vector<formalism::planning::ActionBindingView> get_applicable_action_bindings(const Node<GroundTag>& node,
                                                                                       formalism::planning::ActionView<LiftedTag> action);
    void get_applicable_action_bindings(const Node<GroundTag>& node,
                                        formalism::planning::ActionView<LiftedTag> action,
                                        std::vector<formalism::planning::ActionBindingView>& out_bindings);

    /// Callbacks return true to continue, false to stop. The result is true iff enumeration was exhausted.
    /// Enumeration must not reenter this generator's enumeration APIs: they share scratch storage.
    /// Generating a single successor inside an action-binding callback is supported.
    bool for_each_applicable_action_binding(const Node<GroundTag>& node, const std::function<bool(formalism::planning::ActionBindingView)>& callback);
    bool for_each_successor_node(const Node<GroundTag>& node,
                                 StateRepository<GroundTag>& state_repository,
                                 AxiomEvaluator<GroundTag>& axiom_evaluator,
                                 const std::function<bool(Node<GroundTag>)>& callback);
    bool for_each_labeled_successor_node(const Node<GroundTag>& node,
                                         StateRepository<GroundTag>& state_repository,
                                         AxiomEvaluator<GroundTag>& axiom_evaluator,
                                         const std::function<bool(LabeledNode<GroundTag>)>& callback);
    bool for_each_applicable_action_binding(const Node<GroundTag>& node,
                                            formalism::planning::ActionView<LiftedTag> action,
                                            const std::function<bool(formalism::planning::ActionBindingView)>& callback);
    bool for_each_successor_node(const Node<GroundTag>& node,
                                 formalism::planning::ActionView<LiftedTag> action,
                                 StateRepository<GroundTag>& state_repository,
                                 AxiomEvaluator<GroundTag>& axiom_evaluator,
                                 const std::function<bool(Node<GroundTag>)>& callback);
    bool for_each_labeled_successor_node(const Node<GroundTag>& node,
                                         formalism::planning::ActionView<LiftedTag> action,
                                         StateRepository<GroundTag>& state_repository,
                                         AxiomEvaluator<GroundTag>& axiom_evaluator,
                                         const std::function<bool(LabeledNode<GroundTag>)>& callback);

    // Packed output retains registered state handles without retaining unpacked builders.
    PackedNode<GroundTag> get_packed_initial_node(StateRepository<GroundTag>& state_repository, AxiomEvaluator<GroundTag>& axiom_evaluator);
    PackedNode<GroundTag> get_packed_node(StateRepository<GroundTag>& state_repository, ygg::Index<State<GroundTag>> state_index);
    PackedNode<GroundTag> get_packed_successor_node(const Node<GroundTag>& node,
                                                    formalism::planning::ActionBindingView binding,
                                                    StateRepository<GroundTag>& state_repository,
                                                    AxiomEvaluator<GroundTag>& axiom_evaluator);
    PackedNode<GroundTag> get_packed_successor_node(const Node<GroundTag>& node,
                                                    formalism::planning::ActionView<GroundTag> action,
                                                    StateRepository<GroundTag>& state_repository,
                                                    AxiomEvaluator<GroundTag>& axiom_evaluator);
    PackedNodeList<GroundTag>
    get_packed_successor_nodes(const Node<GroundTag>& node, StateRepository<GroundTag>& state_repository, AxiomEvaluator<GroundTag>& axiom_evaluator);
    void get_packed_successor_nodes(const Node<GroundTag>& node,
                                    StateRepository<GroundTag>& state_repository,
                                    AxiomEvaluator<GroundTag>& axiom_evaluator,
                                    PackedNodeList<GroundTag>& out_nodes);
    PackedNodeList<GroundTag> get_packed_successor_nodes(const Node<GroundTag>& node,
                                                         formalism::planning::ActionView<LiftedTag> action,
                                                         StateRepository<GroundTag>& state_repository,
                                                         AxiomEvaluator<GroundTag>& axiom_evaluator);
    void get_packed_successor_nodes(const Node<GroundTag>& node,
                                    formalism::planning::ActionView<LiftedTag> action,
                                    StateRepository<GroundTag>& state_repository,
                                    AxiomEvaluator<GroundTag>& axiom_evaluator,
                                    PackedNodeList<GroundTag>& out_nodes);
    PackedLabeledNodeList<GroundTag>
    get_packed_labeled_successor_nodes(const Node<GroundTag>& node, StateRepository<GroundTag>& state_repository, AxiomEvaluator<GroundTag>& axiom_evaluator);
    void get_packed_labeled_successor_nodes(const Node<GroundTag>& node,
                                            StateRepository<GroundTag>& state_repository,
                                            AxiomEvaluator<GroundTag>& axiom_evaluator,
                                            PackedLabeledNodeList<GroundTag>& out_nodes);
    PackedLabeledNodeList<GroundTag> get_packed_labeled_successor_nodes(const Node<GroundTag>& node,
                                                                        formalism::planning::ActionView<LiftedTag> action,
                                                                        StateRepository<GroundTag>& state_repository,
                                                                        AxiomEvaluator<GroundTag>& axiom_evaluator);
    void get_packed_labeled_successor_nodes(const Node<GroundTag>& node,
                                            formalism::planning::ActionView<LiftedTag> action,
                                            StateRepository<GroundTag>& state_repository,
                                            AxiomEvaluator<GroundTag>& axiom_evaluator,
                                            PackedLabeledNodeList<GroundTag>& out_nodes);

    /// Writes an unregistered successor. Pass the same pooled builder and auxiliary value to finalize_successor_state().
    ygg::float_t
    generate_successor_state(const Node<GroundTag>& node, formalism::planning::ActionBindingView binding, ygg::Builder<State<GroundTag>>& out_state);
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
