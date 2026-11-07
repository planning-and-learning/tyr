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

#include <urd/tags.hpp>

namespace tyr::planning
{
/// Stores state components as vectors in urd pools that carry the extra urd tags `Tags`.
template<typename... Tags>
struct Vectors
{
};

/// The vectors as trees: a successor shares all but the paths to its changed values with its parent.
using TreeVector = Vectors<urd::structural_sharing>;

/// The vectors whole, in hash tables.
using Hashset = Vectors<>;

/// Like TreeVector, but stores fluent facts and derived atoms as position-stable bitset trees, inserting successors as deltas to their parent.
struct TreeBitset
{
};
}

#endif
