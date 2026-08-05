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

#include "tyr/planning/ground/state_storage/tree_compression/atom.hpp"

#include "tyr/planning/ground/state_storage/tree_compression/context.hpp"
#include "tyr/planning/ground/task.hpp"

#include <yggdrasil/core/bit.hpp>

namespace tyr::planning
{

template<bool ThreadSafe>
AtomStorageBackend<GroundTag, TreeCompression, ThreadSafe>::AtomStorageBackend(Context& ctx) :
    m_array_set(ctx.derived_array_set),
    m_num_bits(ctx.derived_num_bits),
    m_buffer(m_array_set.array_size())
{
}

template<bool ThreadSafe>
typename AtomStorageBackend<GroundTag, TreeCompression, ThreadSafe>::Packed
AtomStorageBackend<GroundTag, TreeCompression, ThreadSafe>::insert(const Unpacked& unpacked)
{
    auto data = m_buffer.data();

    std::fill(m_buffer.begin(), m_buffer.end(), ygg::uint_t(0));
    for (ygg::uint_t i = 0; i < m_num_bits; ++i)
        ygg::bit::bit_reference(data, i) = unpacked.indices.test(i);
    return Packed { m_array_set.insert(m_buffer) };
}

template<bool ThreadSafe>
void AtomStorageBackend<GroundTag, TreeCompression, ThreadSafe>::unpack(const Packed& packed, Unpacked& unpacked)
{
    const auto data = m_array_set[packed.index];
    auto& indices = unpacked.indices;

    if (indices.size() != m_num_bits)
        indices.resize(m_num_bits);

    for (ygg::uint_t i = 0; i < m_num_bits; ++i)
        indices[i] = bool(ygg::bit::bit_reference(data.data(), i));
}

template class AtomStorageBackend<GroundTag, TreeCompression, false>;
template class AtomStorageBackend<GroundTag, TreeCompression, true>;

}
