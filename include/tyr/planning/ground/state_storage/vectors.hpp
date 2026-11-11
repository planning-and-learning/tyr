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

#ifndef TYR_PLANNING_GROUND_STATE_STORAGE_VECTORS_HPP_
#define TYR_PLANNING_GROUND_STATE_STORAGE_VECTORS_HPP_

#include "tyr/planning/declarations.hpp"
#include "tyr/planning/ground/state_storage.hpp"
#include "tyr/planning/state_delta.hpp"
#include "tyr/planning/state_storage.hpp"
#include "tyr/planning/state_storage/tags.hpp"
#include "tyr/planning/state_storage/tree_bitset/block.hpp"

#include <span>
#include <type_traits>
#include <urd/memory.hpp>
#include <urd/repr/pools/pools.hpp>
#include <urd/repr/pools/tree_vector.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <yggdrasil/core/config.hpp>

namespace tyr::planning
{

/// Fluent facts and derived atoms packed bitwise into blocks, each interned whole; numeric values in a pool with `Tags`.
template<typename... Tags, bool ThreadSafe>
struct StateStorageContext<GroundTag, Vectors<Tags...>, ThreadSafe>
{
    using ArraySet =
        urd::Store<urd::pool<urd::Store<urd::sequence<ygg::uint_t>, urd::frozen>>, std::conditional_t<ThreadSafe, urd::concurrent, urd::single_threaded>>;
    using FloatVectorSet = urd::
        Store<urd::pool<urd::Store<urd::sequence<ygg::float_t>, urd::frozen>>, Tags..., std::conditional_t<ThreadSafe, urd::concurrent, urd::single_threaded>>;

    explicit StateStorageContext(const Task<GroundTag>& task) : layout(task) {}

    StateLayout<GroundTag> layout;
    ArraySet fluent_array_set;
    ArraySet derived_array_set;
    FloatVectorSet float_vectors;

    [[nodiscard]] size_t memory_usage() const noexcept
    { return urd::memory_usage(fluent_array_set) + urd::memory_usage(derived_array_set) + urd::memory_usage(float_vectors); }
};

template<typename... Tags, bool ThreadSafe>
class FactStorageBackend<GroundTag, Vectors<Tags...>, ThreadSafe>
{
public:
    using Context = StateStorageContext<GroundTag, Vectors<Tags...>, ThreadSafe>;
    using Unpacked = FactUnpackedStorage<GroundTag>;
    using Packed = typename Context::ArraySet::handle_type;

    explicit FactStorageBackend(Context& ctx);

    Packed insert(const Unpacked& unpacked);

    /// Inserts `unpacked` as the exact `changes` to the stored `parent`, as writes to the changed blocks.
    Packed insert(const Unpacked& unpacked, const Packed& parent, std::span<const StateDelta::FluentChange> changes);

    void unpack(const Packed& packed, Unpacked& unpacked);

private:
    typename Context::ArraySet* m_array_set;
    const StateLayout<GroundTag>* m_layout;

    urd::Store<urd::sequence<ygg::uint_t>> m_buffer;
    urd::Store<urd::sequence<tree_bitset::Flip>> m_flips;
    urd::Store<urd::sequence<urd::Write<ygg::uint_t>>> m_writes;
};

template<typename... Tags, bool ThreadSafe>
class AtomStorageBackend<GroundTag, Vectors<Tags...>, ThreadSafe>
{
public:
    using Context = StateStorageContext<GroundTag, Vectors<Tags...>, ThreadSafe>;
    using Unpacked = AtomUnpackedStorage<GroundTag>;
    using Packed = typename Context::ArraySet::handle_type;

    explicit AtomStorageBackend(Context& ctx);

    Packed insert(const Unpacked& unpacked);

    /// Inserts `unpacked` as writes to the blocks in which it differs from `parent_unpacked`, stored as `parent`.
    Packed insert(const Unpacked& unpacked, const Packed& parent, const Unpacked& parent_unpacked);

    void unpack(const Packed& packed, Unpacked& unpacked);

private:
    typename Context::ArraySet* m_array_set;

    urd::Store<urd::sequence<ygg::uint_t>> m_buffer;
    urd::Store<urd::sequence<urd::Write<ygg::uint_t>>> m_writes;
};

}

#endif
