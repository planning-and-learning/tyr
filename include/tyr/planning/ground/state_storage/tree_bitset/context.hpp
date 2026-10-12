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

#ifndef TYR_PLANNING_GROUND_STATE_STORAGE_TREE_BITSET_CONTEXT_HPP_
#define TYR_PLANNING_GROUND_STATE_STORAGE_TREE_BITSET_CONTEXT_HPP_

#include "tyr/planning/declarations.hpp"
#include "tyr/planning/state_storage.hpp"
#include "tyr/planning/state_storage/tags.hpp"
#include "tyr/planning/state_storage/tree_vector/arity.hpp"
#include "tyr/planning/state_storage/tree_bitset/block.hpp"

#include <cstdint>
#include <vector>
#include <yggdrasil/containers/tree_bitset_set.hpp>
#include <yggdrasil/containers/tree_vector_set.hpp>
#include <yggdrasil/core/config.hpp>

namespace tyr::planning
{

/**
 * Context
 */

template<bool ThreadSafe>
struct StateStorageContext<GroundTag, TreeBitset, ThreadSafe>
{
    using FloatVectorSet = ygg::TreeVectorSet<ygg::float_t, 32, ThreadSafe, tree_vector::arity>;
    using BitsetSet = ygg::TreeBitsetSet<tree_bitset::Block, 32, ThreadSafe, tree_bitset::arity>;

    explicit StateStorageContext(const Task<GroundTag>& task);

    /// Bit range of a fluent FDR variable's value within the packed blocks.
    struct VariableInfo
    {
        ygg::uint_t begin;
        uint8_t offset;
        uint8_t length;
    };

    std::vector<VariableInfo> fluent_infos;
    ygg::uint_t fluent_num_blocks;
    ygg::uint_t derived_num_bits;

    FloatVectorSet float_vectors;
    BitsetSet bitsets;

    size_t memory_usage() const noexcept
    {
        size_t bytes = 0;
        bytes += float_vectors.memory_usage();
        bytes += bitsets.memory_usage();
        return bytes;
    }
};

}

#endif
