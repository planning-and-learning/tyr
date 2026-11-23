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
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/repository.hpp"

#include <nanobind/nanobind.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/variant.h>
#include <nanobind/stl/vector.h>
#include <tyr/formalism/datalog/formatter.hpp>
#include <urd/elements/index.hpp>
#include <urd/python/bindings.hpp>
#include <yggdrasil/formalism/declarations.hpp>
#include <yggdrasil/formalism/row_view.hpp>
#include <yggdrasil/python/bindings.hpp>

namespace tyr::formalism
{

void bind_rule(nb::module_& m, RepositoryBinding& repository)
{
    urd::python::bind_index<Rule<LiftedTag>>(m);
    urd::python::bind_data<Rule<LiftedTag>>(m);
    urd::python::bind_view<RuleView<LiftedTag>>(m);
    ygg::bind_insert<Rule<LiftedTag>>(repository);
    urd::python::bind_builder<Rule<LiftedTag>, Repository>(m);

    urd::python::bind_index<Rule<GroundTag>>(m);

    urd::python::bind_data<Rule<GroundTag>>(m);
    urd::python::bind_view<RuleView<GroundTag>>(m);
    ygg::bind_insert<Rule<GroundTag>>(repository);
    urd::python::bind_builder<Rule<GroundTag>, Repository>(m);

    // The rows of lifted rules.
    urd::python::bind_index<ygg::Row<Rule<LiftedTag>>>(m);
    urd::python::bind_index<urd::key<Rule<LiftedTag>, ygg::Row<Rule<LiftedTag>>>>(m);
    urd::python::bind_data<ygg::Row<Rule<LiftedTag>>>(m);
    urd::python::bind_view<ygg::RowView<Rule<LiftedTag>, Repository>>(m);
    ygg::bind_insert<ygg::Row<Rule<LiftedTag>>>(repository);
}

}  // namespace tyr::formalism
