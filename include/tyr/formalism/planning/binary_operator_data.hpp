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

#include <yggdrasil/core/types.hpp>
#include <yggdrasil/core/types_utils.hpp>

namespace ygg
{

template<::tyr::TaskKind T, ::tyr::formalism::BinaryOperatorKind O>
struct Data<::tyr::formalism::planning::BinaryOperator<T, O>>
{
    using Operand = ygg::Data<::tyr::formalism::planning::FunctionExpression<T>>;
    using OperatorType = O;

    ygg::Index<::tyr::formalism::planning::BinaryOperator<T, O>> index;
    OperatorType operator_kind {};
    Operand lhs;
    Operand rhs;

    Data() = default;
    Data(OperatorType operator_kind_, Operand lhs_, Operand rhs_) : index(), operator_kind(operator_kind_), lhs(lhs_), rhs(rhs_) {}
    // Python constructor
    template<typename C>
    Data(OperatorType operator_kind_, ::ygg::View<Operand, C> lhs_, ::ygg::View<Operand, C> rhs_) : index(), operator_kind(operator_kind_), lhs(), rhs()
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

static_assert(!ygg::uses_trivial_storage_v<::tyr::formalism::planning::BinaryOperator<::tyr::LiftedTag, ::tyr::formalism::ArithmeticOperatorKind>>);

}

#endif
