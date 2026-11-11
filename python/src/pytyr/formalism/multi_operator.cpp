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

#include <tyr/formalism/multi_operator_data.hpp>
#include <tyr/formalism/multi_operator_view.hpp>

/// Multi operators: their indices and data are the same for datalog and planning, bound once here (and built from
/// either's views); their views are each repository's.
namespace tyr::formalism
{

namespace
{
template<TaskKind T>
void bind_multi_operator_data(nb::module_& m, const std::string& name)
{
    using namespace nb::literals;
    urd::python::bind_index<urd::index<MultiOperator<T, ArithmeticOperatorKind>>>(m, name + "Index");

    using V = urd::data<MultiOperator<T, ArithmeticOperatorKind>>;
    auto cls = nb::class_<V>(m, (name + "Data").c_str());
    cls.def(urd::python::init_from_views<ArithmeticOperatorKind, const urd::Store<urd::sequence<datalog::FunctionExpressionView<T>>>&>(),
            "operator"_a,
            "args"_a);
    cls.def(urd::python::init_from_views<ArithmeticOperatorKind, const urd::Store<urd::sequence<planning::FunctionExpressionView<T>>>&>(),
            "operator"_a,
            "args"_a);
    cls.def(nb::init<ArithmeticOperatorKind, FunctionExpressionList<T>>(), "operator"_a, "args"_a);
    ygg::add_print(cls);
    urd::python::add_comparison(cls);
    urd::python::add_hash(cls);
}

template<TaskKind T, typename C>
void bind_multi_operator_view(nb::module_& m, nb::class_<C>& repository, const std::string& name)
{
    using V = urd::index_view<MultiOperator<T, ArithmeticOperatorKind>, C>;
    auto cls = nb::class_<V>(m, name.c_str()).def("index", &V::index).def("get_operator", &V::get_operator).def("get_args", &V::get_args);
    ygg::add_print(cls);
    urd::python::add_comparison(cls);
    urd::python::add_hash(cls);

    ygg::bind_insert<MultiOperator<T, ArithmeticOperatorKind>>(repository);
}

template<typename C>
void bind_multi_operator_views(nb::module_& m, nb::class_<C>& repository)
{
    bind_multi_operator_view<LiftedTag>(m, repository, "MultiOperator");
    bind_multi_operator_view<GroundTag>(m, repository, "GroundMultiOperator");
}
}  // namespace

void bind_multi_operator(nb::module_& m)
{
    bind_multi_operator_data<LiftedTag>(m, "MultiOperator");
    bind_multi_operator_data<GroundTag>(m, "GroundMultiOperator");
}

}  // namespace tyr::formalism

namespace tyr::formalism::datalog
{

void bind_multi_operator(nb::module_& m, RepositoryBinding& repository) { bind_multi_operator_views(m, repository); }

}  // namespace tyr::formalism::datalog

namespace tyr::formalism::planning
{

void bind_multi_operator(nb::module_& m, RepositoryBinding& repository) { bind_multi_operator_views(m, repository); }

}  // namespace tyr::formalism::planning
