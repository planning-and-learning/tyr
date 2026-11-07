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

#include "tyr/planning/lifted/state_storage/vectors.hpp"


namespace tyr::planning
{

template<typename U, typename Context>
BitPositionsBackend<U, Context>::BitPositionsBackend(Context& ctx) : m_uint_vectors(&ctx.uint_vectors)
{
}

template<typename U, typename Context>
auto BitPositionsBackend<U, Context>::insert(const Unpacked& unpacked) -> Packed
{
    m_buffer.clear();
    for (const auto i : unpacked.indices)
        m_buffer.push_back(i);

    return m_uint_vectors->insert(m_buffer).first;
}

template<typename U, typename Context>
auto BitPositionsBackend<U, Context>::insert(const Unpacked& unpacked, const Packed& parent) -> Packed
{
    // A write cannot grow or shrink a stored row, so only rows with as many true positions as the parent go in as writes.
    const auto& bits = unpacked.indices;
    if (bits.size() != m_uint_vectors->row_size(parent))
        return insert(unpacked);

    m_parent.resize(m_uint_vectors->row_size(parent));
    m_uint_vectors->read(parent, m_parent);

    m_writes.clear();
    auto position = std::size_t { 0 };
    for (const auto i : bits)
    {
        if (m_parent[position] != i)
            m_writes.push_back(urd::Write<ygg::uint_t> { position, i });
        ++position;
    }

    return m_uint_vectors->insert(parent, m_writes).first;
}

template<typename U, typename Context>
void BitPositionsBackend<U, Context>::unpack(const Packed& packed, Unpacked& unpacked)
{
    m_buffer.resize(m_uint_vectors->row_size(packed));
    m_uint_vectors->read(packed, m_buffer);

    unpacked.indices.clear();
    unpacked.indices.insert(m_buffer.begin(), m_buffer.end());
}

template class BitPositionsBackend<FactUnpackedStorage<LiftedTag>, StateStorageContext<LiftedTag, TreeVector, false>>;
template class BitPositionsBackend<FactUnpackedStorage<LiftedTag>, StateStorageContext<LiftedTag, TreeVector, true>>;
template class BitPositionsBackend<FactUnpackedStorage<LiftedTag>, StateStorageContext<LiftedTag, Hashset, false>>;
template class BitPositionsBackend<FactUnpackedStorage<LiftedTag>, StateStorageContext<LiftedTag, Hashset, true>>;
template class BitPositionsBackend<AtomUnpackedStorage<LiftedTag>, StateStorageContext<LiftedTag, TreeVector, false>>;
template class BitPositionsBackend<AtomUnpackedStorage<LiftedTag>, StateStorageContext<LiftedTag, TreeVector, true>>;
template class BitPositionsBackend<AtomUnpackedStorage<LiftedTag>, StateStorageContext<LiftedTag, Hashset, false>>;
template class BitPositionsBackend<AtomUnpackedStorage<LiftedTag>, StateStorageContext<LiftedTag, Hashset, true>>;

}
