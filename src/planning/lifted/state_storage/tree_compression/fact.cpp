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

namespace tyr::planning
{

FactStorageBackend<LiftedTag, TreeCompression>::FactStorageBackend(StateStorageContext<LiftedTag, TreeCompression>& ctx) :
    m_uint_vectors(ctx.uint_vectors)
{
}

typename FactStorageBackend<LiftedTag, TreeCompression>::Packed
FactStorageBackend<LiftedTag, TreeCompression>::insert(const typename FactStorageBackend<LiftedTag, TreeCompression>::Unpacked& unpacked)
{
    m_buffer.clear();
    const auto& bits = unpacked.indices;
    for (auto i = bits.find_first(); i != boost::dynamic_bitset<>::npos; i = bits.find_next(i))
        m_buffer.push_back(i);

    return FactStorageBackend<LiftedTag, TreeCompression>::Packed { m_uint_vectors.insert(m_buffer) };
}

void FactStorageBackend<LiftedTag, TreeCompression>::unpack(const typename FactStorageBackend<LiftedTag, TreeCompression>::Packed& packed,
                                                            typename FactStorageBackend<LiftedTag, TreeCompression>::Unpacked& unpacked)
{
    auto& indices = unpacked.indices;

    m_buffer.resize(packed.index.size);
    indices.clear();

    m_uint_vectors.read(packed.index, m_buffer);

    for (const auto i : m_buffer)
        ygg::set(i, true, indices);
}

}
