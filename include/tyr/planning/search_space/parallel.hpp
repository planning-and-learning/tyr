/*
 * Copyright (C) 2026 Dominik Drexler
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

#ifndef TYR_PLANNING_SEARCH_SPACE_PARALLEL_HPP_
#define TYR_PLANNING_SEARCH_SPACE_PARALLEL_HPP_

#include "tyr/planning/algorithms/utils.hpp"
#include "tyr/planning/search_space/search_node.hpp"
#include "tyr/planning/worker_state_index.hpp"

#include <algorithm>
#include <cassert>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>
#include <yggdrasil/containers/segmented_vector.hpp>
#include <yggdrasil/semantics/equal_to.hpp>

namespace tyr::planning
{

template<TaskKind Kind, SearchNodeConcept<WorkerStateIndex<Kind>> SearchNode>
struct WorkerSearchSpaceView
{
    SuccessorGenerator<Kind>& successor_generator;
    const ygg::SegmentedVector<SearchNode>& search_nodes;
};

template<>
struct PlanReconstructionPolicy<ParallelSearch>
{
    template<TaskKind Kind, SearchNodeConcept<WorkerStateIndex<Kind>> SearchNode>
    static Plan<Kind> extract_total_ordered_plan(WorkerStateIndex<Kind> final_state,
                                                 std::span<const WorkerSearchSpaceView<Kind, SearchNode>> workers,
                                                 SuccessorGenerator<Kind>& caller_successor_generator,
                                                 CostMode action_cost_mode = CostMode::GENERAL)
    {
        auto trajectory = std::vector<WorkerStateIndex<Kind>> {};
        auto current = final_state;

        while (true)
        {
            trajectory.push_back(current);
            const auto& search_node = get_search_node(current, workers);
            assert(search_node.parent_state.worker.is_max() == search_node.parent_state.state.is_max());
            if (search_node.parent_state.is_max())
            {
                break;
            }
            current = search_node.parent_state;
        }
        std::reverse(trajectory.begin(), trajectory.end());

        auto labels = std::vector<::tyr::formalism::planning::ActionBindingView> {};
        labels.reserve(trajectory.size() - 1);
        auto successors = LabeledNodeList<Kind> {};

        for (size_t i = 1; i < trajectory.size(); ++i)
        {
            const auto& source_worker = get_worker(trajectory[i - 1], workers);
            const auto& source_search_node = get_search_node(trajectory[i - 1], workers);
            const auto& target_search_node = get_search_node(trajectory[i], workers);
            const auto source_state = source_worker.successor_generator.get_state_repository()->get_registered_state(trajectory[i - 1].state);
            const auto target_state = get_worker(trajectory[i], workers).successor_generator.get_state_repository()->get_registered_state(trajectory[i].state);

            source_worker.successor_generator.get_labeled_successor_nodes(Node<Kind>(source_state, source_search_node.g_value), successors);

            const auto match =
                std::ranges::find_if(successors,
                                     [&](const auto& successor)
                                     {
                                         const auto g_value =
                                             compute_successor_g_value(source_search_node.g_value, successor.node.get_metric(), action_cost_mode);
                                         return g_value == target_search_node.g_value && equal_unextended(successor.node.get_state(), target_state);
                                     });
            if (match == successors.end())
                throw std::logic_error("Plan reconstruction could not reproduce a worker transition.");

            labels.push_back(match->label);
        }

        auto& caller_repository = *caller_successor_generator.get_state_repository();
        auto start_node = materialize(trajectory.front(), workers, caller_repository);
        auto labeled_trajectory = LabeledNodeList<Kind> {};
        labeled_trajectory.reserve(labels.size());
        for (size_t i = 0; i < labels.size(); ++i)
            labeled_trajectory.push_back(LabeledNode<Kind> { labels[i], materialize(trajectory[i + 1], workers, caller_repository) });

        return Plan<Kind>(std::move(start_node), std::move(labeled_trajectory));
    }

private:
    template<TaskKind Kind, SearchNodeConcept<WorkerStateIndex<Kind>> SearchNode>
    static const WorkerSearchSpaceView<Kind, SearchNode>& get_worker(WorkerStateIndex<Kind> state,
                                                                     std::span<const WorkerSearchSpaceView<Kind, SearchNode>> workers)
    {
        const auto index = static_cast<size_t>(ygg::uint_t(state.worker));
        assert(index < workers.size());
        return workers[index];
    }

    template<TaskKind Kind, SearchNodeConcept<WorkerStateIndex<Kind>> SearchNode>
    static const SearchNode& get_search_node(WorkerStateIndex<Kind> state, std::span<const WorkerSearchSpaceView<Kind, SearchNode>> workers)
    {
        const auto& search_nodes = get_worker(state, workers).search_nodes;
        const auto index = static_cast<size_t>(ygg::uint_t(state.state));
        assert(index < search_nodes.size());
        return search_nodes[index];
    }

    template<TaskKind Kind>
    static bool equal_unextended(const StateView<Kind>& lhs, const StateView<Kind>& rhs) noexcept
    {
        const auto& lhs_builder = lhs.get_state_builder();
        const auto& rhs_builder = rhs.get_state_builder();
        return ygg::EqualTo<> {}(lhs_builder.template get_atoms<::tyr::formalism::FluentTag>(), rhs_builder.template get_atoms<::tyr::formalism::FluentTag>())
               && ygg::EqualTo<> {}(lhs_builder.get_numeric_variables(), rhs_builder.get_numeric_variables());
    }

    template<TaskKind Kind, SearchNodeConcept<WorkerStateIndex<Kind>> SearchNode>
    static Node<Kind>
    materialize(WorkerStateIndex<Kind> state, std::span<const WorkerSearchSpaceView<Kind, SearchNode>> workers, StateRepository<Kind>& caller_repository)
    {
        const auto& worker = get_worker(state, workers);
        auto source_state = worker.successor_generator.get_state_repository()->get_registered_state(state.state);
        const auto g_value = get_search_node(state, workers).g_value;
        if (source_state.get_state_repository().get() == &caller_repository)
            return Node<Kind>(std::move(source_state), g_value);

        auto state_builder = caller_repository.get_state_builder();
        state_builder->assign_unextended_part(source_state.get_state_builder());
        return Node<Kind>(caller_repository.register_state(std::move(state_builder)), g_value);
    }
};

}

#endif
