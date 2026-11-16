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

#include <tyr/formalism/function_term_value_data.hpp>
#include <tyr/formalism/function_term_value_view.hpp>

/// Ground function term values: their indices and data are the same for datalog and planning, bound once here (and
/// built from either's views); their views are each repository's.
namespace tyr::formalism
{

namespace
{
template<FactKind F, typename C>
void bind_function_term_value_view(nb::module_& m, nb::class_<C>& repository)
{
    urd::python::bind_view<urd::index_view<FunctionTermValue<GroundTag, F>, C>>(m);
    ygg::bind_insert<FunctionTermValue<GroundTag, F>>(repository);
}

template<typename C>
void bind_function_term_value_views(nb::module_& m, nb::class_<C>& repository)
{
    bind_function_term_value_view<StaticTag>(m, repository);
    bind_function_term_value_view<FluentTag>(m, repository);
    bind_function_term_value_view<AuxiliaryTag>(m, repository);
}
}  // namespace

void bind_function_term_value(nb::module_& m)
{
    urd::python::bind_data<FunctionTermValue<GroundTag, StaticTag>>(m);
    urd::python::bind_data<FunctionTermValue<GroundTag, FluentTag>>(m);
    urd::python::bind_data<FunctionTermValue<GroundTag, AuxiliaryTag>>(m);
}

}  // namespace tyr::formalism

namespace tyr::formalism::datalog
{

void bind_function_term_value(nb::module_& m, RepositoryBinding& repository) { bind_function_term_value_views(m, repository); }

}  // namespace tyr::formalism::datalog

namespace tyr::formalism::planning
{

void bind_function_term_value(nb::module_& m, RepositoryBinding& repository) { bind_function_term_value_views(m, repository); }

}  // namespace tyr::formalism::planning
