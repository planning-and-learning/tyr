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

#ifndef TYR_PLANNING_ALGORITHMS_UTILS_HPP_
#define TYR_PLANNING_ALGORITHMS_UTILS_HPP_

#include "tyr/planning/algorithms/statistics.hpp"
#include "tyr/planning/node.hpp"
#include "tyr/planning/plan.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>
#include <yggdrasil/core/config.hpp>

namespace tyr::planning
{

enum class SearchStatus
{
    IN_PROGRESS,
    OUT_OF_TIME,
    OUT_OF_MEMORY,
    OUT_OF_STATES,
    FAILED,
    EXHAUSTED,
    CYCLE,
    SOLVED,
    UNSOLVABLE
};

enum class TransitionOutcome : uint8_t
{
    OPENED,
    RELAXED,
    DUPLICATE,
    PRUNED,
    DEAD_END,
    GOAL,
};

inline ygg::float_t compute_successor_g_value(ygg::float_t source_g_value, ygg::float_t generated_successor_g_value, CostMode mode)
{
    switch (mode)
    {
        case CostMode::UNIT:
            return ygg::FloatTolerance<ygg::float_t>::canonicalize(source_g_value + 1);
        case CostMode::GENERAL:
            return ygg::FloatTolerance<ygg::float_t>::canonicalize(generated_successor_g_value);
    }

    throw std::runtime_error("compute_successor_g_value(...): unknown action cost mode.");
}

template<TaskKind Kind>
struct SearchResult
{
    SearchStatus status = SearchStatus::IN_PROGRESS;
    std::optional<Plan<Kind>> plan = std::nullopt;
    std::optional<Node<Kind>> goal_node = std::nullopt;
    std::optional<std::pair<size_t, size_t>> cycle_range = std::nullopt;
    /// Idle time is summed worker time and may exceed the aggregate wall time.
    Statistics statistics {};
    /// Ordered by worker index. Shared task-repository memory is reported only by statistics.
    std::vector<Statistics> worker_statistics;
};

}

#endif
