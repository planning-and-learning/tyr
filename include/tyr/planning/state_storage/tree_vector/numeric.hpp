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
#include <urd/repr/pools/tree_vector.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <vector>
#include <yggdrasil/core/config.hpp>

namespace tyr::planning
{

template<TaskKind Kind, TreeCompressionTag Tag>
struct NumericPackedStorage<Kind, Tag> : urd::compared_by_identity<NumericPackedStorage<Kind, Tag>>
{
    urd::repr::TreeHandle index;

    NumericPackedStorage() = default;
    explicit NumericPackedStorage(urd::repr::TreeHandle index_) : index(index_) {}

    static constexpr auto identifying_members() noexcept { return urd::Members<&NumericPackedStorage::index> {}; }
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
    typename Context::FloatVectorSet* m_float_vectors;

    urd::Store<urd::sequence<urd::Write<ygg::float_t>>> m_writes;
};

}

#endif
