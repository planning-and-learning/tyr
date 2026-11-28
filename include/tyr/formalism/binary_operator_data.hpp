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

#ifndef TYR_FORMALISM_BINARY_OPERATOR_DATA_HPP_
#define TYR_FORMALISM_BINARY_OPERATOR_DATA_HPP_

#include "tyr/declarations.hpp"
#include "tyr/formalism/declarations.hpp"

#include <urd/elements/concepts.hpp>
#include <urd/elements/data.hpp>
#include <urd/elements/index.hpp>

namespace urd
{

template<::tyr::TaskKind T, ::tyr::formalism::OperatorKind O>
struct data<::tyr::formalism::BinaryOperator<T, O>>
{
    urd::key<::tyr::formalism::BinaryOperator<T, O>> index;
    O operation {};
    ::tyr::formalism::FunctionExpression<T> left_operand;
    ::tyr::formalism::FunctionExpression<T> right_operand;

    data() = default;

    data(O operation_, ::tyr::formalism::FunctionExpression<T> left_operand_, ::tyr::formalism::FunctionExpression<T> right_operand_) :
        index(),
        operation(operation_),
        left_operand(left_operand_),
        right_operand(right_operand_)
    {
    }

    data(const data& other) = delete;
    data& operator=(const data& other) = delete;
    data(data&& other) = default;
    data& operator=(data&& other) = default;
    ~data() = default;

    static constexpr auto fields() noexcept
    {
        return urd::Fields<urd::field<"operation", &data::operation>,
                           urd::field<"left_operand", &data::left_operand>,
                           urd::field<"right_operand", &data::right_operand>> {};
    }

    static constexpr auto cista_members() noexcept { return urd::cista_members_of<data>(); }

    [[nodiscard]] urd::key<::tyr::formalism::BinaryOperator<T, O>> key() const noexcept { return urd::key<::tyr::formalism::BinaryOperator<T, O>>(index); }
};

}

#endif
