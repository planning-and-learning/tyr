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

#ifndef TYR_PLANNING_STATE_STORAGE_TAGS_HPP_
#define TYR_PLANNING_STATE_STORAGE_TAGS_HPP_

#include <concepts>

namespace tyr::planning
{
/// Stores state components as tree-compressed vectors.
struct TreeVector
{
};

/// Like TreeVector, but stores fluent facts and derived atoms as position-stable bitset trees, inserting successors as deltas to their parent.
struct TreeBitset
{
};

/// Numeric storage, which both tree-compressed policies share, is specialized on this concept.
template<typename Tag>
concept TreeCompressionTag = std::same_as<Tag, TreeVector> || std::same_as<Tag, TreeBitset>;

struct HashSet
{
};
}

#endif
