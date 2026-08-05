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

#include "tyr/planning/lifted/state_storage/hash_set/fact.hpp"

#include "tyr/planning/lifted/state_storage/hash_set/context.hpp"
#include "tyr/planning/lifted/task.hpp"

namespace tyr::planning
{

template<bool ThreadSafe>
FactStorageBackend<LiftedTag, HashSet, ThreadSafe>::FactStorageBackend(Context& ctx) : m_uint_vec_set(ctx.uint_vec_set), m_buffer()
{
}

template<bool ThreadSafe>
typename FactStorageBackend<LiftedTag, HashSet, ThreadSafe>::Packed FactStorageBackend<LiftedTag, HashSet, ThreadSafe>::insert(const Unpacked& unpacked)
{
    m_buffer.clear();
    const auto& bits = unpacked.indices;
    for (auto i = bits.find_first(); i != boost::dynamic_bitset<>::npos; i = bits.find_next(i))
        m_buffer.push_back(i);

    return Packed { m_uint_vec_set.insert(m_buffer) };
}

template<bool ThreadSafe>
void FactStorageBackend<LiftedTag, HashSet, ThreadSafe>::unpack(const Packed& packed, Unpacked& unpacked)
{
    const auto view = m_uint_vec_set[packed.index];

    unpacked.indices.clear();
    for (ygg::uint_t i = 0; i < view.size(); ++i)
        ygg::set(view[i], true, unpacked.indices);
}

template class FactStorageBackend<LiftedTag, HashSet, false>;
template class FactStorageBackend<LiftedTag, HashSet, true>;

}
