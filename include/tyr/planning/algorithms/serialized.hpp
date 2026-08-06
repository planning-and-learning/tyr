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

#ifndef TYR_PLANNING_ALGORITHMS_SERIALIZED_HPP_
#define TYR_PLANNING_ALGORITHMS_SERIALIZED_HPP_

#include "tyr/planning/algorithms/concepts.hpp"
#include "tyr/planning/algorithms/serialized/event_handler.hpp"
#include "tyr/planning/algorithms/serialized/statistics.hpp"
#include "tyr/planning/algorithms/strategies/goal.hpp"
#include "tyr/planning/algorithms/utils.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/plan.hpp"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace tyr::planning::serialized
{

template<TaskKind Kind, SolverConcept<Kind> Subsolver>
struct Options
{
    /// Optional initial node for the first subsearch; when a subsearch runs, it must belong to the subsolver's task.
    std::optional<Node<Kind>> start_node = std::nullopt;
    EventHandlerPtr<Kind, Subsolver> event_handler = nullptr;
    GoalStrategyPtr<Kind> subgoal_strategy = nullptr;
    GoalStrategyPtr<Kind> goal_strategy = nullptr;
    ygg::uint_t max_num_subsearches = std::numeric_limits<ygg::uint_t>::max();

    Options() = default;
};

template<typename T, typename Kind>
concept SerializedSolverConcept =
    TaskKind<Kind> && SolverConcept<T, Kind> && requires(T solver, std::optional<Node<Kind>> start_node, GoalStrategyPtr<Kind> goal_strategy) {
        typename T::EventHandlerType;
        typename T::EventHandlerType::StatisticsType;
        solver.options.start_node = start_node;
        solver.options.goal_strategy = goal_strategy;
    } && (std::same_as<typename T::EventHandlerType::StatisticsType, tyr::planning::Statistics> || requires(T solver) {
        solver.options.event_handler;
        solver.options.event_handler->get_statistics();
    });

namespace detail
{

template<TaskKind Kind>
SearchResult<Kind> make_empty_result(const Node<Kind>& start_node)
{
    auto result = SearchResult<Kind> {};
    result.status = SearchStatus::SOLVED;
    result.goal_node = start_node;
    result.plan = Plan<Kind>(start_node);
    return result;
}

template<TaskKind Kind>
void append_plan(const Plan<Kind>& subplan, Node<Kind>& current_node, LabeledNodeList<Kind>& labeled_succ_nodes)
{
    auto previous_metric = subplan.get_start_node().get_metric();
    auto cumulative_metric = current_node.get_metric();

    for (const auto& labeled_succ_node : subplan.get_labeled_succ_nodes())
    {
        const auto step_cost = labeled_succ_node.node.get_metric() - previous_metric;
        cumulative_metric = ygg::FloatTolerance<ygg::float_t>::canonicalize(cumulative_metric + step_cost);
        current_node = Node<Kind>(labeled_succ_node.node.get_state(), cumulative_metric);
        labeled_succ_nodes.emplace_back(labeled_succ_node.label, current_node);
        previous_metric = labeled_succ_node.node.get_metric();
    }
}

template<TaskKind Kind>
struct ReachedSubgoal
{
    Node<Kind> node;
    size_t plan_position;
};

template<TaskKind Kind>
std::optional<size_t> find_reached_subgoal(const std::vector<ReachedSubgoal<Kind>>& reached_subgoals, const Node<Kind>& node)
{
    for (const auto& reached_subgoal : reached_subgoals)
    {
        if (reached_subgoal.node.get_state().get_index() == node.get_state().get_index())
            return reached_subgoal.plan_position;
    }

    return std::nullopt;
}

}

template<TaskKind Kind, SerializedSolverConcept<Kind> Solver>
SearchResult<Kind> find_solution(Solver& solver, const Options<Kind, Solver>& options = Options<Kind, Solver>())
{
    const auto event_handler = options.event_handler ? options.event_handler : DefaultEventHandler<Kind, Solver>::create();

    if (!options.subgoal_strategy)
        throw std::invalid_argument("serialized::find_solution(...): subgoal strategy is required.");
    if (!options.goal_strategy)
        throw std::invalid_argument("serialized::find_solution(...): goal strategy is required.");

    if (options.max_num_subsearches == 0 && !options.start_node && !solver.options.start_node)
        throw std::invalid_argument("serialized::find_solution(...): start node is required if max_num_subsearches is 0.");

    auto statistics = tyr::planning::Statistics {};
    auto worker_statistics = std::vector<tyr::planning::Statistics> {};
    const auto search_start = std::chrono::steady_clock::now();
    statistics.set_search_start_time_point(search_start);

    event_handler->on_start_search();

    const auto finalize = [&](SearchResult<Kind> result)
    {
        const auto search_end = std::chrono::steady_clock::now();
        statistics.set_search_end_time_point(search_end);
        for (auto& worker : worker_statistics)
        {
            worker.set_search_start_time_point(search_start);
            worker.set_search_end_time_point(search_end);
        }
        result.statistics = statistics;
        result.worker_statistics = worker_statistics;
        if (result.plan && result.status == SearchStatus::SOLVED)
            event_handler->on_solved(*result.plan);
        event_handler->on_end_search(result.status, result.statistics);
        return result;
    };

    if (options.max_num_subsearches == 0)
        return finalize(detail::make_empty_result(options.start_node ? *options.start_node : *solver.options.start_node));

    auto current_start_node = options.start_node ? options.start_node : solver.options.start_node;
    auto combined_start_node = std::optional<Node<Kind>> {};
    auto combined_labeled_succ_nodes = LabeledNodeList<Kind> {};
    auto reached_subgoals = std::vector<detail::ReachedSubgoal<Kind>> {};

    if (current_start_node)
        reached_subgoals.push_back(detail::ReachedSubgoal<Kind> { *current_start_node, 0 });

    if (current_start_node && options.goal_strategy->is_dynamic_goal_satisfied(current_start_node->get_state(), current_start_node->get_state()))
    {
        auto result = detail::make_empty_result(*current_start_node);
        return finalize(std::move(result));
    }

    if (auto serialized_subgoal_strategy = std::dynamic_pointer_cast<SerializedGoalStrategy<Kind>>(options.subgoal_strategy))
        serialized_subgoal_strategy->clear();

    for (auto subsearch_index = ygg::uint_t { 0 }; subsearch_index < options.max_num_subsearches; ++subsearch_index)
    {
        auto local_solver = solver;
        local_solver.options.start_node = current_start_node;
        local_solver.options.goal_strategy = options.subgoal_strategy;

        event_handler->on_start_subsearch(subsearch_index);

        auto sub_result = local_solver.solve();
        statistics.add(sub_result.statistics);
        worker_statistics.resize(std::max(worker_statistics.size(), sub_result.worker_statistics.size()));
        for (size_t i = 0; i < sub_result.worker_statistics.size(); ++i)
            worker_statistics[i].add(sub_result.worker_statistics[i]);

        if (!combined_start_node && sub_result.plan)
        {
            combined_start_node = sub_result.plan->get_start_node();
            if (reached_subgoals.empty())
                reached_subgoals.push_back(detail::ReachedSubgoal<Kind> { *combined_start_node, 0 });
        }

        if constexpr (std::is_same_v<typename Solver::EventHandlerType::StatisticsType, tyr::planning::Statistics>)
            event_handler->add_subsearch_statistics(sub_result.statistics, sub_result.statistics);
        else if (local_solver.options.event_handler)
            event_handler->add_subsearch_statistics(sub_result.statistics, local_solver.options.event_handler->get_statistics());

        event_handler->on_end_subsearch(subsearch_index, sub_result.status);

        if (sub_result.status != SearchStatus::SOLVED)
            return finalize(std::move(sub_result));

        if (!sub_result.plan || !sub_result.goal_node)
        {
            auto result = SearchResult<Kind> {};
            result.status = SearchStatus::FAILED;
            return finalize(std::move(result));
        }

        auto current_node = combined_labeled_succ_nodes.empty() ? *combined_start_node : combined_labeled_succ_nodes.back().node;
        if (sub_result.plan->empty())
        {
            auto result = SearchResult<Kind> {};
            result.status =
                (combined_start_node && options.goal_strategy->is_dynamic_goal_satisfied(combined_start_node->get_state(), current_node.get_state())) ?
                    SearchStatus::SOLVED :
                    SearchStatus::EXHAUSTED;
            if (combined_start_node)
            {
                result.plan = Plan<Kind>(*combined_start_node, std::move(combined_labeled_succ_nodes));
                result.goal_node = current_node;
            }

            return finalize(std::move(result));
        }

        detail::append_plan(*sub_result.plan, current_node, combined_labeled_succ_nodes);

        if (combined_start_node && options.goal_strategy->is_dynamic_goal_satisfied(combined_start_node->get_state(), sub_result.goal_node->get_state()))
        {
            auto result = SearchResult<Kind> {};
            result.status = SearchStatus::SOLVED;
            result.plan = Plan<Kind>(*combined_start_node, std::move(combined_labeled_succ_nodes));
            result.goal_node = result.plan->get_labeled_succ_nodes().back().node;

            return finalize(std::move(result));
        }

        if (const auto cycle_begin = detail::find_reached_subgoal(reached_subgoals, *sub_result.goal_node))
        {
            auto result = SearchResult<Kind> {};
            result.status = SearchStatus::CYCLE;
            result.plan = Plan<Kind>(*combined_start_node, std::move(combined_labeled_succ_nodes));
            result.goal_node = result.plan->get_labeled_succ_nodes().back().node;
            result.cycle_range = std::pair<size_t, size_t> { *cycle_begin, result.plan->get_length() };

            return finalize(std::move(result));
        }

        current_start_node = Node<Kind>(sub_result.goal_node->get_state(), 0);
        reached_subgoals.push_back(detail::ReachedSubgoal<Kind> { *current_start_node, combined_labeled_succ_nodes.size() });
    }

    auto result = SearchResult<Kind> {};
    result.status = SearchStatus::EXHAUSTED;
    if (combined_start_node)
    {
        result.plan = Plan<Kind>(*combined_start_node, std::move(combined_labeled_succ_nodes));
        result.goal_node = !result.plan->empty() ? result.plan->get_labeled_succ_nodes().back().node : result.plan->get_start_node();
    }
    return finalize(std::move(result));
}

template<TaskKind Kind, SerializedSolverConcept<Kind> Subsolver>
struct Solver
{
    using EventHandlerType = EventHandler<Kind, Subsolver>;

    Subsolver subsolver;
    Options<Kind, Subsolver> options;

    SearchResult<Kind> solve() { return find_solution<Kind>(subsolver, options); }
};

}  // namespace tyr::planning::serialized

#endif
