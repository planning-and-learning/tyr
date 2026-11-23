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
#include "pytyr/conversions.hpp"
#include "tyr/declarations.hpp"
#include "tyr/formalism/repository.hpp"

#include <nanobind/stl/optional.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/variant.h>
#include <nanobind/stl/vector.h>
#include <tyr/formalism/planning/declarations.hpp>
#include <tyr/formalism/planning/formatter.hpp>
#include <urd/python/bindings.hpp>
#include <yggdrasil/python/bindings.hpp>

namespace tyr::formalism
{

void bind_action(nb::module_& m, RepositoryBinding& repository)
{
    urd::python::bind_index<Action<LiftedTag>>(m);
    urd::python::bind_data<Action<LiftedTag>>(m);
    urd::python::bind_view<ActionView<LiftedTag>>(m);
    ygg::bind_insert<Action<LiftedTag>>(repository);
    urd::python::bind_builder<Action<LiftedTag>, Repository>(m);

    urd::python::bind_index<Action<GroundTag>>(m);

    urd::python::bind_data<Action<GroundTag>>(m);
    urd::python::bind_view<ActionView<GroundTag>>(m);
    ygg::bind_insert<Action<GroundTag>>(repository);
    urd::python::bind_builder<Action<GroundTag>, Repository>(m);
}

}  // namespace tyr::formalism
