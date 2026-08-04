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

#include "tyr/planning/algorithms/brfs.hpp"

#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/planning/views.hpp"
#include "tyr/planning/algorithms/brfs/event_handler.hpp"
#include "tyr/planning/algorithms/concepts.hpp"
#include "tyr/planning/algorithms/strategies/goal.hpp"
#include "tyr/planning/algorithms/strategies/pruning.hpp"
#include "tyr/planning/algorithms/utils.hpp"
#include "tyr/planning/applicability.hpp"
#include "tyr/planning/ground/state_builder.hpp"
#include "tyr/planning/ground/state_repository.hpp"
#include "tyr/planning/ground/state_view.hpp"
#include "tyr/planning/ground/successor_generator.hpp"
#include "tyr/planning/ground/task.hpp"
#include "tyr/planning/lifted/state_builder.hpp"
#include "tyr/planning/lifted/state_repository.hpp"
#include "tyr/planning/lifted/state_view.hpp"
#include "tyr/planning/lifted/successor_generator.hpp"
#include "tyr/planning/lifted/task.hpp"
#include "tyr/planning/node.hpp"
#include "tyr/planning/search_space/sequential.hpp"
#include "tyr/planning/state_index.hpp"

#include <algorithm>
#include <deque>
#include <random>
#include <yggdrasil/containers/segmented_vector.hpp>
#include <yggdrasil/core/chrono.hpp>
#include <yggdrasil/core/portable_shuffle.hpp>

