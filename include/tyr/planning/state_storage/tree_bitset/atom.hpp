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

#ifndef TYR_PLANNING_STATE_STORAGE_TREE_BITSET_ATOM_HPP_
#define TYR_PLANNING_STATE_STORAGE_TREE_BITSET_ATOM_HPP_

#include "tyr/planning/declarations.hpp"
#include "tyr/planning/state_storage.hpp"
#include "tyr/planning/state_storage/tags.hpp"
#include "tyr/planning/state_storage/tree_bitset/block.hpp"
#include "tyr/planning/state_storage/tree_bitset/context.hpp"

#include <urd/elements/flat.hpp>
#include <urd/repr/pools/tree_bitset.hpp>
#include <span>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/repr/values/dense_set.hpp>
#include <urd/store.hpp>
#include <utility>
#include <vector>

namespace tyr::planning
{

/// Stores derived atoms, a dense set in ground and lifted tasks alike, as a bitset tree, by its words.
template<TaskKind Kind, bool ThreadSafe>
class StateComponentRepository<AtomStorage<Kind>, TreeBitset, ThreadSafe>
{
public:
    using Context = StateStorageContext<Kind, TreeBitset, ThreadSafe>;
    using Unpacked = urd::builder<AtomStorage<Kind>, StateRepository<Kind>>;
    using Handle = typename Context::BitsetSet::handle_type;
    using Key = urd::key<AtomStorage<Kind>>;

    explicit StateComponentRepository(Context& ctx);

    /// The store of a state repository's derived atom storages (urd's InsertsRecordsOf, FindsRecordsOf,
    /// HandsOutRecordsOf, ContainsKeysOf): a record's words as a bitset tree, stored once under its handle.
    template<typename Make>
    std::pair<Key, bool> insert(const urd::data<AtomStorage<Kind>>& record, Make&& /*make*/)
    {
        m_bits.assign_words(std::span<const urd::word_t>(record.indices.words.data(), record.indices.words.size()));
        const auto [handle, inserted] = m_context->bitsets.insert(m_bits);
        return { Key(handle), inserted };
    }

    [[nodiscard]] Key find(const urd::data<AtomStorage<Kind>>& record) const
    {
        auto bits = StateBits();
        bits.assign_words(std::span<const urd::word_t>(record.indices.words.data(), record.indices.words.size()));
        return Key(m_context->bitsets.find(bits));
    }

    [[nodiscard]] StoredStorage<AtomStorage<Kind>, StateComponentRepository> operator[](Key key) const noexcept { return { *this, key }; }

    /// Every handle of its kind names a storage of this pool: the state repository's workers share one pool, not layered.
    [[nodiscard]] bool contains(Key /*key*/) const noexcept { return true; }

    /// A stored storage's bits, in their built form.
    [[nodiscard]] StateBits unpack(Key key) const;

    /// The storages in its pool (its set is shared with the other bitset storage of the state, whose rows it counts too): its set's rows, none below it (not layered: urd::count_at's size and base).
    [[nodiscard]] std::size_t size() const noexcept { return m_context->bitsets.size(); }
    [[nodiscard]] std::size_t base() const noexcept { return 0; }

    /// Its own memory (urd::memory_usage): its buffers; its pool's set is its state repository's (memory_usage()).
    static constexpr auto memory_members() noexcept { return urd::Members<&StateComponentRepository::m_flips, &StateComponentRepository::m_bits> {}; }

    Handle insert(const Unpacked& unpacked);

    /// Inserts `unpacked` as a delta to the stored `parent`, whose derived atoms `parent_atoms` are.
    /// Derived atoms are recomputed per state, so the changed blocks are found by comparing both rows' words.
    Handle insert(const Unpacked& unpacked, const Handle& parent, const AtomStorageView<Kind>& parent_atoms);

    void unpack(const Handle& packed, Unpacked& unpacked);

private:
    Context* m_context;

    /// A record's bits, as the dense set its pool takes (its buffer kept: the generic insert's only copy).
    StateBits m_bits;

    urd::Store<urd::sequence<tree_bitset::Flip>> m_flips;
};

}

#endif
