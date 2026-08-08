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

    // Labeled successor API.
    LabeledNodeList<GroundTag>
    get_labeled_successor_nodes(const Node<GroundTag>& node, StateRepository<GroundTag>& state_repository, AxiomEvaluator<GroundTag>& axiom_evaluator);
    void get_labeled_successor_nodes(const Node<GroundTag>& node,
                                     StateRepository<GroundTag>& state_repository,
                                     AxiomEvaluator<GroundTag>& axiom_evaluator,
                                     LabeledNodeList<GroundTag>& out_nodes);

    Node<GroundTag> get_successor_node(const Node<GroundTag>& node,
                                       ::tyr::formalism::planning::ActionBindingView binding,
                                       StateRepository<GroundTag>& state_repository,
                                       AxiomEvaluator<GroundTag>& axiom_evaluator);
    Node<GroundTag> get_successor_node(const Node<GroundTag>& node,
                                       ::tyr::formalism::planning::GroundActionView action,
                                       StateRepository<GroundTag>& state_repository,
                                       AxiomEvaluator<GroundTag>& axiom_evaluator);
    ::tyr::formalism::planning::GroundActionView ground_action(::tyr::formalism::planning::ActionBindingView binding) const;

    std::vector<::tyr::formalism::planning::ActionBindingView> get_applicable_action_bindings(const Node<GroundTag>& node);
    void get_applicable_action_bindings(const Node<GroundTag>& node, std::vector<::tyr::formalism::planning::ActionBindingView>& out_bindings);

    /// Writes an unregistered successor. Pass the same pooled builder and result to finalize_successor_state().
    PendingActionResult
    generate_successor_state(const Node<GroundTag>& node, ::tyr::formalism::planning::ActionBindingView binding, ygg::Builder<State<GroundTag>>& out_state);
    /// Computes axiom closure and the final metric, then interns the completed state.
    Node<GroundTag> finalize_successor_state(StateRepository<GroundTag>& state_repository,
                                             AxiomEvaluator<GroundTag>& axiom_evaluator,
                                             ygg::SharedObjectPoolPtr<ygg::Builder<State<GroundTag>>, true> state,
                                             PendingActionResult result);

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
