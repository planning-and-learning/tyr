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

#ifndef TYR_PYTHON_PLANNING_PLANS_PLANS_HPP_
#define TYR_PYTHON_PLANNING_PLANS_PLANS_HPP_

#include "pytyr/planning/module.hpp"

#include <nanobind/nanobind.h>
#include <tyr/planning/declarations.hpp>
#include <tyr/planning/plans/plan_view.hpp>
#include <tyr/planning/states/state_repository.hpp>
#include <urd/python/bindings.hpp>

namespace tyr::planning
{

/// A plan, as a record (S4): its record, builder and views, each borrowed view bound before the owned one, which lends it.
template<TaskKind Kind>
void bind_plans(nb::module_& m)
{
    urd::python::bind_data<Plan<DeterministicTag, Kind>>(m);
    urd::python::bind_builder<Plan<DeterministicTag, Kind>, StateRepository<Kind>>(m);
    urd::python::bind_record_views<Plan<DeterministicTag, Kind>, StateRepository<Kind>>(m);
    urd::python::bind_view<OwnedPlanBuilderView<DeterministicTag, Kind>>(m);
    urd::python::bind_view<OwnedPlanRecordView<DeterministicTag, Kind>>(m);
    bind_plan_serialization<Kind>();
}

}  // namespace tyr::planning

#endif
