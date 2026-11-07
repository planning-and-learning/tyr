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

template<TaskKind Kind>
struct AtomUnpackedStorage;

template<TaskKind Kind>
struct FactUnpackedStorage;

template<TaskKind Kind>
struct NumericUnpackedStorage
{
    urd::Store<urd::sequence<ygg::float_t>> values;

    [[nodiscard]] ygg::float_t get(urd::index<formalism::planning::FunctionTerm<GroundTag, formalism::FluentTag>> index) const
    { return tyr::get(static_cast<ygg::uint_t>(index.value), values, std::numeric_limits<ygg::float_t>::quiet_NaN()); }
    void set(urd::index<formalism::planning::FunctionTerm<GroundTag, formalism::FluentTag>> index, ygg::float_t value)
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
                       using FunctionTermIndex = urd::index<formalism::planning::FunctionTerm<GroundTag, formalism::FluentTag>>;
                       return std::pair<FunctionTermIndex, ygg::float_t> { FunctionTermIndex { static_cast<ygg::uint_t>(i) }, values[i] };
                   });
    }
    void clear() { values.clear(); }
    void swap(NumericUnpackedStorage& other) noexcept { values.swap(other.values); }

    static constexpr auto identifying_members() noexcept { return urd::Members<&NumericUnpackedStorage::values> {}; }
};

}

#endif
