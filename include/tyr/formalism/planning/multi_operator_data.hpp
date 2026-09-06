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

#ifndef TYR_FORMALISM_PLANNING_MULTI_OPERATOR_DATA_HPP_
#define TYR_FORMALISM_PLANNING_MULTI_OPERATOR_DATA_HPP_

#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/function_expression_data.hpp"
#include "tyr/formalism/planning/multi_operator_index.hpp"

#include <stdexcept>
#include <yggdrasil/core/types.hpp>
#include <yggdrasil/core/types_utils.hpp>

namespace ygg
{

template<typename T>
struct Data<::tyr::formalism::planning::MultiOperator<T>>
{
    using OperatorType = ::tyr::formalism::ArithmeticOperatorKind;

    ygg::Index<::tyr::formalism::planning::MultiOperator<T>> index;
    OperatorType operator_kind = OperatorType::Add;
    ::cista::offset::vector<T> args;

    Data() = default;
    Data(OperatorType operator_kind_, ::cista::offset::vector<T> args_) : index(), operator_kind(operator_kind_), args(std::move(args_))
    {
        if (!is_multi(operator_kind))
            throw std::invalid_argument("multi operator must be Add or Mul");
    }
    // Python constructor
    template<typename C>
    Data(OperatorType operator_kind_, const std::vector<::ygg::View<T, C>>& args_) : index(), operator_kind(operator_kind_), args()
    {
        if (!is_multi(operator_kind))
            throw std::invalid_argument("multi operator must be Add or Mul");
        set(args_, args);
    }
    Data(const Data& other) = delete;
    Data& operator=(const Data& other) = delete;
    Data(Data&& other) = default;
    Data& operator=(Data&& other) = default;

    void clear() noexcept
    {
        ygg::clear(index);
        operator_kind = OperatorType::Add;
        ygg::clear(args);
    }

    auto cista_members() const noexcept { return std::tie(index, operator_kind, args); }
    auto identifying_members() const noexcept { return std::tie(operator_kind, args); }
};

static_assert(
    !ygg::uses_trivial_storage_v<::tyr::formalism::planning::MultiOperator<ygg::Data<::tyr::formalism::planning::FunctionExpression<::tyr::LiftedTag>>>>);

}

#endif
