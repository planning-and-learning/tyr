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

// boost::dynamic_bitset to and from urd's tree bitset sets (urd::repr::TreeBitsetSet: insert_blocks, insert_flips,
// read_blocks, flip_type), for the tree_bitset state storages.

#include <algorithm>
#include <array>
#include <bit>
#include <boost/dynamic_bitset.hpp>
#include <boost/iterator/function_output_iterator.hpp>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <urd/repr/containers/bit.hpp>
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
    urd::repr::bit::write_int<Block>(masks, value, offset, length);
    for (std::uint32_t i = 0; i < masks.size(); ++i)
        if (masks[i] != Block { 0 })
            flips.push_back(Flip { position + i, masks[i] });
}

/// Appends one flip per block in which `from` and `to` differ, in increasing position order.
template<std::unsigned_integral Block, typename Flip>
void append_diff_flips(std::span<const Block> from, std::span<const Block> to, urd::Store<urd::sequence<Flip>>& flips)
{
    for (std::size_t i = 0; i < std::max(from.size(), to.size()); ++i)
    {
        const auto mask = Block((i < from.size() ? from[i] : Block { 0 }) ^ (i < to.size() ? to[i] : Block { 0 }));
        if (mask != Block { 0 })
            flips.push_back(Flip { static_cast<std::uint32_t>(i), mask });
    }
}

/// Appends the bits of `bitset` as blocks of type Block, which may be narrower than the blocks of `bitset`.
template<std::unsigned_integral Block, std::unsigned_integral BitsetBlock, typename Allocator>
    requires(sizeof(BitsetBlock) % sizeof(Block) == 0)
void append_blocks(const boost::dynamic_bitset<BitsetBlock, Allocator>& bitset, urd::Store<urd::sequence<Block>>& blocks)
{
    constexpr auto parts = std::numeric_limits<BitsetBlock>::digits / std::numeric_limits<Block>::digits;
    boost::to_block_range(bitset,
                          boost::make_function_output_iterator(
                              [&](BitsetBlock block)
                              {
                                  for (std::size_t i = 0; i < parts; ++i)
                                      blocks.push_back(Block(block >> (i * std::numeric_limits<Block>::digits)));
                              }));
}

/// Removes trailing zero blocks, as inserting blocks requires a nonzero last block.
template<std::unsigned_integral Block>
void trim_trailing_zero_blocks(urd::Store<urd::sequence<Block>>& blocks)
{
    while (!blocks.empty() && blocks.back() == Block { 0 })
        blocks.pop_back();
}

/// Replaces the bits of `bitset` by those of `blocks`. The size becomes a multiple of the block width of `bitset`.
template<std::unsigned_integral Block, std::unsigned_integral BitsetBlock, typename Allocator>
    requires(sizeof(BitsetBlock) % sizeof(Block) == 0)
void assign_blocks(std::span<const Block> blocks, boost::dynamic_bitset<BitsetBlock, Allocator>& bitset)
{
    constexpr auto parts = std::numeric_limits<BitsetBlock>::digits / std::numeric_limits<Block>::digits;
    bitset.clear();
    for (std::size_t i = 0; i < blocks.size(); i += parts)
    {
        auto block = BitsetBlock { 0 };
        for (std::size_t j = 0; j < parts && i + j < blocks.size(); ++j)
            block |= BitsetBlock(blocks[i + j]) << (j * std::numeric_limits<Block>::digits);
        bitset.append(block);
    }
}

/// Inserts `bits` into `set`. `blocks` is scratch space.
template<typename Set, std::unsigned_integral BitsetBlock, typename Allocator>
    requires(sizeof(BitsetBlock) % sizeof(typename Set::block_type) == 0)
typename Set::handle_type insert_bitset(Set& set, const boost::dynamic_bitset<BitsetBlock, Allocator>& bits, urd::Store<urd::sequence<typename Set::block_type>>& blocks)
{
    blocks.clear();
    append_blocks(bits, blocks);
    trim_trailing_zero_blocks(blocks);
    return set.insert_blocks(blocks);
}

/// Inserts `bits` into `set` as a delta to the stored `parent`, whose bits are `parent_bits`. Linear in the bitset
/// size; when the changed bits are known, inserting their flips directly is cheaper. `blocks` and `flips` are scratch space.
template<typename Set, std::unsigned_integral BitsetBlock, typename Allocator>
    requires(sizeof(BitsetBlock) % sizeof(typename Set::block_type) == 0)
typename Set::handle_type insert_bitset(Set& set,
                                        typename Set::handle_type parent,
                                        const boost::dynamic_bitset<BitsetBlock, Allocator>& parent_bits,
                                        const boost::dynamic_bitset<BitsetBlock, Allocator>& bits,
                                        urd::Store<urd::sequence<typename Set::block_type>>& blocks,
                                        urd::Store<urd::sequence<typename Set::flip_type>>& flips)
{
    using Block = typename Set::block_type;
    blocks.clear();
    append_blocks(parent_bits, blocks);
    const auto num_parent_blocks = blocks.size();
    append_blocks(bits, blocks);

    flips.clear();
    append_diff_flips<Block>(std::span<const Block>(blocks).first(num_parent_blocks), std::span<const Block>(blocks).subspan(num_parent_blocks), flips);
    return set.insert_flips(parent, flips);
}

/// Reads `index` from `set` into `bits`, which then ends at its last set bit. `blocks` is scratch space.
template<typename Set, std::unsigned_integral BitsetBlock, typename Allocator>
    requires(sizeof(BitsetBlock) % sizeof(typename Set::block_type) == 0)
void read_bitset(const Set& set, typename Set::handle_type index, boost::dynamic_bitset<BitsetBlock, Allocator>& bits, urd::Store<urd::sequence<typename Set::block_type>>& blocks)
{
    using Block = typename Set::block_type;
    blocks.resize(index.size);
    set.read_blocks(index, blocks);
    assign_blocks<Block>(blocks, bits);
    if (!blocks.empty())
        bits.resize((blocks.size() - 1) * std::numeric_limits<Block>::digits + std::bit_width(blocks.back()));
}

}

#endif
