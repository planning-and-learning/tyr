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

#include "tyr/datalog/policies/cost.hpp"

#include "tyr/datalog/policies/cost_concept.hpp"

namespace tyr::datalog
{

static_assert(RuleCostPolicyConcept<RuleCostPolicy>);
static_assert(RuleCostPolicyConcept<RuleCostOverridePolicy<GroundTag>>);
static_assert(RuleCostPolicyConcept<RuleCostOverridePolicy<LiftedTag>>);

template struct NumericTransitionCostKey<formalism::PredicateTag>;
template struct NumericTransitionCostKey<formalism::FunctionTag>;
template class RuleCostOverrideStorage<formalism::PredicateTag>;
template class RuleCostOverrideStorage<formalism::FunctionTag>;
template class RuleCostOverridePolicy<GroundTag>;
template class RuleCostOverridePolicy<LiftedTag>;

}
