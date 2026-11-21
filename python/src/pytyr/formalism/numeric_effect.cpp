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
#include "pytyr/formalism/planning/bindings.hpp"
#include "tyr/declarations.hpp"

#include <nanobind/stl/optional.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/variant.h>
#include <nanobind/stl/vector.h>
#include <tyr/formalism/declarations.hpp>
#include <tyr/formalism/planning/formatter.hpp>
#include <tyr/formalism/planning/interning.hpp>
#include <urd/elements/view.hpp>
#include <urd/python/bindings.hpp>
#include <yggdrasil/python/bindings.hpp>

/// Numeric effects: their indices and data, bound here; their views, bound with the repository.
namespace tyr::formalism
{

namespace
{
template<TaskKind T, FactKind F, typename C>
void bind_numeric_effect_view(nb::module_& m, nb::class_<C>& repository)
{
    urd::python::bind_view<urd::index_view<NumericEffect<T, F>, C>>(m);
    ygg::bind_insert<NumericEffect<T, F>>(repository);
    urd::python::bind_builder<NumericEffect<T, F>, C>(m);
}
}  // namespace

void bind_numeric_effect(nb::module_& m)
{
    urd::python::bind_index<NumericEffect<LiftedTag, FluentTag>>(m);
    urd::python::bind_data<NumericEffect<LiftedTag, FluentTag>>(m);
    urd::python::bind_index<NumericEffect<LiftedTag, AuxiliaryTag>>(m);
    urd::python::bind_data<NumericEffect<LiftedTag, AuxiliaryTag>>(m);
    urd::python::bind_index<NumericEffect<GroundTag, FluentTag>>(m);
    urd::python::bind_data<NumericEffect<GroundTag, FluentTag>>(m);
    urd::python::bind_index<NumericEffect<GroundTag, AuxiliaryTag>>(m);
    urd::python::bind_data<NumericEffect<GroundTag, AuxiliaryTag>>(m);
}

}  // namespace tyr::formalism

namespace tyr::formalism
{

void bind_numeric_effect(nb::module_& m, RepositoryBinding& repository)
{
    bind_numeric_effect_view<LiftedTag, FluentTag>(m, repository);
    bind_numeric_effect_view<LiftedTag, AuxiliaryTag>(m, repository);
    bind_numeric_effect_view<GroundTag, FluentTag>(m, repository);
    bind_numeric_effect_view<GroundTag, AuxiliaryTag>(m, repository);
}

}  // namespace tyr::formalism
