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

#ifndef TYR_PLANNING_STATE_STORAGE_NUMERIC_HPP_
#define TYR_PLANNING_STATE_STORAGE_NUMERIC_HPP_

#include "tyr/planning/state_delta.hpp"
#include "tyr/planning/state_storage.hpp"

#include <span>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <yggdrasil/core/config.hpp>

namespace tyr::planning
{

/// Numeric values, in every storage policy, as vectors in the context's FloatVectorSet.
template<TaskKind Kind, typename Tag, bool ThreadSafe>
class NumericStorageBackend
{
public:
    using Context = StateStorageContext<Kind, Tag, ThreadSafe>;
    using Unpacked = NumericUnpackedStorage<Kind>;
    using Packed = typename Context::FloatVectorSet::handle_type;

    explicit NumericStorageBackend(Context& ctx);

    Packed insert(Unpacked& unpacked);

    /// Inserts `unpacked` as the exact `changes` to the stored `parent`. A pool with structural sharing rebuilds only the tree paths to changed values.
    Packed insert(Unpacked& unpacked, const Packed& parent, std::span<const StateDelta::NumericChange> changes);

    void unpack(const Packed& packed, Unpacked& unpacked);

private:
    typename Context::FloatVectorSet* m_float_vectors;

    urd::Store<urd::sequence<urd::Write<ygg::float_t>>> m_writes;
};

}

#endif
