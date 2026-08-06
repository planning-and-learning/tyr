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

#ifndef TYR_PLANNING_SEARCH_SPACE_SEQUENTIAL_HPP_
#define TYR_PLANNING_SEARCH_SPACE_SEQUENTIAL_HPP_

#include "tyr/planning/algorithms/utils.hpp"
#include "tyr/planning/search_space/search_node.hpp"

#include <algorithm>
#include <cassert>
#include <utility>
#include <vector>
#include <yggdrasil/containers/segmented_vector.hpp>

namespace tyr::planning
{

template<TaskKind Kind, SearchNodeConcept<ygg::Index<State<Kind>>> SearchNode>
NodeList<Kind> extract_node_trajectory(const ygg::SegmentedVector<SearchNode>& search_nodes,
                                       const SearchNode& final_search_node,
                                       const Node<Kind>& final_node,
                                       SuccessorGenerator<Kind>& successor_generator)
{
    auto trajectory = NodeList<Kind> {};
    trajectory.push_back(final_node);

    auto cur_search_node = &final_search_node;
    auto& state_repository = *successor_generator.get_state_repository();

    while (cur_search_node->parent_state != ygg::Index<State<Kind>>::max())
    {
        const auto parent_state_index = cur_search_node->parent_state;

        cur_search_node = &search_nodes.at(ygg::uint_t(cur_search_node->parent_state));

        trajectory.push_back(Node<Kind>(state_repository.get_registered_state(parent_state_index), cur_search_node->g_value));
    }

    std::reverse(trajectory.begin(), trajectory.end());

    return trajectory;
}

template<TaskKind Kind>
LabeledNodeList<Kind> extract_labeled_node_trajectory(const NodeList<Kind>& node_trajectory,
                                                      SuccessorGenerator<Kind>& successor_generator,
                                                      CostMode action_cost_mode = CostMode::GENERAL)
{
    assert(!node_trajectory.empty());

    auto labeled_node_trajectory = LabeledNodeList<Kind> {};
    auto cur_node = node_trajectory.front();
    auto applicable_actions = std::vector<::tyr::formalism::planning::ActionBindingView> {};

    for (size_t i = 1; i < node_trajectory.size(); ++i)
    {
        successor_generator.get_applicable_action_bindings(cur_node, applicable_actions);
        [[maybe_unused]] auto found = false;

        for (const auto action : applicable_actions)
        {
            const auto successor = successor_generator.get_successor_node(cur_node, action);
            const auto successor_g_value = compute_successor_g_value(cur_node.get_metric(), successor.get_metric(), action_cost_mode);
            const auto normalized_succ_node = Node<Kind>(successor.get_state(), successor_g_value);

            if (normalized_succ_node == node_trajectory[i])
            {
                labeled_node_trajectory.push_back(LabeledNode<Kind> { action, normalized_succ_node });
                cur_node = normalized_succ_node;
                found = true;
                break;
            }
        }
        assert(found);
    }

    return labeled_node_trajectory;
}

template<TaskKind Kind, SearchNodeConcept<ygg::Index<State<Kind>>> SearchNode>
inline Plan<Kind> extract_total_ordered_plan(const SearchNode& final_search_node,
                                             const Node<Kind>& final_node,
                                             const ygg::SegmentedVector<SearchNode>& search_nodes,
                                             SuccessorGenerator<Kind>& successor_generator,
                                             CostMode action_cost_mode = CostMode::GENERAL)
{
    const auto node_trajectory = extract_node_trajectory(search_nodes, final_search_node, final_node, successor_generator);

    auto labeled_node_trajectory = extract_labeled_node_trajectory(node_trajectory, successor_generator, action_cost_mode);

    return Plan<Kind>(node_trajectory.front(), std::move(labeled_node_trajectory));
}

template<>
struct PlanReconstructionPolicy<SequentialSearch>
{
    template<TaskKind Kind, SearchNodeConcept<ygg::Index<State<Kind>>> SearchNode>
    static Plan<Kind> extract_total_ordered_plan(const SearchNode& final_search_node,
                                                 const Node<Kind>& final_node,
                                                 const ygg::SegmentedVector<SearchNode>& search_nodes,
                                                 SuccessorGenerator<Kind>& successor_generator,
                                                 CostMode action_cost_mode = CostMode::GENERAL)
    {
        return planning::extract_total_ordered_plan(final_search_node, final_node, search_nodes, successor_generator, action_cost_mode);
    }
};

}

#endif
