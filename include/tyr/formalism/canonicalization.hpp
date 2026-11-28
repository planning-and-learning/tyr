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

#ifndef TYR_FORMALISM_CANONICALIZATION_HPP_
#define TYR_FORMALISM_CANONICALIZATION_HPP_

#include "tyr/declarations.hpp"
#include "tyr/formalism/binary_operator_data.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/enums.hpp"

#include <concepts>
#include <urd/elements/data.hpp>
#include <utility>

namespace tyr::formalism
{

/// The canonical form of a record that is more than its fields promise (found by ADL from prepare_for_insert; a list a
/// record holds as a set says so on its field, urd::sorted and urd::unique, and yggdrasil sorts it): a commutative
/// binary operator's operands in order.
template<TaskKind T, OperatorKind O>
void canonicalize(urd::data<BinaryOperator<T, O>>& data)
{
    if constexpr (std::same_as<O, ArithmeticOperatorKind>)
    {
        if ((data.operation == ArithmeticOperatorKind::Add || data.operation == ArithmeticOperatorKind::Mul) && data.left_operand > data.right_operand)
            std::swap(data.left_operand, data.right_operand);
    }
}

}

#endif
