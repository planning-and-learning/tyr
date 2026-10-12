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

#ifndef TYR_PLANNING_GROUND_STATE_STORAGE_TREE_BITSET_ATOM_HPP_
#define TYR_PLANNING_GROUND_STATE_STORAGE_TREE_BITSET_ATOM_HPP_

#include "tyr/planning/declarations.hpp"
#include "tyr/planning/ground/state_storage.hpp"
#include "tyr/planning/ground/state_storage/tree_bitset/context.hpp"
#include "tyr/planning/state_storage.hpp"
#include "tyr/planning/state_storage/tags.hpp"
#include "tyr/planning/state_storage/tree_bitset/block.hpp"

#include <vector>
#include <yggdrasil/containers/tree_bitset_set.hpp>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/semantics/comparison.hpp>

namespace tyr::planning
{

template<>
struct AtomPackedStorage<GroundTag, TreeBitset> : ygg::comparison::Mixin<AtomPackedStorage<GroundTag, TreeBitset>>
{
    using Index = ygg::TreeBitsetIndex<tree_bitset::Block>;

    Index index;

    AtomPackedStorage() = default;
    explicit AtomPackedStorage(Index index) : index(index) {}

    auto identifying_members() const noexcept { return std::tie(index); }
};

template<bool ThreadSafe>
class AtomStorageBackend<GroundTag, TreeBitset, ThreadSafe>
{
public:
    using Context = StateStorageContext<GroundTag, TreeBitset, ThreadSafe>;
    using Unpacked = AtomUnpackedStorage<GroundTag>;
    using Packed = AtomPackedStorage<GroundTag, TreeBitset>;

    explicit AtomStorageBackend(Context& ctx);

    Packed insert(const Unpacked& unpacked);

    /// Inserts `unpacked` relative to the stored `parent`, whose unpacked form is `parent_unpacked`.
    /// Derived atoms are recomputed per state, so the changed blocks are found by comparing both bitsets.
    Packed insert(const Unpacked& unpacked, const Packed& parent, const Unpacked& parent_unpacked);

    void unpack(const Packed& packed, Unpacked& unpacked);

private:
    typename Context::BitsetSet& m_bitsets;
    ygg::uint_t m_num_bits;

    std::vector<tree_bitset::Block> m_blocks;
    std::vector<tree_bitset::Flip> m_flips;
};

}

#endif
