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

#ifndef TYR_PLANNING_STATE_ROUTING_STATE_TRANSFER_HPP_
#define TYR_PLANNING_STATE_ROUTING_STATE_TRANSFER_HPP_

#include "tyr/planning/ground/state_builder.hpp"
#include "tyr/planning/ground/state_repository.hpp"
#include "tyr/planning/lifted/state_builder.hpp"
#include "tyr/planning/lifted/state_repository.hpp"

#include <type_traits>
#include <utility>
#include <yggdrasil/containers/unique_object_pool.hpp>

namespace tyr::planning
{

template<TaskKind Kind>
class StateTransferPool
{
public:
    using Builder = ygg::Builder<State<Kind>>;
    using LocalState = ygg::SharedObjectPoolPtr<Builder>;
    using TransferredState = ygg::UniqueObjectPoolPtr<Builder, true>;

    static_assert(std::is_nothrow_swappable_v<Builder>);

    [[nodiscard]] TransferredState export_state(LocalState&& state)
    {
        auto transferred = m_pool.get_or_allocate();
        using std::swap;
        swap(*transferred, *state);
        return transferred;
    }

    [[nodiscard]] LocalState import_state(StateRepository<Kind>& repository, TransferredState state)
    {
        auto local = repository.get_state_builder();
        using std::swap;
        swap(*local, *state);
        return local;
    }

private:
    // Handles retain the pool address and may be released by another worker.
    ygg::UniqueObjectPool<Builder, true> m_pool;
};

}

#endif
