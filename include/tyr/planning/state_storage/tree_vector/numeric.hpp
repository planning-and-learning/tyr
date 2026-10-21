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

#ifndef TYR_PLANNING_STATE_STORAGE_NUMERIC_TREE_VECTOR_HPP_
#define TYR_PLANNING_STATE_STORAGE_NUMERIC_TREE_VECTOR_HPP_

#include "tyr/planning/state_delta.hpp"
#include "tyr/planning/state_storage.hpp"
#include "tyr/planning/state_storage/tags.hpp"

#include <limits>
#include <span>
#include <vector>
#include <yggdrasil/containers/tree_vector_set.hpp>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/semantics/comparison.hpp>

namespace tyr::planning
{

template<TaskKind Kind, TreeCompressionTag Tag>
struct NumericPackedStorage<Kind, Tag> : ygg::comparison::Mixin<NumericPackedStorage<Kind, Tag>>
{
    ygg::TreeVectorIndex<ygg::float_t> index;

    NumericPackedStorage() = default;
    explicit NumericPackedStorage(ygg::TreeVectorIndex<ygg::float_t> index) : index(index) {}

    static constexpr auto identifying_members() noexcept { return ygg::Members<&NumericPackedStorage::index> {}; }
};

template<TaskKind Kind, TreeCompressionTag Tag, bool ThreadSafe>
class NumericStorageBackend<Kind, Tag, ThreadSafe>
{
public:
    using Context = StateStorageContext<Kind, Tag, ThreadSafe>;
    using Unpacked = NumericUnpackedStorage<Kind>;
    using Packed = NumericPackedStorage<Kind, Tag>;

    explicit NumericStorageBackend(Context& ctx);

    Packed insert(Unpacked& unpacked);

    /// Inserts `unpacked` as the exact `changes` to the stored `parent`. Only the tree paths to changed values are rebuilt.
    Packed insert(Unpacked& unpacked, const Packed& parent, std::span<const StateDelta::NumericChange> changes);

    void unpack(const Packed& packed, Unpacked& unpacked);

private:
    typename Context::FloatVectorSet& m_float_vectors;

    std::vector<ygg::TreeVectorWrite<ygg::float_t>> m_writes;
};

}

#endif
