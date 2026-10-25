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

namespace tyr::formalism::planning::invariant
{

/// Invariant orders for GroundTaskInstantiationOptions::invariant_order, computed from the invariants alone.
///
/// Sizes are estimated under uniform object-domain growth, using a common object count n. Each residual atom (whose
/// predicate no earlier invariant took) contributes n^k, where k is its number of distinct counted variables, including
/// 1 when k is zero. Their sum G(n) estimates the group size; an invariant with r rigid variables has estimated coverage
/// n^r * G(n). Polynomials compare asymptotically by degree, then coefficients from highest to lowest degree. Empty
/// residuals and constant singleton groups carry no mutex information and are left out.
///
/// The result depends only on the set of invariants, not on their order: ties go to the invariant interned first.

enum class GreedyCriterion : std::uint8_t
{
    /// The residual with the largest estimated group G(n) first, a lifted proxy for Fast Downward's largest-first
    /// ground mutex-group cover: prefer more atoms per FDR variable. Ties: larger estimated coverage n^r * G(n).
    MaxGroupSize,
    /// The residual with the largest estimated coverage n^r * G(n) first. Can prefer many small groups to one large group.
    MaxCoverage,
};

/// Repeatedly takes the best residual under `criterion`.
InvariantViewList compute_invariant_order_greedy(const InvariantViewList& invariants, GreedyCriterion criterion = GreedyCriterion::MaxGroupSize);

}

#endif
