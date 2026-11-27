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


#include "tyr/planning/states/state_data.hpp"
#include "tyr/planning/states/storages/numeric_storage_builder.hpp"
#include "tyr/planning/states/storages/numeric_storage_data.hpp"
#include "tyr/planning/states/storages/policies/numeric.hpp"
#include "tyr/planning/states/storages/policies/policy.hpp"
#include "tyr/planning/states/storages/temporal_storage_builder.hpp"
#include "tyr/planning/states/successor_hint.hpp"

#include <urd/elements/equal_to.hpp>

namespace tyr::planning
{

template<TaskKind Kind, typename Tag, bool ThreadSafe>
StateComponentRepository<NumericStorage<Kind>, Tag, ThreadSafe>::StateComponentRepository(Context& ctx) : m_float_vectors(&ctx.float_vectors)
{
}

namespace
{
/// A builder's values, each canonical, into `canonical` (its buffer reused).
template<TaskKind Kind>
void canonicalize(const urd::builder<NumericStorage<Kind>, StateRepository<Kind>>& unpacked, std::vector<ygg::float_t>& canonical)
{
    canonical.clear();
    for (const auto value : unpacked.values)
        canonical.push_back(ygg::FloatTolerance<ygg::float_t>::canonicalize(value));
}
}

template<TaskKind Kind, typename Tag, bool ThreadSafe>
auto StateComponentRepository<NumericStorage<Kind>, Tag, ThreadSafe>::insert_built(const Unpacked& unpacked) -> std::pair<Key, bool>
{
    canonicalize(unpacked, m_canonical);
    const auto [handle, inserted] = m_float_vectors->insert(m_canonical);
    return { Key(handle), inserted };
}

template<TaskKind Kind, typename Tag, bool ThreadSafe>
auto StateComponentRepository<NumericStorage<Kind>, Tag, ThreadSafe>::insert_built(const Unpacked& unpacked, const SuccessorHint<Kind>& hint)
    -> std::pair<Key, bool>
{
    canonicalize(unpacked, m_canonical);
    const auto parent = hint.template parent_key<NumericStorage<Kind>>().value;

    // Value vectors grow with the highest function term set, so a successor may be longer than its parent,
    // and a write delta cannot grow a stored vector.
    const auto& values = m_canonical;
    if (values.size() != m_float_vectors->row_size(parent))
    {
        const auto [handle, inserted] = m_float_vectors->insert(values);
        return { Key(handle), inserted };
    }

    m_writes.clear();
    for (const auto& change : hint.delta->numerics)
    {
        const auto position = static_cast<ygg::uint_t>(change.fterm.value);
        m_writes.push_back(urd::Write<ygg::float_t> { position, values[position] });
    }

    const auto [handle, inserted] = m_float_vectors->insert(parent, m_writes);
    return { Key(handle), inserted };
}

template<TaskKind Kind, typename Tag, bool ThreadSafe>
void StateComponentRepository<NumericStorage<Kind>, Tag, ThreadSafe>::unpack(const Handle& packed, Unpacked& unpacked) const
{
    unpacked.values.resize(m_float_vectors->row_size(packed));
    m_float_vectors->copy(packed, unpacked.values);
}

template<TaskKind Kind, typename Tag, bool ThreadSafe>
std::vector<ygg::float_t> StateComponentRepository<NumericStorage<Kind>, Tag, ThreadSafe>::unpack(Key key) const
{
    auto values = std::vector<ygg::float_t>(m_float_vectors->row_size(key.value));
    m_float_vectors->copy(key.value, values);
    return values;
}

template class StateComponentRepository<NumericStorage<GroundTag>, StateStoragePolicyTag, true>;
template class StateComponentRepository<NumericStorage<LiftedTag>, StateStoragePolicyTag, true>;

}
