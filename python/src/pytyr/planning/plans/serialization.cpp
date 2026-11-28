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

#include "pytyr/planning/module.hpp"
#include "tyr/formalism/planning/formatter.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/plans/plan_view.hpp"
#include "tyr/planning/states/state_repository.hpp"
#include "tyr/planning/states/state_view.hpp"
#include "tyr/planning/tasks/ground/task.hpp"
#include "tyr/planning/tasks/lifted/task.hpp"

#include <boost/mp11.hpp>
#include <yggdrasil/python/serialization.hpp>

namespace tyr::planning
{

/// The serialization hooks of a task kind's plan views, once they are bound.
template<TaskKind Kind>
void bind_plan_serialization()
{ ygg::python::bind_serialization(boost::mp11::mp_list<PlanBuilderView<DeterministicTag, Kind>, PlanRecordView<DeterministicTag, Kind>> {}); }

template void bind_plan_serialization<GroundTag>();
template void bind_plan_serialization<LiftedTag>();

}  // namespace tyr::planning
