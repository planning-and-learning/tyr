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

#include "tyr/planning/lifted/state_storage/tree_bitset/fact.hpp"

#include "tyr/planning/lifted/state_storage/tree_bitset/context.hpp"
#include "tyr/planning/lifted/task.hpp"

namespace tyr::planning
{

template<bool ThreadSafe>
FactStorageBackend<LiftedTag, TreeBitset, ThreadSafe>::FactStorageBackend(Context& ctx) : m_bitsets(ctx.bitsets)
{
}

template<bool ThreadSafe>
typename FactStorageBackend<LiftedTag, TreeBitset, ThreadSafe>::Packed
FactStorageBackend<LiftedTag, TreeBitset, ThreadSafe>::insert(const Unpacked& unpacked)
{
    return Packed { tree_bitset::insert_bitset(m_bitsets, unpacked.indices, m_blocks) };
}

template<bool ThreadSafe>
typename FactStorageBackend<LiftedTag, TreeBitset, ThreadSafe>::Packed
FactStorageBackend<LiftedTag, TreeBitset, ThreadSafe>::insert(const Unpacked& unpacked, const Packed& parent, const Unpacked& parent_unpacked)
{
    return Packed { tree_bitset::insert_bitset(m_bitsets, parent.index, parent_unpacked.indices, unpacked.indices, m_blocks, m_flips) };
}

template<bool ThreadSafe>
void FactStorageBackend<LiftedTag, TreeBitset, ThreadSafe>::unpack(const Packed& packed, Unpacked& unpacked)
{
    tree_bitset::read_bitset(m_bitsets, packed.index, unpacked.indices, m_blocks);  // Ends at the last true atom, as FactUnpackedStorage::set does.
}

template class FactStorageBackend<LiftedTag, TreeBitset, false>;
template class FactStorageBackend<LiftedTag, TreeBitset, true>;

}
