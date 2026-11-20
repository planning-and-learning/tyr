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

#include "tyr/planning/state_storage/numeric.hpp"

#include "tyr/planning/ground/state_storage/vectors.hpp"
#include "tyr/planning/lifted/state_storage/vectors.hpp"
#include "tyr/planning/state_storage/tree_bitset/context.hpp"

#include <urd/elements/equal_to.hpp>

namespace tyr::planning
{

template<TaskKind Kind, typename Tag, bool ThreadSafe>
StateComponentRepository<NumericStorage<Kind>, Tag, ThreadSafe>::StateComponentRepository(Context& ctx) : m_float_vectors(&ctx.float_vectors)
{
}

namespace
{
template<TaskKind Kind>
void canonicalize(urd::builder<NumericStorage<Kind>, formalism::planning::Repository>& unpacked)
{
    for (auto& value : unpacked.values)
        value = ygg::FloatTolerance<ygg::float_t>::canonicalize(value);
}
}

template<TaskKind Kind, typename Tag, bool ThreadSafe>
auto StateComponentRepository<NumericStorage<Kind>, Tag, ThreadSafe>::insert(Unpacked& unpacked) -> Handle
{
    canonicalize(unpacked);
    return m_float_vectors->insert(unpacked.values).first;
}

template<TaskKind Kind, typename Tag, bool ThreadSafe>
auto StateComponentRepository<NumericStorage<Kind>, Tag, ThreadSafe>::insert(Unpacked& unpacked,
                                                                             const Handle& parent,
                                                                             std::span<const StateDelta::NumericChange> changes) -> Handle
{
    canonicalize(unpacked);

    // Value vectors grow with the highest function term set, so a successor may be longer than its parent,
    // and a write delta cannot grow a stored vector.
    const auto& values = unpacked.values;
    if (values.size() != m_float_vectors->row_size(parent))
        return m_float_vectors->insert(values).first;

    m_writes.clear();
    for (const auto& change : changes)
    {
        const auto position = static_cast<ygg::uint_t>(change.fterm.value);
        m_writes.push_back(urd::Write<ygg::float_t> { position, values[position] });
    }

    return m_float_vectors->insert(parent, m_writes).first;
}

template<TaskKind Kind, typename Tag, bool ThreadSafe>
void StateComponentRepository<NumericStorage<Kind>, Tag, ThreadSafe>::unpack(const Handle& packed, Unpacked& unpacked)
{
    unpacked.values.resize(m_float_vectors->row_size(packed));
    m_float_vectors->copy(packed, unpacked.values);
}

template class StateComponentRepository<NumericStorage<GroundTag>, TreeBitset, false>;
template class StateComponentRepository<NumericStorage<GroundTag>, TreeBitset, true>;
template class StateComponentRepository<NumericStorage<GroundTag>, TreeVector, false>;
template class StateComponentRepository<NumericStorage<GroundTag>, TreeVector, true>;
template class StateComponentRepository<NumericStorage<GroundTag>, Hashset, false>;
template class StateComponentRepository<NumericStorage<GroundTag>, Hashset, true>;
template class StateComponentRepository<NumericStorage<LiftedTag>, TreeBitset, false>;
template class StateComponentRepository<NumericStorage<LiftedTag>, TreeBitset, true>;
template class StateComponentRepository<NumericStorage<LiftedTag>, TreeVector, false>;
template class StateComponentRepository<NumericStorage<LiftedTag>, TreeVector, true>;
template class StateComponentRepository<NumericStorage<LiftedTag>, Hashset, false>;
template class StateComponentRepository<NumericStorage<LiftedTag>, Hashset, true>;

}
