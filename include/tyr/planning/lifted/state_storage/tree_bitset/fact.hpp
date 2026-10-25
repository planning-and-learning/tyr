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

#ifndef TYR_PLANNING_LIFTED_STATE_STORAGE_TREE_BITSET_FACT_HPP_
#define TYR_PLANNING_LIFTED_STATE_STORAGE_TREE_BITSET_FACT_HPP_

#include "tyr/planning/declarations.hpp"
#include "tyr/planning/lifted/state_storage.hpp"
#include "tyr/planning/lifted/state_storage/tree_bitset/context.hpp"
#include "tyr/planning/state_delta.hpp"
#include "tyr/planning/state_storage.hpp"
#include "tyr/planning/state_storage/tags.hpp"

#include <span>
#include <vector>
#include <yggdrasil/containers/tree_bitset_set.hpp>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/semantics/comparison.hpp>

namespace tyr::planning
{

template<>
struct FactPackedStorage<LiftedTag, TreeBitset> : ygg::comparison::Mixin<FactPackedStorage<LiftedTag, TreeBitset>>
{
    using Index = ygg::TreeBitsetIndex<tree_bitset::Block>;

    Index index;

    FactPackedStorage() = default;
    explicit FactPackedStorage(Index index) : index(index) {}

    static constexpr auto identifying_members() noexcept { return ygg::Members<&FactPackedStorage::index> {}; }
};

template<bool ThreadSafe>
class FactStorageBackend<LiftedTag, TreeBitset, ThreadSafe>
{
public:
    using Context = StateStorageContext<LiftedTag, TreeBitset, ThreadSafe>;
    using Unpacked = FactUnpackedStorage<LiftedTag>;
    using Packed = FactPackedStorage<LiftedTag, TreeBitset>;

    explicit FactStorageBackend(Context& ctx);

    Packed insert(const Unpacked& unpacked);

    /// Inserts `unpacked` as the exact `changes` to the stored `parent`. Only the tree paths to changed blocks are rebuilt.
    Packed insert(const Unpacked& unpacked, const Packed& parent, std::span<const StateDelta::FluentChange> changes);

    void unpack(const Packed& packed, Unpacked& unpacked);

private:
    typename Context::BitsetSet* m_bitsets;

    std::vector<tree_bitset::Block> m_blocks;
    std::vector<tree_bitset::Flip> m_flips;
};

}

#endif
