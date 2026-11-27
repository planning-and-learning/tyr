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

#include "tyr/declarations.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/states/state_view_concepts.hpp"

#include <memory>
#include <urd/elements/index.hpp>

namespace tyr::planning
{

template<TaskKind Kind, typename V>
class PruningStrategy
{
    static_assert(StateBuilderViewConcept<V, Kind>, "PruningStrategy: V is a view of a state of Kind (StateBuilderViewConcept).");

public:
    PruningStrategy() = default;
    virtual ~PruningStrategy() = default;

    static PruningStrategyPtr<Kind, V> create();

    /// Custom strategies opt into parallel search by returning an independently usable worker. Each returned worker is called serially, without OS-thread
    /// affinity. State views retain the repository wrappers that produced them; these wrappers may differ in hash-distributed mode and need not identify the
    /// logical owner in shared mode. A strategy must not re-enter the search or wait for work from the same logical worker.
    [[nodiscard]] virtual PruningStrategyPtr<Kind, V> make_worker(urd::index<Worker>) const { return nullptr; }

    virtual bool should_prune_state(const V&) { return false; }

    virtual bool should_prune_successor_state(const V&, const V&, bool) { return false; }

protected:
    PruningStrategy(const PruningStrategy&) = default;
    PruningStrategy& operator=(const PruningStrategy&) = default;
    PruningStrategy(PruningStrategy&&) = default;
    PruningStrategy& operator=(PruningStrategy&&) = default;
};

namespace detail
{

template<TaskKind Kind, typename V = StateBuilderView<Kind>>
class NoPruningStrategy final : public PruningStrategy<Kind, V>
{
public:
    [[nodiscard]] PruningStrategyPtr<Kind, V> make_worker(urd::index<Worker>) const override { return PruningStrategy<Kind, V>::create(); }
};

}

template<TaskKind Kind, typename V>
PruningStrategyPtr<Kind, V> PruningStrategy<Kind, V>::create()
{ return std::make_shared<detail::NoPruningStrategy<Kind, V>>(); }

}

#endif
