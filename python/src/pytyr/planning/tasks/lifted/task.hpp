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

#ifndef TYR_PYTHON_PLANNING_TASKS_LIFTED_TASK_HPP_
#define TYR_PYTHON_PLANNING_TASKS_LIFTED_TASK_HPP_

#include "pytyr/planning/module.hpp"

#include <nanobind/nanobind.h>

namespace tyr::planning
{

/// The lifted planning task and its grounding (tyr/planning/tasks/lifted/task.hpp, task_grounder.hpp).
void bind_lifted_task(nb::module_& m);

}  // namespace tyr::planning

#endif
