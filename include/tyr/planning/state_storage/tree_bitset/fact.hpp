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

#ifndef TYR_PLANNING_STATE_STORAGE_TREE_BITSET_FACT_HPP_
#define TYR_PLANNING_STATE_STORAGE_TREE_BITSET_FACT_HPP_

#include "tyr/planning/declarations.hpp"
#include "tyr/planning/ground/state_storage.hpp"
#include "tyr/planning/lifted/state_storage.hpp"
#include "tyr/planning/state_delta.hpp"
#include "tyr/planning/state_storage.hpp"
#include "tyr/planning/state_storage/tags.hpp"
#include "tyr/planning/state_storage/tree_bitset/block.hpp"
#include "tyr/planning/state_storage/tree_bitset/context.hpp"

#include <span>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>

namespace tyr::planning
{

/// Stores fluent facts as a bitset tree: ground tasks pack the FDR values bitwise into blocks, as in TreeVector;
/// lifted tasks store one bit per (binary) fluent variable.
template<TaskKind Kind, bool ThreadSafe>
class FactStorageBackend<Kind, TreeBitset, ThreadSafe>
{
public:
    using Context = StateStorageContext<Kind, TreeBitset, ThreadSafe>;
    using Unpacked = FactUnpackedStorage<Kind>;
    using Packed = typename Context::BitsetSet::handle_type;

    explicit FactStorageBackend(Context& ctx);

    Packed insert(const Unpacked& unpacked);

    /// Inserts `unpacked` as the exact `changes` to the stored `parent`. Only the tree paths to changed blocks are rebuilt.
    Packed insert(const Unpacked& unpacked, const Packed& parent, std::span<const StateDelta::FluentChange> changes);

    void unpack(const Packed& packed, Unpacked& unpacked);

private:
    Context* m_context;

    urd::Store<urd::sequence<tree_bitset::Block>> m_blocks;
    urd::Store<urd::sequence<tree_bitset::Flip>> m_flips;
};

}

#endif
