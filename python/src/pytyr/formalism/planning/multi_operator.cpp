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
#include "pytyr/module.hpp"
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <urd/python/sequences/sequence.hpp>
#include "bindings.hpp"
#include "tyr/formalism/planning/declarations.hpp"

#include <tyr/formalism/planning/multi_operator_data.hpp>
#include <tyr/formalism/planning/multi_operator_view.hpp>

namespace tyr::formalism::planning
{

namespace
{
template<TaskKind T>
void bind_multi_operator_kind(nb::module_& m, RepositoryBinding& repository, const std::string& name)
{
    using namespace nb::literals;
    using Tag = MultiOperator<T, ArithmeticOperatorKind>;
    urd::python::bind_index<urd::index<Tag>>(m, name + "Index");

    {
        using V = urd::data<Tag>;
        auto cls = nb::class_<V>(m, (name + "Data").c_str())
                       .def(urd::python::init_from_views<ArithmeticOperatorKind, const urd::Store<urd::sequence<FunctionExpressionView<T>>>&>(), "operator"_a, "args"_a);
        cls.def(nb::init<ArithmeticOperatorKind, FunctionExpressionList<T>>(), "operator"_a, "args"_a);
        ygg::add_print(cls);
        urd::python::add_comparison(cls);
        urd::python::add_hash(cls);
    }

    {
        using V = MultiOperatorView<T, ArithmeticOperatorKind>;
        auto cls = nb::class_<V>(m, name.c_str()).def("get_index", &V::index).def("get_operator", &V::get_operator).def("get_args", &V::get_args);
        ygg::add_print(cls);
        urd::python::add_comparison(cls);
        urd::python::add_hash(cls);
    }

    ygg::bind_insert<Tag>(repository);
}
}  // namespace

void bind_multi_operator(nb::module_& m, RepositoryBinding& repository)
{
    bind_multi_operator_kind<LiftedTag>(m, repository, "MultiOperator");
    bind_multi_operator_kind<GroundTag>(m, repository, "GroundMultiOperator");
}

}  // namespace tyr::formalism::planning
