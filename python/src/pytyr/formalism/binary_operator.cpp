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
#include "datalog/binding_utils.hpp"
#include "planning/binding_utils.hpp"
#include "pytyr/module.hpp"

#include <tyr/formalism/binary_operator_data.hpp>
#include <tyr/formalism/binary_operator_view.hpp>

/// Binary operators: their indices and data are the same for datalog and planning, bound once here (and built from
/// either's views); their views are each repository's.
namespace tyr::formalism
{

namespace
{
template<TaskKind T, OperatorKind O>
void bind_binary_operator_data(nb::module_& m, const std::string& name)
{
    using namespace nb::literals;
    urd::python::bind_index<urd::index<BinaryOperator<T, O>>>(m, name + "Index");

    using V = urd::data<BinaryOperator<T, O>>;
    auto cls = nb::class_<V>(m, (name + "Data").c_str());
    cls.def(urd::python::init_from_views<O, datalog::FunctionExpressionView<T>, datalog::FunctionExpressionView<T>>(), "operator"_a, "lhs"_a, "rhs"_a);
    cls.def(urd::python::init_from_views<O, planning::FunctionExpressionView<T>, planning::FunctionExpressionView<T>>(), "operator"_a, "lhs"_a, "rhs"_a);
    cls.def(nb::init<O, FunctionExpression<T>, FunctionExpression<T>>(), "operator"_a, "lhs"_a, "rhs"_a);
    ygg::add_print(cls);
    urd::python::add_comparison(cls);
    urd::python::add_hash(cls);
}

template<TaskKind T, OperatorKind O, typename C>
void bind_binary_operator_view(nb::module_& m, nb::class_<C>& repository, const std::string& name)
{
    using V = urd::index_view<BinaryOperator<T, O>, C>;
    auto cls =
        nb::class_<V>(m, name.c_str()).def("index", &V::index).def("get_operator", &V::get_operator).def("get_lhs", &V::get_lhs).def("get_rhs", &V::get_rhs);
    ygg::add_print(cls);
    urd::python::add_comparison(cls);
    urd::python::add_hash(cls);

    ygg::bind_insert<BinaryOperator<T, O>>(repository);
}

template<typename C>
void bind_binary_operator_views(nb::module_& m, nb::class_<C>& repository)
{
    bind_binary_operator_view<LiftedTag, ArithmeticOperatorKind>(m, repository, "BinaryArithmeticOperator");
    bind_binary_operator_view<LiftedTag, BooleanOperatorKind>(m, repository, "BinaryBooleanOperator");
    bind_binary_operator_view<GroundTag, ArithmeticOperatorKind>(m, repository, "GroundBinaryArithmeticOperator");
    bind_binary_operator_view<GroundTag, BooleanOperatorKind>(m, repository, "GroundBinaryBooleanOperator");
}
}  // namespace

void bind_binary_operator(nb::module_& m)
{
    bind_binary_operator_data<LiftedTag, ArithmeticOperatorKind>(m, "BinaryArithmeticOperator");
    bind_binary_operator_data<LiftedTag, BooleanOperatorKind>(m, "BinaryBooleanOperator");
    bind_binary_operator_data<GroundTag, ArithmeticOperatorKind>(m, "GroundBinaryArithmeticOperator");
    bind_binary_operator_data<GroundTag, BooleanOperatorKind>(m, "GroundBinaryBooleanOperator");
}

}  // namespace tyr::formalism

namespace tyr::formalism::datalog
{

void bind_binary_operator(nb::module_& m, RepositoryBinding& repository) { bind_binary_operator_views(m, repository); }

}  // namespace tyr::formalism::datalog

namespace tyr::formalism::planning
{

void bind_binary_operator(nb::module_& m, RepositoryBinding& repository) { bind_binary_operator_views(m, repository); }

}  // namespace tyr::formalism::planning
