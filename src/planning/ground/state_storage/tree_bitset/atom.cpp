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

#include "tyr/planning/ground/state_storage/tree_bitset/atom.hpp"

#include "tyr/planning/ground/state_storage/tree_bitset/context.hpp"
#include "tyr/planning/ground/task.hpp"

#include <span>

namespace tyr::planning
{

template<bool ThreadSafe>
AtomStorageBackend<GroundTag, TreeBitset, ThreadSafe>::AtomStorageBackend(Context& ctx) : m_bitsets(ctx.bitsets), m_num_bits(ctx.derived_num_bits)
{
}

template<bool ThreadSafe>
typename AtomStorageBackend<GroundTag, TreeBitset, ThreadSafe>::Packed
AtomStorageBackend<GroundTag, TreeBitset, ThreadSafe>::insert(const Unpacked& unpacked)
{
    m_blocks.clear();
    ygg::append_blocks(unpacked.indices, m_blocks);
    ygg::trim_trailing_zero_blocks(m_blocks);
    return Packed { m_bitsets.insert(m_blocks) };
}

template<bool ThreadSafe>
typename AtomStorageBackend<GroundTag, TreeBitset, ThreadSafe>::Packed
AtomStorageBackend<GroundTag, TreeBitset, ThreadSafe>::insert(const Unpacked& unpacked, const Packed& parent, const Unpacked& parent_unpacked)
{
    m_blocks.clear();
    ygg::append_blocks(parent_unpacked.indices, m_blocks);
    const auto num_parent_blocks = m_blocks.size();
    ygg::append_blocks(unpacked.indices, m_blocks);

    m_flips.clear();
    ygg::append_diff_flips<tree_bitset::Block>(std::span(m_blocks).first(num_parent_blocks), std::span(m_blocks).subspan(num_parent_blocks), m_flips);
    return Packed { m_bitsets.insert(parent.index, m_flips) };
}

template<bool ThreadSafe>
void AtomStorageBackend<GroundTag, TreeBitset, ThreadSafe>::unpack(const Packed& packed, Unpacked& unpacked)
{
    m_blocks.resize(packed.index.size);
    m_bitsets.read(packed.index, m_blocks);

    auto& indices = unpacked.indices;
    ygg::assign_blocks<tree_bitset::Block>(m_blocks, indices);
    indices.resize(m_num_bits);  // Ground derived atoms always span all derived atoms.
}

template class AtomStorageBackend<GroundTag, TreeBitset, false>;
template class AtomStorageBackend<GroundTag, TreeBitset, true>;

}
