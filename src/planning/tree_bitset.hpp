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

#ifndef TYR_SRC_PLANNING_TREE_BITSET_HPP_
#define TYR_SRC_PLANNING_TREE_BITSET_HPP_

// Blocks and block flips of packed state rows (urd's tree bitset sets, tyr's packed ground rows), for the state storages.

#include "tyr/planning/state_storage/tree_bitset/block.hpp"

#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <span>
#include <urd/bits.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>

namespace tyr::planning
{
/// Sorts `flips` by position and combines flips of the same block, dropping flips that cancel out: what inserting
/// flips requires.
template<typename Flip>
void normalize_flips(urd::Store<urd::sequence<Flip>>& flips)
{
    std::ranges::sort(flips, {}, &Flip::position);
    auto last = std::size_t { 0 };
    for (std::size_t i = 0; i < flips.size(); ++i)
    {
        if (last > 0 && flips[last - 1].position == flips[i].position)
            flips[last - 1].mask ^= flips[i].mask;
        else
            flips[last++] = flips[i];
    }
    flips.resize(last);
    erase_if(flips, [](const Flip& flip) { return flip.mask == 0; });
}

/// Appends the flips that XOR the `length`-bit integer `value` into the bits starting at bit `offset` of block
/// `position`, as used for bit-packed integers. The integer may span two blocks.
template<std::unsigned_integral Block, typename Flip>
void append_int_flips(std::uint32_t position, std::uint8_t offset, std::uint8_t length, Block value, urd::Store<urd::sequence<Flip>>& flips)
{
    auto masks = std::array<Block, 2> {};
    urd::bits::write_int<Block>(masks, value, offset, length);
    for (std::uint32_t i = 0; i < masks.size(); ++i)
        if (masks[i] != Block { 0 })
            flips.push_back(Flip { position + i, masks[i] });
}

/// Block `i` of a dense set's 64-bit words (StateBits::significant_words), 0 past them: the low (even i) or high (odd i)
/// half of word i / 2, as urd's tree bitsets read a dense set.
inline tree_bitset::Block block_of(std::span<const std::uint64_t> words, std::size_t i) noexcept
{ return i / 2 < words.size() ? tree_bitset::Block(words[i / 2] >> (tree_bitset::bits_per_block * (i % 2))) : tree_bitset::Block { 0 }; }

/// Removes trailing zero blocks, as inserting blocks requires a nonzero last block.
template<std::unsigned_integral Block>
void trim_trailing_zero_blocks(urd::Store<urd::sequence<Block>>& blocks)
{
    while (!blocks.empty() && blocks.back() == Block { 0 })
        blocks.pop_back();
}

}

#endif
