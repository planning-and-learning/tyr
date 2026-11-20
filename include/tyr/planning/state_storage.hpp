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
#include "tyr/planning/state_storage/config.hpp"
#include "tyr/planning/task.hpp"

#include <cmath>
#include <concepts>
#include <cstddef>
#include <limits>
#include <ranges>
#include <string_view>
#include <tuple>
#include <urd/elements/builder.hpp>
#include <urd/elements/children.hpp>
#include <urd/elements/concepts.hpp>
#include <urd/elements/flat.hpp>
#include <urd/elements/view.hpp>
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

/// A state's storage S packed: a key, its handle in the pool of its component repository by the state storage policy.
/// Only its pool makes it (StateComponentRepository::insert); it reads as S unpacked, through its state repository
/// (urd::resolution<PackedStorage<S>>, StateRepository::unpack), and is built as S unpacked (urd::built_form).
template<typename S>
struct PackedStorage
{
    static constexpr std::string_view name = urd::joined_name<"Packed", S::name>;

    typename StateComponentRepository<S, StateStoragePolicyTag>::Handle handle;

    friend constexpr auto operator<=>(const PackedStorage&, const PackedStorage&) = default;

    static constexpr auto fields() noexcept { return urd::Fields<urd::field<"handle", &PackedStorage::handle>> {}; }

    static constexpr auto identifying_members() noexcept { return urd::members_of(fields()); }
};

}

namespace urd
{

/// A packed storage reads as its storage unpacked, through its state repository (StateRepository::unpack).
template<typename S>
struct resolution<::tyr::planning::PackedStorage<S>>
{
    template<typename C>
    static auto of(const ::tyr::planning::PackedStorage<S>& key, const C& context)
    { return context.unpack(key); }

    template<std::same_as<::tyr::planning::PackedStorage<S>> R>
    static ::tyr::planning::PackedStorage<S> stored(const R& readout) noexcept
    { return readout; }
};

/// A packed storage is built as its storage unpacked: a ground state's fluent facts as a value per FDR variable, a lifted
/// state's as their bits, derived atoms as their bits, numeric values as a value per function term. Its pool packs and
/// unpacks it, so it has no from(): a builder sets it through its mutators, never from a record's.
template<typename C>
struct built_form<::tyr::planning::PackedStorage<::tyr::planning::FactStorage<::tyr::GroundTag>>, C>
{
    using type = urd::Store<urd::sequence<ygg::uint_t>>;
};

template<typename C>
struct built_form<::tyr::planning::PackedStorage<::tyr::planning::FactStorage<::tyr::LiftedTag>>, C>
{
    using type = ::tyr::planning::StateBits;
};

template<::tyr::TaskKind Kind, typename C>
struct built_form<::tyr::planning::PackedStorage<::tyr::planning::AtomStorage<Kind>>, C>
{
    using type = ::tyr::planning::StateBits;
};

template<::tyr::TaskKind Kind, typename C>
struct built_form<::tyr::planning::PackedStorage<::tyr::planning::NumericStorage<Kind>>, C>
{
    using type = urd::Store<urd::sequence<ygg::float_t>>;
};

/// A state's derived atoms being built, unpacked: their bits (its record holds them packed, state_data.hpp).
template<::tyr::TaskKind Kind, typename C>
struct builder<::tyr::planning::AtomStorage<Kind>, C>
{
    urd::built_t<::tyr::planning::PackedStorage<::tyr::planning::AtomStorage<Kind>>, C> indices;

    void set(urd::index<::tyr::formalism::Atom<::tyr::GroundTag, ::tyr::formalism::DerivedTag>> index)
    { indices.insert(static_cast<ygg::uint_t>(index.value)); }

    void clear() { indices.clear(); }

    /// Takes the bits of a view of a builder of its kind (another state's).
    void assign(const urd::borrowed_view<::tyr::planning::AtomStorage<Kind>, C, builder>& storage) { indices = storage.get_indices(); }

    void swap(builder& other) noexcept { indices.swap(other.indices); }

    static constexpr auto fields() noexcept { return urd::Fields<urd::field<"indices", &builder::indices>> {}; }

    static constexpr auto functions() noexcept
    {
        return urd::Functions<urd::function<"set", &builder::set, urd::arg<"index">>,
                              urd::function<"clear", &builder::clear>,
                              urd::function<"assign", &builder::assign, urd::arg<"storage">>,
                              urd::function<"swap", &builder::swap, urd::arg<"other">>> {};
    }

    static constexpr auto identifying_members() noexcept { return urd::members_of(fields()); }
};

/// A state's numeric values being built, unpacked: a value per function term, NaN where it has none (its record holds
/// them packed, state_data.hpp).
template<::tyr::TaskKind Kind, typename C>
struct builder<::tyr::planning::NumericStorage<Kind>, C>
{
    urd::built_t<::tyr::planning::PackedStorage<::tyr::planning::NumericStorage<Kind>>, C> values;

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

    static constexpr auto functions() noexcept
    {
        return urd::Functions<urd::function<"set", &builder::set, urd::arg<"index">, urd::arg<"value">>,
                              urd::function<"clear", &builder::clear>,
                              urd::function<"assign", &builder::assign, urd::arg<"storage">>,
                              urd::function<"swap", &builder::swap, urd::arg<"other">>> {};
    }

    static constexpr auto identifying_members() noexcept { return urd::members_of(fields()); }
};

}  // namespace urd

#endif
