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
#include "tyr/formalism/planning/invariants/invariant.hpp"

#include <cstddef>
#include <optional>
#include <vector>

namespace tyr::formalism::planning::invariant
{

/// Atoms of which at most one is true in every reachable state; each group becomes one FDR variable.
struct MutexGroup
{
    AtomViewList<GroundTag, FluentTag> atoms;
    /// Index into MutexGroups::invariants of the invariant this group instantiates; nullopt for an atom no group covers.
    std::optional<size_t> invariant_index;
    /// The objects bound to the invariant's rigid variables.
    std::vector<ygg::Index<Object>> binding;
};

/// An instance of an ordered invariant with more than one true initial atom. The invariant does not hold for it, so its
/// remaining atoms became groups of one atom instead of falling through to later invariants.
struct MutexGroupFallback
{
    size_t invariant_index;
    std::vector<ygg::Index<Object>> binding;
    AtomViewList<GroundTag, FluentTag> atoms;
};

struct MutexGroups
{
    InvariantList invariants;
    /// A partition of all atoms, in FDR variable order.
    std::vector<MutexGroup> groups;
    std::vector<MutexGroupFallback> fallbacks;
};

/// Greedy cover: repeatedly takes the invariant instance with the most uncovered atoms. Only instances with exactly
/// one true initial atom are candidates.
MutexGroups compute_mutex_groups(const AtomViewList<GroundTag, FluentTag>& initial_atoms,
                                 const AtomViewList<GroundTag, FluentTag>& all_atoms,
                                 const InvariantList& invariants);

/// Cover in a fixed invariant order: every atom joins the first invariant in the order that contains it, and atoms of
/// no listed invariant stay single. Instances without a true initial atom are kept, so the variable structure is
/// determined by the order rather than by the task.
MutexGroups compute_mutex_groups_in_order(const AtomViewList<GroundTag, FluentTag>& initial_atoms,
                                          const AtomViewList<GroundTag, FluentTag>& all_atoms,
                                          const InvariantList& invariant_order);

std::vector<AtomViewList<GroundTag, FluentTag>> to_atom_groups(const MutexGroups& mutex_groups);
}

#endif
