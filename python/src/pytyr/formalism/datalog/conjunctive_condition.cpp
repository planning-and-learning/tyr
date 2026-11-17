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
#include <tyr/formalism/datalog/declarations.hpp>
#include <tyr/formalism/datalog/formatter.hpp>
#include <tyr/formalism/datalog/interning.hpp>
#include <urd/python/bindings.hpp>
#include <yggdrasil/python/bindings.hpp>

namespace tyr::formalism::datalog
{

void bind_conjunctive_condition(nb::module_& m, RepositoryBinding& repository)
{
    urd::python::bind_data<ConjunctiveCondition<LiftedTag>>(m);
    urd::python::bind_view<ConjunctiveConditionView<LiftedTag>>(m);
    ygg::bind_insert<ConjunctiveCondition<LiftedTag>>(repository);

    urd::python::bind_data<ConjunctiveCondition<GroundTag>>(m);
    urd::python::bind_view<ConjunctiveConditionView<GroundTag>>(m);
    ygg::bind_insert<ConjunctiveCondition<GroundTag>>(repository);
}

}  // namespace tyr::formalism::datalog
