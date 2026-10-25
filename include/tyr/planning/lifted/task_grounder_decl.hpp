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

#ifndef TYR_PLANNING_LIFTED_TASK_GROUNDER_DECL_HPP_
#define TYR_PLANNING_LIFTED_TASK_GROUNDER_DECL_HPP_

#include "tyr/formalism/planning/invariant_view.hpp"
#include "tyr/planning/declarations.hpp"

#include <cstdint>
#include <optional>
#include <yggdrasil/execution/declarations.hpp>

namespace tyr::planning
{

enum class GroundTaskInstantiationStatus : std::uint8_t
{
    SUCCESS,
    PROVEN_UNSOLVABLE,
};

struct GroundTaskInstantiationResult
{
    TaskPtr<GroundTag> task = nullptr;
    GroundTaskInstantiationStatus status = GroundTaskInstantiationStatus::PROVEN_UNSOLVABLE;
};

struct GroundTaskInstantiationOptions
{
    bool disable_invariant_synthesis = false;
    /// Used unless invariant synthesis is disabled. nullopt: greedy cover over all synthesized invariants. Otherwise
    /// exactly these invariants, in this order (see compute_mutex_groups_in_order). Each must be an invariant
    /// synthesize_invariants interned into the repository of the task's domain. Unlisted invariants are not used.
    std::optional<formalism::planning::InvariantViewList> invariant_order = std::nullopt;
};

}

#endif
