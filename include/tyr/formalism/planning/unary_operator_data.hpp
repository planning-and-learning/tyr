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

#ifndef TYR_FORMALISM_PLANNING_UNARY_OPERATOR_DATA_HPP_
#define TYR_FORMALISM_PLANNING_UNARY_OPERATOR_DATA_HPP_

#include "tyr/formalism/planning/declarations.hpp"

#include <stdexcept>
#include <tuple>
#include <utility>
#include <urd/elements/flat.hpp>

namespace urd
{

template<::tyr::TaskKind T, ::tyr::formalism::OperatorKind O>
struct data<::tyr::formalism::planning::UnaryOperator<T, O>>
{
    static_assert(std::same_as<O, ::tyr::formalism::ArithmeticOperatorKind>);
    using Operand = ::tyr::formalism::planning::FunctionExpression<T>;
    using OperatorType = O;

    urd::index<::tyr::formalism::planning::UnaryOperator<T, O>> index;
    OperatorType operator_kind = OperatorType::Sub;
    Operand arg;

    data() = default;
    data(OperatorType operator_kind_, Operand arg_) : index(), operator_kind(operator_kind_), arg(arg_)
    {
        if (!is_unary(operator_kind))
            throw std::invalid_argument("unary operator must be Sub");
    }

    static constexpr auto cista_members() noexcept { return urd::Members<&data::index, &data::operator_kind, &data::arg> {}; }
    static constexpr auto identifying_members() noexcept { return urd::Members<&data::operator_kind, &data::arg> {}; }
};

}

#endif
