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

#ifndef TYR_PLANNING_STATE_STORAGE_TREE_BITSET_CONTEXT_HPP_
#define TYR_PLANNING_STATE_STORAGE_TREE_BITSET_CONTEXT_HPP_

#include "tyr/planning/declarations.hpp"
#include "tyr/planning/ground/state_storage.hpp"
#include "tyr/planning/state_storage.hpp"
#include "tyr/planning/state_storage/tags.hpp"
#include "tyr/planning/state_storage/tree_bitset/block.hpp"

#include <type_traits>
#include <urd/memory.hpp>
#include <urd/repr/pools/tree_bitset.hpp>
#include <urd/repr/pools/tree_vector.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/repr/values/dense_set.hpp>
#include <urd/store.hpp>
#include <yggdrasil/core/config.hpp>

namespace tyr::planning
{

template<TaskKind Kind, bool ThreadSafe>
struct StateStorageContext<Kind, TreeBitset, ThreadSafe>
{
    using FloatVectorSet = urd::Store<urd::pool<urd::Store<urd::sequence<ygg::float_t>, urd::frozen>>, urd::structural_sharing, std::conditional_t<ThreadSafe, urd::concurrent, urd::single_threaded>>;
    using BitsetSet = urd::Store<urd::pool<urd::Store<urd::set<ygg::uint_t>, urd::dense, urd::frozen>>, urd::structural_sharing, std::conditional_t<ThreadSafe, urd::concurrent, urd::single_threaded>>;

    explicit StateStorageContext(const Task<Kind>& task) : layout(task) {}

    StateLayout<Kind> layout;
    FloatVectorSet float_vectors;
    BitsetSet bitsets;

    [[nodiscard]] size_t memory_usage() const noexcept { return urd::memory_usage(float_vectors) + urd::memory_usage(bitsets); }
};

}

#endif
