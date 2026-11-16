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

#include "binding_utils.hpp"
#include "bindings.hpp"
#include "datalog/binding_utils.hpp"
#include "planning/binding_utils.hpp"
#include "pytyr/module.hpp"

#include <tyr/formalism/numeric_effect_data.hpp>
#include <tyr/formalism/numeric_effect_view.hpp>

/// Numeric effects: their indices and data are the same for datalog and planning, bound once here (and built from
/// either's views); their views are each repository's (datalog has fluent ones only).
namespace tyr::formalism
{

namespace
{
template<TaskKind T, FactKind F, typename C>
void bind_numeric_effect_view(nb::module_& m, nb::class_<C>& repository)
{
    urd::python::bind_view<urd::index_view<NumericEffect<T, F>, C>>(m);
    ygg::bind_insert<NumericEffect<T, F>>(repository);
}
}  // namespace

void bind_numeric_effect(nb::module_& m)
{
    urd::python::bind_data<NumericEffect<LiftedTag, FluentTag>>(m);
    urd::python::bind_data<NumericEffect<LiftedTag, AuxiliaryTag>>(m);
    urd::python::bind_data<NumericEffect<GroundTag, FluentTag>>(m);
    urd::python::bind_data<NumericEffect<GroundTag, AuxiliaryTag>>(m);
}

}  // namespace tyr::formalism

namespace tyr::formalism::datalog
{

void bind_numeric_effect(nb::module_& m, RepositoryBinding& repository)
{
    bind_numeric_effect_view<LiftedTag, FluentTag>(m, repository);
    bind_numeric_effect_view<GroundTag, FluentTag>(m, repository);
}

}  // namespace tyr::formalism::datalog

namespace tyr::formalism::planning
{

void bind_numeric_effect(nb::module_& m, RepositoryBinding& repository)
{
    bind_numeric_effect_view<LiftedTag, FluentTag>(m, repository);
    bind_numeric_effect_view<LiftedTag, AuxiliaryTag>(m, repository);
    bind_numeric_effect_view<GroundTag, FluentTag>(m, repository);
    bind_numeric_effect_view<GroundTag, AuxiliaryTag>(m, repository);
}

}  // namespace tyr::formalism::planning
