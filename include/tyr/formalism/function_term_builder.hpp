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

#ifndef TYR_FORMALISM_FUNCTION_TERM_BUILDER_HPP_
#define TYR_FORMALISM_FUNCTION_TERM_BUILDER_HPP_

#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/function_builder.hpp"
#include "tyr/formalism/function_term_data.hpp"
#include "tyr/formalism/function_term_view.hpp"
#include "tyr/formalism/object_builder.hpp"
#include "tyr/formalism/parameters_builder.hpp"

#include <concepts>
#include <optional>
#include <type_traits>
#include <urd/elements/builder.hpp>
#include <urd/elements/children.hpp>
#include <variant>
#include <vector>
#include <yggdrasil/core/config.hpp>

namespace urd
{

/// The builder of FunctionTerm: its record's members, each in its built form (urd::built_t).
template<::tyr::formalism::FactKind F, typename C>
struct builder<::tyr::formalism::FunctionTerm<::tyr::LiftedTag, F>, C>
{
    urd::index<::tyr::formalism::FunctionTerm<::tyr::LiftedTag, F>> index;
    urd::built_t<urd::index<::tyr::formalism::Parameters>, C> parameters;
    urd::built_t<urd::index<::tyr::formalism::Function<F>>, C> function;
    urd::built_t<::tyr::formalism::TermList, C> terms;

    static constexpr auto fields() noexcept
    { return urd::Fields<urd::field<"parameters", &builder::parameters>, urd::field<"function", &builder::function>, urd::field<"terms", &builder::terms>> {}; }

    static constexpr auto identifying_members() noexcept { return urd::members_of(fields()); }
};

/// The builder of FunctionTerm: its record's members, each in its built form (urd::built_t).
template<::tyr::formalism::FactKind F, typename C>
struct builder<::tyr::formalism::FunctionTerm<::tyr::GroundTag, F>, C>
{
    urd::index<::tyr::formalism::FunctionTerm<::tyr::GroundTag, F>> index;
    urd::built_t<urd::key<::tyr::formalism::Function<F>, ygg::Row<::tyr::formalism::Function<F>>>, C> row;

    static constexpr auto fields() noexcept { return urd::Fields<urd::field<"row", &builder::row>> {}; }

    static constexpr auto identifying_members() noexcept { return urd::Members<&builder::row> {}; }
};

}  // namespace urd

namespace tyr::formalism
{

/// A function expression as a builder holds it (FunctionExpression built): a number, or an operator or a function term
/// as a child (built, or interned).
template<TaskKind T, typename C>
using BuiltFunctionExpression = std::conditional_t<std::same_as<T, GroundTag>,
                                                   std::variant<ygg::float_t,
                                                                urd::child<UnaryOperator<T, ArithmeticOperatorKind>, C>,
                                                                urd::child<BinaryOperator<T, ArithmeticOperatorKind>, C>,
                                                                urd::child<MultiOperator<T, ArithmeticOperatorKind>, C>,
                                                                urd::child<FunctionTerm<T, StaticTag>, C>,
                                                                urd::child<FunctionTerm<T, FluentTag>, C>,
                                                                urd::child<FunctionTerm<T, AuxiliaryTag>, C>>,
                                                   std::variant<ygg::float_t,
                                                                urd::child<UnaryOperator<T, ArithmeticOperatorKind>, C>,
                                                                urd::child<BinaryOperator<T, ArithmeticOperatorKind>, C>,
                                                                urd::child<MultiOperator<T, ArithmeticOperatorKind>, C>,
                                                                urd::child<FunctionTerm<T, StaticTag>, C>,
                                                                urd::child<FunctionTerm<T, FluentTag>, C>>>;

}  // namespace tyr::formalism

#endif
