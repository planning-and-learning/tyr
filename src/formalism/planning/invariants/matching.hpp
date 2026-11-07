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

#ifndef TYR_FORMALISM_PLANNING_INVARIANTS_MATCHING_HPP_
#define TYR_FORMALISM_PLANNING_INVARIANTS_MATCHING_HPP_

#include "tyr/formalism/planning/mutable/atom.hpp"
#include "tyr/formalism/planning/mutable/invariant.hpp"
#include "tyr/formalism/unification/substitution.hpp"

namespace tyr::formalism::planning::invariant
{

using InvariantSubstitution = unification::SubstitutionFunction<Term>;

bool covers(const MutableInvariant& inv, const MutableAtom<FluentTag>& element);

std::optional<InvariantSubstitution>
match_invariant_against_ground_atom(const MutableInvariant& inv, const MutableAtom<FluentTag>& pattern, const MutableAtom<FluentTag>& ground_atom);

}  // namespace tyr::formalism::planning::invariant

#endif