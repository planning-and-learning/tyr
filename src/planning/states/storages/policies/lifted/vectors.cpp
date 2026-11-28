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

#include "tyr/planning/states/storages/policies/lifted/vectors.hpp"

#include "tyr/planning/states/state_data.hpp"
#include "tyr/planning/states/successor_hint.hpp"

#include <bit>
#include <cstddef>
#include <limits>
#include <span>

namespace tyr::planning
{

template<typename U, typename Context>
BitPositionsRepository<U, Context>::BitPositionsRepository(Context& ctx) : m_uint_vectors(&ctx.uint_vectors)
{
}

template<typename U, typename Context>
void BitPositionsRepository<U, Context>::positions(std::span<const urd::word_t> words, urd::Store<urd::sequence<ygg::uint_t>>& result)
{
    result.clear();
    for (std::size_t w = 0; w < words.size(); ++w)
        for (auto word = words[w]; word != 0; word &= word - 1)
            result.push_back(static_cast<ygg::uint_t>(w * std::numeric_limits<urd::word_t>::digits + static_cast<std::size_t>(std::countr_zero(word))));
}

template<typename U, typename Context>
auto BitPositionsRepository<U, Context>::insert_built(const Unpacked& unpacked) -> std::pair<Key, bool>
{
    positions(unpacked.indices.significant_words(), m_buffer);
    const auto [handle, inserted] = m_uint_vectors->insert(m_buffer);
    return { Key(handle), inserted };
}

template<typename U, typename Context>
auto BitPositionsRepository<U, Context>::find(const urd::data<U>& record) const -> Key
{
    auto buffer = urd::Store<urd::sequence<ygg::uint_t>>();
    positions(std::span<const urd::word_t>(record.indices.words.data(), record.indices.words.size()), buffer);
    return Key(m_uint_vectors->find(buffer));
}

template<typename U, typename Context>
auto BitPositionsRepository<U, Context>::insert_built(const Unpacked& unpacked, const SuccessorHint<LiftedTag>& hint) -> std::pair<Key, bool>
{
    // A write cannot grow or shrink a stored row, so only rows with as many true positions as the parent go in as writes.
    const auto parent = hint.template parent_key<U>().value;
    const auto& bits = unpacked.indices;
    if (bits.size() != m_uint_vectors->row_size(parent))
        return insert_built(unpacked);

    m_parent.resize(m_uint_vectors->row_size(parent));
    m_uint_vectors->copy(parent, m_parent);

    m_writes.clear();
    auto position = std::size_t { 0 };
    for (const auto i : bits)
    {
        if (m_parent[position] != i)
            m_writes.push_back(urd::Write<ygg::uint_t> { position, i });
        ++position;
    }

    const auto [handle, inserted] = m_uint_vectors->insert(parent, m_writes);
    return { Key(handle), inserted };
}

template<typename U, typename Context>
void BitPositionsRepository<U, Context>::unpack(const Handle& packed, Unpacked& unpacked) const
{ read(packed, unpacked.indices); }

template<typename U, typename Context>
StateBits BitPositionsRepository<U, Context>::unpack(Key key) const
{
    auto bits = StateBits();
    read(key.value, bits);
    return bits;
}

template<typename U, typename Context>
void BitPositionsRepository<U, Context>::read(const Handle& packed, StateBits& bits) const
{
    auto positions = urd::Store<urd::sequence<ygg::uint_t>>(m_uint_vectors->row_size(packed));
    m_uint_vectors->copy(packed, positions);

    bits.clear();
    bits.insert(positions.begin(), positions.end());
}

template class BitPositionsRepository<FactStorage<LiftedTag>, StateStorageContext<LiftedTag, StateStoragePolicyTag, true>>;
template class BitPositionsRepository<AtomStorage<LiftedTag>, StateStorageContext<LiftedTag, StateStoragePolicyTag, true>>;

}
