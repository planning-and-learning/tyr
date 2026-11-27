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

#ifndef TYR_PLANNING_STATES_STORAGES_POLICIES_TEMPORAL_HPP_
#define TYR_PLANNING_STATES_STORAGES_POLICIES_TEMPORAL_HPP_

#include "tyr/planning/states/storages/state_storage.hpp"
#include "tyr/planning/states/storages/temporal_parts.hpp"

#include <cstddef>
#include <cstdint>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <utility>

namespace tyr::planning
{

/// Temporal parts, in every storage policy, packed into word sequences in the context's TemporalSet, each stored whole:
/// a successor's projected network shares nothing to speak of with its parent's.
template<TaskKind Kind, typename Tag, bool ThreadSafe>
class StateComponentRepository<TemporalStorage<Kind>, Tag, ThreadSafe>
{
public:
    using Context = StateStorageContext<Kind, Tag, ThreadSafe>;
    using Unpacked = urd::builder<TemporalStorage<Kind>, StateRepository<Kind>>;
    using Handle = typename Context::TemporalSet::handle_type;
    using Key = urd::key<TemporalStorage<Kind>>;

    explicit StateComponentRepository(Context& ctx);

    /// The store of a state repository's temporal parts (urd's InsertsRecordsOf, FindsRecordsOf, HandsOutRecordsOf,
    /// ContainsKeysOf): a record, stored once under its handle.
    template<typename Make>
    std::pair<Key, bool> insert(const urd::data<TemporalStorage<Kind>>& record, Make&& /*make*/)
    {
        const auto [handle, inserted] = m_temporals->insert(record.parts.words);
        return { Key(handle), inserted };
    }

    [[nodiscard]] Key find(const urd::data<TemporalStorage<Kind>>& record) const { return Key(m_temporals->find(record.parts.words)); }

    [[nodiscard]] StoredStorage<TemporalStorage<Kind>, StateComponentRepository> operator[](Key key) const noexcept { return { *this, key }; }

    /// Every handle of its kind names a storage of this pool: the state repository's workers share one pool, not layered.
    [[nodiscard]] bool contains(Key /*key*/) const noexcept { return true; }

    /// A stored temporal part, in its built form (StoredStorage's fields).
    [[nodiscard]] TemporalParts unpack(Key key) const;

    /// The temporal parts in its pool: its set's rows, none below it (not layered: urd::count_at's size and base).
    [[nodiscard]] std::size_t size() const noexcept { return m_temporals->size(); }

    [[nodiscard]] std::size_t base() const noexcept { return 0; }

    /// Its own memory (urd::memory_usage): its packing buffer; its pool's set is its state repository's (memory_usage()).
    static constexpr auto memory_members() noexcept { return urd::Members<&StateComponentRepository::m_words> {}; }

    Handle insert(Unpacked& unpacked);

    void unpack(const Handle& packed, Unpacked& unpacked);

private:
    typename Context::TemporalSet* m_temporals;
    urd::Store<urd::sequence<std::uint64_t>> m_words;
};

}

#endif
