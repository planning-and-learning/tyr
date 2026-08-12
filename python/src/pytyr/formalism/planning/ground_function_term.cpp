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

#include <tyr/formalism/planning/ground_function_term_data.hpp>
#include <tyr/formalism/planning/ground_function_term_index.hpp>
#include <tyr/formalism/planning/ground_function_term_view.hpp>

namespace tyr::formalism::planning
{

namespace
{
template<FactKind T>
void bind_ground_function_term_data(nb::module_& m, const char* name)
{
    using V = ygg::Data<GroundFunctionTerm<T>>;
    auto cls = nb::class_<V>(m, name).def(nb::init<FunctionBindingView<T>>(), "binding"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

template<FactKind T>
void bind_ground_function_term_view(nb::module_& m, const char* name)
{
    using V = GroundFunctionTermView<T>;
    auto cls = nb::class_<V>(m, name)
                   .def("get_index", &V::get_index)
                   .def("get_function", &V::get_function, nb::keep_alive<0, 1>())
                   .def("get_objects", &V::get_objects);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}
}  // namespace

void bind_ground_function_term(nb::module_& m, RepositoryBinding& repository)
{
    ygg::bind_index<ygg::Index<GroundFunctionTerm<StaticTag>>>(m, "StaticGroundFunctionTermIndex");
    ygg::bind_index<ygg::Index<GroundFunctionTerm<FluentTag>>>(m, "FluentGroundFunctionTermIndex");
    ygg::bind_index<ygg::Index<GroundFunctionTerm<AuxiliaryTag>>>(m, "AuxiliaryGroundFunctionTermIndex");

    bind_ground_function_term_data<StaticTag>(m, "StaticGroundFunctionTermData");
    bind_ground_function_term_data<FluentTag>(m, "FluentGroundFunctionTermData");
    bind_ground_function_term_data<AuxiliaryTag>(m, "AuxiliaryGroundFunctionTermData");

    bind_ground_function_term_view<StaticTag>(m, "StaticGroundFunctionTerm");
    bind_ground_function_term_view<FluentTag>(m, "FluentGroundFunctionTerm");
    bind_ground_function_term_view<AuxiliaryTag>(m, "AuxiliaryGroundFunctionTerm");

    repository.def("get_or_create", &get_or_create_data<GroundFunctionTerm<StaticTag>>, "data"_a, nb::keep_alive<0, 1>());
    repository.def("get_or_create", &get_or_create_data<GroundFunctionTerm<FluentTag>>, "data"_a, nb::keep_alive<0, 1>());
    repository.def("get_or_create", &get_or_create_data<GroundFunctionTerm<AuxiliaryTag>>, "data"_a, nb::keep_alive<0, 1>());
}

}  // namespace tyr::formalism::planning
