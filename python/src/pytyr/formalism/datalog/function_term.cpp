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

#include <tyr/formalism/datalog/function_term_data.hpp>
#include <tyr/formalism/datalog/function_term_index.hpp>
#include <tyr/formalism/datalog/function_term_view.hpp>

namespace tyr::formalism::datalog
{

namespace
{
template<FactKind T>
void bind_function_term_data(nb::module_& m, const char* name)
{
    using V = ygg::Data<FunctionTerm<T>>;
    auto cls = nb::class_<V>(m, name).def(nb::init<FunctionView<T>, const TermViewList&>(), "function"_a, "terms"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

template<FactKind T>
void bind_function_term_view(nb::module_& m, const char* name)
{
    using V = FunctionTermView<T>;
    auto cls =
        nb::class_<V>(m, name).def("get_index", &V::get_index).def("get_function", &V::get_function, nb::keep_alive<0, 1>()).def("get_terms", &V::get_terms);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}
}  // namespace

void bind_function_term(nb::module_& m, RepositoryBinding& repository)
{
    ygg::bind_index<ygg::Index<FunctionTerm<StaticTag>>>(m, "StaticFunctionTermIndex");
    ygg::bind_index<ygg::Index<FunctionTerm<FluentTag>>>(m, "FluentFunctionTermIndex");
    ygg::bind_index<ygg::Index<FunctionTerm<AuxiliaryTag>>>(m, "AuxiliaryFunctionTermIndex");

    bind_function_term_data<StaticTag>(m, "StaticFunctionTermData");
    bind_function_term_data<FluentTag>(m, "FluentFunctionTermData");
    bind_function_term_data<AuxiliaryTag>(m, "AuxiliaryFunctionTermData");

    bind_function_term_view<StaticTag>(m, "StaticFunctionTerm");
    bind_function_term_view<FluentTag>(m, "FluentFunctionTerm");
    bind_function_term_view<AuxiliaryTag>(m, "AuxiliaryFunctionTerm");

    repository.def("get_or_create", &get_or_create_data<FunctionTerm<StaticTag>>, "data"_a, nb::keep_alive<0, 1>());
    repository.def("get_or_create", &get_or_create_data<FunctionTerm<FluentTag>>, "data"_a, nb::keep_alive<0, 1>());
    repository.def("get_or_create", &get_or_create_data<FunctionTerm<AuxiliaryTag>>, "data"_a, nb::keep_alive<0, 1>());
}

}  // namespace tyr::formalism::datalog
