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

#include "tyr/planning/ground/state_storage/tree_bitset/fact.hpp"

#include "tyr/planning/ground/state_storage/tree_bitset/context.hpp"
#include "tyr/planning/ground/task.hpp"

#include <yggdrasil/core/bit.hpp>

namespace tyr::planning
{

template<bool ThreadSafe>
FactStorageBackend<GroundTag, TreeBitset, ThreadSafe>::FactStorageBackend(Context& ctx) :
    m_bitsets(ctx.bitsets),
    m_infos(ctx.fluent_infos),
    m_num_blocks(ctx.fluent_num_blocks)
{
}

template<bool ThreadSafe>
typename FactStorageBackend<GroundTag, TreeBitset, ThreadSafe>::Packed FactStorageBackend<GroundTag, TreeBitset, ThreadSafe>::insert(const Unpacked& unpacked)
{
    m_blocks.assign(m_num_blocks, tree_bitset::Block { 0 });
    for (ygg::uint_t i = 0; i < m_infos.size(); ++i)
    {
        const auto& info = m_infos[i];
        ygg::bit::write_int(std::span(m_blocks).subspan(info.begin), tree_bitset::Block(unpacked.values[i]), info.offset, info.length);
    }

    ygg::trim_trailing_zero_blocks(m_blocks);
    return Packed { m_bitsets.insert(m_blocks) };
}

template<bool ThreadSafe>
typename FactStorageBackend<GroundTag, TreeBitset, ThreadSafe>::Packed
FactStorageBackend<GroundTag, TreeBitset, ThreadSafe>::insert(const Unpacked&, const Packed& parent, std::span<const StateDelta::FluentChange> changes)
{
    // XOR-ing old and new value into a variable's bits turns its old encoding into the new one.
    m_flips.clear();
    for (const auto& change : changes)
    {
        const auto& info = m_infos[ygg::uint_t(change.variable)];
        ygg::append_int_flips(info.begin, info.offset, info.length, tree_bitset::Block(ygg::uint_t(change.from) ^ ygg::uint_t(change.to)), m_flips);
    }
    ygg::normalize_flips(m_flips);

    return Packed { m_bitsets.insert(parent.index, m_flips) };
}

template<bool ThreadSafe>
void FactStorageBackend<GroundTag, TreeBitset, ThreadSafe>::unpack(const Packed& packed, Unpacked& unpacked)
{
    m_blocks.assign(m_num_blocks, tree_bitset::Block { 0 });
    m_bitsets.read(packed.index, std::span(m_blocks).first(packed.index.size));

    auto& values = unpacked.values;
    values.resize(m_infos.size());
    for (ygg::uint_t i = 0; i < m_infos.size(); ++i)
    {
        const auto& info = m_infos[i];
        values[i] = ygg::uint_t(ygg::bit::read_int<tree_bitset::Block>(std::span(m_blocks).subspan(info.begin), info.offset, info.length));
    }
}

template class FactStorageBackend<GroundTag, TreeBitset, false>;
template class FactStorageBackend<GroundTag, TreeBitset, true>;

}
