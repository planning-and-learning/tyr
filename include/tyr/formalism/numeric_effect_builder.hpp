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

#ifndef TYR_FORMALISM_NUMERIC_EFFECT_BUILDER_HPP_
#define TYR_FORMALISM_NUMERIC_EFFECT_BUILDER_HPP_

#include "tyr/declarations.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/enums.hpp"
#include "tyr/formalism/function_term_builder.hpp"
#include "tyr/formalism/numeric_effect_data.hpp"
#include "tyr/formalism/numeric_effect_view.hpp"

#include <urd/elements/builder.hpp>
#include <urd/elements/children.hpp>
#include <urd/elements/concepts.hpp>
#include <urd/elements/index.hpp>

namespace urd
{

/// The builder of NumericEffect: its record's members, each in its built form (urd::built_t).
template<::tyr::TaskKind T, ::tyr::formalism::NumericEffectFactKind F, typename C>
struct builder<::tyr::formalism::NumericEffect<T, F>, C>
{
    urd::key<::tyr::formalism::NumericEffect<T, F>> index;
    urd::built_t<::tyr::formalism::NumericEffectOperatorKind, C> operation = ::tyr::formalism::default_numeric_effect_operator<F>;
    urd::built_t<urd::index<::tyr::formalism::FunctionTerm<T, F>>, C> fterm;
    urd::built_t<::tyr::formalism::FunctionExpression<T>, C> fexpr;

    static constexpr auto fields() noexcept
    { return urd::Fields<urd::field<"operation", &builder::operation>, urd::field<"fterm", &builder::fterm>, urd::field<"fexpr", &builder::fexpr>> {}; }

    static constexpr auto identifying_members() noexcept { return urd::Members<&builder::operation, &builder::fterm, &builder::fexpr> {}; }
};

}  // namespace urd

#endif
