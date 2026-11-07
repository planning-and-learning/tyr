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
#include "../../../tree_bitset.hpp"

#include <urd/repr/containers/bit.hpp>

namespace tyr::planning
{

template<bool ThreadSafe>
FactStorageBackend<GroundTag, TreeBitset, ThreadSafe>::FactStorageBackend(Context& ctx) :
    m_bitsets(&ctx.bitsets),
    m_layout(&ctx.layout)
{
}

template<bool ThreadSafe>
typename FactStorageBackend<GroundTag, TreeBitset, ThreadSafe>::Packed FactStorageBackend<GroundTag, TreeBitset, ThreadSafe>::insert(const Unpacked& unpacked)
{
    const auto& infos = m_layout->fluent_infos;
    m_blocks.assign(m_layout->fluent_num_blocks, tree_bitset::Block { 0 });
    for (ygg::uint_t i = 0; i < infos.size(); ++i)
        urd::repr::bit::write_int(std::span(m_blocks).subspan(infos[i].begin), static_cast<tree_bitset::Block>(unpacked.values[i]), infos[i].offset, infos[i].length);

    trim_trailing_zero_blocks(m_blocks);
    return m_bitsets->insert_blocks(m_blocks);
}

template<bool ThreadSafe>
typename FactStorageBackend<GroundTag, TreeBitset, ThreadSafe>::Packed
FactStorageBackend<GroundTag, TreeBitset, ThreadSafe>::insert(const Unpacked&, const Packed& parent, std::span<const StateDelta::FluentChange> changes)
{
    // XOR-ing old and new value into a variable's bits turns its old encoding into the new one.
    m_flips.clear();
    for (const auto& change : changes)
    {
        const auto& info = m_layout->fluent_infos[static_cast<ygg::uint_t>(change.variable.value)];
        append_int_flips(info.begin,
                              info.offset,
                              info.length,
                              static_cast<tree_bitset::Block>(change.from.value ^ change.to.value),
                              m_flips);
    }
    normalize_flips(m_flips);

    return m_bitsets->insert_flips(parent, m_flips);
}

template<bool ThreadSafe>
void FactStorageBackend<GroundTag, TreeBitset, ThreadSafe>::unpack(const Packed& packed, Unpacked& unpacked)
{
    const auto& infos = m_layout->fluent_infos;
    m_blocks.assign(m_layout->fluent_num_blocks, tree_bitset::Block { 0 });
    m_bitsets->read_blocks(packed, std::span(m_blocks).first(packed.size));

    auto& values = unpacked.values;
    values.resize(infos.size());
    for (ygg::uint_t i = 0; i < infos.size(); ++i)
        values[i] = ygg::uint_t(urd::repr::bit::read_int<tree_bitset::Block>(std::span(m_blocks).subspan(infos[i].begin), infos[i].offset, infos[i].length));
}

template class FactStorageBackend<GroundTag, TreeBitset, false>;
template class FactStorageBackend<GroundTag, TreeBitset, true>;

}
