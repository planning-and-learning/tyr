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

#include "bindings.hpp"
#include "pytyr/formalism/conversions.hpp"
#include "pytyr/formalism/planning/bindings.hpp"
#include "tyr/declarations.hpp"
#include "tyr/formalism/repository.hpp"

#include <concepts>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/variant.h>
#include <nanobind/stl/vector.h>
#include <tyr/formalism/declarations.hpp>
#include <tyr/formalism/planning/formatter.hpp>
#include <urd/elements/view.hpp>
#include <urd/python/bindings.hpp>
#include <yggdrasil/python/bindings.hpp>

/// Function terms: their indices and data, bound here; their views, bound with the repository.
namespace tyr::formalism
{

namespace
{
template<TaskKind T, FactKind F, typename C>
void bind_function_term_view(nb::module_& m, nb::class_<C>& repository)
{
    using V = urd::index_view<FunctionTerm<T, F>, C>;
    urd::python::bind_view<V>(m);
    python::bind_insert<FunctionTerm<T, F>>(repository);
    urd::python::bind_builder<FunctionTerm<T, F>, C>(m);
}

template<typename C>
void bind_function_term_views(nb::module_& m, nb::class_<C>& repository)
{
    bind_function_term_view<LiftedTag, StaticTag>(m, repository);
    bind_function_term_view<LiftedTag, FluentTag>(m, repository);
    bind_function_term_view<LiftedTag, AuxiliaryTag>(m, repository);
    bind_function_term_view<GroundTag, StaticTag>(m, repository);
    bind_function_term_view<GroundTag, FluentTag>(m, repository);
    bind_function_term_view<GroundTag, AuxiliaryTag>(m, repository);
}
}  // namespace

void bind_function_term(nb::module_& m)
{
    urd::python::bind_index<FunctionTerm<LiftedTag, StaticTag>>(m);
    urd::python::bind_data<FunctionTerm<LiftedTag, StaticTag>>(m);
    urd::python::bind_index<FunctionTerm<LiftedTag, FluentTag>>(m);
    urd::python::bind_data<FunctionTerm<LiftedTag, FluentTag>>(m);
    urd::python::bind_index<FunctionTerm<LiftedTag, AuxiliaryTag>>(m);
    urd::python::bind_data<FunctionTerm<LiftedTag, AuxiliaryTag>>(m);
    urd::python::bind_index<FunctionTerm<GroundTag, StaticTag>>(m);
    urd::python::bind_data<FunctionTerm<GroundTag, StaticTag>>(m);
    urd::python::bind_index<FunctionTerm<GroundTag, FluentTag>>(m);
    urd::python::bind_data<FunctionTerm<GroundTag, FluentTag>>(m);
    urd::python::bind_index<FunctionTerm<GroundTag, AuxiliaryTag>>(m);
    urd::python::bind_data<FunctionTerm<GroundTag, AuxiliaryTag>>(m);
}

}  // namespace tyr::formalism

namespace tyr::formalism
{

void bind_function_term(nb::module_& m, RepositoryBinding& repository) { bind_function_term_views(m, repository); }

}  // namespace tyr::formalism
