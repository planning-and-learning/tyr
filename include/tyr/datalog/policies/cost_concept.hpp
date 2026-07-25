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

#ifndef TYR_DATALOG_POLICIES_COST_CONCEPT_HPP_
#define TYR_DATALOG_POLICIES_COST_CONCEPT_HPP_

#include "tyr/datalog/policies/annotation_types.hpp"

#include <concepts>

namespace tyr::datalog
{

template<typename T, typename Kind>
concept RuleCostPolicyConcept = TaskKind<Kind>
                                && requires(T& policy,
                                            const T& const_policy,
                                            WitnessRuleKeyT<Kind, ::tyr::formalism::PredicateTag> predicate_rule_key,
                                            WitnessRuleKeyT<Kind, ::tyr::formalism::FunctionTag> function_rule_key,
                                            NumericSupportKeyT<Kind> numeric_key,
                                            ygg::ClosedInterval<ygg::float_t> interval,
                                            Cost cost) {
                                       { const_policy.get_cost(predicate_rule_key) } -> std::same_as<Cost>;
                                       { const_policy.get_cost(function_rule_key) } -> std::same_as<Cost>;
                                       { const_policy.get_cost(function_rule_key, numeric_key, interval) } -> std::same_as<Cost>;
                                       { policy.clear() } -> std::same_as<void>;
                                       { policy.set_cost(predicate_rule_key, cost) } -> std::same_as<void>;
                                       { policy.set_cost(function_rule_key, cost) } -> std::same_as<void>;
                                       { policy.set_cost(function_rule_key, numeric_key, interval, cost) } -> std::same_as<void>;
                                   };

template<typename T, typename Kind>
concept MutableRuleCostPolicyConcept = RuleCostPolicyConcept<T, Kind>;

}

#endif
