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

#ifndef TYR_PLANNING_LIFTED_STATE_STORAGE_TREE_COMPRESSION_ATOM_HPP_
#define TYR_PLANNING_LIFTED_STATE_STORAGE_TREE_COMPRESSION_ATOM_HPP_

#include "tyr/planning/declarations.hpp"
#include "tyr/planning/lifted/state_storage.hpp"
#include "tyr/planning/lifted/state_storage/tree_compression/context.hpp"
#include "tyr/planning/state_storage.hpp"
#include "tyr/planning/state_storage/tags.hpp"

#include <yggdrasil/containers/tree_vector_set.hpp>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/semantics/comparison.hpp>

namespace tyr::planning
{

template<>
struct AtomPackedStorage<LiftedTag, TreeCompression> : ygg::comparison::Mixin<AtomPackedStorage<LiftedTag, TreeCompression>>
{
    ygg::TreeVectorIndex<ygg::uint_t> index;

    AtomPackedStorage() = default;
    explicit AtomPackedStorage(ygg::TreeVectorIndex<ygg::uint_t> index) : index(index) {}

    auto identifying_members() const noexcept { return std::tie(index); }
};

template<>
class AtomStorageBackend<LiftedTag, TreeCompression>
{
public:
    using Unpacked = AtomUnpackedStorage<LiftedTag>;
    using Packed = AtomPackedStorage<LiftedTag, TreeCompression>;

    explicit AtomStorageBackend(StateStorageContext<LiftedTag, TreeCompression>& ctx);

    Packed insert(const Unpacked& unpacked);

    void unpack(const Packed& packed, Unpacked& unpacked);

private:
    ygg::TreeVectorSet<ygg::uint_t>& m_uint_vectors;

    std::vector<ygg::uint_t> m_buffer;
};

}

#endif
