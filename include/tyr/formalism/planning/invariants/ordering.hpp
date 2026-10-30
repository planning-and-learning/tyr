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

#ifndef TYR_FORMALISM_PLANNING_INVARIANTS_ORDERING_HPP_
#define TYR_FORMALISM_PLANNING_INVARIANTS_ORDERING_HPP_

#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/mutable/invariant.hpp"

#include <cstdint>
#include <vector>

namespace tyr::formalism::planning::invariant
{

/// Invariant orders for GroundTaskInstantiationOptions::invariant_order, computed from the invariants alone.
///
/// The order is built greedily. Taking an invariant takes its predicates: later invariants keep only their residual,
/// the atoms whose predicate no earlier invariant took. An invariant whose residual is empty or a single atom without
/// counted variables carries no mutex information and is left out. At each step the criteria narrow the remaining
/// invariants in turn, in any composition; of those left, the first in the input is taken.

enum class InvariantOrderCriterion : std::uint8_t
{
    /// Keep the invariants that take the fewest predicates from the others: residual predicates that another
    /// remaining invariant's residual also contains.
    MinStolenPredicates,
    /// Keep the invariants whose residual has the most atoms: more atom forms in one variable, counted without
    /// assumptions about the number of objects.
    MaxResidualAtoms,
    /// Keep the invariants whose fmt text (to_string) is smallest.
    Lexicographic,
    /// Keep one invariant drawn uniformly at random with InvariantOrderOptions::random_seed.
    Random,
};

struct InvariantOrderOptions
{
    /// Applied in this order at each greedy step.
    std::vector<InvariantOrderCriterion> criteria = { InvariantOrderCriterion::MinStolenPredicates,
                                                      InvariantOrderCriterion::MaxResidualAtoms,
                                                      InvariantOrderCriterion::Lexicographic };
    std::uint64_t random_seed = 0;
};

/// Repeatedly takes the invariant the criteria choose among those whose residual carries mutex information.
InvariantViewList compute_invariant_order_greedy(const InvariantViewList& invariants, const InvariantOrderOptions& options = {});

}

#endif
