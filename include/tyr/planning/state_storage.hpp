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

#include "tyr/formalism/planning/declarations.hpp"
#include <urd/repr/sequences/sequence.hpp>
#include <urd/repr/values/dense_set.hpp>
#include <urd/store.hpp>
#include "tyr/planning/task.hpp"

#include <cmath>
#include <concepts>
#include <cstddef>
#include <limits>
#include <ranges>
#include <tuple>
#include <utility>
#include <vector>
#include "tyr/containers.hpp"
#include <urd/elements/flat.hpp>
#include <yggdrasil/core/config.hpp>

namespace tyr::planning
{

/**
 * Context
 */

template<TaskKind Kind, typename Tag, bool ThreadSafe = false>
struct StateStorageContext;

/**
 * Backend
 */

template<TaskKind Kind, typename Tag, bool ThreadSafe = false>
class AtomStorageBackend;

template<TaskKind Kind, typename Tag, bool ThreadSafe = false>
class FactStorageBackend;

template<TaskKind Kind, typename Tag, bool ThreadSafe = false>
class NumericStorageBackend;

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

template<TaskKind Kind>
struct AtomUnpackedStorage
{
    StateBits indices;

    [[nodiscard]] bool test(urd::index<formalism::Atom<GroundTag, formalism::DerivedTag>> index) const
    { return indices.contains(static_cast<ygg::uint_t>(index.value)); }
    void set(urd::index<formalism::Atom<GroundTag, formalism::DerivedTag>> index) { indices.insert(static_cast<ygg::uint_t>(index.value)); }
    void clear() { indices.clear(); }
    void swap(AtomUnpackedStorage& other) noexcept { indices.swap(other.indices); }

    static constexpr auto identifying_members() noexcept { return urd::Members<&AtomUnpackedStorage::indices> {}; }
};

template<TaskKind Kind>
struct FactUnpackedStorage;

template<TaskKind Kind>
struct NumericUnpackedStorage
{
    urd::Store<urd::sequence<ygg::float_t>> values;

    [[nodiscard]] ygg::float_t get(urd::index<formalism::FunctionTerm<GroundTag, formalism::FluentTag>> index) const
    { return tyr::get(static_cast<ygg::uint_t>(index.value), values, std::numeric_limits<ygg::float_t>::quiet_NaN()); }
    void set(urd::index<formalism::FunctionTerm<GroundTag, formalism::FluentTag>> index, ygg::float_t value)
    {
        tyr::set(static_cast<ygg::uint_t>(index.value),
                 ygg::FloatTolerance<ygg::float_t>::canonicalize(value),
                 values,
                 std::numeric_limits<ygg::float_t>::quiet_NaN());
    }
    /// The function terms with a value (not NaN) and their values, by ascending function term.
    [[nodiscard]] auto fterm_values() const noexcept
    {
        return std::views::iota(std::size_t { 0 }, values.size()) | std::views::filter([this](std::size_t i) { return !std::isnan(values[i]); })
               | std::views::transform(
                   [this](std::size_t i)
                   {
                       using FunctionTermIndex = urd::index<formalism::FunctionTerm<GroundTag, formalism::FluentTag>>;
                       return std::pair<FunctionTermIndex, ygg::float_t> { FunctionTermIndex { static_cast<ygg::uint_t>(i) }, values[i] };
                   });
    }
    void clear() { values.clear(); }
    void swap(NumericUnpackedStorage& other) noexcept { values.swap(other.values); }

    static constexpr auto identifying_members() noexcept { return urd::Members<&NumericUnpackedStorage::values> {}; }
};

}

#endif
