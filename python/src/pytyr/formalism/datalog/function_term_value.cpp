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

#include <tyr/formalism/datalog/function_term_value_data.hpp>
#include <tyr/formalism/datalog/function_term_value_index.hpp>
#include <tyr/formalism/datalog/function_term_value_view.hpp>

namespace tyr::formalism::datalog
{

namespace
{
template<FactKind F>
void bind_function_term_value_data(nb::module_& m, const char* name)
{
    using V = ygg::Data<FunctionTermValue<GroundTag, F>>;
    auto cls = nb::class_<V>(m, name).def(nb::init<FunctionTermView<GroundTag, F>, ygg::float_t>(), "fterm"_a, "value"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

template<FactKind F>
void bind_function_term_value_view(nb::module_& m, const char* name)
{
    using V = FunctionTermValueView<GroundTag, F>;
    auto cls = nb::class_<V>(m, name).def("get_index", &V::get_index).def("get_fterm", &V::get_fterm, nb::keep_alive<0, 1>()).def("get_value", &V::get_value);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}
}  // namespace

void bind_function_term_value(nb::module_& m, RepositoryBinding& repository)
{
    ygg::bind_index<ygg::Index<FunctionTermValue<GroundTag, StaticTag>>>(m, "StaticGroundFunctionTermValueIndex");
    ygg::bind_index<ygg::Index<FunctionTermValue<GroundTag, FluentTag>>>(m, "FluentGroundFunctionTermValueIndex");
    ygg::bind_index<ygg::Index<FunctionTermValue<GroundTag, AuxiliaryTag>>>(m, "AuxiliaryGroundFunctionTermValueIndex");

    bind_function_term_value_data<StaticTag>(m, "StaticGroundFunctionTermValueData");
    bind_function_term_value_data<FluentTag>(m, "FluentGroundFunctionTermValueData");
    bind_function_term_value_data<AuxiliaryTag>(m, "AuxiliaryGroundFunctionTermValueData");

    bind_function_term_value_view<StaticTag>(m, "StaticGroundFunctionTermValue");
    bind_function_term_value_view<FluentTag>(m, "FluentGroundFunctionTermValue");
    bind_function_term_value_view<AuxiliaryTag>(m, "AuxiliaryGroundFunctionTermValue");

    repository.def("get_or_create", &get_or_create_data<FunctionTermValue<GroundTag, StaticTag>>, "data"_a, nb::keep_alive<0, 1>());
    repository.def("get_or_create", &get_or_create_data<FunctionTermValue<GroundTag, FluentTag>>, "data"_a, nb::keep_alive<0, 1>());
    repository.def("get_or_create", &get_or_create_data<FunctionTermValue<GroundTag, AuxiliaryTag>>, "data"_a, nb::keep_alive<0, 1>());
}

}  // namespace tyr::formalism::datalog
