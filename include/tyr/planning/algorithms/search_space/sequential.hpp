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

#ifndef TYR_PLANNING_ALGORITHMS_SEARCH_SPACE_SEQUENTIAL_HPP_
#define TYR_PLANNING_ALGORITHMS_SEARCH_SPACE_SEQUENTIAL_HPP_

#include "tyr/planning/algorithms/search_space/search_node.hpp"
#include "tyr/planning/algorithms/utils.hpp"
#include "tyr/planning/nodes/node_concepts.hpp"
#include "tyr/planning/successors/successors.hpp"

#include <algorithm>
#include <cassert>
#include <optional>
#include <urd/repr/sequences/segmented.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <utility>
#include <vector>

namespace tyr::planning
{

/// The nodes from the start to `final_node`, packed: each search node's state and g value, followed through its parents.
template<TaskKind Kind, SearchNodeConcept<urd::index<State<Kind>>> SearchNode>
NodeRecordList<Kind> extract_node_trajectory(const urd::Store<urd::sequence<SearchNode>, urd::stable_refs>& search_nodes,
                                             const SearchNode& final_search_node,
                                             const urd::data<Node<Kind>>& final_node)
{
    auto trajectory = NodeRecordList<Kind> {};
    trajectory.push_back(final_node);

    auto cur_search_node = &final_search_node;
    while (cur_search_node->parent_state != urd::index<State<Kind>>())
    {
        const auto parent_state_index = cur_search_node->parent_state;

        cur_search_node = &search_nodes.at(ygg::uint_t(cur_search_node->parent_state.value));

        trajectory.push_back(urd::data<Node<Kind>>(parent_state_index, cur_search_node->g_value));
    }

    std::reverse(trajectory.begin(), trajectory.end());

    return trajectory;
}

/// The plan along a node trajectory: its start node unpacked, and each step the successor of the applicable binding that
/// reproduces the next node, labeled by its binding, interned (the only bindings a search interns).
template<TaskKind Kind>
PlanBuilder<DeterministicTag, Kind> extract_plan(const NodeRecordList<Kind>& node_trajectory,
                                                 StateRepository<Kind>& state_repository,
                                                 AxiomEvaluator<Kind>& axiom_evaluator,
                                                 SuccessorGenerator<Kind>& successor_generator,
                                                 CostMode action_cost_mode = CostMode::GENERAL)
{
    assert(!node_trajectory.empty());

    auto plan = PlanBuilder<DeterministicTag, Kind>();
    plan.start_node = get_node(state_repository, node_trajectory.front());
    plan.labeled_succ_nodes.reserve(node_trajectory.size() - 1);
    auto builder = formalism::Builder {};

    for (size_t i = 1; i < node_trajectory.size(); ++i)
    {
        const auto& cur_node = plan.labeled_succ_nodes.empty() ? plan.start_node : plan.labeled_succ_nodes.back().node;
        const auto cur_view = urd::resolve(cur_node, std::as_const(state_repository));
        auto next = std::optional<LabeledNodeBuilder<Kind>> {};
        for_each_successor<DeterministicTag>(
            successor_generator,
            cur_view,
            [&](const TransitionLabel<Kind>& transition, SuccessorState<Kind>& successor_state)
            {
                find_successor(state_repository, cur_view, successor_state);
                extend_successor(axiom_evaluator, cur_view, successor_state);
                auto successor = register_successor(state_repository, cur_view, successor_state);
                successor.metric = compute_successor_g_value(cur_view.metric(), successor.metric, action_cost_mode);
                if (!urd::EqualTo<> {}(urd::resolve(successor, std::as_const(state_repository)).pack(), node_trajectory[i]))
                    return true;
                next =
                    LabeledNodeBuilder<Kind> { intern_action_binding(successor_generator, builder, transition.binding.value()).index(), std::move(successor) };
                return false;
            });
        assert(next.has_value());
        plan.labeled_succ_nodes.push_back(std::move(*next));
    }

    return plan;
}

template<TaskKind Kind, SearchNodeConcept<urd::index<State<Kind>>> SearchNode>
inline PlanBuilder<DeterministicTag, Kind> extract_total_ordered_plan(const SearchNode& final_search_node,
                                                                      const urd::data<Node<Kind>>& final_node,
                                                                      const urd::Store<urd::sequence<SearchNode>, urd::stable_refs>& search_nodes,
                                                                      StateRepository<Kind>& state_repository,
                                                                      AxiomEvaluator<Kind>& axiom_evaluator,
                                                                      SuccessorGenerator<Kind>& successor_generator,
                                                                      CostMode action_cost_mode = CostMode::GENERAL)
{
    const auto node_trajectory = extract_node_trajectory<Kind>(search_nodes, final_search_node, final_node);
    return extract_plan(node_trajectory, state_repository, axiom_evaluator, successor_generator, action_cost_mode);
}

template<>
struct PlanReconstructionPolicy<SequentialSearch>
{
    template<TaskKind Kind, SearchNodeConcept<urd::index<State<Kind>>> SearchNode>
    static PlanBuilder<DeterministicTag, Kind> extract_total_ordered_plan(const SearchNode& final_search_node,
                                                                          const urd::data<Node<Kind>>& final_node,
                                                                          const urd::Store<urd::sequence<SearchNode>, urd::stable_refs>& search_nodes,
                                                                          StateRepository<Kind>& state_repository,
                                                                          AxiomEvaluator<Kind>& axiom_evaluator,
                                                                          SuccessorGenerator<Kind>& successor_generator,
                                                                          CostMode action_cost_mode = CostMode::GENERAL)
    {
        return planning::extract_total_ordered_plan(final_search_node,
                                                    final_node,
                                                    search_nodes,
                                                    state_repository,
                                                    axiom_evaluator,
                                                    successor_generator,
                                                    action_cost_mode);
    }
};

}

#endif
