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
    ::tyr::formalism::FunctionExpression<T> lhs;
    ::tyr::formalism::FunctionExpression<T> rhs;

    data() = default;

    data(O operation_, ::tyr::formalism::FunctionExpression<T> lhs_, ::tyr::formalism::FunctionExpression<T> rhs_) :
        index(),
        operation(operation_),
        lhs(lhs_),
        rhs(rhs_)
    {
    }

    data(const data& other) = delete;
    data& operator=(const data& other) = delete;
    data(data&& other) = default;
    data& operator=(data&& other) = default;
    ~data() = default;

    static constexpr auto fields() noexcept
    { return urd::Fields<urd::field<"operation", &data::operation>, urd::field<"lhs", &data::lhs>, urd::field<"rhs", &data::rhs>> {}; }

    static constexpr auto identifying_members() noexcept { return urd::Members<&data::operation, &data::lhs, &data::rhs> {}; }

    static constexpr auto cista_members() noexcept { return urd::Members<&data::index, &data::operation, &data::lhs, &data::rhs> {}; }

    [[nodiscard]] urd::key<::tyr::formalism::BinaryOperator<T, O>> key() const noexcept { return urd::key<::tyr::formalism::BinaryOperator<T, O>>(index); }
};

}

#endif
