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

#ifndef TYR_PLANNING_STATES_STORAGES_NUMERIC_STORAGE_VIEW_HPP_
#define TYR_PLANNING_STATES_STORAGES_NUMERIC_STORAGE_VIEW_HPP_

#include "tyr/declarations.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/planning/fdr_fact_data.hpp"
#include "tyr/formalism/planning/fdr_fact_view.hpp"
#include "tyr/formalism/planning/fdr_value.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/states/storages/state_storage.hpp"

#include <cassert>
#include <climits>
#include <cmath>
#include <cstddef>
#include <limits>
#include <ranges>
#include <urd/elements/concepts.hpp>
#include <urd/elements/index.hpp>
#include <urd/elements/view.hpp>
#include <utility>
#include <yggdrasil/core/config.hpp>

/// A state's numeric values (a view of their builder, read through the formalism repository): a value per function term, NaN where it
/// has none.
template<::tyr::TaskKind Kind, typename V>
struct urd::view_mixin<::tyr::planning::NumericStorage<Kind>, V>
{
private:
    [[nodiscard]] const V& self() const noexcept { return static_cast<const V&>(*this); }

public:
    [[nodiscard]] decltype(auto) values() const noexcept { return self().template member<&urd::record_of<V>::values>(); }

    [[nodiscard]] ygg::float_t get(urd::index<::tyr::formalism::FunctionTerm<::tyr::GroundTag, ::tyr::formalism::FluentTag>> index) const
    { return ::tyr::get(static_cast<ygg::uint_t>(index.value), values(), std::numeric_limits<ygg::float_t>::quiet_NaN()); }

    /// The function terms with a value (not NaN) and their values, by ascending function term.
    [[nodiscard]] auto fterm_values() const noexcept
    {
        const auto* values = &self().values();
        return std::views::iota(std::size_t { 0 }, values->size()) | std::views::filter([values](std::size_t i) { return !std::isnan((*values)[i]); })
               | std::views::transform(
                   [values](std::size_t i)
                   {
                       using FunctionTermIndex = urd::index<::tyr::formalism::FunctionTerm<::tyr::GroundTag, ::tyr::formalism::FluentTag>>;
                       return std::pair<FunctionTermIndex, ygg::float_t> { FunctionTermIndex { static_cast<ygg::uint_t>(i) }, (*values)[i] };
                   });
    }

    static constexpr auto functions() noexcept
    { return urd::Functions<urd::function<"get", &view_mixin::get, urd::arg<"index">>, urd::function<"fterm_values", &view_mixin::fterm_values>> {}; }
};

#endif
