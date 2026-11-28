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

#ifndef TYR_PLANNING_STATES_STORAGES_NUMERIC_STORAGE_BUILDER_HPP_
#define TYR_PLANNING_STATES_STORAGES_NUMERIC_STORAGE_BUILDER_HPP_

#include "tyr/declarations.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/states/storages/numeric_storage_data.hpp"
#include "tyr/planning/states/storages/state_storage.hpp"

#include <cmath>
#include <cstddef>
#include <limits>
#include <span>
#include <urd/elements/builder.hpp>
#include <urd/elements/children.hpp>
#include <urd/elements/concepts.hpp>
#include <urd/elements/index.hpp>
#include <vector>
#include <yggdrasil/core/config.hpp>

namespace urd
{

/// A state's numeric values being built: a value per function term, NaN where it has none (state_data.hpp).
template<::tyr::TaskKind Kind, typename C>
struct builder<::tyr::planning::NumericStorage<Kind>, C>
{
    urd::key<::tyr::planning::NumericStorage<Kind>> index;
    urd::built_t<urd::flat_vector<ygg::float_t>, C> values;

    void set(urd::index<::tyr::formalism::FunctionTerm<::tyr::GroundTag, ::tyr::formalism::FluentTag>> term, ygg::float_t value)
    {
        ::tyr::set(static_cast<ygg::uint_t>(term.value),
                   ygg::FloatTolerance<ygg::float_t>::canonicalize(value),
                   values,
                   std::numeric_limits<ygg::float_t>::quiet_NaN());
    }

    static constexpr auto fields() noexcept { return urd::Fields<urd::field<"values", &builder::values>> {}; }

    static constexpr auto functions() noexcept { return urd::Functions<urd::function<"set", &builder::set, urd::arg<"index">, urd::arg<"value">>> {}; }
};

}  // namespace urd

#endif
