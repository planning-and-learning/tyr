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

#ifndef TYR_PLANNING_ALGORITHMS_SIW_HPP_
#define TYR_PLANNING_ALGORITHMS_SIW_HPP_

#include "tyr/planning/algorithms/iw.hpp"
#include "tyr/planning/algorithms/serialized.hpp"
#include "tyr/planning/algorithms/siw/event_handler.hpp"
#include "tyr/planning/algorithms/siw/statistics.hpp"
#include "tyr/planning/algorithms/strategies/goal.hpp"
#include "tyr/planning/algorithms/utils.hpp"
#include "tyr/planning/declarations.hpp"

#include <limits>
#include <optional>
#include <stdexcept>
#include <utility>

namespace tyr::planning::siw
{

template<TaskKind Kind>
struct Options
{
    /// Optional initial node for the first serialized IW subsearch; when a subsearch runs, it must belong to the underlying solver's task.
    std::optional<Node<Kind>> start_node = std::nullopt;
    EventHandlerPtr<Kind> event_handler = nullptr;
    GoalStrategyPtr<Kind> subgoal_strategy = nullptr;
    GoalStrategyPtr<Kind> goal_strategy = nullptr;
    ygg::uint_t max_num_subsearches = std::numeric_limits<ygg::uint_t>::max();

    Options() = default;
};

template<TaskKind Kind>
SearchResult<Kind> find_solution(iw::Solver<Kind>& iw_solver, const Options<Kind>& options = Options<Kind>())
{
    if (!iw_solver.brfs_solver.task)
        throw std::invalid_argument("siw::find_solution(...): IW BRFS task is required.");
    if (!iw_solver.brfs_solver.state_repository)
        throw std::invalid_argument("siw::find_solution(...): IW BRFS state repository is required.");
    if (!iw_solver.brfs_solver.axiom_evaluator)
        throw std::invalid_argument("siw::find_solution(...): IW BRFS axiom evaluator is required.");
    if (!iw_solver.brfs_solver.successor_generator)
        throw std::invalid_argument("siw::find_solution(...): IW BRFS successor generator is required.");

    auto serialized_options = serialized::Options<Kind, iw::Solver<Kind>> {};
    serialized_options.start_node = options.start_node;
    serialized_options.event_handler = options.event_handler ? options.event_handler : DefaultEventHandler<Kind>::create();
    serialized_options.subgoal_strategy =
        options.subgoal_strategy ? options.subgoal_strategy : SerializedGoalStrategy<Kind>::create(*iw_solver.brfs_solver.task);
    serialized_options.goal_strategy = options.goal_strategy ? options.goal_strategy : ConjunctiveGoalStrategy<Kind>::create(*iw_solver.brfs_solver.task);
    serialized_options.max_num_subsearches = options.max_num_subsearches;
    serialized_options.max_time =
        iw_solver.options.search_budget.max_time ? iw_solver.options.search_budget.max_time : iw_solver.brfs_solver.options.search_budget.max_time;

    if (!iw_solver.options.event_handler)
        iw_solver.options.event_handler = iw::DefaultEventHandler<Kind>::create();

    return serialized::find_solution(iw_solver, serialized_options);
}

template<TaskKind Kind>
struct Solver
{
    using EventHandlerType = EventHandler<Kind>;

    iw::Solver<Kind> iw_solver;
    Options<Kind> options;

    Node<Kind> normalize_start_node(std::optional<Node<Kind>> start_node)
    {
        if (!start_node)
            start_node = options.start_node;
        return iw_solver.normalize_start_node(std::move(start_node));
    }

    SearchResult<Kind> solve() { return find_solution(iw_solver, options); }
};

}  // namespace tyr::planning::siw

#endif
