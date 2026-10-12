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

#ifndef TYR_PLANNING_LIFTED_STATE_STORAGE_TREE_COMPRESSION_FACT_HPP_
#define TYR_PLANNING_LIFTED_STATE_STORAGE_TREE_COMPRESSION_FACT_HPP_

#include "tyr/planning/declarations.hpp"
#include "tyr/planning/lifted/state_storage.hpp"
#include "tyr/planning/lifted/state_storage/tree_compression/context.hpp"
#include "tyr/planning/state_storage.hpp"
#include "tyr/planning/state_storage/tags.hpp"

#include <vector>
#include <yggdrasil/containers/tree_bitset_set.hpp>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/semantics/comparison.hpp>

namespace tyr::planning
{

template<>
struct FactPackedStorage<LiftedTag, TreeCompression> : ygg::comparison::Mixin<FactPackedStorage<LiftedTag, TreeCompression>>
{
    ygg::TreeBitsetIndex<FactUnpackedStorage<LiftedTag>::Block> index;

    FactPackedStorage() = default;
    explicit FactPackedStorage(ygg::TreeBitsetIndex<FactUnpackedStorage<LiftedTag>::Block> index) : index(index) {}

    auto identifying_members() const noexcept { return std::tie(index); }
};

template<bool ThreadSafe>
class FactStorageBackend<LiftedTag, TreeCompression, ThreadSafe>
{
public:
    using Context = StateStorageContext<LiftedTag, TreeCompression, ThreadSafe>;
    using Unpacked = FactUnpackedStorage<LiftedTag>;
    using Packed = FactPackedStorage<LiftedTag, TreeCompression>;

    explicit FactStorageBackend(Context& ctx);

    /// Rebases `unpacked` onto the returned index.
    Packed insert(Unpacked& unpacked);

    void unpack(const Packed& packed, Unpacked& unpacked);

private:
    typename Context::BitsetSet& m_bitsets;
    ygg::uint_t m_identity;

    std::vector<Unpacked::Block> m_blocks;
    std::vector<ygg::TreeBitsetFlip<Unpacked::Block>> m_flips;
};

}

#endif
