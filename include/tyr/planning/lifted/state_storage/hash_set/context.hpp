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

#ifndef TYR_PLANNING_LIFTED_STATE_STORAGE_HASH_SET_CONTEXT_HPP_
#define TYR_PLANNING_LIFTED_STATE_STORAGE_HASH_SET_CONTEXT_HPP_

#include "tyr/planning/declarations.hpp"
#include <urd/repr/pools/pools.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include "tyr/planning/state_storage.hpp"
#include "tyr/planning/state_storage/tags.hpp"

#include <concepts>
#include <vector>
#include <yggdrasil/core/config.hpp>

namespace tyr::planning
{

/**
 * Context
 */

template<bool ThreadSafe>
struct StateStorageContext<LiftedTag, HashSet, ThreadSafe>
{
    using UintVectorSet = urd::Store<urd::pool<urd::Store<urd::sequence<ygg::uint_t>, urd::frozen>>, std::conditional_t<ThreadSafe, urd::concurrent, urd::single_threaded>>;
    using FloatVectorSet = urd::Store<urd::pool<urd::Store<urd::sequence<ygg::float_t>, urd::frozen>>, std::conditional_t<ThreadSafe, urd::concurrent, urd::single_threaded>>;

    UintVectorSet uint_vec_set;
    FloatVectorSet float_vec_set;

    [[nodiscard]] size_t memory_usage() const noexcept
    {
        size_t bytes = 0;
        bytes += urd::memory_usage(uint_vec_set);
        bytes += urd::memory_usage(float_vec_set);
        return bytes;
    }
};

}

#endif
