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

#include "tyr/planning/state_storage/tree_bitset/fact.hpp"

#include "tyr/planning/ground/task.hpp"
#include "tyr/planning/lifted/task.hpp"
#include "../../tree_bitset.hpp"

#include <urd/repr/containers/bit.hpp>

namespace tyr::planning
{
namespace
{
using Blocks = urd::Store<urd::sequence<tree_bitset::Block>>;
using Flips = urd::Store<urd::sequence<tree_bitset::Flip>>;

template<typename Set>
auto insert_facts(Set& bitsets, const StateLayout<GroundTag>& layout, const FactUnpackedStorage<GroundTag>& unpacked, Blocks& blocks)
{
    const auto& infos = layout.fluent_infos;
    blocks.assign(layout.fluent_num_blocks, tree_bitset::Block { 0 });
    for (ygg::uint_t i = 0; i < infos.size(); ++i)
        urd::repr::bit::write_int(std::span(blocks).subspan(infos[i].begin), static_cast<tree_bitset::Block>(unpacked.values[i]), infos[i].offset, infos[i].length);

    trim_trailing_zero_blocks(blocks);
    return bitsets.insert_blocks(blocks);
}

template<typename Set>
auto insert_facts(Set& bitsets, const StateLayout<LiftedTag>&, const FactUnpackedStorage<LiftedTag>& unpacked, Blocks&)
{ return bitsets.insert(unpacked.indices).first; }

/// XOR-ing old and new value into a variable's bits turns its old encoding into the new one.
void append_flips(const StateLayout<GroundTag>& layout, const StateDelta::FluentChange& change, Flips& flips)
{
    const auto& info = layout.fluent_infos[static_cast<ygg::uint_t>(change.variable.value)];
    append_int_flips(info.begin, info.offset, info.length, static_cast<tree_bitset::Block>(change.from.value ^ change.to.value), flips);
}

/// Lifted fluent variables are binary and stored as one bit each, so every change flips the variable's bit.
void append_flips(const StateLayout<LiftedTag>&, const StateDelta::FluentChange& change, Flips& flips)
{
    const auto bit = static_cast<ygg::uint_t>(change.variable.value);
    flips.push_back(tree_bitset::Flip { static_cast<ygg::uint_t>(bit / tree_bitset::bits_per_block), tree_bitset::Block { 1 } << (bit % tree_bitset::bits_per_block) });
}

template<typename Set>
void unpack_facts(Set& bitsets, const StateLayout<GroundTag>& layout, typename Set::handle_type packed, FactUnpackedStorage<GroundTag>& unpacked, Blocks& blocks)
{
    const auto& infos = layout.fluent_infos;
    blocks.assign(layout.fluent_num_blocks, tree_bitset::Block { 0 });
    bitsets.read_blocks(packed, std::span(blocks).first(packed.size));

    auto& values = unpacked.values;
    values.resize(infos.size());
    for (ygg::uint_t i = 0; i < infos.size(); ++i)
        values[i] = ygg::uint_t(urd::repr::bit::read_int<tree_bitset::Block>(std::span(blocks).subspan(infos[i].begin), infos[i].offset, infos[i].length));
}

template<typename Set>
void unpack_facts(Set& bitsets, const StateLayout<LiftedTag>&, typename Set::handle_type packed, FactUnpackedStorage<LiftedTag>& unpacked, Blocks&)
{ bitsets.read(packed, unpacked.indices); }
}

template<TaskKind Kind, bool ThreadSafe>
FactStorageBackend<Kind, TreeBitset, ThreadSafe>::FactStorageBackend(Context& ctx) : m_context(&ctx)
{
}

template<TaskKind Kind, bool ThreadSafe>
auto FactStorageBackend<Kind, TreeBitset, ThreadSafe>::insert(const Unpacked& unpacked) -> Packed
{ return insert_facts(m_context->bitsets, m_context->layout, unpacked, m_blocks); }

template<TaskKind Kind, bool ThreadSafe>
auto FactStorageBackend<Kind, TreeBitset, ThreadSafe>::insert(const Unpacked&, const Packed& parent, std::span<const StateDelta::FluentChange> changes) -> Packed
{
    m_flips.clear();
    for (const auto& change : changes)
        append_flips(m_context->layout, change, m_flips);
    normalize_flips(m_flips);

    return m_context->bitsets.insert_flips(parent, m_flips);
}

template<TaskKind Kind, bool ThreadSafe>
void FactStorageBackend<Kind, TreeBitset, ThreadSafe>::unpack(const Packed& packed, Unpacked& unpacked)
{ unpack_facts(m_context->bitsets, m_context->layout, packed, unpacked, m_blocks); }

template class FactStorageBackend<GroundTag, TreeBitset, false>;
template class FactStorageBackend<GroundTag, TreeBitset, true>;
template class FactStorageBackend<LiftedTag, TreeBitset, false>;
template class FactStorageBackend<LiftedTag, TreeBitset, true>;

}
