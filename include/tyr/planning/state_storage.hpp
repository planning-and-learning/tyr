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

#ifndef TYR_PLANNING_STATE_STORAGE_HPP_
#define TYR_PLANNING_STATE_STORAGE_HPP_

#include "tyr/containers.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/planning/task.hpp"

#include <cmath>
#include <concepts>
#include <cstddef>
#include <limits>
#include <ranges>
#include <tuple>
#include <urd/elements/builder.hpp>
#include <urd/elements/flat.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/repr/values/dense_set.hpp>
#include <urd/store.hpp>
#include <utility>
#include <vector>
#include <yggdrasil/core/config.hpp>

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

}

namespace urd
{

/// A state's derived atoms being built, unpacked: their bits (its record holds them packed, state_data.hpp).
template<::tyr::TaskKind Kind, typename C>
struct builder<::tyr::planning::AtomStorage<Kind>, C>
{
    ::tyr::planning::StateBits indices;

    void set(urd::index<::tyr::formalism::Atom<::tyr::GroundTag, ::tyr::formalism::DerivedTag>> index) { indices.insert(static_cast<ygg::uint_t>(index.value)); }

    void clear() { indices.clear(); }

    /// Takes the bits of a view of a builder of its kind (another state's).
    void assign(const urd::borrowed_view<::tyr::planning::AtomStorage<Kind>, C, builder>& storage) { indices = storage.get_indices(); }

    void swap(builder& other) noexcept { indices.swap(other.indices); }

    static constexpr auto fields() noexcept { return urd::Fields<urd::field<"indices", &builder::indices>> {}; }

    static constexpr auto identifying_members() noexcept { return urd::members_of(fields()); }
};

/// A state's numeric values being built, unpacked: a value per function term, NaN where it has none (its record holds
/// them packed, state_data.hpp).
template<::tyr::TaskKind Kind, typename C>
struct builder<::tyr::planning::NumericStorage<Kind>, C>
{
    urd::Store<urd::sequence<ygg::float_t>> values;

    void set(urd::index<::tyr::formalism::FunctionTerm<::tyr::GroundTag, ::tyr::formalism::FluentTag>> index, ygg::float_t value)
    {
        ::tyr::set(static_cast<ygg::uint_t>(index.value),
                   ygg::FloatTolerance<ygg::float_t>::canonicalize(value),
                   values,
                   std::numeric_limits<ygg::float_t>::quiet_NaN());
    }

    void clear() { values.clear(); }

    /// Takes the values of a view of a builder of its kind (another state's).
    void assign(const urd::borrowed_view<::tyr::planning::NumericStorage<Kind>, C, builder>& storage) { values = storage.get_values(); }

    void swap(builder& other) noexcept { values.swap(other.values); }

    static constexpr auto fields() noexcept { return urd::Fields<urd::field<"values", &builder::values>> {}; }

    static constexpr auto identifying_members() noexcept { return urd::members_of(fields()); }
};

}  // namespace urd

#endif
