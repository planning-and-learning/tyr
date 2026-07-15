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

#ifndef TYR_FORMALISM_PLANNING_INVARIANTS_MUTEXES_HPP_
#define TYR_FORMALISM_PLANNING_INVARIANTS_MUTEXES_HPP_

#include "tyr/formalism/planning/invariants/invariant.hpp"
#include "tyr/formalism/planning/declarations.hpp"

#include <vector>

namespace tyr::formalism::planning::invariant
{
std::vector<GroundAtomViewList<FluentTag>>
compute_mutex_groups(const GroundAtomViewList<FluentTag>& initial_atoms, const GroundAtomViewList<FluentTag>& all_atoms, const InvariantList& invariants);
}

#endif
