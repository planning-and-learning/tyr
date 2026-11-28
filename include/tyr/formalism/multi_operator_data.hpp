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

#ifndef TYR_FORMALISM_MULTI_OPERATOR_DATA_HPP_
#define TYR_FORMALISM_MULTI_OPERATOR_DATA_HPP_

#include "tyr/declarations.hpp"
#include "tyr/formalism/declarations.hpp"

#include <stdexcept>
#include <urd/elements/concepts.hpp>
#include <urd/elements/data.hpp>
#include <urd/elements/flat.hpp>
#include <urd/elements/index.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/tags.hpp>
#include <utility>

namespace urd
{

template<::tyr::TaskKind T, ::tyr::formalism::ArithmeticOperatorKindOnly O>
struct data<::tyr::formalism::MultiOperator<T, O>>
{
    urd::key<::tyr::formalism::MultiOperator<T, O>> index;
    O operation = O::Add;
    urd::flat_vector<::tyr::formalism::FunctionExpression<T>> operands;

    data() = default;

    data(O operation_, urd::flat_vector<::tyr::formalism::FunctionExpression<T>> operands_) : index(), operation(operation_), operands(std::move(operands_))
    {
        if (!is_multi(operation))
            throw std::invalid_argument("multi operator must be Add or Mul");
    }

    data(const data& other) = delete;
    data& operator=(const data& other) = delete;
    data(data&& other) = default;
    data& operator=(data&& other) = default;
    ~data() = default;

    static constexpr auto fields() noexcept
    { return urd::Fields<urd::field<"operation", &data::operation>, urd::field<"operands", &data::operands, urd::sorted>> {}; }

    static constexpr auto cista_members() noexcept { return urd::cista_members_of<data>(); }

    [[nodiscard]] urd::key<::tyr::formalism::MultiOperator<T, O>> key() const noexcept { return urd::key<::tyr::formalism::MultiOperator<T, O>>(index); }
};

}

#endif
