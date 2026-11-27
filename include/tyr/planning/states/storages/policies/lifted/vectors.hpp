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

#ifndef TYR_PLANNING_STATES_STORAGES_POLICIES_LIFTED_VECTORS_HPP_
#define TYR_PLANNING_STATES_STORAGES_POLICIES_LIFTED_VECTORS_HPP_

#include "tyr/planning/declarations.hpp"
#include "tyr/planning/states/state_data.hpp"
#include "tyr/planning/states/state_delta.hpp"
#include "tyr/planning/states/storages/state_storage.hpp"
#include "tyr/planning/states/storages/policies/tags.hpp"

#include <span>
#include <type_traits>
#include <urd/memory.hpp>
#include <urd/repr/pools/pools.hpp>
#include <urd/repr/pools/tree_vector.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <utility>
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

    /// Temporal parts, packed into words, stored whole (temporal.hpp).
    using TemporalSet =
        urd::Store<urd::pool<urd::Store<urd::sequence<std::uint64_t>, urd::frozen>>, std::conditional_t<ThreadSafe, urd::concurrent, urd::single_threaded>>;

    explicit StateStorageContext(const Task<LiftedTag>&) noexcept {}

    UintVectorSet uint_vectors;
    FloatVectorSet float_vectors;
    TemporalSet temporals;

    [[nodiscard]] size_t memory_usage() const noexcept
    { return urd::memory_usage(uint_vectors) + urd::memory_usage(float_vectors) + urd::memory_usage(temporals); }
};

/// A bitset of a lifted state (S: its fluent facts or derived atoms) as the vector of its true positions.
template<typename S, typename Context>
class BitPositionsRepository
{
public:
    using Unpacked = urd::builder<S, StateRepository<LiftedTag>>;
    using Handle = typename Context::UintVectorSet::handle_type;
    using Key = urd::key<S>;

    explicit BitPositionsRepository(Context& ctx);

    /// The store of a state repository's storages of S (urd's InsertsRecordsOf, FindsRecordsOf, ContainsKeysOf; the
    /// derived pool hands its records out): a record's true positions, stored once under their handle.
    template<typename Make>
    std::pair<Key, bool> insert(const urd::data<S>& record, Make&& /*make*/)
    {
        positions(std::span<const urd::word_t>(record.indices.words.data(), record.indices.words.size()), m_buffer);
        const auto [handle, inserted] = m_uint_vectors->insert(m_buffer);
        return { Key(handle), inserted };
    }

    [[nodiscard]] Key find(const urd::data<S>& record) const;

    /// Every handle of its kind names a storage of this pool: the state repository's workers share one pool, not layered.
    [[nodiscard]] bool contains(Key /*key*/) const noexcept { return true; }

    /// A stored storage's bits, in their built form.
    [[nodiscard]] StateBits unpack(Key key) const;

    /// The storages in its pool: its set's rows, none below it (not layered: urd::count_at's size and base).
    [[nodiscard]] std::size_t size() const noexcept { return m_uint_vectors->size(); }

    [[nodiscard]] std::size_t base() const noexcept { return 0; }

    /// Its own memory (urd::memory_usage): its buffers; its pool's set is its state repository's (memory_usage()).
    static constexpr auto memory_members() noexcept
    { return urd::Members<&BitPositionsRepository::m_buffer, &BitPositionsRepository::m_parent, &BitPositionsRepository::m_writes> {}; }

    Handle insert(const Unpacked& unpacked);

    /// Inserts `unpacked` as writes to the stored `parent` where it has as many true positions; else in full.
    Handle insert(const Unpacked& unpacked, const Handle& parent);

    /// The positions to write are found by comparing with `parent`, so the changes (the third argument) are not needed.
    Handle insert(const Unpacked& unpacked, const Handle& parent, const auto&) { return insert(unpacked, parent); }

    void unpack(const Handle& packed, Unpacked& unpacked);

private:
    /// The true positions of bits given by their words, ascending.
    static void positions(std::span<const urd::word_t> words, urd::Store<urd::sequence<ygg::uint_t>>& result);
    /// A stored storage's bits, read into `bits`.
    void read(const Handle& packed, StateBits& bits) const;

    typename Context::UintVectorSet* m_uint_vectors;

    urd::Store<urd::sequence<ygg::uint_t>> m_buffer;
    urd::Store<urd::sequence<ygg::uint_t>> m_parent;
    urd::Store<urd::sequence<urd::Write<ygg::uint_t>>> m_writes;
};

template<typename... Tags, bool ThreadSafe>
class StateComponentRepository<FactStorage<LiftedTag>, Vectors<Tags...>, ThreadSafe> :
    public BitPositionsRepository<FactStorage<LiftedTag>, StateStorageContext<LiftedTag, Vectors<Tags...>, ThreadSafe>>
{
public:
    using BitPositionsRepository<FactStorage<LiftedTag>, StateStorageContext<LiftedTag, Vectors<Tags...>, ThreadSafe>>::BitPositionsRepository;

    [[nodiscard]] StoredStorage<FactStorage<LiftedTag>, StateComponentRepository> operator[](urd::key<FactStorage<LiftedTag>> key) const noexcept
    { return { *this, key }; }
};

template<typename... Tags, bool ThreadSafe>
class StateComponentRepository<AtomStorage<LiftedTag>, Vectors<Tags...>, ThreadSafe> :
    public BitPositionsRepository<AtomStorage<LiftedTag>, StateStorageContext<LiftedTag, Vectors<Tags...>, ThreadSafe>>
{
public:
    using BitPositionsRepository<AtomStorage<LiftedTag>, StateStorageContext<LiftedTag, Vectors<Tags...>, ThreadSafe>>::BitPositionsRepository;

    [[nodiscard]] StoredStorage<AtomStorage<LiftedTag>, StateComponentRepository> operator[](urd::key<AtomStorage<LiftedTag>> key) const noexcept
    { return { *this, key }; }
};

}

#endif
