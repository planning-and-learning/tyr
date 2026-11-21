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

#ifndef TYR_FORMALISM_PLANNING_INVARIANTS_REFINEMENT_HPP_
#define TYR_FORMALISM_PLANNING_INVARIANTS_REFINEMENT_HPP_

#include "proof.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/invariant_builder.hpp"
#include "utils.hpp"

#include <vector>

namespace tyr::formalism::invariant
{

/// The candidates that extend `inv` by an atom a delete effect of the threat's action can balance its add with, each held
/// in `storage`.
std::vector<urd::builder<Invariant, Repository>> refine_candidate(const urd::builder<Invariant, Repository>& inv,
                                                                  const Threat& threat,
                                                                  const std::vector<ActionView<LiftedTag>>& ops,
                                                                  CandidateStorage& storage);

/// One candidate per predicate and position of its counted variable (or none), each held in `storage`.
std::vector<urd::builder<Invariant, Repository>> make_initial_candidates(PredicateListView<FluentTag> predicates, CandidateStorage& storage);

}

#endif
