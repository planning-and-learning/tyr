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


#include "../tree_bitset.hpp"
#include "tyr/planning/states/state_data.hpp"
#include "tyr/planning/states/storages/atom_storage_view.hpp"
#include "tyr/planning/states/storages/fact_storage_view.hpp"
#include "tyr/planning/states/storages/numeric_storage_view.hpp"
#include "tyr/planning/states/storages/policies/ground/vectors.hpp"
#include "tyr/planning/states/storages/temporal_storage_view.hpp"
#include "tyr/planning/states/state_repository.hpp"
#include "tyr/planning/states/successor_hint.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <urd/bits.hpp>
#include <vector>

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
    const auto blocks = set.at(parent);
    writes.clear();
    for (const auto& flip : flips)
        writes.push_back(urd::Write<ygg::uint_t> { flip.position, ygg::uint_t(blocks[flip.position] ^ flip.mask) });
}
}

template<typename... Tags, bool ThreadSafe>
StateComponentRepository<FactStorage<GroundTag>, Vectors<Tags...>, ThreadSafe>::StateComponentRepository(Context& ctx) :
    m_array_set(&ctx.fluent_array_set),
    m_layout(&ctx.layout),
    m_buffer(ctx.layout.fluent_num_blocks)
{
}

template<typename... Tags, bool ThreadSafe>
void StateComponentRepository<FactStorage<GroundTag>, Vectors<Tags...>, ThreadSafe>::pack(std::span<const ygg::uint_t> values,
                                                                                          urd::Store<urd::sequence<ygg::uint_t>>& blocks) const
{
    const auto& infos = m_layout->fluent_infos;

    std::ranges::fill(blocks, ygg::uint_t { 0 });
    for (ygg::uint_t i = 0; i < infos.size(); ++i)
        urd::bits::int_reference(std::span(blocks).subspan(infos[i].begin), infos[i].offset, infos[i].length) = values[i];
}

template<typename... Tags, bool ThreadSafe>
auto StateComponentRepository<FactStorage<GroundTag>, Vectors<Tags...>, ThreadSafe>::insert_built(const Unpacked& unpacked) -> std::pair<Key, bool>
{
    pack(std::span<const ygg::uint_t>(unpacked.values), m_buffer);
    const auto [handle, inserted] = m_array_set->insert(m_buffer);
    return { Key(handle), inserted };
}

template<typename... Tags, bool ThreadSafe>
auto StateComponentRepository<FactStorage<GroundTag>, Vectors<Tags...>, ThreadSafe>::find(const urd::data<FactStorage<GroundTag>>& record) const -> Key
{
    auto blocks = urd::Store<urd::sequence<ygg::uint_t>>(m_layout->fluent_num_blocks);
    pack(std::span<const ygg::uint_t>(record.values.data(), record.values.size()), blocks);
    return Key(m_array_set->find(blocks));
}

template<typename... Tags, bool ThreadSafe>
auto StateComponentRepository<FactStorage<GroundTag>, Vectors<Tags...>, ThreadSafe>::insert_built(const Unpacked& /*unpacked*/,
                                                                                                  const SuccessorHint<GroundTag>& hint) -> std::pair<Key, bool>
{
    // XOR-ing old and new value into a variable's bits turns its old encoding into the new one.
    const auto parent = hint.template parent_key<FactStorage<GroundTag>>().value;
    m_flips.clear();
    for (const auto& change : hint.delta->fluents)
    {
        const auto& info = m_layout->fluent_infos[static_cast<ygg::uint_t>(change.variable.value)];
        append_int_flips(info.begin, info.offset, info.length, static_cast<ygg::uint_t>(change.from.value ^ change.to.value), m_flips);
    }
    normalize_flips(m_flips);
    flips_to_writes(*m_array_set, parent, m_flips, m_writes);

    const auto [handle, inserted] = m_array_set->insert(parent, m_writes);
    return { Key(handle), inserted };
}

template<typename... Tags, bool ThreadSafe>
void StateComponentRepository<FactStorage<GroundTag>, Vectors<Tags...>, ThreadSafe>::read(const Handle& packed, std::vector<ygg::uint_t>& values) const
{
    const auto& infos = m_layout->fluent_infos;
    const auto view = m_array_set->at(packed);
    const auto data = std::span(view.data(), view.size());

    values.resize(infos.size());
    for (ygg::uint_t i = 0; i < infos.size(); ++i)
        values[i] = urd::bits::read_int<ygg::uint_t>(data.subspan(infos[i].begin), infos[i].offset, infos[i].length);
}

