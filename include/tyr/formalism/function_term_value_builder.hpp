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

#ifndef TYR_FORMALISM_FUNCTION_TERM_VALUE_BUILDER_HPP_
#define TYR_FORMALISM_FUNCTION_TERM_VALUE_BUILDER_HPP_

#include "tyr/formalism/function_term_builder.hpp"
#include "tyr/formalism/function_term_value_data.hpp"
#include "tyr/formalism/function_term_value_view.hpp"

#include <optional>
#include <urd/elements/builder.hpp>
#include <variant>
#include <vector>

namespace urd
{

/// The builder of FunctionTermValue: its record's members; each child built or interned (urd::child).
template<::tyr::formalism::FactKind F, typename C>
struct builder<::tyr::formalism::FunctionTermValue<::tyr::GroundTag, F>, C>
{
    urd::index<::tyr::formalism::FunctionTermValue<::tyr::GroundTag, F>> index;
    urd::child<::tyr::formalism::FunctionTerm<::tyr::GroundTag, F>, C> fterm;
    ygg::float_t value = 0.0;

    static constexpr auto fields() noexcept { return urd::Fields<urd::field<"fterm", &builder::fterm>, urd::field<"value", &builder::value>> {}; }

    static constexpr auto identifying_members() noexcept { return urd::Members<&builder::fterm, &builder::value> {}; }
};

}  // namespace urd

#endif
