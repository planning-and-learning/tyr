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

#ifndef TYR_PLANNING_STATE_STORAGE_TREE_BITSET_ATOM_HPP_
#define TYR_PLANNING_STATE_STORAGE_TREE_BITSET_ATOM_HPP_

#include "tyr/planning/declarations.hpp"
#include "tyr/planning/state_storage.hpp"
#include "tyr/planning/state_storage/tags.hpp"
#include "tyr/planning/state_storage/tree_bitset/block.hpp"
#include "tyr/planning/state_storage/tree_bitset/context.hpp"

#include <urd/repr/pools/tree_bitset.hpp>
#include <urd/repr/values/dense_set.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <vector>
#include <urd/elements/flat.hpp>

namespace tyr::planning
{

/// Stores derived atoms, given as a bitset in ground and lifted tasks alike, as a bitset tree.
template<TaskKind Kind, bool ThreadSafe>
class AtomStorageBackend<Kind, TreeBitset, ThreadSafe>
{
public:
    using Context = StateStorageContext<Kind, TreeBitset, ThreadSafe>;
    using Unpacked = AtomUnpackedStorage<Kind>;
    using Packed = typename Context::BitsetSet::handle_type;

    explicit AtomStorageBackend(Context& ctx);

    Packed insert(const Unpacked& unpacked);

    /// Inserts `unpacked` as a delta to the stored `parent`, whose unpacked form is `parent_unpacked`.
    /// Derived atoms are recomputed per state, so the changed blocks are found by comparing both bitsets.
    Packed insert(const Unpacked& unpacked, const Packed& parent, const Unpacked& parent_unpacked);

    void unpack(const Packed& packed, Unpacked& unpacked);

private:
    Context* m_context;

    urd::Store<urd::sequence<tree_bitset::Block>> m_blocks;
    urd::Store<urd::sequence<tree_bitset::Flip>> m_flips;
};

}

#endif
