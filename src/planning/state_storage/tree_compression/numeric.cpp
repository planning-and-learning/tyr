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

#include "tyr/planning/state_storage/tree_compression/numeric.hpp"

#include "tyr/planning/ground/state_storage/tree_compression/context.hpp"
#include "tyr/planning/ground/task.hpp"
#include "tyr/planning/lifted/state_storage/tree_compression/context.hpp"
#include "tyr/planning/lifted/task.hpp"

namespace tyr::planning
{

template<TaskKind Kind>
NumericStorageBackend<Kind, TreeCompression>::NumericStorageBackend(StateStorageContext<Kind, TreeCompression>& ctx) :
    m_float_vectors(ctx.float_vectors)
{
}

template<TaskKind Kind>
typename NumericStorageBackend<Kind, TreeCompression>::Packed
NumericStorageBackend<Kind, TreeCompression>::insert(typename NumericStorageBackend<Kind, TreeCompression>::Unpacked& unpacked)
{
    for (auto& value : unpacked.values)
        value = ygg::FloatTolerance<ygg::float_t>::canonicalize(value);

    return NumericStorageBackend<Kind, TreeCompression>::Packed { m_float_vectors.insert(unpacked.values) };
}

template<TaskKind Kind>
void NumericStorageBackend<Kind, TreeCompression>::unpack(const typename NumericStorageBackend<Kind, TreeCompression>::Packed& packed,
                                                          typename NumericStorageBackend<Kind, TreeCompression>::Unpacked& unpacked)
{
    unpacked.values.resize(packed.index.size);
    m_float_vectors.read(packed.index, unpacked.values);
}

template class NumericStorageBackend<LiftedTag, TreeCompression>;
template class NumericStorageBackend<GroundTag, TreeCompression>;

}
