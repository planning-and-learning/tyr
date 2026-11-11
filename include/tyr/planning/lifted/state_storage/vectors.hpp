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

#ifndef TYR_PLANNING_LIFTED_STATE_STORAGE_VECTORS_HPP_
#define TYR_PLANNING_LIFTED_STATE_STORAGE_VECTORS_HPP_

#include "tyr/planning/declarations.hpp"
#include "tyr/planning/lifted/state_storage.hpp"
#include "tyr/planning/state_delta.hpp"
#include "tyr/planning/state_storage.hpp"
#include "tyr/planning/state_storage/tags.hpp"

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

/// Fluent facts and derived atoms as the vectors of their true positions, numeric values as vectors; both in pools with `Tags`.
template<typename... Tags, bool ThreadSafe>
struct StateStorageContext<LiftedTag, Vectors<Tags...>, ThreadSafe>
{
    using UintVectorSet = urd::
        Store<urd::pool<urd::Store<urd::sequence<ygg::uint_t>, urd::frozen>>, Tags..., std::conditional_t<ThreadSafe, urd::concurrent, urd::single_threaded>>;
    using FloatVectorSet = urd::
        Store<urd::pool<urd::Store<urd::sequence<ygg::float_t>, urd::frozen>>, Tags..., std::conditional_t<ThreadSafe, urd::concurrent, urd::single_threaded>>;

    explicit StateStorageContext(const Task<LiftedTag>&) noexcept {}

    UintVectorSet uint_vectors;
    FloatVectorSet float_vectors;

    [[nodiscard]] size_t memory_usage() const noexcept { return urd::memory_usage(uint_vectors) + urd::memory_usage(float_vectors); }
};

/// A bitset of a lifted state (`Unpacked`: fluent facts or derived atoms) as the vector of its true positions.
template<typename Unpacked_, typename Context>
class BitPositionsBackend
{
public:
    using Unpacked = Unpacked_;
    using Packed = typename Context::UintVectorSet::handle_type;

    explicit BitPositionsBackend(Context& ctx);

    Packed insert(const Unpacked& unpacked);

    /// Inserts `unpacked` as writes to the stored `parent` where it has as many true positions; else in full.
    Packed insert(const Unpacked& unpacked, const Packed& parent);

    /// The positions to write are found by comparing with `parent`, so the changes (the third argument) are not needed.
    Packed insert(const Unpacked& unpacked, const Packed& parent, const auto&) { return insert(unpacked, parent); }

    void unpack(const Packed& packed, Unpacked& unpacked);

private:
    typename Context::UintVectorSet* m_uint_vectors;

    urd::Store<urd::sequence<ygg::uint_t>> m_buffer;
    urd::Store<urd::sequence<ygg::uint_t>> m_parent;
    urd::Store<urd::sequence<urd::Write<ygg::uint_t>>> m_writes;
};

template<typename... Tags, bool ThreadSafe>
class FactStorageBackend<LiftedTag, Vectors<Tags...>, ThreadSafe> :
    public BitPositionsBackend<FactUnpackedStorage<LiftedTag>, StateStorageContext<LiftedTag, Vectors<Tags...>, ThreadSafe>>
{
public:
    using BitPositionsBackend<FactUnpackedStorage<LiftedTag>, StateStorageContext<LiftedTag, Vectors<Tags...>, ThreadSafe>>::BitPositionsBackend;
};

template<typename... Tags, bool ThreadSafe>
class AtomStorageBackend<LiftedTag, Vectors<Tags...>, ThreadSafe> :
    public BitPositionsBackend<AtomUnpackedStorage<LiftedTag>, StateStorageContext<LiftedTag, Vectors<Tags...>, ThreadSafe>>
{
public:
    using BitPositionsBackend<AtomUnpackedStorage<LiftedTag>, StateStorageContext<LiftedTag, Vectors<Tags...>, ThreadSafe>>::BitPositionsBackend;
};

}

#endif
