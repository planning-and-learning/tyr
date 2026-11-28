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

#include "pytyr/planning/ground/module.hpp"

#include "pytyr/planning/planning.hpp"
#include "pytyr/planning/successors/ground/match_tree.hpp"
#include "pytyr/planning/tasks/ground/task.hpp"
#include "tyr/planning/states/state_repository.hpp"

namespace tyr::planning
{

void bind_ground_module_definitions(nb::module_& m)
{
    bind_ground_task(m);

    bind_kind_definitions<GroundTag>(m);
    bind_match_tree(m);
}

}  // namespace tyr::planning
