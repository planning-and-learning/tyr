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
    return Packed { ygg::insert_bitset(m_bitsets, unpacked.indices, m_blocks) };
}

template<bool ThreadSafe>
typename FactStorageBackend<LiftedTag, TreeBitset, ThreadSafe>::Packed
FactStorageBackend<LiftedTag, TreeBitset, ThreadSafe>::insert(const Unpacked&, const Packed& parent, std::span<const StateDelta::FluentChange> changes)
{
    // Lifted fluent variables are binary and stored as one bit each, so every change flips the variable's bit.
    m_flips.clear();
    for (const auto& change : changes)
    {
        const auto bit = ygg::uint_t(change.variable);
        m_flips.push_back(tree_bitset::Flip { ygg::uint_t(bit / tree_bitset::bits_per_block), tree_bitset::Block { 1 } << (bit % tree_bitset::bits_per_block) });
    }
    ygg::normalize_flips(m_flips);

    return Packed { m_bitsets.insert(parent.index, m_flips) };
}

template<bool ThreadSafe>
void FactStorageBackend<LiftedTag, TreeBitset, ThreadSafe>::unpack(const Packed& packed, Unpacked& unpacked)
{
    ygg::read_bitset(m_bitsets, packed.index, unpacked.indices, m_blocks);  // Ends at the last true atom, as FactUnpackedStorage::set does.
}

template class FactStorageBackend<LiftedTag, TreeBitset, false>;
template class FactStorageBackend<LiftedTag, TreeBitset, true>;

}
