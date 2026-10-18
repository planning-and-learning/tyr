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

#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/mutable/invariant.hpp"

#include <optional>
#include <vector>

namespace tyr::formalism::planning::invariant
{

/// Ground atoms of which at most one is true in every reachable state. `invariant` is the invariant the group
/// instantiates and `objects` the objects bound to its rigid variables, in variable order. A single atom outside any
/// usable instance has no invariant.
struct MutexGroup
{
    std::optional<InvariantView> invariant;
    ObjectViewList objects;
    AtomViewList<GroundTag, FluentTag> atoms;
};

/// Greedy cover: repeatedly takes the invariant instance with the most uncovered atoms. Only instances with exactly
/// one true initial atom are candidates. Each returned group becomes one FDR variable.
std::vector<MutexGroup> compute_mutex_groups(const AtomViewList<GroundTag, FluentTag>& initial_atoms,
                                             const AtomViewList<GroundTag, FluentTag>& all_atoms,
                                             const InvariantViewList& invariants);

/// Cover in a fixed invariant order: every atom joins the first invariant in the order that contains it, and atoms of
/// no listed invariant stay single. An instance with more than one true initial atom violates its invariant; its
/// atoms stay single. So the variable structure is determined by the order rather than by the task.
std::vector<MutexGroup> compute_mutex_groups_in_order(const AtomViewList<GroundTag, FluentTag>& initial_atoms,
                                                      const AtomViewList<GroundTag, FluentTag>& all_atoms,
                                                      const InvariantViewList& invariant_order);

/// Every instance of the invariants with at least two reachable atoms and at most one true initial atom: all mutex groups
/// the invariants prove for the task, overlapping in general. Deterministic order.
std::vector<MutexGroup> compute_all_mutex_groups(const AtomViewList<GroundTag, FluentTag>& initial_atoms,
                                                 const AtomViewList<GroundTag, FluentTag>& all_atoms,
                                                 const InvariantViewList& invariants);
}

#endif
