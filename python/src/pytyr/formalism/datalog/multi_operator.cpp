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

#include "binding_utils.hpp"
#include "bindings.hpp"

#include <tyr/formalism/datalog/multi_operator_data.hpp>
#include <tyr/formalism/datalog/multi_operator_index.hpp>
#include <tyr/formalism/datalog/multi_operator_view.hpp>

namespace tyr::formalism::datalog
{

namespace
{
template<OpKind Op, typename Expression>
void bind_multi_operator_data(nb::module_& m, const char* name)
{
    using ExpressionData = ygg::Data<Expression>;
    using ExpressionView = ygg::View<ExpressionData, Repository>;
    using Tag = MultiOperator<Op, ExpressionData>;

    using V = ygg::Data<Tag>;
    auto cls = nb::class_<V>(m, name).def(nb::init<const std::vector<ExpressionView>&>(), "args"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

template<OpKind Op, typename Expression>
void bind_multi_operator_view(nb::module_& m, const char* name)
{
    using ExpressionData = ygg::Data<Expression>;

    using V = MultiOperatorView<Op, ExpressionData>;
    auto cls = nb::class_<V>(m, name).def("get_index", &V::get_index).def("get_args", &V::get_args);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}
}  // namespace

void bind_multi_operator(nb::module_& m, RepositoryBinding& repository)
{
    ygg::bind_index<ygg::Index<MultiOperator<Add, ygg::Data<FunctionExpression>>>>(m, "MultiOperatorAddIndex");
    ygg::bind_index<ygg::Index<MultiOperator<Mul, ygg::Data<FunctionExpression>>>>(m, "MultiOperatorMulIndex");
    ygg::bind_index<ygg::Index<MultiOperator<Add, ygg::Data<GroundFunctionExpression>>>>(m, "GroundMultiOperatorAddIndex");
    ygg::bind_index<ygg::Index<MultiOperator<Mul, ygg::Data<GroundFunctionExpression>>>>(m, "GroundMultiOperatorMulIndex");

    bind_multi_operator_data<Add, FunctionExpression>(m, "MultiOperatorAddData");
    bind_multi_operator_data<Mul, FunctionExpression>(m, "MultiOperatorMulData");
    bind_multi_operator_data<Add, GroundFunctionExpression>(m, "GroundMultiOperatorAddData");
    bind_multi_operator_data<Mul, GroundFunctionExpression>(m, "GroundMultiOperatorMulData");

    bind_multi_operator_view<Add, FunctionExpression>(m, "MultiOperatorAdd");
    bind_multi_operator_view<Mul, FunctionExpression>(m, "MultiOperatorMul");
    bind_multi_operator_view<Add, GroundFunctionExpression>(m, "GroundMultiOperatorAdd");
    bind_multi_operator_view<Mul, GroundFunctionExpression>(m, "GroundMultiOperatorMul");

    repository.def("get_or_create", &get_or_create_data<MultiOperator<Add, ygg::Data<FunctionExpression>>>, "data"_a, nb::keep_alive<0, 1>());
    repository.def("get_or_create", &get_or_create_data<MultiOperator<Mul, ygg::Data<FunctionExpression>>>, "data"_a, nb::keep_alive<0, 1>());
    repository.def("get_or_create", &get_or_create_data<MultiOperator<Add, ygg::Data<GroundFunctionExpression>>>, "data"_a, nb::keep_alive<0, 1>());
    repository.def("get_or_create", &get_or_create_data<MultiOperator<Mul, ygg::Data<GroundFunctionExpression>>>, "data"_a, nb::keep_alive<0, 1>());
}

}  // namespace tyr::formalism::datalog
