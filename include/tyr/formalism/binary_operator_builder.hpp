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

#ifndef TYR_FORMALISM_BINARY_OPERATOR_BUILDER_HPP_
#define TYR_FORMALISM_BINARY_OPERATOR_BUILDER_HPP_

#include "tyr/formalism/binary_operator_data.hpp"
#include "tyr/formalism/binary_operator_view.hpp"
#include "tyr/formalism/function_term_builder.hpp"

#include <optional>
#include <urd/elements/builder.hpp>
#include <variant>
#include <vector>

namespace urd
{

/// The builder of BinaryOperator: its record's members; each child built or interned (urd::child).
template<::tyr::TaskKind T, ::tyr::formalism::OperatorKind O, typename C>
struct builder<::tyr::formalism::BinaryOperator<T, O>, C>
{
    urd::index<::tyr::formalism::BinaryOperator<T, O>> index;
    O operation {};
    ::tyr::formalism::BuiltFunctionExpression<T, C> lhs;
    ::tyr::formalism::BuiltFunctionExpression<T, C> rhs;

    static constexpr auto fields() noexcept
    {
        return urd::Fields<urd::field<"operation", &builder::operation>, urd::field<"lhs", &builder::lhs>, urd::field<"rhs", &builder::rhs>> {};
    }

    static constexpr auto identifying_members() noexcept { return urd::Members<&builder::operation, &builder::lhs, &builder::rhs> {}; }
};

}  // namespace urd

#endif
