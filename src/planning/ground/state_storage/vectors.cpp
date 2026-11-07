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

#include "tyr/planning/ground/state_storage/vectors.hpp"

#include "../../tree_bitset.hpp"

#include <algorithm>
#include <span>
#include <urd/repr/containers/bit.hpp>

namespace tyr::planning
{
namespace
{
/// The writes that apply `flips` (normalized) to the blocks of the stored row `parent`.
template<typename ArraySet>
void flips_to_writes(const ArraySet& set,
                     typename ArraySet::handle_type parent,
                     const urd::Store<urd::sequence<tree_bitset::Flip>>& flips,
                     urd::Store<urd::sequence<urd::Write<ygg::uint_t>>>& writes)
{
    const auto blocks = set.view(parent);
    writes.clear();
    for (const auto& flip : flips)
        writes.push_back(urd::Write<ygg::uint_t> { flip.position, ygg::uint_t(blocks[flip.position] ^ flip.mask) });
}
}

template<typename... Tags, bool ThreadSafe>
FactStorageBackend<GroundTag, Vectors<Tags...>, ThreadSafe>::FactStorageBackend(Context& ctx) :
    m_array_set(&ctx.fluent_array_set),
    m_layout(&ctx.layout),
    m_buffer(ctx.layout.fluent_num_blocks)
{
}

template<typename... Tags, bool ThreadSafe>
auto FactStorageBackend<GroundTag, Vectors<Tags...>, ThreadSafe>::insert(const Unpacked& unpacked) -> Packed
{
    const auto& infos = m_layout->fluent_infos;

    std::ranges::fill(m_buffer, ygg::uint_t { 0 });
    for (ygg::uint_t i = 0; i < infos.size(); ++i)
        urd::repr::bit::int_reference(std::span(m_buffer).subspan(infos[i].begin), infos[i].offset, infos[i].length) = unpacked.values[i];

    return m_array_set->insert(m_buffer).first;
}

template<typename... Tags, bool ThreadSafe>
auto FactStorageBackend<GroundTag, Vectors<Tags...>, ThreadSafe>::insert(const Unpacked&, const Packed& parent, std::span<const StateDelta::FluentChange> changes)
    -> Packed
{
    // XOR-ing old and new value into a variable's bits turns its old encoding into the new one.
    m_flips.clear();
    for (const auto& change : changes)
    {
        const auto& info = m_layout->fluent_infos[static_cast<ygg::uint_t>(change.variable.value)];
        append_int_flips(info.begin, info.offset, info.length, ygg::uint_t(change.from.value ^ change.to.value), m_flips);
    }
    normalize_flips(m_flips);
    flips_to_writes(*m_array_set, parent, m_flips, m_writes);

    return m_array_set->insert(parent, m_writes).first;
}

template<typename... Tags, bool ThreadSafe>
void FactStorageBackend<GroundTag, Vectors<Tags...>, ThreadSafe>::unpack(const Packed& packed, Unpacked& unpacked)
{
    const auto& infos = m_layout->fluent_infos;
    const auto view = m_array_set->view(packed);
    const auto data = std::span(view.data(), view.size());
    auto& values = unpacked.values;

    values.resize(infos.size());
    for (ygg::uint_t i = 0; i < infos.size(); ++i)
        values[i] = urd::repr::bit::read_int<ygg::uint_t>(data.subspan(infos[i].begin), infos[i].offset, infos[i].length);
}

template<typename... Tags, bool ThreadSafe>
AtomStorageBackend<GroundTag, Vectors<Tags...>, ThreadSafe>::AtomStorageBackend(Context& ctx) :
    m_array_set(&ctx.derived_array_set),
    m_num_bits(ctx.layout.derived_num_bits),
    m_buffer(urd::repr::bit::ceil_div(m_num_bits, static_cast<ygg::uint_t>(urd::repr::bit::bits_per_block_v<ygg::uint_t>)))
{
}

template<typename... Tags, bool ThreadSafe>
auto AtomStorageBackend<GroundTag, Vectors<Tags...>, ThreadSafe>::insert(const Unpacked& unpacked) -> Packed
{
    std::ranges::fill(m_buffer, ygg::uint_t { 0 });
    for (ygg::uint_t i = 0; i < m_num_bits; ++i)
        urd::repr::bit::bit_reference(std::span(m_buffer), i) = unpacked.indices.test(i);
    return m_array_set->insert(m_buffer).first;
}

template<typename... Tags, bool ThreadSafe>
auto AtomStorageBackend<GroundTag, Vectors<Tags...>, ThreadSafe>::insert(const Unpacked& unpacked, const Packed& parent, const Unpacked& parent_unpacked) -> Packed
{
    // Both bitsets span all derived atoms, so the blocks that differ lie within the stored row.
    m_blocks.clear();
    append_blocks(parent_unpacked.indices, m_blocks);
    const auto num_parent_blocks = m_blocks.size();
    append_blocks(unpacked.indices, m_blocks);

    m_flips.clear();
    append_diff_flips<ygg::uint_t>(std::span<const ygg::uint_t>(m_blocks).first(num_parent_blocks),
                                   std::span<const ygg::uint_t>(m_blocks).subspan(num_parent_blocks),
                                   m_flips);
    flips_to_writes(*m_array_set, parent, m_flips, m_writes);

    return m_array_set->insert(parent, m_writes).first;
}

template<typename... Tags, bool ThreadSafe>
void AtomStorageBackend<GroundTag, Vectors<Tags...>, ThreadSafe>::unpack(const Packed& packed, Unpacked& unpacked)
{
    const auto view = m_array_set->view(packed);
    const auto data = std::span(view.data(), view.size());
    auto& indices = unpacked.indices;

    indices.resize(m_num_bits);
    for (ygg::uint_t i = 0; i < m_num_bits; ++i)
        indices[i] = bool(urd::repr::bit::bit_reference(data, i));
}

template class FactStorageBackend<GroundTag, TreeVector, false>;
template class FactStorageBackend<GroundTag, TreeVector, true>;
template class FactStorageBackend<GroundTag, Hashset, false>;
template class FactStorageBackend<GroundTag, Hashset, true>;
template class AtomStorageBackend<GroundTag, TreeVector, false>;
template class AtomStorageBackend<GroundTag, TreeVector, true>;
template class AtomStorageBackend<GroundTag, Hashset, false>;
template class AtomStorageBackend<GroundTag, Hashset, true>;

}
