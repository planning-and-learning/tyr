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

#include <tyr/formalism/planning/unary_operator_data.hpp>
#include <tyr/formalism/planning/unary_operator_index.hpp>
#include <tyr/formalism/planning/unary_operator_view.hpp>

namespace tyr::formalism::planning
{

namespace
{
template<TaskKind T>
void bind_unary_operator_kind(nb::module_& m, RepositoryBinding& repository, const std::string& name)
{
    using Tag = UnaryOperator<T>;
    ygg::bind_index<ygg::Index<Tag>>(m, (name + "Index").c_str());

    {
        using V = ygg::Data<Tag>;
        auto cls = nb::class_<V>(m, (name + "Data").c_str()).def(nb::init<ArithmeticOperatorKind, FunctionExpressionView<T>>(), "operator"_a, "arg"_a);
        ygg::add_print(cls);
        ygg::add_comparison(cls);
        ygg::add_hash(cls);
    }

    {
        using V = UnaryOperatorView<T>;
        auto cls = nb::class_<V>(m, name.c_str()).def("get_index", &V::get_index).def("get_operator", &V::get_operator).def("get_arg", &V::get_arg);
        ygg::add_print(cls);
        ygg::add_comparison(cls);
        ygg::add_hash(cls);
    }

    repository.def("get_or_create", &get_or_create_data<Tag>, "data"_a, nb::keep_alive<0, 1>());
}
}  // namespace

void bind_unary_operator(nb::module_& m, RepositoryBinding& repository)
{
    bind_unary_operator_kind<LiftedTag>(m, repository, "UnaryOperator");
    bind_unary_operator_kind<GroundTag>(m, repository, "GroundUnaryOperator");
}

}  // namespace tyr::formalism::planning
