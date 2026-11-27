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

#include "tyr/planning/states/storages/policies/tree_bitset/fact.hpp"

#include "../tree_bitset.hpp"
#include "tyr/planning/states/storages/fact_storage_data.hpp"
#include "tyr/planning/states/storages/ground/fact_storage_builder.hpp"
#include "tyr/planning/tasks/ground/task.hpp"
#include "tyr/planning/states/storages/lifted/fact_storage_builder.hpp"
#include "tyr/planning/tasks/lifted/task.hpp"
#include "tyr/planning/states/state_data.hpp"

#include <concepts>
#include <span>
#include <urd/bits.hpp>
#include <utility>
#include <vector>

namespace tyr::planning
{
namespace
{
using Blocks = urd::Store<urd::sequence<tree_bitset::Block>>;
using Flips = urd::Store<urd::sequence<tree_bitset::Flip>>;

/// A ground state's values packed bitwise into blocks (the task's layout), trimmed to the last nonzero block.
void pack_facts(const StateLayout<GroundTag>& layout, std::span<const ygg::uint_t> values, Blocks& blocks)
{
    const auto& infos = layout.fluent_infos;
    blocks.assign(layout.fluent_num_blocks, tree_bitset::Block { 0 });
    for (ygg::uint_t i = 0; i < infos.size(); ++i)
        urd::bits::write_int(std::span(blocks).subspan(infos[i].begin), static_cast<tree_bitset::Block>(values[i]), infos[i].offset, infos[i].length);

    trim_trailing_zero_blocks(blocks);
}

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
    flips.push_back(
        tree_bitset::Flip { static_cast<ygg::uint_t>(bit / tree_bitset::bits_per_block), tree_bitset::Block { 1 } << (bit % tree_bitset::bits_per_block) });
}

template<typename Set>
void unpack_facts(const Set& bitsets, const StateLayout<GroundTag>& layout, typename Set::handle_type packed, std::vector<ygg::uint_t>& values, Blocks& blocks)
{
    const auto& infos = layout.fluent_infos;
    blocks.assign(layout.fluent_num_blocks, tree_bitset::Block { 0 });
    bitsets.read_blocks(packed, std::span(blocks).first(packed.size));

    values.resize(infos.size());
    for (ygg::uint_t i = 0; i < infos.size(); ++i)
        values[i] =
            static_cast<ygg::uint_t>(urd::bits::read_int<tree_bitset::Block>(std::span(blocks).subspan(infos[i].begin), infos[i].offset, infos[i].length));
}

template<typename Set>
void unpack_facts(const Set& bitsets, const StateLayout<LiftedTag>&, typename Set::handle_type packed, StateBits& bits, Blocks&)
{ bitsets.copy(packed, bits); }
}

template<TaskKind Kind, bool ThreadSafe>
StateComponentRepository<FactStorage<Kind>, TreeBitset, ThreadSafe>::StateComponentRepository(Context& ctx) : m_context(&ctx)
{
}

template<TaskKind Kind, bool ThreadSafe>
auto StateComponentRepository<FactStorage<Kind>, TreeBitset, ThreadSafe>::insert(const Unpacked& unpacked) -> Handle
{
    if constexpr (std::same_as<Kind, GroundTag>)
    {
        pack_facts(m_context->layout, std::span<const ygg::uint_t>(unpacked.values), m_blocks);
        return m_context->bitsets.insert_blocks(m_blocks).first;
    }
    else
        return m_context->bitsets.insert(unpacked.indices).first;
}

template<TaskKind Kind, bool ThreadSafe>
auto StateComponentRepository<FactStorage<Kind>, TreeBitset, ThreadSafe>::store(const urd::data<FactStorage<Kind>>& record) -> std::pair<Handle, bool>
{
    if constexpr (std::same_as<Kind, GroundTag>)
    {
        pack_facts(m_context->layout, std::span<const ygg::uint_t>(record.values.data(), record.values.size()), m_blocks);
        return m_context->bitsets.insert_blocks(m_blocks);
    }
    else
    {
        m_bits.assign_words(std::span<const urd::word_t>(record.indices.words.data(), record.indices.words.size()));
        return m_context->bitsets.insert(m_bits);
    }
}

template<TaskKind Kind, bool ThreadSafe>
auto StateComponentRepository<FactStorage<Kind>, TreeBitset, ThreadSafe>::find(const urd::data<FactStorage<Kind>>& record) const -> Key
{
    if constexpr (std::same_as<Kind, GroundTag>)
    {
        auto blocks = Blocks();
        pack_facts(m_context->layout, std::span<const ygg::uint_t>(record.values.data(), record.values.size()), blocks);
        return Key(m_context->bitsets.find_blocks(blocks));
    }
    else
    {
        auto bits = StateBits();
        bits.assign_words(std::span<const urd::word_t>(record.indices.words.data(), record.indices.words.size()));
        return Key(m_context->bitsets.find(bits));
    }
}

template<TaskKind Kind, bool ThreadSafe>
auto StateComponentRepository<FactStorage<Kind>, TreeBitset, ThreadSafe>::insert(const Unpacked&,
                                                                                 const Handle& parent,
                                                                                 std::span<const StateDelta::FluentChange> changes) -> Handle
{
    m_flips.clear();
    for (const auto& change : changes)
        append_flips(m_context->layout, change, m_flips);
    normalize_flips(m_flips);

    return m_context->bitsets.insert_flips(parent, m_flips);
}

template<TaskKind Kind, bool ThreadSafe>
void StateComponentRepository<FactStorage<Kind>, TreeBitset, ThreadSafe>::unpack(const Handle& packed, Unpacked& unpacked)
{
    if constexpr (std::same_as<Kind, GroundTag>)
        unpack_facts(m_context->bitsets, m_context->layout, packed, unpacked.values, m_blocks);
    else
        unpack_facts(m_context->bitsets, m_context->layout, packed, unpacked.indices, m_blocks);
}

template<TaskKind Kind, bool ThreadSafe>
auto StateComponentRepository<FactStorage<Kind>, TreeBitset, ThreadSafe>::unpack(Key key) const -> Built
{
    auto built = Built();
    auto blocks = Blocks();
    unpack_facts(m_context->bitsets, m_context->layout, key.value, built, blocks);
    return built;
}

template class StateComponentRepository<FactStorage<GroundTag>, StateStoragePolicyTag, true>;
template class StateComponentRepository<FactStorage<LiftedTag>, StateStoragePolicyTag, true>;

}
