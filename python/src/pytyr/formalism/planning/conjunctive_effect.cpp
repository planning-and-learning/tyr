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
#include "tyr/declarations.hpp"

#include <nanobind/stl/optional.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/variant.h>
#include <nanobind/stl/vector.h>
#include <tyr/formalism/interning.hpp>
#include <tyr/formalism/planning/declarations.hpp>
#include <tyr/formalism/planning/formatter.hpp>
#include <urd/python/bindings.hpp>
#include <yggdrasil/python/bindings.hpp>

namespace tyr::formalism
{

namespace
{
template<TaskKind T>
void bind_conjunctive_effect_kind(nb::module_& m, RepositoryBinding& repository)
{
    urd::python::bind_view<ConjunctiveEffectView<T>>(m);
    ygg::bind_insert<ConjunctiveEffect<T>>(repository);
    urd::python::bind_builder<ConjunctiveEffect<T>, Repository>(m);
}
}  // namespace

void bind_conjunctive_effect(nb::module_& m, RepositoryBinding& repository)
{
    urd::python::bind_index<ConjunctiveEffect<LiftedTag>>(m);
    urd::python::bind_data<ConjunctiveEffect<LiftedTag>>(m);
    bind_conjunctive_effect_kind<LiftedTag>(m, repository);
    urd::python::bind_index<ConjunctiveEffect<GroundTag>>(m);
    urd::python::bind_data<ConjunctiveEffect<GroundTag>>(m);
    bind_conjunctive_effect_kind<GroundTag>(m, repository);
}

}  // namespace tyr::formalism
