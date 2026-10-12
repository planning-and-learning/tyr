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

#include "tyr/planning/lifted/state_storage/tree_compression/fact.hpp"

#include "tyr/planning/lifted/state_storage/tree_compression/context.hpp"
#include "tyr/planning/lifted/task.hpp"

#include <algorithm>
#include <bit>
#include <cassert>
#include <iterator>
#include <vector>

namespace tyr::planning
{

template<bool ThreadSafe>
FactStorageBackend<LiftedTag, TreeCompression, ThreadSafe>::FactStorageBackend(Context& ctx) : m_bitsets(ctx.bitsets), m_identity(ctx.identity)
{
}

template<bool ThreadSafe>
typename FactStorageBackend<LiftedTag, TreeCompression, ThreadSafe>::Packed
FactStorageBackend<LiftedTag, TreeCompression, ThreadSafe>::insert(Unpacked& unpacked)
{
    using Flip = ygg::TreeBitsetFlip<Unpacked::Block>;

    const auto& bits = unpacked.indices;
    assert(bits.empty() || bits.test(bits.size() - 1));
    const auto num_blocks = ygg::uint_t(bits.num_blocks());

    auto index = ygg::TreeBitsetIndex<Unpacked::Block> {};
    if (unpacked.base_owner == m_identity)
    {
        // Flips toggle, so flips of the same block cancel out pairwise.
        auto& flips = unpacked.flips;
        std::ranges::sort(flips, {}, &Flip::position);
        m_flips.clear();
        for (const auto& flip : flips)
        {
            if (!m_flips.empty() && m_flips.back().position == flip.position)
                m_flips.back().mask ^= flip.mask;
            else
                m_flips.push_back(flip);
        }
        std::erase_if(m_flips, [](const Flip& flip) { return flip.mask == 0; });

        index = m_bitsets.insert(unpacked.base, m_flips, num_blocks);
    }
    else
    {
        m_blocks.clear();
        boost::to_block_range(bits, std::back_inserter(m_blocks));
        index = m_bitsets.insert(m_blocks);
    }

    unpacked.rebase(index, m_identity);
    return Packed { index };
}

template<bool ThreadSafe>
void FactStorageBackend<LiftedTag, TreeCompression, ThreadSafe>::unpack(const Packed& packed, Unpacked& unpacked)
{
    m_blocks.resize(packed.index.size);
    m_bitsets.read(packed.index, m_blocks);

    auto& indices = unpacked.indices;
    indices.clear();
    indices.append(m_blocks.begin(), m_blocks.end());
    if (!m_blocks.empty())  // Trim to the last set bit, as FactUnpackedStorage::set does.
        indices.resize((m_blocks.size() - 1) * boost::dynamic_bitset<>::bits_per_block + std::bit_width(m_blocks.back()));

    unpacked.rebase(packed.index, m_identity);
}

template class FactStorageBackend<LiftedTag, TreeCompression, false>;
template class FactStorageBackend<LiftedTag, TreeCompression, true>;

}
