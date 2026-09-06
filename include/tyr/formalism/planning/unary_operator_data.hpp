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
#include "tyr/formalism/planning/function_expression_data.hpp"
#include "tyr/formalism/planning/unary_operator_index.hpp"

#include <stdexcept>
#include <yggdrasil/core/types.hpp>
#include <yggdrasil/core/types_utils.hpp>

namespace ygg
{

template<typename T>
struct Data<::tyr::formalism::planning::UnaryOperator<T>>
{
    using OperatorType = ::tyr::formalism::ArithmeticOperatorKind;

    ygg::Index<::tyr::formalism::planning::UnaryOperator<T>> index;
    OperatorType operator_kind = OperatorType::Sub;
    T arg;

    Data() = default;
    Data(OperatorType operator_kind_, T arg_) : index(), operator_kind(operator_kind_), arg(arg_)
    {
        if (!is_unary(operator_kind))
            throw std::invalid_argument("unary operator must be Sub");
    }
    // Python constructor
    template<typename C>
    Data(OperatorType operator_kind_, ::ygg::View<T, C> arg_) : index(), operator_kind(operator_kind_), arg()
    {
        if (!is_unary(operator_kind))
            throw std::invalid_argument("unary operator must be Sub");
        set(arg_, arg);
    }
    Data(const Data& other) = default;
    Data& operator=(const Data& other) = default;
    Data(Data&& other) = default;
    Data& operator=(Data&& other) = default;

    void clear() noexcept
    {
        ygg::clear(index);
        operator_kind = OperatorType::Sub;
        ygg::clear(arg);
    }

    auto cista_members() const noexcept { return std::tie(index, operator_kind, arg); }
    auto identifying_members() const noexcept { return std::tie(operator_kind, arg); }
};

static_assert(
    !ygg::uses_trivial_storage_v<::tyr::formalism::planning::UnaryOperator<ygg::Data<::tyr::formalism::planning::FunctionExpression<::tyr::LiftedTag>>>>);

}

#endif
