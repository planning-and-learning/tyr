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

#ifndef TYR_FORMALISM_UNARY_OPERATOR_DATA_HPP_
#define TYR_FORMALISM_UNARY_OPERATOR_DATA_HPP_

#include "tyr/formalism/declarations.hpp"

#include <stdexcept>
#include <tuple>
#include <urd/elements/flat.hpp>
#include <utility>

namespace urd
{

template<::tyr::TaskKind T, ::tyr::formalism::ArithmeticOperatorKindOnly O>
struct data<::tyr::formalism::UnaryOperator<T, O>>
{
    urd::index<::tyr::formalism::UnaryOperator<T, O>> index;
    O operation = O::Sub;
    ::tyr::formalism::FunctionExpression<T> arg;

    data() = default;
    data(O operation_, ::tyr::formalism::FunctionExpression<T> arg_) : index(), operation(operation_), arg(arg_)
    {
        if (!is_unary(operation))
            throw std::invalid_argument("unary operator must be Sub");
    }
    data(const data& other) = delete;
    data& operator=(const data& other) = delete;
    data(data&& other) = default;
    data& operator=(data&& other) = default;
    ~data() = default;

    static constexpr auto fields() noexcept { return urd::Fields<urd::field<"operation", &data::operation>, urd::field<"arg", &data::arg>> {}; }
    static constexpr auto identifying_members() noexcept { return urd::Members<&data::operation, &data::arg> {}; }
    static constexpr auto cista_members() noexcept { return urd::Members<&data::index, &data::operation, &data::arg> {}; }
};

}

#endif
