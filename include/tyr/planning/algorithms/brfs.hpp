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

#ifndef TYR_PLANNING_ALGORITHMS_BRFS_HPP_
#define TYR_PLANNING_ALGORITHMS_BRFS_HPP_

#include "tyr/planning/algorithms/utils.hpp"
#include "tyr/planning/declarations.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>

namespace tyr::planning::brfs
{

template<TaskKind Kind>
struct Options
{
    static constexpr CostMode cost_mode = CostMode::UNIT;

    /// Optional initial node. It must belong to this task; search materializes its state in the caller repository and restarts unit depth at zero.
    std::optional<Node<Kind>> start_node = std::nullopt;
    EventHandlerPtr<Kind> event_handler = nullptr;
    PruningStrategyPtr<Kind> pruning_strategy = nullptr;
    GoalStrategyPtr<Kind> goal_strategy = nullptr;
    /// Maximum solve-local states, including an unsolved start. Existing repository population is ignored, but a pre-interned state counts when first
    /// encountered, and an over-limit successor may already be interned. A satisfied start may solve even when this is zero.
    ygg::uint_t max_num_states = std::numeric_limits<ygg::uint_t>::max();
    std::optional<std::chrono::steady_clock::duration> max_time = std::nullopt;
    /// Values above one enable parallel search. A concurrent state repository shares storage; a private one is hash-distributed.
    size_t num_search_workers = 1;
    DistHashMode dist_hash_mode = DistHashMode::RANDOM;
    bool collect_destination_lock_statistics = false;
    uint64_t random_seed = 0;
    bool shuffle_labeled_succ_nodes = false;

    Options() = default;
};

template<TaskKind Kind>
SearchResult<Kind> find_solution(Task<Kind>& task,
                                 StateRepository<Kind>& state_repository,
                                 AxiomEvaluator<Kind>& axiom_evaluator,
                                 SuccessorGenerator<Kind>& successor_generator,
                                 const Options<Kind>& options = Options<Kind>());

/// @brief Adapter that exposes BrFS search through the generic solver interface.
template<TaskKind Kind>
struct Solver
{
    using EventHandlerType = EventHandler<Kind>;

    TaskPtr<Kind> task;
    StateRepositoryPtr<Kind> state_repository;
    AxiomEvaluatorPtr<Kind> axiom_evaluator;
    SuccessorGeneratorPtr<Kind> successor_generator;
    Options<Kind> options;

    Node<Kind> normalize_start_node(std::optional<Node<Kind>> start_node)
    {
        if (!task)
            throw std::invalid_argument("brfs::Solver::normalize_start_node(): task is required.");
        if (!state_repository)
            throw std::invalid_argument("brfs::Solver::normalize_start_node(): state repository is required.");
        if (!axiom_evaluator)
            throw std::invalid_argument("brfs::Solver::normalize_start_node(): axiom evaluator is required.");
        if (!start_node)
            start_node = options.start_node;

        return tyr::planning::normalize_start_node(*task, *state_repository, *axiom_evaluator, std::move(start_node));
    }

    SearchResult<Kind> solve()
    {
        if (!task)
            throw std::invalid_argument("brfs::Solver::solve(): task is required.");
        if (!state_repository)
            throw std::invalid_argument("brfs::Solver::solve(): state repository is required.");
        if (!axiom_evaluator)
            throw std::invalid_argument("brfs::Solver::solve(): axiom evaluator is required.");
        if (!successor_generator)
            throw std::invalid_argument("brfs::Solver::solve(): successor generator is required.");

        return find_solution(*task, *state_repository, *axiom_evaluator, *successor_generator, options);
    }
};

}

#endif
