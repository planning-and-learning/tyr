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

#include "tyr/formalism/declarations.hpp"

#include <stdexcept>
#include <tuple>
#include <urd/elements/flat.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <utility>
#include <vector>

namespace urd
{

template<::tyr::TaskKind T, ::tyr::formalism::ArithmeticOperatorKindOnly O>
struct data<::tyr::formalism::MultiOperator<T, O>>
{
    urd::index<::tyr::formalism::MultiOperator<T, O>> index;
    O operation = O::Add;
    urd::flat_vector<::tyr::formalism::FunctionExpression<T>> args;

    data() = default;

    data(O operation_, urd::flat_vector<::tyr::formalism::FunctionExpression<T>> args_) : index(), operation(operation_), args(std::move(args_))
    {
        if (!is_multi(operation))
            throw std::invalid_argument("multi operator must be Add or Mul");
    }

    data(const data& other) = delete;
    data& operator=(const data& other) = delete;
    data(data&& other) = default;
    data& operator=(data&& other) = default;
    ~data() = default;

    static constexpr auto fields() noexcept { return urd::Fields<urd::field<"operation", &data::operation>, urd::field<"args", &data::args>> {}; }

    static constexpr auto identifying_members() noexcept { return urd::Members<&data::operation, &data::args> {}; }

    static constexpr auto cista_members() noexcept { return urd::Members<&data::index, &data::operation, &data::args> {}; }
};

}

#endif
