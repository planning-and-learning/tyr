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
#include "tyr/formalism/declarations.hpp"

#include <nanobind/stl/string.h>
#include <nanobind/stl/variant.h>
#include <nanobind/stl/vector.h>
#include <tyr/formalism/predicate_data.hpp>
#include <urd/python/bindings.hpp>

namespace tyr::formalism
{

void bind_predicate(nb::module_& m)
{
    urd::python::bind_index<Predicate<StaticTag>>(m);
    urd::python::bind_data<Predicate<StaticTag>>(m);
    urd::python::bind_index<Predicate<FluentTag>>(m);
    urd::python::bind_data<Predicate<FluentTag>>(m);
    urd::python::bind_index<Predicate<DerivedTag>>(m);
    urd::python::bind_data<Predicate<DerivedTag>>(m);
}

}  // namespace tyr::formalism
