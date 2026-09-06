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

void bind_multi_operator(nb::module_& m, RepositoryBinding& repository)
{
    ygg::bind_index<ygg::Index<MultiOperator<ygg::Data<FunctionExpression<::tyr::LiftedTag>>>>>(m, "MultiOperatorIndex");
    ygg::bind_index<ygg::Index<MultiOperator<ygg::Data<FunctionExpression<::tyr::GroundTag>>>>>(m, "GroundMultiOperatorIndex");

    {
        using ExpressionData = ygg::Data<FunctionExpression<::tyr::LiftedTag>>;
        using ExpressionView = ygg::View<ExpressionData, Repository>;
        using V = ygg::Data<MultiOperator<ExpressionData>>;
        auto cls = nb::class_<V>(m, "MultiOperatorData").def(nb::init<ArithmeticOperatorKind, const std::vector<ExpressionView>&>(), "operator"_a, "args"_a);
        ygg::add_print(cls);
        ygg::add_comparison(cls);
        ygg::add_hash(cls);
    }
    {
        using ExpressionData = ygg::Data<FunctionExpression<::tyr::GroundTag>>;
        using ExpressionView = ygg::View<ExpressionData, Repository>;
        using V = ygg::Data<MultiOperator<ExpressionData>>;
        auto cls =
            nb::class_<V>(m, "GroundMultiOperatorData").def(nb::init<ArithmeticOperatorKind, const std::vector<ExpressionView>&>(), "operator"_a, "args"_a);
        ygg::add_print(cls);
        ygg::add_comparison(cls);
        ygg::add_hash(cls);
    }
    {
        using V = MultiOperatorView<ygg::Data<FunctionExpression<::tyr::LiftedTag>>>;
        auto cls = nb::class_<V>(m, "MultiOperator").def("get_index", &V::get_index).def("get_operator", &V::get_operator).def("get_args", &V::get_args);
        ygg::add_print(cls);
        ygg::add_comparison(cls);
        ygg::add_hash(cls);
    }
    {
        using V = MultiOperatorView<ygg::Data<FunctionExpression<::tyr::GroundTag>>>;
        auto cls = nb::class_<V>(m, "GroundMultiOperator").def("get_index", &V::get_index).def("get_operator", &V::get_operator).def("get_args", &V::get_args);
        ygg::add_print(cls);
        ygg::add_comparison(cls);
        ygg::add_hash(cls);
    }

    repository.def("get_or_create", &get_or_create_data<MultiOperator<ygg::Data<FunctionExpression<::tyr::LiftedTag>>>>, "data"_a, nb::keep_alive<0, 1>());
    repository.def("get_or_create", &get_or_create_data<MultiOperator<ygg::Data<FunctionExpression<::tyr::GroundTag>>>>, "data"_a, nb::keep_alive<0, 1>());
}

}  // namespace tyr::formalism::datalog
