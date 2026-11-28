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


#include "../tree_bitset.hpp"
#include "tyr/planning/states/state_data.hpp"
#include "tyr/planning/states/storages/atom_storage_builder.hpp"
#include "tyr/planning/states/storages/atom_storage_data.hpp"
#include "tyr/planning/states/storages/atom_storage_view.hpp"
#include "tyr/planning/states/storages/fact_storage_view.hpp"
#include "tyr/planning/states/storages/fact_storage_view.hpp"
#include "tyr/planning/states/storages/ground/fact_storage_builder.hpp"
#include "tyr/planning/states/storages/lifted/fact_storage_builder.hpp"
#include "tyr/planning/states/storages/numeric_storage_view.hpp"
#include "tyr/planning/states/storages/policies/tree_bitset/atom.hpp"
#include "tyr/planning/states/storages/policies/tree_bitset/context.hpp"
#include "tyr/planning/states/storages/temporal_storage_view.hpp"
#include "tyr/planning/states/state_repository.hpp"
#include "tyr/planning/states/successor_hint.hpp"
#include "tyr/planning/tasks/ground/task.hpp"
#include "tyr/planning/tasks/lifted/task.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>

namespace tyr::planning
{
template<TaskKind Kind, bool ThreadSafe>
StateComponentRepository<AtomStorage<Kind>, TreeBitset, ThreadSafe>::StateComponentRepository(Context& ctx) : m_context(&ctx)
{
}

template<TaskKind Kind, bool ThreadSafe>
auto StateComponentRepository<AtomStorage<Kind>, TreeBitset, ThreadSafe>::insert_built(const Unpacked& unpacked) -> std::pair<Key, bool>
{
    const auto [handle, inserted] = m_context->bitsets.insert(unpacked.indices);
    return { Key(handle), inserted };
}

template<TaskKind Kind, bool ThreadSafe>
auto StateComponentRepository<AtomStorage<Kind>, TreeBitset, ThreadSafe>::insert_built(const Unpacked& unpacked, const SuccessorHint<Kind>& hint)
    -> std::pair<Key, bool>
{
    // One flip per block in which the two rows differ, by increasing position.
    const auto from = hint.parent_state->atom_storage().indices().significant_words();
    const auto to = unpacked.indices.significant_words();
    m_flips.clear();
    for (std::size_t i = 0; i < 2 * std::max(from.size(), to.size()); ++i)
        if (const auto mask = tree_bitset::Block(block_of(from, i) ^ block_of(to, i)); mask != tree_bitset::Block { 0 })
            m_flips.push_back(tree_bitset::Flip { static_cast<std::uint32_t>(i), mask });
    const auto [handle, inserted] = m_context->bitsets.insert_flips(hint.template parent_key<AtomStorage<Kind>>().value, m_flips);
    return { Key(handle), inserted };
}

template<TaskKind Kind, bool ThreadSafe>
void StateComponentRepository<AtomStorage<Kind>, TreeBitset, ThreadSafe>::unpack(const Handle& packed, Unpacked& unpacked) const
{ m_context->bitsets.copy(packed, unpacked.indices); }

template<TaskKind Kind, bool ThreadSafe>
StateBits StateComponentRepository<AtomStorage<Kind>, TreeBitset, ThreadSafe>::unpack(Key key) const
{
    auto bits = StateBits();
    m_context->bitsets.copy(key.value, bits);
    return bits;
}

template class StateComponentRepository<AtomStorage<GroundTag>, StateStoragePolicyTag, true>;
template class StateComponentRepository<AtomStorage<LiftedTag>, StateStoragePolicyTag, true>;

}
