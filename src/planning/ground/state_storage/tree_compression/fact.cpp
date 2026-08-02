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

#include "tyr/planning/ground/state_storage/tree_compression/fact.hpp"

#include "tyr/planning/ground/state_storage/tree_compression/context.hpp"
#include "tyr/planning/ground/task.hpp"

#include <yggdrasil/core/bit.hpp>

namespace tyr::planning
{

FactStorageBackend<GroundTag, TreeCompression>::FactStorageBackend(StateStorageContext<GroundTag, TreeCompression>& ctx) :
    m_array_set(ctx.fluent_array_set),
    m_infos(ctx.fluent_infos),
    m_buffer(m_array_set.array_size())
{
}

typename FactStorageBackend<GroundTag, TreeCompression>::Packed
FactStorageBackend<GroundTag, TreeCompression>::insert(const typename FactStorageBackend<GroundTag, TreeCompression>::Unpacked& unpacked)
{
    auto data = m_buffer.data();
    const auto& values = unpacked.values;

    std::fill(m_buffer.begin(), m_buffer.end(), ygg::uint_t(0));
    for (ygg::uint_t i = 0; i < m_infos.size(); ++i)
    {
        const auto& info = m_infos[i];

        ygg::bit::int_reference(data + info.begin, info.offset, info.length) = values[i];
    }

    return typename FactStorageBackend<GroundTag, TreeCompression>::Packed { m_array_set.insert(m_buffer) };
}

void FactStorageBackend<GroundTag, TreeCompression>::unpack(const typename FactStorageBackend<GroundTag, TreeCompression>::Packed& packed,
                                                            typename FactStorageBackend<GroundTag, TreeCompression>::Unpacked& unpacked)
{
    const auto data = m_array_set[packed.index];
    auto& values = unpacked.values;

    if (values.size() != m_infos.size())
        values.resize(m_infos.size());

    for (ygg::uint_t i = 0; i < m_infos.size(); ++i)
    {
        const auto& info = m_infos[i];

        values[i] = ygg::bit::read_int(data.data() + info.begin, info.offset, info.length);
    }
}

}
