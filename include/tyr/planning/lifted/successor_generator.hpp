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

#include "tyr/datalog/lifted/policies/annotation.hpp"
#include "tyr/datalog/lifted/workspaces/program.hpp"
#include "tyr/datalog/policies/termination.hpp"
#include "tyr/formalism/planning/ground_action_view.hpp"
#include "tyr/planning/action_executor.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/ground/match_tree/declarations.hpp"
#include "tyr/planning/lifted/axiom_evaluator.hpp"
#include "tyr/planning/lifted/node.hpp"
#include "tyr/planning/lifted/programs/action.hpp"
#include "tyr/planning/lifted/state_repository.hpp"
#include "tyr/planning/lifted/state_view.hpp"
#include "tyr/planning/lifted/task.hpp"
#include "tyr/planning/successor_generator.hpp"

#include <type_traits>
#include <utility>
#include <yggdrasil/containers/associative_containers.hpp>
#include <yggdrasil/core/itertools.hpp>
#include <yggdrasil/execution/onetbb.hpp>

namespace tyr::planning
{

template<>
class SuccessorGenerator<LiftedTag>
{
    friend class SuccessorGeneratorFactory<LiftedTag>;

private:
    SuccessorGenerator(ygg::uint_t index, TaskPtr<LiftedTag> task, ygg::ExecutionContextPtr execution_context, StateRepositoryPtr<LiftedTag> state_repository);

public:
    Node<LiftedTag> get_initial_node();

    // Transition label API
    std::vector<LabeledNode<LiftedTag>> get_labeled_successor_nodes(const Node<LiftedTag>& node);

    void get_labeled_successor_nodes(const Node<LiftedTag>& node, std::vector<LabeledNode<LiftedTag>>& out_nodes);

    Node<LiftedTag> get_successor_node(const Node<LiftedTag>& node, ::tyr::formalism::planning::GroundActionView action);
    ::tyr::formalism::planning::GroundActionView ground_action(::tyr::formalism::planning::ActionBindingView binding);

    // Action binding API (interning)
    Node<LiftedTag> get_successor_node(const Node<LiftedTag>& node, ::tyr::formalism::planning::ActionBindingView binding);

    std::vector<::tyr::formalism::planning::ActionBindingView> get_applicable_action_bindings(const Node<LiftedTag>& node);

    void get_applicable_action_bindings(const Node<LiftedTag>& node, std::vector<::tyr::formalism::planning::ActionBindingView>& out_bindings);

    // Action binding API (no interning)
    Node<LiftedTag> get_successor_node(const Node<LiftedTag>& node,
                                       const ygg::Data<::tyr::formalism::RelationBinding<::tyr::formalism::planning::Action>>& binding);

    template<typename Callback>
    void for_each_applicable_action_binding(const Node<LiftedTag>& node, Callback&& callback);

    template<typename Callback>
    void for_each_applicable_action_binding(const Node<LiftedTag>& node,
                                            ygg::Data<::tyr::formalism::RelationBinding<::tyr::formalism::planning::Action>>& scratch_binding,
                                            Callback&& callback);

    // Lookup
    Node<LiftedTag> get_node(ygg::Index<State<LiftedTag>> state_index);

    // Diagnostics
    void print_summary(size_t verbosity) const;

    const auto& get_action_program() const noexcept { return m_action_program; }
    const auto& get_state_repository() const noexcept { return m_state_repository; }
    const auto& get_workspace() const noexcept { return m_workspace; }
    auto get_index() const noexcept { return m_index; }

private:
    void compute_action_facts(const Node<LiftedTag>& node);

    using ActionBindingCallback = void (*)(const ygg::Data<::tyr::formalism::RelationBinding<::tyr::formalism::planning::Action>>&, void*);

    void for_each_applicable_action_binding_impl(const Node<LiftedTag>& node,
                                                 ygg::Data<::tyr::formalism::RelationBinding<::tyr::formalism::planning::Action>>& scratch_binding,
                                                 ActionBindingCallback callback,
                                                 void* callback_data);

private:
    ygg::uint_t m_index;
    TaskPtr<LiftedTag> m_task;
    ygg::ExecutionContextPtr m_execution_context;
    ygg::Data<::tyr::formalism::RelationBinding<::tyr::formalism::planning::Action>> m_scratch_action_binding;
    ApplicableActionProgram<LiftedTag> m_action_program;
    ygg::UnorderedMap<::tyr::formalism::planning::ActionBindingView, ::tyr::formalism::planning::GroundActionView> m_action_binding_to_ground_action;

    datalog::ProgramWorkspace<LiftedTag> m_workspace;
    ygg::itertools::cartesian_set::Workspace<ygg::Index<::tyr::formalism::Object>> m_cartesian_workspace;

    StateRepositoryPtr<LiftedTag> m_state_repository;

    ActionExecutor m_executor;
};

static_assert(SuccessorGeneratorConcept<SuccessorGenerator<LiftedTag>, LiftedTag>);

/**
 * Implementations
 */

// Action binding API (no interning)

template<typename Callback>
void SuccessorGenerator<LiftedTag>::for_each_applicable_action_binding(const Node<LiftedTag>& node, Callback&& callback)
{
    auto scratch_binding = ygg::Data<::tyr::formalism::RelationBinding<::tyr::formalism::planning::Action>>();
    for_each_applicable_action_binding(node, scratch_binding, std::forward<Callback>(callback));
}

template<typename Callback>
void SuccessorGenerator<LiftedTag>::for_each_applicable_action_binding(
    const Node<LiftedTag>& node,
    ygg::Data<::tyr::formalism::RelationBinding<::tyr::formalism::planning::Action>>& scratch_binding,
    Callback&& callback)
{
    using CallbackStorage = std::remove_reference_t<Callback>;
    auto callback_storage = &callback;
    for_each_applicable_action_binding_impl(
        node,
        scratch_binding,
        [](const ygg::Data<::tyr::formalism::RelationBinding<::tyr::formalism::planning::Action>>& binding, void* callback_data)
        { (*static_cast<CallbackStorage*>(callback_data))(binding); },
        callback_storage);
}
}

#endif
