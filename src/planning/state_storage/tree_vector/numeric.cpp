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

#include "tyr/planning/state_storage/tree_vector/numeric.hpp"

#include "tyr/planning/ground/state_storage/tree_bitset/context.hpp"
#include "tyr/planning/ground/state_storage/tree_vector/context.hpp"
#include "tyr/planning/ground/task.hpp"
#include "tyr/planning/lifted/state_storage/tree_bitset/context.hpp"
#include "tyr/planning/lifted/state_storage/tree_vector/context.hpp"
#include "tyr/planning/lifted/task.hpp"

#include <yggdrasil/semantics/equal_to.hpp>

namespace tyr::planning
{

template<TaskKind Kind, TreeCompressionTag Tag, bool ThreadSafe>
NumericStorageBackend<Kind, Tag, ThreadSafe>::NumericStorageBackend(Context& ctx) : m_float_vectors(&ctx.float_vectors)
{
}

namespace
{
template<TaskKind Kind>
void canonicalize(NumericUnpackedStorage<Kind>& unpacked)
{
    for (auto& value : unpacked.values)
        value = ygg::FloatTolerance<ygg::float_t>::canonicalize(value);
}
}

template<TaskKind Kind, TreeCompressionTag Tag, bool ThreadSafe>
typename NumericStorageBackend<Kind, Tag, ThreadSafe>::Packed NumericStorageBackend<Kind, Tag, ThreadSafe>::insert(Unpacked& unpacked)
{
    canonicalize(unpacked);
    return Packed { m_float_vectors->insert(unpacked.values) };
}

template<TaskKind Kind, TreeCompressionTag Tag, bool ThreadSafe>
typename NumericStorageBackend<Kind, Tag, ThreadSafe>::Packed
NumericStorageBackend<Kind, Tag, ThreadSafe>::insert(Unpacked& unpacked, const Packed& parent, std::span<const StateDelta::NumericChange> changes)
{
    canonicalize(unpacked);

    // Value vectors grow with the highest function term set, so a successor may be longer than its parent,
    // and a write delta cannot grow a stored vector.
    const auto& values = unpacked.values;
    if (values.size() != parent.index.size)
        return Packed { m_float_vectors->insert(values) };

    m_writes.clear();
    for (const auto& change : changes)
    {
        const auto position = ygg::uint_t(change.fterm);
        m_writes.push_back(ygg::TreeVectorWrite<ygg::float_t> { position, values[position] });
    }

    return Packed { m_float_vectors->insert(parent.index, m_writes) };
}

template<TaskKind Kind, TreeCompressionTag Tag, bool ThreadSafe>
void NumericStorageBackend<Kind, Tag, ThreadSafe>::unpack(const Packed& packed, Unpacked& unpacked)
{
    unpacked.values.resize(packed.index.size);
    m_float_vectors->read(packed.index, unpacked.values);
}

template class NumericStorageBackend<LiftedTag, TreeVector, false>;
template class NumericStorageBackend<LiftedTag, TreeBitset, false>;
template class NumericStorageBackend<LiftedTag, TreeVector, true>;
template class NumericStorageBackend<LiftedTag, TreeBitset, true>;
template class NumericStorageBackend<GroundTag, TreeVector, false>;
template class NumericStorageBackend<GroundTag, TreeBitset, false>;
template class NumericStorageBackend<GroundTag, TreeVector, true>;
template class NumericStorageBackend<GroundTag, TreeBitset, true>;

}
