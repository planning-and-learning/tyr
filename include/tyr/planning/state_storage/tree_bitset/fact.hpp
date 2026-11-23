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

#ifndef TYR_PLANNING_STATE_STORAGE_TREE_BITSET_FACT_HPP_
#define TYR_PLANNING_STATE_STORAGE_TREE_BITSET_FACT_HPP_

#include "tyr/planning/declarations.hpp"
#include "tyr/planning/state_delta.hpp"
#include "tyr/planning/state_storage.hpp"
#include "tyr/planning/state_storage/tags.hpp"
#include "tyr/planning/state_storage/tree_bitset/block.hpp"
#include "tyr/planning/state_storage/tree_bitset/context.hpp"

#include <concepts>
#include <span>
#include <type_traits>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <utility>
#include <vector>

namespace tyr::planning
{

/// Stores fluent facts as a bitset tree: ground tasks pack the FDR values bitwise into blocks, as in TreeVector;
/// lifted tasks store one bit per (binary) fluent variable.
template<TaskKind Kind, bool ThreadSafe>
class StateComponentRepository<FactStorage<Kind>, TreeBitset, ThreadSafe>
{
public:
    using Context = StateStorageContext<Kind, TreeBitset, ThreadSafe>;
    using Unpacked = urd::builder<FactStorage<Kind>, StateRepository<Kind>>;
    using Handle = typename Context::BitsetSet::handle_type;
    using Key = urd::key<FactStorage<Kind>>;

    explicit StateComponentRepository(Context& ctx);

    /// The store of a state repository's fluent fact storages (urd's InsertsRecordsOf, FindsRecordsOf,
    /// HandsOutRecordsOf, ContainsKeysOf): a record as its bitset tree, stored once under its handle.
    template<typename Make>
    std::pair<Key, bool> insert(const urd::data<FactStorage<Kind>>& record, Make&& /*make*/)
    {
        const auto [handle, inserted] = store(record);
        return { Key(handle), inserted };
    }

    [[nodiscard]] Key find(const urd::data<FactStorage<Kind>>& record) const;

    [[nodiscard]] StoredStorage<FactStorage<Kind>, StateComponentRepository> operator[](Key key) const noexcept { return { *this, key }; }

    /// Every handle of its kind names a storage of this pool: the state repository's workers share one pool, not layered.
    [[nodiscard]] bool contains(Key /*key*/) const noexcept { return true; }

    /// The built form of a storage: a ground state's values, a lifted state's bits.
    using Built = std::conditional_t<std::same_as<Kind, GroundTag>, std::vector<ygg::uint_t>, StateBits>;

    /// A stored storage's values or bits, in their built form.
    [[nodiscard]] Built unpack(Key key) const;

    /// The storages in its pool (its set is shared with the other bitset storage of the state, whose rows it counts too): its set's rows, none below it (not
    /// layered: urd::count_at's size and base).
    [[nodiscard]] std::size_t size() const noexcept { return m_context->bitsets.size(); }

    [[nodiscard]] std::size_t base() const noexcept { return 0; }

    /// Its own memory (urd::memory_usage): its buffers; its pool's set is its state repository's (memory_usage()).
    static constexpr auto memory_members() noexcept
    { return urd::Members<&StateComponentRepository::m_blocks, &StateComponentRepository::m_flips, &StateComponentRepository::m_bits> {}; }

    Handle insert(const Unpacked& unpacked);

    /// Inserts `unpacked` as the exact `changes` to the stored `parent`. Only the tree paths to changed blocks are rebuilt.
    Handle insert(const Unpacked& unpacked, const Handle& parent, std::span<const StateDelta::FluentChange> changes);

    void unpack(const Handle& packed, Unpacked& unpacked);

private:
    /// A record stored: its values packed bitwise as blocks (ground), or its bits (lifted).
    std::pair<Handle, bool> store(const urd::data<FactStorage<Kind>>& record);

    Context* m_context;

    /// A lifted record's bits, as the dense set its pool takes (its buffer kept: the generic insert's only copy).
    StateBits m_bits;

    urd::Store<urd::sequence<tree_bitset::Block>> m_blocks;
    urd::Store<urd::sequence<tree_bitset::Flip>> m_flips;
};

}

#endif
