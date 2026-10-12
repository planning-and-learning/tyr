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

#include <bit>
#include <initializer_list>

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
    m_blocks.clear();
    ygg::append_blocks(unpacked.indices, m_blocks);
    ygg::trim_trailing_zero_blocks(m_blocks);
    return Packed { m_bitsets.insert(m_blocks) };
}

template<bool ThreadSafe>
typename FactStorageBackend<LiftedTag, TreeBitset, ThreadSafe>::Packed
FactStorageBackend<LiftedTag, TreeBitset, ThreadSafe>::insert(const Packed& parent,
                                                               std::span<const ygg::Data<formalism::planning::FDRFact<formalism::FluentTag>>> added,
                                                               std::span<const ygg::Data<formalism::planning::FDRFact<formalism::FluentTag>>> deleted)
{
    constexpr auto bits = ygg::uint_t(tree_bitset::bits_per_block);

    // Lifted fluent variables are binary atoms, so each changed fact toggles exactly its atom's bit.
    m_flips.clear();
    for (const auto changes : { added, deleted })
        for (const auto& fact : changes)
            m_flips.push_back(tree_bitset::Flip { ygg::uint_t(fact.variable) / bits, tree_bitset::Block { 1 } << (ygg::uint_t(fact.variable) % bits) });
    ygg::normalize_flips(m_flips);

    return Packed { m_bitsets.insert(parent.index, m_flips) };
}

template<bool ThreadSafe>
void FactStorageBackend<LiftedTag, TreeBitset, ThreadSafe>::unpack(const Packed& packed, Unpacked& unpacked)
{
    m_blocks.resize(packed.index.size);
    m_bitsets.read(packed.index, m_blocks);

    auto& indices = unpacked.indices;
    ygg::assign_blocks<tree_bitset::Block>(m_blocks, indices);
    if (!m_blocks.empty())  // Trim to the last set bit, as FactUnpackedStorage::set does.
        indices.resize((m_blocks.size() - 1) * tree_bitset::bits_per_block + std::bit_width(m_blocks.back()));
}

template class FactStorageBackend<LiftedTag, TreeBitset, false>;
template class FactStorageBackend<LiftedTag, TreeBitset, true>;

}
