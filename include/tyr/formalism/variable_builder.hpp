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

#ifndef TYR_FORMALISM_VARIABLE_BUILDER_HPP_
#define TYR_FORMALISM_VARIABLE_BUILDER_HPP_

#include "tyr/formalism/variable_data.hpp"
#include "tyr/formalism/variable_view.hpp"

#include <optional>
#include <urd/elements/builder.hpp>
#include <variant>
#include <vector>

namespace urd
{

/// The builder of Variable: its record's members; each child built or interned (urd::child).
template<typename C>
struct builder<::tyr::formalism::Variable, C>
{
    urd::index<::tyr::formalism::Variable> index;
    urd::flat_string name;

    static constexpr auto fields() noexcept { return urd::Fields<urd::field<"name", &builder::name>> {}; }

    static constexpr auto identifying_members() noexcept { return urd::Members<&builder::name> {}; }
};

}  // namespace urd

#endif
