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

#include "tyr/planning/state_storage/hash_set/numeric.hpp"

#include "tyr/planning/ground/state_storage/hash_set/context.hpp"
#include "tyr/planning/lifted/state_storage/hash_set/context.hpp"
#include "tyr/planning/task.hpp"

namespace tyr::planning
{
template<TaskKind Kind, bool ThreadSafe>
NumericStorageBackend<Kind, HashSet, ThreadSafe>::NumericStorageBackend(Context& ctx) : m_float_vec_set(ctx.float_vec_set)
{
}

template<TaskKind Kind, bool ThreadSafe>
typename NumericStorageBackend<Kind, HashSet, ThreadSafe>::Packed NumericStorageBackend<Kind, HashSet, ThreadSafe>::insert(Unpacked& unpacked)
{
    for (auto& value : unpacked.values)
        value = ygg::FloatTolerance<ygg::float_t>::canonicalize(value);

    return Packed { m_float_vec_set.insert(unpacked.values) };
}

template<TaskKind Kind, bool ThreadSafe>
void NumericStorageBackend<Kind, HashSet, ThreadSafe>::unpack(const Packed& packed, Unpacked& unpacked)
{
    const auto view = m_float_vec_set[packed.index];

    unpacked.values.resize(view.size());
    for (ygg::uint_t i = 0; i < view.size(); ++i)
        unpacked.values[i] = view[i];
}

template class NumericStorageBackend<LiftedTag, HashSet, false>;
template class NumericStorageBackend<LiftedTag, HashSet, true>;
template class NumericStorageBackend<GroundTag, HashSet, false>;
template class NumericStorageBackend<GroundTag, HashSet, true>;

}
