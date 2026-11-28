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

#ifndef TYR_PYTHON_PLANNING_MODULE_HPP_
#define TYR_PYTHON_PLANNING_MODULE_HPP_

#include "tyr/declarations.hpp"
#include "tyr/planning/declarations.hpp"

#include <nanobind/nanobind.h>
#include <nanobind/trampoline.h>

namespace nb = nanobind;

namespace tyr::planning
{

extern void bind_module_definitions(nb::module_& m);

// The serialization hooks (pyyggdrasil.serialization) of a task kind's views: each defined in its component's
// serialization.cpp (its own source, instantiated per kind), called where the component's views are bound.

/// Of the state views.
template<TaskKind Kind>
void bind_state_serialization();

/// Of the node and labeled node views.
template<TaskKind Kind>
void bind_node_serialization();

/// Of the plan views.
template<TaskKind Kind>
void bind_plan_serialization();

/// Of the label views.
template<TaskKind Kind>
void bind_successor_serialization();

/// Of the successor views of environment E and its parts' builder views.
template<NatureEnvironment E, TaskKind Kind>
void bind_environment_successor_serialization();

}  // namespace tyr::planning

#endif
