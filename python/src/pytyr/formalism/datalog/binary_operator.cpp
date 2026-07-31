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

#include <tyr/formalism/datalog/binary_operator_data.hpp>
#include <tyr/formalism/datalog/binary_operator_index.hpp>
#include <tyr/formalism/datalog/binary_operator_view.hpp>

namespace tyr::formalism::datalog
{

namespace
{
template<OpKind Op, typename Expression>
void bind_binary_operator_data(nb::module_& m, const char* name)
{
    using ExpressionData = ygg::Data<Expression>;
    using ExpressionView = ygg::View<ExpressionData, Repository>;
    using Tag = BinaryOperator<Op, ExpressionData>;

    using V = ygg::Data<Tag>;
    auto cls = nb::class_<V>(m, name).def(nb::init<ExpressionView, ExpressionView>(), "lhs"_a, "rhs"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

template<OpKind Op, typename Expression>
void bind_binary_operator_view(nb::module_& m, const char* name)
{
    using ExpressionData = ygg::Data<Expression>;

    using V = BinaryOperatorView<Op, ExpressionData>;
    auto cls = nb::class_<V>(m, name).def("get_index", &V::get_index).def("get_lhs", &V::get_lhs).def("get_rhs", &V::get_rhs);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}
}  // namespace

void bind_binary_operator(nb::module_& m, RepositoryBinding& repository)
{
    ygg::bind_index<ygg::Index<BinaryOperator<Add, ygg::Data<FunctionExpression>>>>(m, "BinaryOperatorAddIndex");
    ygg::bind_index<ygg::Index<BinaryOperator<Sub, ygg::Data<FunctionExpression>>>>(m, "BinaryOperatorSubIndex");
    ygg::bind_index<ygg::Index<BinaryOperator<Mul, ygg::Data<FunctionExpression>>>>(m, "BinaryOperatorMulIndex");
    ygg::bind_index<ygg::Index<BinaryOperator<Div, ygg::Data<FunctionExpression>>>>(m, "BinaryOperatorDivIndex");
    ygg::bind_index<ygg::Index<BinaryOperator<Eq, ygg::Data<FunctionExpression>>>>(m, "BinaryOperatorEqIndex");
    ygg::bind_index<ygg::Index<BinaryOperator<Ne, ygg::Data<FunctionExpression>>>>(m, "BinaryOperatorNeIndex");
    ygg::bind_index<ygg::Index<BinaryOperator<Le, ygg::Data<FunctionExpression>>>>(m, "BinaryOperatorLeIndex");
    ygg::bind_index<ygg::Index<BinaryOperator<Lt, ygg::Data<FunctionExpression>>>>(m, "BinaryOperatorLtIndex");
    ygg::bind_index<ygg::Index<BinaryOperator<Ge, ygg::Data<FunctionExpression>>>>(m, "BinaryOperatorGeIndex");
    ygg::bind_index<ygg::Index<BinaryOperator<Gt, ygg::Data<FunctionExpression>>>>(m, "BinaryOperatorGtIndex");
    ygg::bind_index<ygg::Index<BinaryOperator<Add, ygg::Data<GroundFunctionExpression>>>>(m, "GroundBinaryOperatorAddIndex");
    ygg::bind_index<ygg::Index<BinaryOperator<Sub, ygg::Data<GroundFunctionExpression>>>>(m, "GroundBinaryOperatorSubIndex");
    ygg::bind_index<ygg::Index<BinaryOperator<Mul, ygg::Data<GroundFunctionExpression>>>>(m, "GroundBinaryOperatorMulIndex");
    ygg::bind_index<ygg::Index<BinaryOperator<Div, ygg::Data<GroundFunctionExpression>>>>(m, "GroundBinaryOperatorDivIndex");
    ygg::bind_index<ygg::Index<BinaryOperator<Eq, ygg::Data<GroundFunctionExpression>>>>(m, "GroundBinaryOperatorEqIndex");
    ygg::bind_index<ygg::Index<BinaryOperator<Ne, ygg::Data<GroundFunctionExpression>>>>(m, "GroundBinaryOperatorNeIndex");
    ygg::bind_index<ygg::Index<BinaryOperator<Le, ygg::Data<GroundFunctionExpression>>>>(m, "GroundBinaryOperatorLeIndex");
    ygg::bind_index<ygg::Index<BinaryOperator<Lt, ygg::Data<GroundFunctionExpression>>>>(m, "GroundBinaryOperatorLtIndex");
    ygg::bind_index<ygg::Index<BinaryOperator<Ge, ygg::Data<GroundFunctionExpression>>>>(m, "GroundBinaryOperatorGeIndex");
    ygg::bind_index<ygg::Index<BinaryOperator<Gt, ygg::Data<GroundFunctionExpression>>>>(m, "GroundBinaryOperatorGtIndex");

    bind_binary_operator_data<Add, FunctionExpression>(m, "BinaryOperatorAddData");
    bind_binary_operator_data<Sub, FunctionExpression>(m, "BinaryOperatorSubData");
    bind_binary_operator_data<Mul, FunctionExpression>(m, "BinaryOperatorMulData");
    bind_binary_operator_data<Div, FunctionExpression>(m, "BinaryOperatorDivData");
    bind_binary_operator_data<Eq, FunctionExpression>(m, "BinaryOperatorEqData");
    bind_binary_operator_data<Ne, FunctionExpression>(m, "BinaryOperatorNeData");
    bind_binary_operator_data<Le, FunctionExpression>(m, "BinaryOperatorLeData");
    bind_binary_operator_data<Lt, FunctionExpression>(m, "BinaryOperatorLtData");
    bind_binary_operator_data<Ge, FunctionExpression>(m, "BinaryOperatorGeData");
    bind_binary_operator_data<Gt, FunctionExpression>(m, "BinaryOperatorGtData");
    bind_binary_operator_data<Add, GroundFunctionExpression>(m, "GroundBinaryOperatorAddData");
    bind_binary_operator_data<Sub, GroundFunctionExpression>(m, "GroundBinaryOperatorSubData");
    bind_binary_operator_data<Mul, GroundFunctionExpression>(m, "GroundBinaryOperatorMulData");
    bind_binary_operator_data<Div, GroundFunctionExpression>(m, "GroundBinaryOperatorDivData");
    bind_binary_operator_data<Eq, GroundFunctionExpression>(m, "GroundBinaryOperatorEqData");
    bind_binary_operator_data<Ne, GroundFunctionExpression>(m, "GroundBinaryOperatorNeData");
    bind_binary_operator_data<Le, GroundFunctionExpression>(m, "GroundBinaryOperatorLeData");
    bind_binary_operator_data<Lt, GroundFunctionExpression>(m, "GroundBinaryOperatorLtData");
    bind_binary_operator_data<Ge, GroundFunctionExpression>(m, "GroundBinaryOperatorGeData");
    bind_binary_operator_data<Gt, GroundFunctionExpression>(m, "GroundBinaryOperatorGtData");

    bind_binary_operator_view<Add, FunctionExpression>(m, "BinaryOperatorAdd");
    bind_binary_operator_view<Sub, FunctionExpression>(m, "BinaryOperatorSub");
    bind_binary_operator_view<Mul, FunctionExpression>(m, "BinaryOperatorMul");
    bind_binary_operator_view<Div, FunctionExpression>(m, "BinaryOperatorDiv");
    bind_binary_operator_view<Eq, FunctionExpression>(m, "BinaryOperatorEq");
    bind_binary_operator_view<Ne, FunctionExpression>(m, "BinaryOperatorNe");
    bind_binary_operator_view<Le, FunctionExpression>(m, "BinaryOperatorLe");
    bind_binary_operator_view<Lt, FunctionExpression>(m, "BinaryOperatorLt");
    bind_binary_operator_view<Ge, FunctionExpression>(m, "BinaryOperatorGe");
    bind_binary_operator_view<Gt, FunctionExpression>(m, "BinaryOperatorGt");
    bind_binary_operator_view<Add, GroundFunctionExpression>(m, "GroundBinaryOperatorAdd");
    bind_binary_operator_view<Sub, GroundFunctionExpression>(m, "GroundBinaryOperatorSub");
    bind_binary_operator_view<Mul, GroundFunctionExpression>(m, "GroundBinaryOperatorMul");
    bind_binary_operator_view<Div, GroundFunctionExpression>(m, "GroundBinaryOperatorDiv");
    bind_binary_operator_view<Eq, GroundFunctionExpression>(m, "GroundBinaryOperatorEq");
    bind_binary_operator_view<Ne, GroundFunctionExpression>(m, "GroundBinaryOperatorNe");
    bind_binary_operator_view<Le, GroundFunctionExpression>(m, "GroundBinaryOperatorLe");
    bind_binary_operator_view<Lt, GroundFunctionExpression>(m, "GroundBinaryOperatorLt");
    bind_binary_operator_view<Ge, GroundFunctionExpression>(m, "GroundBinaryOperatorGe");
    bind_binary_operator_view<Gt, GroundFunctionExpression>(m, "GroundBinaryOperatorGt");

    repository.def("get_or_create", &get_or_create_data<BinaryOperator<Add, ygg::Data<FunctionExpression>>>, "data"_a, nb::keep_alive<0, 1>());
    repository.def("get_or_create", &get_or_create_data<BinaryOperator<Sub, ygg::Data<FunctionExpression>>>, "data"_a, nb::keep_alive<0, 1>());
    repository.def("get_or_create", &get_or_create_data<BinaryOperator<Mul, ygg::Data<FunctionExpression>>>, "data"_a, nb::keep_alive<0, 1>());
    repository.def("get_or_create", &get_or_create_data<BinaryOperator<Div, ygg::Data<FunctionExpression>>>, "data"_a, nb::keep_alive<0, 1>());
    repository.def("get_or_create", &get_or_create_data<BinaryOperator<Eq, ygg::Data<FunctionExpression>>>, "data"_a, nb::keep_alive<0, 1>());
    repository.def("get_or_create", &get_or_create_data<BinaryOperator<Ne, ygg::Data<FunctionExpression>>>, "data"_a, nb::keep_alive<0, 1>());
    repository.def("get_or_create", &get_or_create_data<BinaryOperator<Le, ygg::Data<FunctionExpression>>>, "data"_a, nb::keep_alive<0, 1>());
    repository.def("get_or_create", &get_or_create_data<BinaryOperator<Lt, ygg::Data<FunctionExpression>>>, "data"_a, nb::keep_alive<0, 1>());
    repository.def("get_or_create", &get_or_create_data<BinaryOperator<Ge, ygg::Data<FunctionExpression>>>, "data"_a, nb::keep_alive<0, 1>());
    repository.def("get_or_create", &get_or_create_data<BinaryOperator<Gt, ygg::Data<FunctionExpression>>>, "data"_a, nb::keep_alive<0, 1>());
    repository.def("get_or_create", &get_or_create_data<BinaryOperator<Add, ygg::Data<GroundFunctionExpression>>>, "data"_a, nb::keep_alive<0, 1>());
    repository.def("get_or_create", &get_or_create_data<BinaryOperator<Sub, ygg::Data<GroundFunctionExpression>>>, "data"_a, nb::keep_alive<0, 1>());
    repository.def("get_or_create", &get_or_create_data<BinaryOperator<Mul, ygg::Data<GroundFunctionExpression>>>, "data"_a, nb::keep_alive<0, 1>());
    repository.def("get_or_create", &get_or_create_data<BinaryOperator<Div, ygg::Data<GroundFunctionExpression>>>, "data"_a, nb::keep_alive<0, 1>());
    repository.def("get_or_create", &get_or_create_data<BinaryOperator<Eq, ygg::Data<GroundFunctionExpression>>>, "data"_a, nb::keep_alive<0, 1>());
    repository.def("get_or_create", &get_or_create_data<BinaryOperator<Ne, ygg::Data<GroundFunctionExpression>>>, "data"_a, nb::keep_alive<0, 1>());
    repository.def("get_or_create", &get_or_create_data<BinaryOperator<Le, ygg::Data<GroundFunctionExpression>>>, "data"_a, nb::keep_alive<0, 1>());
    repository.def("get_or_create", &get_or_create_data<BinaryOperator<Lt, ygg::Data<GroundFunctionExpression>>>, "data"_a, nb::keep_alive<0, 1>());
    repository.def("get_or_create", &get_or_create_data<BinaryOperator<Ge, ygg::Data<GroundFunctionExpression>>>, "data"_a, nb::keep_alive<0, 1>());
    repository.def("get_or_create", &get_or_create_data<BinaryOperator<Gt, ygg::Data<GroundFunctionExpression>>>, "data"_a, nb::keep_alive<0, 1>());
}

}  // namespace tyr::formalism::datalog
