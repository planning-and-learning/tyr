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

#ifndef TYR_SRC_PLANNING_ALGORITHMS_SEARCH_ENGINE_PARENT_STATE_HPP_
#define TYR_SRC_PLANNING_ALGORITHMS_SEARCH_ENGINE_PARENT_STATE_HPP_

#include "tyr/planning/state_index.hpp"
#include "tyr/planning/worker_state_index.hpp"

namespace tyr::planning::detail
{

template<TaskKind Kind, SearchKind Search>
struct ParentStatePolicy;

template<TaskKind Kind>
struct ParentStatePolicy<Kind, SequentialSearch>
{
    using Type = ygg::Index<State<Kind>>;

    static constexpr Type no_parent() noexcept { return Type::max(); }
    static constexpr Type make_parent(WorkerStateIndex<Kind> parent) noexcept { return parent.state; }
};

template<TaskKind Kind>
struct ParentStatePolicy<Kind, ParallelSearch>
{
    using Type = WorkerStateIndex<Kind>;

    static constexpr Type no_parent() noexcept { return Type { ygg::Index<Worker>::max(), ygg::Index<State<Kind>>::max() }; }
    static constexpr Type make_parent(WorkerStateIndex<Kind> parent) noexcept { return parent; }
};

}

#endif
