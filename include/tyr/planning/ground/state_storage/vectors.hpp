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
#include <utility>
#include <vector>
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
class StateComponentRepository<FactStorage<GroundTag>, Vectors<Tags...>, ThreadSafe>
{
public:
    using Context = StateStorageContext<GroundTag, Vectors<Tags...>, ThreadSafe>;
    using Unpacked = urd::builder<FactStorage<GroundTag>, StateRepository<GroundTag>>;
    using Handle = typename Context::ArraySet::handle_type;
    using Key = urd::key<FactStorage<GroundTag>>;

    explicit StateComponentRepository(Context& ctx);

    /// The store of a state repository's fluent fact storages (urd's InsertsRecordsOf, FindsRecordsOf,
    /// HandsOutRecordsOf, ContainsKeysOf): a record packed bitwise, stored once under its handle.
    template<typename Make>
    std::pair<Key, bool> insert(const urd::data<FactStorage<GroundTag>>& record, Make&& /*make*/)
    {
        pack(std::span<const ygg::uint_t>(record.values.data(), record.values.size()), m_buffer);
        const auto [handle, inserted] = m_array_set->insert(m_buffer);
        return { Key(handle), inserted };
    }

    [[nodiscard]] Key find(const urd::data<FactStorage<GroundTag>>& record) const;

    [[nodiscard]] StoredStorage<FactStorage<GroundTag>, StateComponentRepository> operator[](Key key) const noexcept { return { *this, key }; }

    /// Every handle of its kind names a storage of this pool: the state repository's workers share one pool, not layered.
    [[nodiscard]] bool contains(Key /*key*/) const noexcept { return true; }

    /// A stored storage's values, in their built form (StoredStorage's fields).
    [[nodiscard]] std::vector<ygg::uint_t> unpack(Key key) const;

    /// The storages in its pool: its set's rows, none below it (not layered: urd::count_at's size and base).
    [[nodiscard]] std::size_t size() const noexcept { return m_array_set->size(); }
    [[nodiscard]] std::size_t base() const noexcept { return 0; }

    /// Its own memory (urd::memory_usage): its buffers; its pool's set is its state repository's (memory_usage()).
    static constexpr auto memory_members() noexcept { return urd::Members<&StateComponentRepository::m_buffer, &StateComponentRepository::m_flips, &StateComponentRepository::m_writes> {}; }

    Handle insert(const Unpacked& unpacked);

    /// Inserts `unpacked` as the exact `changes` to the stored `parent`, as writes to the changed blocks.
    Handle insert(const Unpacked& unpacked, const Handle& parent, std::span<const StateDelta::FluentChange> changes);

    void unpack(const Handle& packed, Unpacked& unpacked);

private:
    /// The values, packed bitwise into `blocks` (the task's layout).
    void pack(std::span<const ygg::uint_t> values, urd::Store<urd::sequence<ygg::uint_t>>& blocks) const;
    /// A stored storage's values, read into `values`.
    void read(const Handle& packed, std::vector<ygg::uint_t>& values) const;

    typename Context::ArraySet* m_array_set;
    const StateLayout<GroundTag>* m_layout;

    urd::Store<urd::sequence<ygg::uint_t>> m_buffer;
    urd::Store<urd::sequence<tree_bitset::Flip>> m_flips;
    urd::Store<urd::sequence<urd::Write<ygg::uint_t>>> m_writes;
};

template<typename... Tags, bool ThreadSafe>
class StateComponentRepository<AtomStorage<GroundTag>, Vectors<Tags...>, ThreadSafe>
{
public:
    using Context = StateStorageContext<GroundTag, Vectors<Tags...>, ThreadSafe>;
    using Unpacked = urd::builder<AtomStorage<GroundTag>, StateRepository<GroundTag>>;
    using Handle = typename Context::ArraySet::handle_type;
    using Key = urd::key<AtomStorage<GroundTag>>;

    explicit StateComponentRepository(Context& ctx);

    /// The store of a state repository's derived atom storages (as the fluent facts'): a record's words in blocks,
    /// stored once under its handle.
    template<typename Make>
    std::pair<Key, bool> insert(const urd::data<AtomStorage<GroundTag>>& record, Make&& /*make*/)
    {
        pack(std::span<const urd::word_t>(record.indices.words.data(), record.indices.words.size()), m_buffer);
        const auto [handle, inserted] = m_array_set->insert(m_buffer);
        return { Key(handle), inserted };
    }

    [[nodiscard]] Key find(const urd::data<AtomStorage<GroundTag>>& record) const;

    [[nodiscard]] StoredStorage<AtomStorage<GroundTag>, StateComponentRepository> operator[](Key key) const noexcept { return { *this, key }; }

    /// Every handle of its kind names a storage of this pool: the state repository's workers share one pool, not layered.
    [[nodiscard]] bool contains(Key /*key*/) const noexcept { return true; }

    /// A stored storage's bits, in their built form.
    [[nodiscard]] StateBits unpack(Key key) const;

    /// The storages in its pool: its set's rows, none below it (not layered: urd::count_at's size and base).
    [[nodiscard]] std::size_t size() const noexcept { return m_array_set->size(); }
    [[nodiscard]] std::size_t base() const noexcept { return 0; }

    /// Its own memory (urd::memory_usage): its buffers; its pool's set is its state repository's (memory_usage()).
    static constexpr auto memory_members() noexcept { return urd::Members<&StateComponentRepository::m_buffer, &StateComponentRepository::m_writes> {}; }

    Handle insert(const Unpacked& unpacked);

    /// Inserts `unpacked` as writes to the blocks in which it differs from `parent_atoms` (the parent's derived atoms),
    /// stored as `parent`.
    Handle insert(const Unpacked& unpacked, const Handle& parent, const AtomStorageView<GroundTag>& parent_atoms);

    void unpack(const Handle& packed, Unpacked& unpacked);

private:
    /// The bits' words, as blocks.
    static void pack(std::span<const urd::word_t> words, urd::Store<urd::sequence<ygg::uint_t>>& blocks);
    /// A stored storage's bits, read into `bits`.
    void read(const Handle& packed, StateBits& bits) const;

    typename Context::ArraySet* m_array_set;

    urd::Store<urd::sequence<ygg::uint_t>> m_buffer;
    urd::Store<urd::sequence<urd::Write<ygg::uint_t>>> m_writes;
};

}

#endif
