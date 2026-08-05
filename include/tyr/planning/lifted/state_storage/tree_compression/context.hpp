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

#include <concepts>
#include <vector>
#include <yggdrasil/containers/tree_vector_set.hpp>
#include <yggdrasil/core/config.hpp>

namespace tyr::planning
{

/**
 * Context
 */

template<>
struct StateStorageContext<LiftedTag, TreeCompression>
{
    ygg::TreeVectorSet<ygg::uint_t> uint_vectors;
    ygg::TreeVectorSet<ygg::float_t> float_vectors;

    size_t memory_usage() const noexcept
    {
        size_t bytes = 0;
        bytes += uint_vectors.memory_usage();
        bytes += float_vectors.memory_usage();
        return bytes;
    }
};

}

#endif
