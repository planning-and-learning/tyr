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

#include "tyr/datalog/lifted/policies/cost.hpp"

#include <algorithm>
#include <utility>

namespace tyr::datalog
{
template<::tyr::formalism::RelationKind R>
Cost RuleCostOverridePolicy<LiftedTag>::get_cost(::tyr::formalism::datalog::RuleBindingView<R> rule_binding) const
{
    const auto& rule_storage = storage(R {});
    if (const auto it = rule_storage.get_costs().find(rule_binding); it != rule_storage.get_costs().end())
        return it->second;
    if (const auto* cost = find_prefix_override(rule_binding))
        return *cost;
    return Cost(0);
}

template<::tyr::formalism::RelationKind R>
Cost RuleCostOverridePolicy<LiftedTag>::get_cost(::tyr::formalism::datalog::RuleBindingView<R> rule_binding,
                                                 ::tyr::formalism::datalog::FunctionBindingView<::tyr::formalism::FluentTag> binding,
                                                 ygg::ClosedInterval<ygg::float_t> interval) const
{
    const auto& rule_storage = storage(R {});
    const auto key = NumericTransitionCostKey<LiftedTag, R> { rule_binding, binding, interval };
    if (const auto it = rule_storage.get_numeric_transition_costs().find(key); it != rule_storage.get_numeric_transition_costs().end())
        return it->second;
    return Cost(0);
}

void RuleCostOverridePolicy<LiftedTag>::clear() noexcept
{
    m_predicate_storage.clear();
    m_function_storage.clear();
    m_predicate_prefix_costs.clear();
    m_function_prefix_costs.clear();
}

template<::tyr::formalism::RelationKind R>
void RuleCostOverridePolicy<LiftedTag>::set_prefix_cost(::tyr::formalism::datalog::RuleView<R> rule,
                                                        std::span<const ygg::Index<::tyr::formalism::Object>> objects,
                                                        Cost cost)
{
    auto prefix_cost = PrefixCost<R> { rule, ygg::IndexList<::tyr::formalism::Object>(objects.begin(), objects.end()), cost };
    auto& costs = prefix_costs(R {});
    for (auto& existing : costs)
    {
        if (existing.rule.get_index() == prefix_cost.rule.get_index() && existing.objects == prefix_cost.objects)
        {
            existing.cost = cost;
            return;
        }
    }
    costs.push_back(std::move(prefix_cost));
}

template<::tyr::formalism::RelationKind R>
const Cost* RuleCostOverridePolicy<LiftedTag>::find_prefix_override(::tyr::formalism::datalog::RuleBindingView<R> rule_binding) const
{
    const auto objects = rule_binding.get_data();
    for (const auto& prefix_cost : prefix_costs(R {}))
    {
        if (prefix_cost.rule.get_index() == rule_binding.get_relation().get_index() && prefix_cost.objects.size() <= objects.size()
            && std::equal(prefix_cost.objects.begin(), prefix_cost.objects.end(), objects.begin()))
            return &prefix_cost.cost;
    }
    return nullptr;
}

template Cost RuleCostOverridePolicy<LiftedTag>::get_cost(::tyr::formalism::datalog::RuleBindingView<::tyr::formalism::PredicateTag>) const;
template Cost RuleCostOverridePolicy<LiftedTag>::get_cost(::tyr::formalism::datalog::RuleBindingView<::tyr::formalism::FunctionTag>) const;
template Cost RuleCostOverridePolicy<LiftedTag>::get_cost(::tyr::formalism::datalog::RuleBindingView<::tyr::formalism::PredicateTag>,
                                                          ::tyr::formalism::datalog::FunctionBindingView<::tyr::formalism::FluentTag>,
                                                          ygg::ClosedInterval<ygg::float_t>) const;
template Cost RuleCostOverridePolicy<LiftedTag>::get_cost(::tyr::formalism::datalog::RuleBindingView<::tyr::formalism::FunctionTag>,
                                                          ::tyr::formalism::datalog::FunctionBindingView<::tyr::formalism::FluentTag>,
                                                          ygg::ClosedInterval<ygg::float_t>) const;
template void RuleCostOverridePolicy<LiftedTag>::set_prefix_cost(::tyr::formalism::datalog::RuleView<::tyr::formalism::PredicateTag>,
                                                                 std::span<const ygg::Index<::tyr::formalism::Object>>,
                                                                 Cost);
template void RuleCostOverridePolicy<LiftedTag>::set_prefix_cost(::tyr::formalism::datalog::RuleView<::tyr::formalism::FunctionTag>,
                                                                 std::span<const ygg::Index<::tyr::formalism::Object>>,
                                                                 Cost);

}
