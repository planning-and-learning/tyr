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

#ifndef TYR_PLANNING_GROUND_STATE_STORAGE_TREE_COMPRESSION_FACT_HPP_
#define TYR_PLANNING_GROUND_STATE_STORAGE_TREE_COMPRESSION_FACT_HPP_

#include "tyr/planning/declarations.hpp"
#include "tyr/planning/ground/state_storage.hpp"
#include "tyr/planning/ground/state_storage/tree_compression/context.hpp"
#include "tyr/planning/state_storage.hpp"
#include "tyr/planning/state_storage/tags.hpp"

#include <boost/dynamic_bitset.hpp>
#include <limits>
#include <valla/valla.hpp>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/semantics/comparison.hpp>

namespace tyr::planning
{

template<>
struct FactPackedStorage<GroundTag, TreeCompression> : ygg::comparison::Mixin<FactPackedStorage<GroundTag, TreeCompression>>
{
    ygg::uint_t index;

    FactPackedStorage() = default;
    explicit FactPackedStorage(ygg::uint_t index) : index(index) {}

    auto identifying_members() const noexcept { return std::tie(index); }
};

template<>
class FactStorageBackend<GroundTag, TreeCompression>
{
public:
    using Unpacked = FactUnpackedStorage<GroundTag>;
    using Packed = FactPackedStorage<GroundTag, TreeCompression>;
    using VariableInfo = typename StateStorageContext<GroundTag, TreeCompression>::VariableInfo;

    explicit FactStorageBackend(StateStorageContext<GroundTag, TreeCompression>& ctx);

    Packed insert(const Unpacked& unpacked);

    void unpack(const Packed& packed, Unpacked& unpacked);

private:
    ygg::RawArraySet<ygg::uint_t>& m_array_set;
    const std::vector<VariableInfo>& m_infos;

    std::vector<ygg::uint_t> m_buffer;
};

}

#endif
