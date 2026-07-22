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
#include "tyr/formalism/planning/ground_function_expression_data.hpp"
#include "tyr/formalism/planning/multi_operator_index.hpp"

#include <yggdrasil/core/types.hpp>
#include <yggdrasil/core/types_utils.hpp>

namespace ygg
{
using namespace ::tyr;

template<::tyr::formalism::OpKind Op, typename T>
struct Data<::tyr::formalism::planning::MultiOperator<Op, T>>
{
    using OpType = Op;

    ygg::Index<::tyr::formalism::planning::MultiOperator<Op, T>> index;
    ::cista::offset::vector<T> args;

    Data() = default;
    Data(::cista::offset::vector<T> args_) : index(), args(std::move(args_)) {}
    // Python constructor
    template<typename C>
    Data(const std::vector<::ygg::View<T, C>>& args_) : index(), args()
    {
        set(args_, args);
    }
    Data(const Data& other) = delete;
    Data& operator=(const Data& other) = delete;
    Data(Data&& other) = default;
    Data& operator=(Data&& other) = default;

    void clear() noexcept
    {
        ygg::clear(index);
        ygg::clear(args);
    }

    auto cista_members() const noexcept { return std::tie(index, args); }
    auto identifying_members() const noexcept { return std::tie(Op::kind, args); }
};

static_assert(
    !ygg::uses_trivial_storage_v<::tyr::formalism::planning::MultiOperator<::tyr::formalism::Add, ygg::Data<::tyr::formalism::planning::FunctionExpression>>>);

}

#endif
