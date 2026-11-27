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

#include "pytyr/planning/lifted/module.hpp"

#include "pytyr/planning/planning.hpp"
#include "pytyr/planning/programs/lifted/programs.hpp"
#include "pytyr/planning/tasks/lifted/task.hpp"

#include <tyr/planning/programs/lifted/action.hpp>
#include "tyr/planning/states/state_repository.hpp"

namespace tyr::planning
{

void bind_lifted_module_definitions(nb::module_& m)
{
    bind_lifted_task(m);

    bind_lifted_programs(m);

    bind_kind_definitions<LiftedTag>(m);

}

}  // namespace tyr::planning
