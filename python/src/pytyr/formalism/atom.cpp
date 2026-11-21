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

#include <concepts>
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

/// Atoms: their indices and data, bound here; their views, bound with the repository.
namespace tyr::formalism
{
namespace
{
template<TaskKind T, FactKind F, typename C>
void bind_atom_view(nb::module_& m, nb::class_<C>& repository)
{
    using V = urd::index_view<Atom<T, F>, C>;
    urd::python::bind_view<V>(m);
    ygg::bind_insert<Atom<T, F>>(repository);
    urd::python::bind_builder<Atom<T, F>, C>(m);
}
}  // namespace

void bind_atom(nb::module_& m)
{
    urd::python::bind_index<Atom<LiftedTag, StaticTag>>(m);
    urd::python::bind_data<Atom<LiftedTag, StaticTag>>(m);
    urd::python::bind_index<Atom<LiftedTag, FluentTag>>(m);
    urd::python::bind_data<Atom<LiftedTag, FluentTag>>(m);
    urd::python::bind_index<Atom<LiftedTag, DerivedTag>>(m);
    urd::python::bind_data<Atom<LiftedTag, DerivedTag>>(m);
    urd::python::bind_index<Atom<GroundTag, StaticTag>>(m);
    urd::python::bind_data<Atom<GroundTag, StaticTag>>(m);
    urd::python::bind_index<Atom<GroundTag, FluentTag>>(m);
    urd::python::bind_data<Atom<GroundTag, FluentTag>>(m);
    urd::python::bind_index<Atom<GroundTag, DerivedTag>>(m);
    urd::python::bind_data<Atom<GroundTag, DerivedTag>>(m);
}

}  // namespace tyr::formalism

namespace tyr::formalism
{

void bind_atom(nb::module_& m, RepositoryBinding& repository)
{
    bind_atom_view<LiftedTag, StaticTag>(m, repository);
    bind_atom_view<LiftedTag, FluentTag>(m, repository);
    bind_atom_view<LiftedTag, DerivedTag>(m, repository);
    bind_atom_view<GroundTag, StaticTag>(m, repository);
    bind_atom_view<GroundTag, FluentTag>(m, repository);
    bind_atom_view<GroundTag, DerivedTag>(m, repository);
}

}  // namespace tyr::formalism
