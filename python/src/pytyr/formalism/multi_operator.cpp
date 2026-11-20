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
#include "pytyr/formalism/datalog/bindings.hpp"
#include "pytyr/formalism/planning/bindings.hpp"
#include "tyr/declarations.hpp"
#include "tyr/formalism/enums.hpp"

#include <nanobind/stl/optional.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/variant.h>
#include <nanobind/stl/vector.h>
#include <tyr/formalism/datalog/formatter.hpp>
#include <tyr/formalism/datalog/interning.hpp>
#include <tyr/formalism/declarations.hpp>
#include <tyr/formalism/planning/interning.hpp>
#include <urd/elements/view.hpp>
#include <urd/python/bindings.hpp>
#include <yggdrasil/python/bindings.hpp>

/// Multi operators: their indices and data are the same for datalog and planning, bound once here (and built from
/// either's views); their views are each repository's.
namespace tyr::formalism
{

namespace
{
template<TaskKind T, typename C>
void bind_multi_operator_view(nb::module_& m, nb::class_<C>& repository)
{
    urd::python::bind_view<urd::index_view<MultiOperator<T, ArithmeticOperatorKind>, C>>(m);
    ygg::bind_insert<MultiOperator<T, ArithmeticOperatorKind>>(repository);
    urd::python::bind_builder<MultiOperator<T, ArithmeticOperatorKind>, C>(m);
}

template<typename C>
void bind_multi_operator_views(nb::module_& m, nb::class_<C>& repository)
{
    bind_multi_operator_view<LiftedTag>(m, repository);
    bind_multi_operator_view<GroundTag>(m, repository);
}
}  // namespace

void bind_multi_operator(nb::module_& m)
{
    urd::python::bind_index<MultiOperator<LiftedTag, ArithmeticOperatorKind>>(m);
    urd::python::bind_data<MultiOperator<LiftedTag, ArithmeticOperatorKind>>(m);
    urd::python::bind_index<MultiOperator<GroundTag, ArithmeticOperatorKind>>(m);
    urd::python::bind_data<MultiOperator<GroundTag, ArithmeticOperatorKind>>(m);
}

}  // namespace tyr::formalism

namespace tyr::formalism::datalog
{

void bind_multi_operator(nb::module_& m, RepositoryBinding& repository) { bind_multi_operator_views(m, repository); }

}  // namespace tyr::formalism::datalog

namespace tyr::formalism::planning
{

void bind_multi_operator(nb::module_& m, RepositoryBinding& repository) { bind_multi_operator_views(m, repository); }

}  // namespace tyr::formalism::planning
