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

#ifndef TYR_PLANNING_STATE_STORAGE_NUMERIC_TREE_COMPRESSION_HPP_
#define TYR_PLANNING_STATE_STORAGE_NUMERIC_TREE_COMPRESSION_HPP_

#include "tyr/planning/state_storage.hpp"
#include "tyr/planning/state_storage/tags.hpp"

#include <limits>
#include <yggdrasil/containers/tree_vector_set.hpp>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/semantics/comparison.hpp>

namespace tyr::planning
{

template<TaskKind Kind>
struct NumericPackedStorage<Kind, TreeCompression> : ygg::comparison::Mixin<NumericPackedStorage<Kind, TreeCompression>>
{
    ygg::TreeVectorIndex<ygg::float_t> index;

    NumericPackedStorage() = default;
    explicit NumericPackedStorage(ygg::TreeVectorIndex<ygg::float_t> index) : index(index) {}

    auto identifying_members() const noexcept { return std::tie(index); }
};

template<TaskKind Kind, bool ThreadSafe>
class NumericStorageBackend<Kind, TreeCompression, ThreadSafe>
{
public:
    using Context = StateStorageContext<Kind, TreeCompression, ThreadSafe>;
    using Unpacked = NumericUnpackedStorage<Kind>;
    using Packed = NumericPackedStorage<Kind, TreeCompression>;

    explicit NumericStorageBackend(Context& ctx);

    Packed insert(Unpacked& unpacked);

    void unpack(const Packed& packed, Unpacked& unpacked);

private:
    typename Context::FloatVectorSet& m_float_vectors;
};

}

#endif
