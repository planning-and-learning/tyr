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

#ifndef TYR_PLANNING_LIFTED_STATE_STORAGE_TREE_COMPRESSION_CONTEXT_HPP_
#define TYR_PLANNING_LIFTED_STATE_STORAGE_TREE_COMPRESSION_CONTEXT_HPP_

#include "tyr/planning/declarations.hpp"
#include "tyr/planning/state_storage.hpp"
#include "tyr/planning/state_storage/tags.hpp"

#include <atomic>
#include <boost/dynamic_bitset.hpp>
#include <concepts>
#include <vector>
#include <yggdrasil/containers/tree_bitset_set.hpp>
#include <yggdrasil/containers/tree_vector_set.hpp>
#include <yggdrasil/core/config.hpp>

namespace tyr::planning
{

namespace detail
{
/// Shared by all ThreadSafe instantiations, since unpacked storage can move between them.
inline ygg::uint_t next_lifted_tree_context_identity() noexcept
{
    static std::atomic<ygg::uint_t> next { 0 };
    return next.fetch_add(1, std::memory_order_relaxed);
}
}

/**
 * Context
 */

template<bool ThreadSafe>
struct StateStorageContext<LiftedTag, TreeCompression, ThreadSafe>
{
    using UintVectorSet = ygg::TreeVectorSet<ygg::uint_t, 32, ThreadSafe>;
    using FloatVectorSet = ygg::TreeVectorSet<ygg::float_t, 32, ThreadSafe>;
    using BitsetSet = ygg::TreeBitsetSet<boost::dynamic_bitset<>::block_type, 32, ThreadSafe>;

    /// Distinguishes contexts, so that unpacked storage only reuses packed indices of its own context.
    ygg::uint_t identity = detail::next_lifted_tree_context_identity();

    UintVectorSet uint_vectors;
    FloatVectorSet float_vectors;
    BitsetSet bitsets;

    size_t memory_usage() const noexcept
    {
        size_t bytes = 0;
        bytes += uint_vectors.memory_usage();
        bytes += float_vectors.memory_usage();
        bytes += bitsets.memory_usage();
        return bytes;
    }
};

}

#endif
