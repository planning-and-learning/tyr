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

#ifndef TYR_PLANNING_STATES_STORAGES_STATE_STORAGE_HPP_
#define TYR_PLANNING_STATES_STORAGES_STATE_STORAGE_HPP_

#include "tyr/containers.hpp"
#include "tyr/declarations.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/states/storages/policies/config.hpp"
#include "tyr/planning/tasks/task.hpp"
#include "tyr/planning/states/storages/temporal_parts.hpp"

#include <algorithm>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <fmt/format.h>
#include <limits>
#include <ranges>
#include <span>
#include <string_view>
#include <tuple>
#include <urd/config.hpp>
#include <urd/elements/builder.hpp>
#include <urd/elements/children.hpp>
#include <urd/elements/concepts.hpp>
#include <urd/elements/data.hpp>
#include <urd/elements/flat.hpp>
#include <urd/elements/index.hpp>
#include <urd/elements/view.hpp>
#include <urd/format.hpp>
#include <urd/kinds.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/repr/values/dense_set.hpp>
#include <urd/store.hpp>
#include <urd/tags.hpp>
#include <utility>
#include <vector>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/formalism/declarations.hpp>

namespace tyr::planning
{

/**
 * Context
 */

template<TaskKind Kind, typename Tag, bool ThreadSafe = false>
struct StateStorageContext;

/**
 * Repositories
 */

/// What interns a state component's builder (urd::builder<T, Task<Kind>>: a FactStorage, AtomStorage or NumericStorage)
/// into its record (insert: the handle the record holds) and opens a record as its builder (unpack), in the storage of
/// the policy Tag.
template<typename T, typename Tag, bool ThreadSafe = false>
class StateComponentRepository;

/**
 * Unpacked
 */

/// Where a task's state components lie in its packed states; only ground tasks lay out anything.
template<TaskKind Kind>
struct StateLayout
{
    explicit StateLayout(const Task<Kind>&) noexcept {}
};

/// A state's bits by position (derived atoms; a lifted state's binary fluent variables): a dense set, equal and hashed
/// by its keys however many words it keeps. The rows of the TreeBitset pool are these, so they go in and come back by
/// their words.
using StateBits = urd::Store<urd::set<ygg::uint_t>, urd::dense, urd::sorted>;

/// A state's bits as its storage's record holds them, flat: the words of their dense set (StateBits), which is their
/// built form.
struct StateBitWords
{
    urd::flat_vector<urd::word_t> words;

    /// The words of built bits (a builder's, written into its record: urd's AssignsBuilt).
    StateBitWords& operator=(const StateBits& bits)
    {
        const auto significant = bits.significant_words();
        words.set(significant.begin(), significant.end());
        return *this;
    }

    static constexpr std::string_view name = "StateBitWords";

    static constexpr auto fields() noexcept { return urd::Fields<urd::field<"words", &StateBitWords::words>> {}; }

    static constexpr auto identifying_members() noexcept { return urd::members_of(fields()); }

    static constexpr auto cista_members() noexcept { return urd::Members<&StateBitWords::words> {}; }
};

/// Whether S is a state's storage (its fluent facts, derived atoms, numeric values or agenda), stored in a pool of its state
/// repository under the pool's handle.
template<typename S>
concept StateStorageKind = S::state_storage;

/// The pool of a state's storage S: the component repository of the build's storage policy, shared by the workers of a
/// state repository (thread-safe).
template<typename S>
using StoragePool = StateComponentRepository<S, StateStoragePolicyTag, true>;

/// What a lookup of a stored storage hands out (urd::stored_ref_of): its pool and its key there, by value; its fields
/// read unpacked, in their built form (urd::stored_field), as a builder holds them.
template<typename S, typename Pool>
class StoredStorage
{
    const Pool* m_pool;
    urd::key<S> m_key;

public:
    StoredStorage(const Pool& pool, urd::key<S> key) noexcept : m_pool(&pool), m_key(key) {}

    [[nodiscard]] const Pool& pool() const noexcept { return *m_pool; }

    [[nodiscard]] urd::key<S> key() const noexcept { return m_key; }
};

}

namespace urd
{

/// A state's storage is named by its pool's handle (key_value_of): a key of one part; none is a handle's default.
template<::tyr::planning::StateStorageKind S>
struct key_value_of<S>
{
    using type = typename ::tyr::planning::StoragePool<S>::Handle;
    static constexpr type none = type {};
};

/// A lookup of a stored storage hands out its reader (StoredStorage).
template<::tyr::planning::StateStorageKind S>
struct stored_ref_of<S>
{
    using type = ::tyr::planning::StoredStorage<S, ::tyr::planning::StoragePool<S>>;
};

/// A stored storage's values are read from its pool, unpacked into their built form.
template<auto Member, ::tyr::planning::StateStorageKind S, typename Pool>
struct stored_field<Member, ::tyr::planning::StoredStorage<S, Pool>>
{
    [[nodiscard]] static auto read(const ::tyr::planning::StoredStorage<S, Pool>& stored) { return stored.pool().unpack(stored.key()); }
};

/// A state's bits are built as their dense set, the words a record holds written into it (in place: its buffer kept).
template<typename C>
struct built_form<::tyr::planning::StateBitWords, C>
{
    using type = ::tyr::planning::StateBits;

    static void assign(const ::tyr::planning::StateBitWords& bits, type& into)
    { into.assign_words(std::span<const word_t>(bits.words.data(), bits.words.size())); }
};

}  // namespace urd

#endif