template<typename... Tags, bool ThreadSafe>
void StateComponentRepository<FactStorage<GroundTag>, Vectors<Tags...>, ThreadSafe>::unpack(const Handle& packed, Unpacked& unpacked) const
{ read(packed, unpacked.values); }

template<typename... Tags, bool ThreadSafe>
std::vector<ygg::uint_t> StateComponentRepository<FactStorage<GroundTag>, Vectors<Tags...>, ThreadSafe>::unpack(Key key) const
{
    auto values = std::vector<ygg::uint_t>();
    read(key.value, values);
    return values;
}

template<typename... Tags, bool ThreadSafe>
StateComponentRepository<AtomStorage<GroundTag>, Vectors<Tags...>, ThreadSafe>::StateComponentRepository(Context& ctx) :
    m_array_set(&ctx.derived_array_set),
    m_buffer(urd::bits::ceil_div(ctx.layout.derived_num_bits, static_cast<ygg::uint_t>(urd::bits::bits_per_block_v<ygg::uint_t>)))
{
}

template<typename... Tags, bool ThreadSafe>
void StateComponentRepository<AtomStorage<GroundTag>, Vectors<Tags...>, ThreadSafe>::pack(std::span<const urd::word_t> words,
                                                                                          urd::Store<urd::sequence<ygg::uint_t>>& blocks)
{
    for (std::size_t i = 0; i < blocks.size(); ++i)
        blocks[i] = block_of(words, i);
}

template<typename... Tags, bool ThreadSafe>
auto StateComponentRepository<AtomStorage<GroundTag>, Vectors<Tags...>, ThreadSafe>::insert_built(const Unpacked& unpacked) -> std::pair<Key, bool>
{
    pack(unpacked.indices.significant_words(), m_buffer);
    const auto [handle, inserted] = m_array_set->insert(m_buffer);
    return { Key(handle), inserted };
}

template<typename... Tags, bool ThreadSafe>
auto StateComponentRepository<AtomStorage<GroundTag>, Vectors<Tags...>, ThreadSafe>::find(const urd::data<AtomStorage<GroundTag>>& record) const -> Key
{
    auto blocks = urd::Store<urd::sequence<ygg::uint_t>>(m_buffer.size());
    pack(std::span<const urd::word_t>(record.indices.words.data(), record.indices.words.size()), blocks);
    return Key(m_array_set->find(blocks));
}

template<typename... Tags, bool ThreadSafe>
auto StateComponentRepository<AtomStorage<GroundTag>, Vectors<Tags...>, ThreadSafe>::insert_built(const Unpacked& unpacked,
                                                                                                  const SuccessorHint<GroundTag>& hint) -> std::pair<Key, bool>
{
    // Derived atoms lie within the stored row, so the blocks that differ are written there.
    const auto parent = hint.template parent_key<AtomStorage<GroundTag>>().value;
    const auto from = urd::built_view(hint.parent_state->get_atom_storage()).get_indices().significant_words();
    const auto to = unpacked.indices.significant_words();
    m_writes.clear();
    for (std::size_t i = 0; i < m_buffer.size(); ++i)
        if (const auto block = block_of(to, i); block != block_of(from, i))
            m_writes.push_back(urd::Write<ygg::uint_t> { i, block });

    const auto [handle, inserted] = m_array_set->insert(parent, m_writes);
    return { Key(handle), inserted };
}

template<typename... Tags, bool ThreadSafe>
void StateComponentRepository<AtomStorage<GroundTag>, Vectors<Tags...>, ThreadSafe>::unpack(const Handle& packed, Unpacked& unpacked) const
{ read(packed, unpacked.indices); }

template<typename... Tags, bool ThreadSafe>
StateBits StateComponentRepository<AtomStorage<GroundTag>, Vectors<Tags...>, ThreadSafe>::unpack(Key key) const
{
    auto bits = StateBits();
    read(key.value, bits);
    return bits;
}

template<typename... Tags, bool ThreadSafe>
void StateComponentRepository<AtomStorage<GroundTag>, Vectors<Tags...>, ThreadSafe>::read(const Handle& packed, StateBits& bits) const
{
    const auto view = m_array_set->at(packed);
    const auto blocks = std::span(view.data(), view.size());
    bits.assign_words((blocks.size() + 1) / 2,
                      [blocks](std::span<std::uint64_t> words)
                      {
                          for (std::size_t i = 0; i < blocks.size(); ++i)
                              words[i / 2] |= std::uint64_t { blocks[i] } << (tree_bitset::bits_per_block * (i % 2));
                      });
}

template class StateComponentRepository<FactStorage<GroundTag>, StateStoragePolicyTag, true>;
template class StateComponentRepository<AtomStorage<GroundTag>, StateStoragePolicyTag, true>;

}
