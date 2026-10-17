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

#include "tyr/formalism/planning/invariants/mutexes.hpp"
#include "tyr/planning/declarations.hpp"

#include <optional>
#include <yggdrasil/execution/onetbb.hpp>

namespace tyr::planning
{

enum class GroundTaskInstantiationStatus
{
    SUCCESS,
    PROVEN_UNSOLVABLE,
};

struct GroundTaskInstantiationResult
{
    TaskPtr<GroundTag> task = nullptr;
    GroundTaskInstantiationStatus status = GroundTaskInstantiationStatus::PROVEN_UNSOLVABLE;
    /// The atoms, invariant and binding behind every FDR variable, in variable order.
    formalism::planning::invariant::MutexGroups mutex_groups;
};

struct GroundTaskInstantiationOptions
{
    /// nullopt: greedy cover over all synthesized invariants.
    /// Otherwise exactly these invariants, in this order (see compute_mutex_groups_in_order). Each must equal an
    /// invariant synthesized for the task's domain, which includes invariants synthesized from another parser of the
    /// same domain file; that parser must outlive the call. Unlisted invariants are not used, so an empty list yields
    /// one variable per atom.
    std::optional<formalism::planning::invariant::InvariantList> invariant_order = std::nullopt;
};

}

#endif
