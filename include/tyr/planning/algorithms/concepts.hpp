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

#ifndef TYR_PLANNING_ALGORITHMS_CONCEPTS_HPP_
#define TYR_PLANNING_ALGORITHMS_CONCEPTS_HPP_

#include "tyr/planning/algorithms/utils.hpp"
#include "tyr/planning/declarations.hpp"

#include <chrono>
#include <concepts>
#include <optional>
#include <utility>

namespace tyr::planning
{
template<typename T, typename Kind>
concept SolverConcept = TaskKind<Kind> && requires(T& solver, std::optional<Node<Kind>> start_node) {
    { solver.solve() } -> std::same_as<SearchResult<Kind>>;
    { solver.normalize_start_node(start_node) } -> std::same_as<Node<Kind>>;
};

template<typename Options, typename Kind>
concept SolverOptionsConcept = TaskKind<Kind>
                               && requires(Options& options,
                                           std::optional<Node<Kind>> start_node,
                                           GoalStrategyPtr<Kind> goal_strategy,
                                           std::optional<ygg::uint_t> max_num_states,
                                           std::optional<std::chrono::steady_clock::duration> max_time) {
                                      options.start_node = start_node;
                                      options.goal_strategy = goal_strategy;
                                      { std::as_const(options).search_budget.max_num_states } -> std::convertible_to<std::optional<ygg::uint_t>>;
                                      options.search_budget.max_num_states = max_num_states;
                                      {
                                          std::as_const(options).search_budget.max_time
                                      } -> std::convertible_to<std::optional<std::chrono::steady_clock::duration>>;
                                      options.search_budget.max_time = max_time;
                                  };
}

#endif