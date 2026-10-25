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

#ifndef TYR_PLANNING_GROUND_STATE_STORAGE_TREE_VECTOR_CONTEXT_HPP_
#define TYR_PLANNING_GROUND_STATE_STORAGE_TREE_VECTOR_CONTEXT_HPP_

#include "tyr/planning/declarations.hpp"
#include "tyr/planning/state_storage.hpp"
#include "tyr/planning/state_storage/tags.hpp"
#include "tyr/planning/state_storage/tree_vector/arity.hpp"

#include <concepts>
#include <vector>
#include <yggdrasil/containers/raw_array_set.hpp>
#include <yggdrasil/containers/tree_vector_set.hpp>
#include <yggdrasil/core/config.hpp>

namespace tyr::planning
{

/**
 * Context
 */

template<bool ThreadSafe>
struct StateStorageContext<GroundTag, TreeVector, ThreadSafe>
{
    using ArraySet = ygg::RawArraySet<ygg::uint_t, hash_set::first_segment_bytes, ThreadSafe>;
    using FloatVectorSet = ygg::TreeVectorSet<ygg::float_t, tree_vector::first_segment_size, ThreadSafe, tree_vector::arity>;

    explicit StateStorageContext(const Task<GroundTag>& task);

    struct VariableInfo
    {
        ygg::uint_t begin;
        uint8_t offset;
        uint8_t length;
    };

    struct LayoutData
    {
        std::vector<VariableInfo> fluent_infos;
        ygg::uint_t fluent_array_size;
        ygg::uint_t derived_num_bits;
        ygg::uint_t derived_array_size;
    };

    // Ordered to minimize padding.
    ArraySet fluent_array_set;
    ArraySet derived_array_set;
    FloatVectorSet float_vectors;

    std::vector<VariableInfo> fluent_infos;
    ygg::uint_t derived_num_bits;

    [[nodiscard]] size_t memory_usage() const noexcept
    {
        size_t bytes = 0;
        bytes += fluent_array_set.memory_usage();
        bytes += derived_array_set.memory_usage();
        bytes += float_vectors.memory_usage();
        return bytes;
    }

private:
    explicit StateStorageContext(LayoutData&& layout_data);
};

}

#endif
