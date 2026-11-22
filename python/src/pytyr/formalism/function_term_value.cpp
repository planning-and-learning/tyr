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

#include "tyr/formalism/repository.hpp"
#include "bindings.hpp"
#include "pytyr/formalism/planning/bindings.hpp"
#include "tyr/declarations.hpp"

#include <nanobind/stl/optional.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/variant.h>
#include <nanobind/stl/vector.h>
#include <tyr/formalism/declarations.hpp>
#include <urd/elements/view.hpp>
#include <urd/python/bindings.hpp>
#include <yggdrasil/python/bindings.hpp>

/// Ground function term values: their indices and data, bound here; their views, bound with the repository.
namespace tyr::formalism
{

namespace
{
template<FactKind F, typename C>
void bind_function_term_value_view(nb::module_& m, nb::class_<C>& repository)
{
    urd::python::bind_view<urd::index_view<FunctionTermValue<GroundTag, F>, C>>(m);
    ygg::bind_insert<FunctionTermValue<GroundTag, F>>(repository);
    urd::python::bind_builder<FunctionTermValue<GroundTag, F>, C>(m);
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
    urd::python::bind_index<FunctionTermValue<GroundTag, StaticTag>>(m);
    urd::python::bind_data<FunctionTermValue<GroundTag, StaticTag>>(m);
    urd::python::bind_index<FunctionTermValue<GroundTag, FluentTag>>(m);
    urd::python::bind_data<FunctionTermValue<GroundTag, FluentTag>>(m);
    urd::python::bind_index<FunctionTermValue<GroundTag, AuxiliaryTag>>(m);
    urd::python::bind_data<FunctionTermValue<GroundTag, AuxiliaryTag>>(m);
}

}  // namespace tyr::formalism

namespace tyr::formalism
{

void bind_function_term_value(nb::module_& m, RepositoryBinding& repository) { bind_function_term_value_views(m, repository); }

}  // namespace tyr::formalism