namespace tyr::planning::brfs
{

template<TaskKind Kind>
struct SearchNode
{
    ygg::uint_t g_value;
    ygg::Index<State<Kind>> parent_state;
    SearchNodeStatus status;
};

static_assert(sizeof(SearchNode<LiftedTag>) == 12);
static_assert(sizeof(SearchNode<GroundTag>) == 12);

template<TaskKind Kind>
using SearchNodeVector = ygg::SegmentedVector<SearchNode<Kind>>;

template<TaskKind Kind>
static SearchNode<Kind>& get_or_create_search_node(ygg::Index<State<Kind>> state_index, SearchNodeVector<Kind>& search_nodes)
{
    static auto default_node = SearchNode { std::numeric_limits<ygg::uint_t>::max(), ygg::Index<State<Kind>>::max(), SearchNodeStatus::NEW };

    while (ygg::uint_t(state_index) >= search_nodes.size())
    {
        search_nodes.push_back(default_node);
    }
    return search_nodes[ygg::uint_t(state_index)];
}

template<TaskKind Kind>
SearchResult<Kind> find_solution(Task<Kind>& task, SuccessorGenerator<Kind>& successor_generator, const Options<Kind>& options)
{
    const auto start_node = (options.start_node) ? options.start_node.value() : successor_generator.get_initial_node();
    const auto& start_state = start_node.get_state();
    const auto start_state_index = start_state.get_index();
    const auto event_handler = (options.event_handler) ? options.event_handler : DefaultEventHandler<Kind>::create(0);
    const auto pruning_strategy = (options.pruning_strategy) ? options.pruning_strategy : PruningStrategy<Kind>::create();
    const auto goal_strategy = (options.goal_strategy) ? options.goal_strategy : ConjunctiveGoalStrategy<Kind>::create(task);
    auto rng = std::mt19937_64(options.random_seed);
    auto& state_repository = *successor_generator.get_state_repository();

    auto result = SearchResult<Kind>();
    auto search_nodes = SearchNodeVector<Kind>();
    auto queue = std::deque<ygg::Index<State<Kind>>> {};

    auto& start_search_node = get_or_create_search_node(start_state_index, search_nodes);
    start_search_node.status = SearchNodeStatus::OPEN;
    start_search_node.g_value = 0;

    event_handler->on_start_search(start_node);

    if (!goal_strategy->is_static_goal_satisfied(task))
    {
        result.status = SearchStatus::UNSOLVABLE;
        event_handler->on_end_search(result.status);
        return result;
    }

    if (goal_strategy->is_dynamic_goal_satisfied(start_state, start_state))
    {
        result.plan = Plan(start_node, LabeledNodeList<Kind> {});
        result.goal_node = start_node;
        result.status = SearchStatus::SOLVED;

        event_handler->on_end_search(result.status);
        event_handler->on_solved(result.plan.value());

        return result;
    }

    if (pruning_strategy->should_prune_state(start_state))
    {
        result.status = SearchStatus::EXHAUSTED;
        event_handler->on_end_search(result.status);
        return result;
    }

    auto labeled_succ_nodes = std::vector<LabeledNode<Kind>> {};
    auto current_layer = ygg::uint_t { 0 };
    queue.push_back(start_state_index);

    auto stopwatch = options.max_time ? std::optional<ygg::CountdownWatch>(options.max_time.value()) : std::nullopt;

    while (!queue.empty())
    {
        if (stopwatch && stopwatch->has_finished())
        {
            result.status = SearchStatus::OUT_OF_TIME;
            event_handler->on_end_search(result.status);
            return result;
        }

        const auto state_index = queue.front();
        queue.pop_front();

        const auto state = state_repository.get_registered_state(state_index);
        auto& search_node = get_or_create_search_node(state_index, search_nodes);
        auto node = Node<Kind>(state, search_node.g_value);

        if (search_node.status == SearchNodeStatus::CLOSED || search_node.status == SearchNodeStatus::DEAD_END)
            continue;

        if (search_node.g_value > current_layer)
        {
            event_handler->on_finish_layer(current_layer);
            current_layer = search_node.g_value;
        }

        if (search_node.status == SearchNodeStatus::GOAL)
        {
            event_handler->on_expand_goal_node(node);

            result.plan = extract_total_ordered_plan(search_node, node, search_nodes, successor_generator, CostMode::UNIT);
            result.goal_node = node;
            result.status = SearchStatus::SOLVED;

            event_handler->on_end_search(result.status);
            event_handler->on_solved(result.plan.value());

            return result;
        }

        event_handler->on_expand_node(node);

        search_node.status = SearchNodeStatus::CLOSED;

        successor_generator.get_labeled_successor_nodes(node, labeled_succ_nodes);

        if (stopwatch && stopwatch->has_finished())
        {
            result.status = SearchStatus::OUT_OF_TIME;
            event_handler->on_end_search(result.status);
            return result;
        }

        if (options.shuffle_labeled_succ_nodes)
            ygg::portable_shuffle(labeled_succ_nodes.begin(), labeled_succ_nodes.end(), rng);

        for (const auto& labeled_succ_node : labeled_succ_nodes)
        {
            if (stopwatch && stopwatch->has_finished())
            {
                result.status = SearchStatus::OUT_OF_TIME;
                event_handler->on_end_search(result.status);
                return result;
            }

            const auto& succ_node = labeled_succ_node.node;
            const auto& succ_state = succ_node.get_state();
            const auto succ_state_index = succ_state.get_index();

            auto& successor_search_node = get_or_create_search_node(succ_state_index, search_nodes);

            const auto is_new_successor_state = (successor_search_node.status == SearchNodeStatus::NEW);

            if (is_new_successor_state && search_nodes.size() >= options.max_num_states)
            {
                result.status = SearchStatus::OUT_OF_STATES;
                event_handler->on_end_search(result.status);
                return result;
            }

            if (!is_new_successor_state)
                continue;

            successor_search_node.status = SearchNodeStatus::OPEN;
            successor_search_node.parent_state = state_index;
            successor_search_node.g_value = search_node.g_value + 1;
            const auto brfs_succ_node = Node<Kind>(succ_state, successor_search_node.g_value);
            const auto brfs_labeled_succ_node = LabeledNode<Kind> { labeled_succ_node.label, brfs_succ_node };

            if (pruning_strategy->should_prune_successor_state(state, succ_state, is_new_successor_state))
            {
                successor_search_node.status = SearchNodeStatus::CLOSED;
                event_handler->on_prune_node(node, brfs_labeled_succ_node);
                continue;
            }

            event_handler->on_generate_node(node, brfs_labeled_succ_node);

            if (goal_strategy->is_dynamic_goal_satisfied(start_state, succ_state))
            {
                successor_search_node.status = SearchNodeStatus::GOAL;

                event_handler->on_expand_goal_node(brfs_succ_node);

                result.plan = extract_total_ordered_plan(successor_search_node, brfs_succ_node, search_nodes, successor_generator, CostMode::UNIT);
                result.goal_node = brfs_succ_node;
                result.status = SearchStatus::SOLVED;

                event_handler->on_end_search(result.status);
                event_handler->on_solved(result.plan.value());

                return result;
            }

            queue.push_back(succ_state_index);
        }
    }

    result.status = SearchStatus::EXHAUSTED;
    event_handler->on_end_search(result.status);
    return result;
}

template SearchResult<LiftedTag>
find_solution<LiftedTag>(Task<LiftedTag>& task, SuccessorGenerator<LiftedTag>& successor_generator, const Options<LiftedTag>& options);

template SearchResult<GroundTag>
find_solution<GroundTag>(Task<GroundTag>& task, SuccessorGenerator<GroundTag>& successor_generator, const Options<GroundTag>& options);

static_assert(SolverConcept<Solver<LiftedTag>, LiftedTag>);
static_assert(SolverConcept<Solver<GroundTag>, GroundTag>);

}
