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

#ifndef TYR_PLANNING_STATES_STORAGES_POLICIES_NUMERIC_HPP_
#define TYR_PLANNING_STATES_STORAGES_POLICIES_NUMERIC_HPP_

#include "tyr/planning/states/state_delta.hpp"
#include "tyr/planning/states/storages/state_storage.hpp"

#include <span>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <utility>
#include <vector>
#include <yggdrasil/core/config.hpp>

namespace tyr::planning
{

/// A state's numeric values made canonical before they are stored or looked up (ygg's prepare_for_insert): each value
/// by the float tolerance.
template<TaskKind Kind>
void canonicalize(urd::data<NumericStorage<Kind>>& record)
{
    for (auto& value : record.values)
        value = ygg::FloatTolerance<ygg::float_t>::canonicalize(value);
}

/// Numeric values, in every storage policy, as vectors in the context's FloatVectorSet.
template<TaskKind Kind, typename Tag, bool ThreadSafe>
class StateComponentRepository<NumericStorage<Kind>, Tag, ThreadSafe>
{
public:
    using Context = StateStorageContext<Kind, Tag, ThreadSafe>;
    using Unpacked = urd::builder<NumericStorage<Kind>, StateRepository<Kind>>;
    using Handle = typename Context::FloatVectorSet::handle_type;
    using Key = urd::key<NumericStorage<Kind>>;

    explicit StateComponentRepository(Context& ctx);

    /// The store of a state repository's numeric storages (urd's InsertsRecordsOf, FindsRecordsOf, HandsOutRecordsOf,
    /// ContainsKeysOf): a record, canonical (prepare_for_insert), stored once under its handle.
    template<typename Make>
    std::pair<Key, bool> insert(const urd::data<NumericStorage<Kind>>& record, Make&& /*make*/)
    {
        const auto [handle, inserted] = m_float_vectors->insert(record.values);
        return { Key(handle), inserted };
    }

    [[nodiscard]] Key find(const urd::data<NumericStorage<Kind>>& record) const { return Key(m_float_vectors->find(record.values)); }

    [[nodiscard]] StoredStorage<NumericStorage<Kind>, StateComponentRepository> operator[](Key key) const noexcept { return { *this, key }; }

    /// Every handle of its kind names a storage of this pool: the state repository's workers share one pool, not layered.
    [[nodiscard]] bool contains(Key /*key*/) const noexcept { return true; }

    /// A stored storage's values, in their built form (StoredStorage's fields).
    [[nodiscard]] std::vector<ygg::float_t> unpack(Key key) const;

    /// The storages in its pool: its set's rows, none below it (not layered: urd::count_at's size and base).
    [[nodiscard]] std::size_t size() const noexcept { return m_float_vectors->size(); }

    [[nodiscard]] std::size_t base() const noexcept { return 0; }

    /// Its own memory (urd::memory_usage): its buffers; its pool's set is its state repository's (memory_usage()).
    static constexpr auto memory_members() noexcept { return urd::Members<&StateComponentRepository::m_writes, &StateComponentRepository::m_canonical> {}; }

    /// Interns a builder (urd's insert_built): its key, and whether it was stored now.
    std::pair<Key, bool> insert_built(const Unpacked& unpacked);

    /// Interns a successor's builder as the exact changes to its parent's (the hint). A pool with structural sharing
    /// rebuilds only the tree paths to changed values.
    std::pair<Key, bool> insert_built(const Unpacked& unpacked, const SuccessorHint<Kind>& hint);

    void unpack(const Handle& packed, Unpacked& unpacked) const;

    /// Reads its stored storage of a key into a builder (urd's unpack_built).
    void unpack_built(Key key, Unpacked& unpacked) const { unpack(key.value, unpacked); }

private:
    typename Context::FloatVectorSet* m_float_vectors;

    urd::Store<urd::sequence<urd::Write<ygg::float_t>>> m_writes;
    /// A builder's values, canonical (reused: no allocation once grown).
    std::vector<ygg::float_t> m_canonical;
};

}

#endif
