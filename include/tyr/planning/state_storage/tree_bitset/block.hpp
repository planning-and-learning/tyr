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

#ifndef TYR_PLANNING_STATE_STORAGE_TREE_BITSET_BLOCK_HPP_
#define TYR_PLANNING_STATE_STORAGE_TREE_BITSET_BLOCK_HPP_

#include <cstddef>
#include <limits>
#include <yggdrasil/containers/tree_bitset_set.hpp>
#include <yggdrasil/core/config.hpp>

namespace tyr::planning::tree_bitset
{
/// Blocks have the width of a tree child slot, so TreeBitsetSet stores them inline without a leaf table.
using Block = ygg::uint_t;
using Flip = ygg::TreeBitsetFlip<Block>;

inline constexpr size_t bits_per_block = std::numeric_limits<Block>::digits;
}

#endif
