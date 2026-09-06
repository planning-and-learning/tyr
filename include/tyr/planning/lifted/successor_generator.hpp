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

    // Labeled successor API. Interns action bindings.
    LabeledNodeList<LiftedTag>
    get_labeled_successor_nodes(const Node<LiftedTag>& node, StateRepository<LiftedTag>& state_repository, AxiomEvaluator<LiftedTag>& axiom_evaluator);
    void get_labeled_successor_nodes(const Node<LiftedTag>& node,
                                     StateRepository<LiftedTag>& state_repository,
                                     AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                     LabeledNodeList<LiftedTag>& out_nodes);

    Node<LiftedTag> get_successor_node(const Node<LiftedTag>& node,
                                       ::tyr::formalism::planning::ActionView<::tyr::GroundTag> action,
                                       StateRepository<LiftedTag>& state_repository,
                                       AxiomEvaluator<LiftedTag>& axiom_evaluator);
    ::tyr::formalism::planning::ActionView<::tyr::GroundTag> ground_action(::tyr::formalism::planning::ActionBindingView binding);

    // Action binding API (interning)
    Node<LiftedTag> get_successor_node(const Node<LiftedTag>& node,
                                       ::tyr::formalism::planning::ActionBindingView binding,
                                       StateRepository<LiftedTag>& state_repository,
                                       AxiomEvaluator<LiftedTag>& axiom_evaluator);

    std::vector<::tyr::formalism::planning::ActionBindingView> get_applicable_action_bindings(const Node<LiftedTag>& node);

    void get_applicable_action_bindings(const Node<LiftedTag>& node, std::vector<::tyr::formalism::planning::ActionBindingView>& out_bindings);

    /// Writes an unregistered successor. Pass the same pooled builder and result to finalize_successor_state().
    PendingActionResult
    generate_successor_state(const Node<LiftedTag>& node, ::tyr::formalism::planning::ActionBindingView binding, ygg::Builder<State<LiftedTag>>& out_state);
    /// Computes axiom closure and the final metric, then interns the completed state.
    Node<LiftedTag> finalize_successor_state(StateRepository<LiftedTag>& state_repository,
                                             AxiomEvaluator<LiftedTag>& axiom_evaluator,
                                             ygg::SharedObjectPoolPtr<ygg::Builder<State<LiftedTag>>, true> state,
                                             PendingActionResult result);

    // Action binding API (no interning)
    Node<LiftedTag> get_successor_node(const Node<LiftedTag>& node,
                                       const ygg::Data<::tyr::formalism::RelationBinding<::tyr::formalism::planning::Action<::tyr::LiftedTag>>>& binding,
                                       StateRepository<LiftedTag>& state_repository,
                                       AxiomEvaluator<LiftedTag>& axiom_evaluator);

    // Lookup
    Node<LiftedTag> get_node(StateRepository<LiftedTag>& state_repository, ygg::Index<State<LiftedTag>> state_index);
    [[nodiscard]] SuccessorGeneratorPtr<LiftedTag> make_worker(ygg::ExecutionContextPtr execution_context) const;

    // Diagnostics
    void print_summary(size_t verbosity) const;

    const ApplicableActionProgram<LiftedTag>& get_action_program() const noexcept;
    const TaskPtr<LiftedTag>& get_task() const noexcept;
    ygg::uint_t get_index() const noexcept;

private:
    std::unique_ptr<Impl> m_impl;
};

static_assert(SuccessorGeneratorConcept<SuccessorGenerator<LiftedTag>, LiftedTag>);
}

#endif
