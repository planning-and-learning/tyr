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
#include "pytyr/module.hpp"

namespace tyr::formalism::datalog
{

void bind_conjunctive_condition(nb::module_& m, RepositoryBinding& repository)
{
    urd::python::bind_data<ConjunctiveCondition<LiftedTag>>(m);
    urd::python::bind_view<ConjunctiveConditionView<LiftedTag>>(m).def("get_arity", &ConjunctiveConditionView<LiftedTag>::get_arity);
    ygg::bind_insert<ConjunctiveCondition<LiftedTag>>(repository);

    urd::python::bind_data<ConjunctiveCondition<GroundTag>>(m);
    urd::python::bind_view<ConjunctiveConditionView<GroundTag>>(m);
    ygg::bind_insert<ConjunctiveCondition<GroundTag>>(repository);
}

}  // namespace tyr::formalism::datalog
