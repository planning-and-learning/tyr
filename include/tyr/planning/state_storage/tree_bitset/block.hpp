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

#include <bit>
#include <boost/dynamic_bitset.hpp>
#include <cstddef>
#include <limits>
#include <span>
#include <vector>
#include <yggdrasil/containers/tree_bitset_set.hpp>
#include <yggdrasil/core/config.hpp>

namespace tyr::planning::tree_bitset
{
/// Blocks have the width of a tree child slot, so TreeBitsetSet stores them inline without a leaf table.
using Block = ygg::uint_t;
using Flip = ygg::TreeBitsetFlip<Block>;

inline constexpr size_t bits_per_block = std::numeric_limits<Block>::digits;

/// Children per internal node of the TreeBitsetSets that store fluent facts and derived atoms.
inline constexpr size_t arity = 4;

/// Inserts `bits` into `set`. `blocks` is scratch space.
template<typename Set>
auto insert_bitset(Set& set, const boost::dynamic_bitset<>& bits, std::vector<Block>& blocks)
{
    blocks.clear();
    ygg::append_blocks(bits, blocks);
    ygg::trim_trailing_zero_blocks(blocks);
    return set.insert(blocks);
}

/// Inserts `bits` into `set` as a delta to the stored `parent`, whose bits are `parent_bits`. `blocks` and `flips` are scratch space.
/// Linear in the bitset size; used for derived atoms, whose changes are not known from action effects.
template<typename Set>
auto insert_bitset(Set& set,
                   typename Set::index_type parent,
                   const boost::dynamic_bitset<>& parent_bits,
                   const boost::dynamic_bitset<>& bits,
                   std::vector<Block>& blocks,
                   std::vector<Flip>& flips)
{
    blocks.clear();
    ygg::append_blocks(parent_bits, blocks);
    const auto num_parent_blocks = blocks.size();
    ygg::append_blocks(bits, blocks);

    flips.clear();
    ygg::append_diff_flips<Block>(std::span(blocks).first(num_parent_blocks), std::span(blocks).subspan(num_parent_blocks), flips);
    return set.insert(parent, flips);
}

/// Reads `index` from `set` into `bits`, which ends at its last set bit. `blocks` is scratch space.
template<typename Set>
void read_bitset(const Set& set, typename Set::index_type index, boost::dynamic_bitset<>& bits, std::vector<Block>& blocks)
{
    blocks.resize(index.size);
    set.read(index, blocks);
    ygg::assign_blocks<Block>(blocks, bits);
    if (!blocks.empty())
        bits.resize((blocks.size() - 1) * bits_per_block + std::bit_width(blocks.back()));
}
}

#endif
