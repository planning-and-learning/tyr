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

#include "tyr/planning/state_storage/tree_bitset/atom.hpp"

#include "tyr/planning/ground/state_storage.hpp"
#include "tyr/planning/ground/state_storage/tree_bitset/context.hpp"
#include "tyr/planning/ground/task.hpp"
#include "tyr/planning/lifted/state_storage.hpp"
#include "tyr/planning/lifted/state_storage/tree_bitset/context.hpp"
#include "tyr/planning/lifted/task.hpp"

namespace tyr::planning
{
namespace
{
/// Ground derived atoms span all derived atoms of the task.
template<bool ThreadSafe>
void restore_unpacked_size(AtomUnpackedStorage<GroundTag>& unpacked, const StateStorageContext<GroundTag, TreeBitset, ThreadSafe>& context)
{
    unpacked.indices.resize(context.derived_num_bits);
}

/// Lifted derived atoms end at their last true atom, as read_bitset leaves them.
template<bool ThreadSafe>
void restore_unpacked_size(AtomUnpackedStorage<LiftedTag>&, const StateStorageContext<LiftedTag, TreeBitset, ThreadSafe>&)
{
}
}

template<TaskKind Kind, bool ThreadSafe>
AtomStorageBackend<Kind, TreeBitset, ThreadSafe>::AtomStorageBackend(Context& ctx) : m_context(ctx)
{
}

template<TaskKind Kind, bool ThreadSafe>
typename AtomStorageBackend<Kind, TreeBitset, ThreadSafe>::Packed AtomStorageBackend<Kind, TreeBitset, ThreadSafe>::insert(const Unpacked& unpacked)
{
    return Packed { tree_bitset::insert_bitset(m_context.bitsets, unpacked.indices, m_blocks) };
}

template<TaskKind Kind, bool ThreadSafe>
typename AtomStorageBackend<Kind, TreeBitset, ThreadSafe>::Packed
AtomStorageBackend<Kind, TreeBitset, ThreadSafe>::insert(const Unpacked& unpacked, const Packed& parent, const Unpacked& parent_unpacked)
{
    return Packed { tree_bitset::insert_bitset(m_context.bitsets, parent.index, parent_unpacked.indices, unpacked.indices, m_blocks, m_flips) };
}

template<TaskKind Kind, bool ThreadSafe>
void AtomStorageBackend<Kind, TreeBitset, ThreadSafe>::unpack(const Packed& packed, Unpacked& unpacked)
{
    tree_bitset::read_bitset(m_context.bitsets, packed.index, unpacked.indices, m_blocks);
    restore_unpacked_size(unpacked, m_context);
}

template class AtomStorageBackend<GroundTag, TreeBitset, false>;
template class AtomStorageBackend<GroundTag, TreeBitset, true>;
template class AtomStorageBackend<LiftedTag, TreeBitset, false>;
template class AtomStorageBackend<LiftedTag, TreeBitset, true>;

}
