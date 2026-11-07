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
#include "tyr/planning/ground/task.hpp"
#include "tyr/planning/lifted/state_storage.hpp"
#include "tyr/planning/state_storage/tree_bitset/context.hpp"
#include "tyr/planning/lifted/task.hpp"
#include "../../tree_bitset.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>

namespace tyr::planning
{
template<TaskKind Kind, bool ThreadSafe>
AtomStorageBackend<Kind, TreeBitset, ThreadSafe>::AtomStorageBackend(Context& ctx) : m_context(&ctx)
{
}

template<TaskKind Kind, bool ThreadSafe>
typename AtomStorageBackend<Kind, TreeBitset, ThreadSafe>::Packed AtomStorageBackend<Kind, TreeBitset, ThreadSafe>::insert(const Unpacked& unpacked)
{ return m_context->bitsets.insert(unpacked.indices).first; }

template<TaskKind Kind, bool ThreadSafe>
typename AtomStorageBackend<Kind, TreeBitset, ThreadSafe>::Packed
AtomStorageBackend<Kind, TreeBitset, ThreadSafe>::insert(const Unpacked& unpacked, const Packed& parent, const Unpacked& parent_unpacked)
{
    // One flip per block in which the two rows differ, by increasing position.
    const auto from = parent_unpacked.indices.significant_words();
    const auto to = unpacked.indices.significant_words();
    m_flips.clear();
    for (std::size_t i = 0; i < 2 * std::max(from.size(), to.size()); ++i)
        if (const auto mask = tree_bitset::Block(block_of(from, i) ^ block_of(to, i)); mask != tree_bitset::Block { 0 })
            m_flips.push_back(tree_bitset::Flip { static_cast<std::uint32_t>(i), mask });
    return m_context->bitsets.insert_flips(parent, m_flips);
}

template<TaskKind Kind, bool ThreadSafe>
void AtomStorageBackend<Kind, TreeBitset, ThreadSafe>::unpack(const Packed& packed, Unpacked& unpacked)
{ m_context->bitsets.read(packed, unpacked.indices); }

template class AtomStorageBackend<GroundTag, TreeBitset, false>;
template class AtomStorageBackend<GroundTag, TreeBitset, true>;
template class AtomStorageBackend<LiftedTag, TreeBitset, false>;
template class AtomStorageBackend<LiftedTag, TreeBitset, true>;

}
