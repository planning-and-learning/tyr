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

namespace tyr::formalism::planning::invariant
{

/// Invariant orders for GroundTaskInstantiationOptions::invariant_order, computed from the invariants alone.
///
/// Sizes are polynomials in a common object count n: an invariant with r rigid variables has n^r instances, and its
/// residual (the parts whose predicates no earlier invariant took) has a*n + b atoms per instance, with a parts with a
/// counted variable and b parts without. Polynomials compare by degree, then coefficient. A residual without counted
/// variable and with one part yields groups of one atom; such invariants are left out.
///
/// The result depends only on the set of invariants: ties are broken by more rigid variables, rigid variables at
/// earlier argument positions, and the parts' text (predicate and constant names; MutableInvariant::canonicalize()
/// numbers the rigid variables canonically, so the order does not depend on how an implementation numbers them).

enum class GreedyCriterion
{
    /// The residual with the largest group a*n + b first, like Fast Downward's largest-first cover; ties: larger
    /// total coverage n^r * (a*n + b).
    GroupSize,
    /// The residual covering the most atoms, n^r * (a*n + b), first. Can prefer many small groups to one large group.
    TotalCoverage,
};

/// Repeatedly takes the best residual under `criterion`.
InvariantViewList compute_invariant_order_greedy(const InvariantViewList& invariants, GreedyCriterion criterion = GreedyCriterion::GroupSize);

/// FDR variables counted below: n^r per used invariant (groups of a*n + b atoms) plus n^arity per predicate left
/// binary (groups of one atom).
enum class ExhaustiveObjective
{
    /// Fewest FDR variables, binary ones included (the set cover Fast Downward approximates); ties: LargestVariables.
    FewestVariables,
    /// Largest FDR variables: the larger largest group, then more groups of that size, then the next size down to the
    /// binary variables; ties: fewer FDR variables.
    LargestVariables,
    /// Fewest atoms left binary, then fewest invariant variables. Can pick many small groups to avoid few binary atoms.
    Coverage,
};

/// The order optimizing `objective` over all predicate -> {invariant | binary} assignments that an order induces;
/// remaining ties are broken as described above. Predicates that share no invariant are decided independently; a
/// component with more than a million assignments falls back to the greedy order (GroupSize, or TotalCoverage for
/// Coverage).
InvariantViewList compute_invariant_order_exhaustive(const InvariantViewList& invariants, ExhaustiveObjective objective = ExhaustiveObjective::FewestVariables);

}

#endif
