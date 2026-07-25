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

#include "tyr/planning/ground/node.hpp"        // for Node
#include "tyr/planning/ground/state_view.hpp"  // for State
//
#include "tyr/formalism/planning/ground_action_index.hpp"  // for ygg::Index
#include "tyr/formalism/planning/ground_action_view.hpp"
#include "tyr/planning/action_executor.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/ground/match_tree/match_tree.hpp"
#include "tyr/planning/successor_generator.hpp"

#include <yggdrasil/containers/associative_containers.hpp>

namespace tyr::planning
{

template<>
class SuccessorGenerator<GroundTag>
{
    friend class SuccessorGeneratorFactory<GroundTag>;

private:
    SuccessorGenerator(ygg::uint_t index, TaskPtr<GroundTag> task, ygg::ExecutionContextPtr execution_context, StateRepositoryPtr<GroundTag> state_repository);

public:
    Node<GroundTag> get_initial_node();

    std::vector<LabeledNode<GroundTag>> get_labeled_successor_nodes(const Node<GroundTag>& node);
    void get_labeled_successor_nodes(const Node<GroundTag>& node, std::vector<LabeledNode<GroundTag>>& out_nodes);

    Node<GroundTag> get_successor_node(const Node<GroundTag>& node, ::tyr::formalism::planning::GroundActionView action);
    ::tyr::formalism::planning::GroundActionView get_ground_action(::tyr::formalism::planning::ActionBindingView binding) const;

    Node<GroundTag> get_node(ygg::Index<State<GroundTag>> state_index);

    const auto& get_state_repository() const noexcept { return m_state_repository; }
    auto get_index() const noexcept { return m_index; }

private:
    ygg::uint_t m_index;
    TaskPtr<GroundTag> m_task;
    match_tree::MatchTreePtr<::tyr::formalism::planning::GroundAction> m_action_match_tree;
    ygg::UnorderedMap<::tyr::formalism::planning::ActionBindingView, ::tyr::formalism::planning::GroundActionView> m_action_binding_to_ground_action;

    ::tyr::formalism::planning::GroundActionViewList m_applicable_actions;

    StateRepositoryPtr<GroundTag> m_state_repository;

    ActionExecutor m_executor;
};

}

#endif
