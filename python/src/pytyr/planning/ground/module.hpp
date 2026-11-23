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

#ifndef TYR_PYTHON_PLANNING_GROUND_MODULE_HPP_
#define TYR_PYTHON_PLANNING_GROUND_MODULE_HPP_

#include "../module.hpp"

#include <nanobind/nanobind.h>

namespace tyr::planning
{

void bind_ground_module_definitions(nb::module_& m);
/// The records of the ground match trees (match_tree.cpp).
void bind_match_tree(nb::module_& m);

}  // namespace tyr::planning

#endif
