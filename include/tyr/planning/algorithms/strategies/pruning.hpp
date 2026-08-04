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

#ifndef TYR_PLANNING_ALGORITHMS_STRATEGIES_PRUNING_HPP_
#define TYR_PLANNING_ALGORITHMS_STRATEGIES_PRUNING_HPP_

#include "tyr/planning/declarations.hpp"
#include "tyr/planning/state_view.hpp"
#include "tyr/planning/worker_index.hpp"

#include <memory>

namespace tyr::planning
{

template<TaskKind Kind>
class PruningStrategy
{
public:
    virtual ~PruningStrategy() = default;

    static PruningStrategyPtr<Kind> create();

    /// Custom strategies opt into parallel search by returning an independently usable worker.
    [[nodiscard]] virtual PruningStrategyPtr<Kind> make_worker(ygg::Index<Worker>) const { return nullptr; }

    virtual bool should_prune_state(const StateView<Kind>& state) { return false; }

    virtual bool should_prune_successor_state(const StateView<Kind>& state, const StateView<Kind>& succ_state, bool is_new_succ) { return false; }
};

namespace detail
{

template<TaskKind Kind>
class NoPruningStrategy final : public PruningStrategy<Kind>
{
public:
    [[nodiscard]] PruningStrategyPtr<Kind> make_worker(ygg::Index<Worker>) const override { return PruningStrategy<Kind>::create(); }
};

}

template<TaskKind Kind>
PruningStrategyPtr<Kind> PruningStrategy<Kind>::create()
{
    return std::make_shared<detail::NoPruningStrategy<Kind>>();
}

}

#endif
