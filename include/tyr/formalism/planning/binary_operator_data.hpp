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

#ifndef TYR_FORMALISM_PLANNING_BINARY_OPERATOR_DATA_HPP_
#define TYR_FORMALISM_PLANNING_BINARY_OPERATOR_DATA_HPP_

#include "tyr/formalism/planning/binary_operator_index.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/function_expression_data.hpp"
#include "tyr/formalism/planning/ground_function_expression_data.hpp"

#include <yggdrasil/core/types.hpp>
#include <yggdrasil/core/types_utils.hpp>

namespace ygg
{
using namespace ::tyr;

template<::tyr::formalism::BinaryOperatorKind Operator, typename T>
struct Data<::tyr::formalism::planning::BinaryOperator<Operator, T>>
{
    using OperatorType = Operator;

    ygg::Index<::tyr::formalism::planning::BinaryOperator<Operator, T>> index;
    OperatorType operator_kind {};
    T lhs;
    T rhs;

    Data() = default;
    Data(OperatorType operator_kind_, T lhs_, T rhs_) : index(), operator_kind(operator_kind_), lhs(lhs_), rhs(rhs_) {}
    // Python constructor
    template<typename C>
    Data(OperatorType operator_kind_, ::ygg::View<T, C> lhs_, ::ygg::View<T, C> rhs_) : index(), operator_kind(operator_kind_), lhs(), rhs()
    {
        set(lhs_, lhs);
        set(rhs_, rhs);
    }
    Data(const Data& other) = default;
    Data& operator=(const Data& other) = default;
    Data(Data&& other) = default;
    Data& operator=(Data&& other) = default;

    void clear() noexcept
    {
        ygg::clear(index);
        operator_kind = {};
        ygg::clear(lhs);
        ygg::clear(rhs);
    }

    auto cista_members() const noexcept { return std::tie(index, operator_kind, lhs, rhs); }
    auto identifying_members() const noexcept { return std::tie(operator_kind, lhs, rhs); }
};

static_assert(!ygg::uses_trivial_storage_v<
              ::tyr::formalism::planning::BinaryOperator<::tyr::formalism::ArithmeticOperatorKind, ygg::Data<::tyr::formalism::planning::FunctionExpression>>>);

}

#endif
