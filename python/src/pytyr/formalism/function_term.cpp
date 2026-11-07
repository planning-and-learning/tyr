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

#include <tyr/formalism/function_term_data.hpp>
#include <tyr/formalism/function_term_view.hpp>

/// Function terms: their indices and data are the same for datalog and planning, bound once here (and built from
/// either's views); their views are each repository's.
namespace tyr::formalism
{

namespace
{
template<TaskKind T, FactKind F>
void bind_function_term_data(nb::module_& m, const std::string& name)
{
    using namespace nb::literals;
    urd::python::bind_index<urd::index<FunctionTerm<T, F>>>(m, name + "Index");

    using V = urd::data<FunctionTerm<T, F>>;
    auto cls = nb::class_<V>(m, (name + "Data").c_str());
    if constexpr (std::same_as<T, LiftedTag>)
    {
        cls.def(urd::python::init_from_views<datalog::FunctionView<F>, const datalog::TermViewList&>(), "function"_a, "terms"_a);
        cls.def(urd::python::init_from_views<planning::FunctionView<F>, const planning::TermViewList&>(), "function"_a, "terms"_a);
        cls.def(nb::init<urd::index<Function<F>>, TermList>(), "function"_a, "terms"_a);
    }
    else
    {
        cls.def(urd::python::init_from_views<datalog::FunctionBindingView<F>>(), "binding"_a);
        cls.def(urd::python::init_from_views<planning::FunctionBindingView<F>>(), "binding"_a);
        cls.def(nb::init<ygg::formalism::RelationBindingIndex<Function<F>, ObjectTag>>(), "binding"_a);
    }
    ygg::add_print(cls);
    urd::python::add_comparison(cls);
    urd::python::add_hash(cls);
}

template<TaskKind T, FactKind F, typename C>
void bind_function_term_view(nb::module_& m, nb::class_<C>& repository, const std::string& name)
{
    using V = urd::index_view<FunctionTerm<T, F>, C>;
    auto cls = nb::class_<V>(m, name.c_str());
    cls.def("index", &V::index);
    cls.def("get_function", &V::get_function);
    if constexpr (std::same_as<T, LiftedTag>)
    {
        cls.def("get_terms", &V::get_terms);
    }
    else
    {
        cls.def("get_row", &V::get_row);
        cls.def("get_objects", &V::get_objects);
    }
    ygg::add_print(cls);
    urd::python::add_comparison(cls);
    urd::python::add_hash(cls);

    ygg::bind_insert<FunctionTerm<T, F>>(repository);
}

template<typename C>
void bind_function_term_views(nb::module_& m, nb::class_<C>& repository)
{
    bind_function_term_view<LiftedTag, StaticTag>(m, repository, "StaticFunctionTerm");
    bind_function_term_view<LiftedTag, FluentTag>(m, repository, "FluentFunctionTerm");
    bind_function_term_view<LiftedTag, AuxiliaryTag>(m, repository, "AuxiliaryFunctionTerm");
    bind_function_term_view<GroundTag, StaticTag>(m, repository, "StaticGroundFunctionTerm");
    bind_function_term_view<GroundTag, FluentTag>(m, repository, "FluentGroundFunctionTerm");
    bind_function_term_view<GroundTag, AuxiliaryTag>(m, repository, "AuxiliaryGroundFunctionTerm");
}
}  // namespace

void bind_function_term(nb::module_& m)
{
    bind_function_term_data<LiftedTag, StaticTag>(m, "StaticFunctionTerm");
    bind_function_term_data<LiftedTag, FluentTag>(m, "FluentFunctionTerm");
    bind_function_term_data<LiftedTag, AuxiliaryTag>(m, "AuxiliaryFunctionTerm");
    bind_function_term_data<GroundTag, StaticTag>(m, "StaticGroundFunctionTerm");
    bind_function_term_data<GroundTag, FluentTag>(m, "FluentGroundFunctionTerm");
    bind_function_term_data<GroundTag, AuxiliaryTag>(m, "AuxiliaryGroundFunctionTerm");
}

}  // namespace tyr::formalism

namespace tyr::formalism::datalog
{

void bind_function_term(nb::module_& m, RepositoryBinding& repository) { bind_function_term_views(m, repository); }

}  // namespace tyr::formalism::datalog

namespace tyr::formalism::planning
{

void bind_function_term(nb::module_& m, RepositoryBinding& repository) { bind_function_term_views(m, repository); }

}  // namespace tyr::formalism::planning
