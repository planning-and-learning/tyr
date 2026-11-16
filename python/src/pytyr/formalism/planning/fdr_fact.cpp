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

#include <nanobind/stl/optional.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/variant.h>
#include <nanobind/stl/vector.h>
#include <tyr/formalism/declarations.hpp>
#include <tyr/formalism/planning/declarations.hpp>
#include <tyr/formalism/planning/formatter.hpp>
#include <tyr/formalism/planning/interning.hpp>
#include <urd/elements/view.hpp>
#include <urd/python/bindings.hpp>

namespace tyr::formalism::planning
{

namespace
{
/// A fact's view: borrowed (a stored fact) or owning (a state's, computed on the fly).
template<typename V>
void bind_fdr_fact_view(nb::module_& m)
{ urd::python::bind_view<V>(m).def("has_value", &V::has_value).def("get_atom", &V::get_atom); }
}  // namespace

void bind_fdr_fact(nb::module_& m, RepositoryBinding& /*repository*/)
{
    urd::python::bind_data<FDRFact<FluentTag>>(m);
    bind_fdr_fact_view<FDRFactView<FluentTag>>(m);
    bind_fdr_fact_view<urd::owning_view<FDRFact<FluentTag>, Repository>>(m);
}

}  // namespace tyr::formalism::planning
