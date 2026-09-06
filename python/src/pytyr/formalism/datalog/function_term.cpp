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
template<TaskKind T, FactKind F>
void bind_function_term_kind(nb::module_& m, RepositoryBinding& repository, const std::string& name)
{
    using Tag = FunctionTerm<T, F>;
    ygg::bind_index<ygg::Index<Tag>>(m, (name + "Index").c_str());

    {
        using V = ygg::Data<Tag>;
        auto cls = nb::class_<V>(m, (name + "Data").c_str());
        if constexpr (std::same_as<T, LiftedTag>)
        {
            cls.def(nb::init<FunctionView<F>, const TermViewList&>(), "function"_a, "terms"_a);
        }
        else
        {
            cls.def(nb::init<FunctionBindingView<F>>(), "binding"_a);
        }
        ygg::add_print(cls);
        ygg::add_comparison(cls);
        ygg::add_hash(cls);
    }

    {
        using V = FunctionTermView<T, F>;
        auto cls = nb::class_<V>(m, name.c_str());
        cls.def("get_index", &V::get_index);
        cls.def("get_function", &V::get_function, nb::keep_alive<0, 1>());
        if constexpr (std::same_as<T, LiftedTag>)
        {
            cls.def("get_terms", &V::get_terms);
        }
        else
        {
            cls.def("get_row", &V::get_row, nb::keep_alive<0, 1>());
            cls.def("get_objects", &V::get_objects);
        }
        ygg::add_print(cls);
        ygg::add_comparison(cls);
        ygg::add_hash(cls);
    }

    repository.def("get_or_create", &get_or_create_data<Tag>, "data"_a, nb::keep_alive<0, 1>());
}
}  // namespace

void bind_function_term(nb::module_& m, RepositoryBinding& repository)
{
    bind_function_term_kind<LiftedTag, StaticTag>(m, repository, "StaticFunctionTerm");
    bind_function_term_kind<LiftedTag, FluentTag>(m, repository, "FluentFunctionTerm");
    bind_function_term_kind<LiftedTag, AuxiliaryTag>(m, repository, "AuxiliaryFunctionTerm");
    bind_function_term_kind<GroundTag, StaticTag>(m, repository, "StaticGroundFunctionTerm");
    bind_function_term_kind<GroundTag, FluentTag>(m, repository, "FluentGroundFunctionTerm");
    bind_function_term_kind<GroundTag, AuxiliaryTag>(m, repository, "AuxiliaryGroundFunctionTerm");
}

}  // namespace tyr::formalism::datalog
