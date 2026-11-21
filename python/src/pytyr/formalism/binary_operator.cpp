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
#include "tyr/formalism/enums.hpp"

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

/// Binary operators: their indices and data, bound here; their views, bound with the repository.
namespace tyr::formalism
{

namespace
{
template<TaskKind T, typename O, typename C>
void bind_binary_operator_view(nb::module_& m, nb::class_<C>& repository)
{
    urd::python::bind_view<urd::index_view<BinaryOperator<T, O>, C>>(m);
    ygg::bind_insert<BinaryOperator<T, O>>(repository);
    urd::python::bind_builder<BinaryOperator<T, O>, C>(m);
}

template<typename C>
void bind_binary_operator_views(nb::module_& m, nb::class_<C>& repository)
{
    bind_binary_operator_view<LiftedTag, ArithmeticOperatorKind>(m, repository);
    bind_binary_operator_view<LiftedTag, BooleanOperatorKind>(m, repository);
    bind_binary_operator_view<GroundTag, ArithmeticOperatorKind>(m, repository);
    bind_binary_operator_view<GroundTag, BooleanOperatorKind>(m, repository);
}
}  // namespace

void bind_binary_operator(nb::module_& m)
{
    urd::python::bind_index<BinaryOperator<LiftedTag, ArithmeticOperatorKind>>(m);
    urd::python::bind_data<BinaryOperator<LiftedTag, ArithmeticOperatorKind>>(m);
    urd::python::bind_index<BinaryOperator<LiftedTag, BooleanOperatorKind>>(m);
    urd::python::bind_data<BinaryOperator<LiftedTag, BooleanOperatorKind>>(m);
    urd::python::bind_index<BinaryOperator<GroundTag, ArithmeticOperatorKind>>(m);
    urd::python::bind_data<BinaryOperator<GroundTag, ArithmeticOperatorKind>>(m);
    urd::python::bind_index<BinaryOperator<GroundTag, BooleanOperatorKind>>(m);
    urd::python::bind_data<BinaryOperator<GroundTag, BooleanOperatorKind>>(m);
}

}  // namespace tyr::formalism

namespace tyr::formalism::planning
{

void bind_binary_operator(nb::module_& m, RepositoryBinding& repository) { bind_binary_operator_views(m, repository); }

}  // namespace tyr::formalism::planning
