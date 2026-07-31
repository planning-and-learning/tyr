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

#include <tyr/formalism/planning/binary_operator_data.hpp>
#include <tyr/formalism/planning/binary_operator_index.hpp>
#include <tyr/formalism/planning/binary_operator_view.hpp>

namespace tyr::formalism::planning
{

void bind_binary_operator(nb::module_& m, RepositoryBinding& repository)
{
    using Arithmetic = ArithmeticOperatorKind;
    using Boolean = BooleanOperatorKind;
    using LiftedExpression = ygg::Data<FunctionExpression>;
    using GroundExpression = ygg::Data<GroundFunctionExpression>;

    ygg::bind_index<ygg::Index<BinaryOperator<Arithmetic, LiftedExpression>>>(m, "BinaryArithmeticOperatorIndex");
    ygg::bind_index<ygg::Index<BinaryOperator<Boolean, LiftedExpression>>>(m, "BinaryBooleanOperatorIndex");
    ygg::bind_index<ygg::Index<BinaryOperator<Arithmetic, GroundExpression>>>(m, "GroundBinaryArithmeticOperatorIndex");
    ygg::bind_index<ygg::Index<BinaryOperator<Boolean, GroundExpression>>>(m, "GroundBinaryBooleanOperatorIndex");

    {
        using ExpressionView = ygg::View<LiftedExpression, Repository>;
        using V = ygg::Data<BinaryOperator<Arithmetic, LiftedExpression>>;
        auto cls = nb::class_<V>(m, "BinaryArithmeticOperatorData").def(nb::init<Arithmetic, ExpressionView, ExpressionView>(), "operator"_a, "lhs"_a, "rhs"_a);
        ygg::add_print(cls);
        ygg::add_comparison(cls);
        ygg::add_hash(cls);
    }
    {
        using ExpressionView = ygg::View<LiftedExpression, Repository>;
        using V = ygg::Data<BinaryOperator<Boolean, LiftedExpression>>;
        auto cls = nb::class_<V>(m, "BinaryBooleanOperatorData").def(nb::init<Boolean, ExpressionView, ExpressionView>(), "operator"_a, "lhs"_a, "rhs"_a);
        ygg::add_print(cls);
        ygg::add_comparison(cls);
        ygg::add_hash(cls);
    }
    {
        using ExpressionView = ygg::View<GroundExpression, Repository>;
        using V = ygg::Data<BinaryOperator<Arithmetic, GroundExpression>>;
        auto cls =
            nb::class_<V>(m, "GroundBinaryArithmeticOperatorData").def(nb::init<Arithmetic, ExpressionView, ExpressionView>(), "operator"_a, "lhs"_a, "rhs"_a);
        ygg::add_print(cls);
        ygg::add_comparison(cls);
        ygg::add_hash(cls);
    }
    {
        using ExpressionView = ygg::View<GroundExpression, Repository>;
        using V = ygg::Data<BinaryOperator<Boolean, GroundExpression>>;
        auto cls = nb::class_<V>(m, "GroundBinaryBooleanOperatorData").def(nb::init<Boolean, ExpressionView, ExpressionView>(), "operator"_a, "lhs"_a, "rhs"_a);
        ygg::add_print(cls);
        ygg::add_comparison(cls);
        ygg::add_hash(cls);
    }
    {
        using V = BinaryOperatorView<Arithmetic, LiftedExpression>;
        auto cls = nb::class_<V>(m, "BinaryArithmeticOperator")
                       .def("get_index", &V::get_index)
                       .def("get_operator", &V::get_operator)
                       .def("get_lhs", &V::get_lhs)
                       .def("get_rhs", &V::get_rhs);
        ygg::add_print(cls);
        ygg::add_comparison(cls);
        ygg::add_hash(cls);
    }
    {
        using V = BinaryOperatorView<Boolean, LiftedExpression>;
        auto cls = nb::class_<V>(m, "BinaryBooleanOperator")
                       .def("get_index", &V::get_index)
                       .def("get_operator", &V::get_operator)
                       .def("get_lhs", &V::get_lhs)
                       .def("get_rhs", &V::get_rhs);
        ygg::add_print(cls);
        ygg::add_comparison(cls);
        ygg::add_hash(cls);
    }
    {
        using V = BinaryOperatorView<Arithmetic, GroundExpression>;
        auto cls = nb::class_<V>(m, "GroundBinaryArithmeticOperator")
                       .def("get_index", &V::get_index)
                       .def("get_operator", &V::get_operator)
                       .def("get_lhs", &V::get_lhs)
                       .def("get_rhs", &V::get_rhs);
        ygg::add_print(cls);
        ygg::add_comparison(cls);
        ygg::add_hash(cls);
    }
    {
        using V = BinaryOperatorView<Boolean, GroundExpression>;
        auto cls = nb::class_<V>(m, "GroundBinaryBooleanOperator")
                       .def("get_index", &V::get_index)
                       .def("get_operator", &V::get_operator)
                       .def("get_lhs", &V::get_lhs)
                       .def("get_rhs", &V::get_rhs);
        ygg::add_print(cls);
        ygg::add_comparison(cls);
        ygg::add_hash(cls);
    }

    repository.def("get_or_create", &get_or_create_data<BinaryOperator<Arithmetic, LiftedExpression>>, "data"_a, nb::keep_alive<0, 1>());
    repository.def("get_or_create", &get_or_create_data<BinaryOperator<Boolean, LiftedExpression>>, "data"_a, nb::keep_alive<0, 1>());
    repository.def("get_or_create", &get_or_create_data<BinaryOperator<Arithmetic, GroundExpression>>, "data"_a, nb::keep_alive<0, 1>());
    repository.def("get_or_create", &get_or_create_data<BinaryOperator<Boolean, GroundExpression>>, "data"_a, nb::keep_alive<0, 1>());
}

}  // namespace tyr::formalism::planning
