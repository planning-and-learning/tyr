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

#ifndef TYR_DATALOG_POLICIES_COST_HPP_
#define TYR_DATALOG_POLICIES_COST_HPP_

#include "tyr/datalog/declarations.hpp"
#include "tyr/datalog/policies/annotation_types.hpp"

#include <tuple>
#include <utility>
#include <yggdrasil/containers/associative_containers.hpp>
#include <yggdrasil/core/closed_interval.hpp>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/semantics/comparison.hpp>

namespace tyr::datalog
{

template<TaskKind Kind>
struct NumericTransitionCostKey : ygg::comparison::Mixin<NumericTransitionCostKey<Kind>>
{
    WitnessRuleKeyT<Kind> rule_key;
    NumericSupportKeyT<Kind> numeric_key;
    ygg::ClosedInterval<ygg::float_t> interval;

    NumericTransitionCostKey() = default;
    NumericTransitionCostKey(WitnessRuleKeyT<Kind> rule_key, NumericSupportKeyT<Kind> numeric_key, ygg::ClosedInterval<ygg::float_t> interval) :
        rule_key(rule_key),
        numeric_key(numeric_key),
        interval(interval)
    {
    }

    auto identifying_members() const noexcept { return std::make_tuple(rule_key, numeric_key, lower(interval), upper(interval)); }
};

template<TaskKind Kind>
class RuleCostPolicy
{
public:
    Cost get_cost(WitnessRuleKeyT<Kind>) const noexcept { return Cost(0); }
    Cost get_cost(WitnessRuleKeyT<Kind>, NumericSupportKeyT<Kind>, ygg::ClosedInterval<ygg::float_t>) const noexcept { return Cost(0); }

    void clear() noexcept {}
    void set_cost(WitnessRuleKeyT<Kind>, Cost) noexcept {}
    void set_cost(WitnessRuleKeyT<Kind>, NumericSupportKeyT<Kind>, ygg::ClosedInterval<ygg::float_t>, Cost) noexcept {}
};

template<TaskKind Kind>
class RuleCostOverrideStorage
{
public:
    using RuleKey = WitnessRuleKeyT<Kind>;
    using NumericKey = NumericSupportKeyT<Kind>;
    using CostMap = ygg::UnorderedMap<RuleKey, Cost>;
    using NumericTransitionCostMap = ygg::UnorderedMap<NumericTransitionCostKey<Kind>, Cost>;

    RuleCostOverrideStorage() = default;
    explicit RuleCostOverrideStorage(CostMap costs) : m_costs(std::move(costs)), m_numeric_transition_costs() {}

    Cost get_cost(RuleKey rule_key) const
    {
        if (const auto it = m_costs.find(rule_key); it != m_costs.end())
            return it->second;
        return Cost(0);
    }

    Cost get_cost(RuleKey rule_key, NumericKey numeric_key, ygg::ClosedInterval<ygg::float_t> interval) const
    {
        if (const auto it = m_numeric_transition_costs.find(NumericTransitionCostKey<Kind> { rule_key, numeric_key, interval });
            it != m_numeric_transition_costs.end())
            return it->second;
        return Cost(0);
    }

    void clear() noexcept
    {
        m_costs.clear();
        m_numeric_transition_costs.clear();
    }

    void set_cost(RuleKey rule_key, Cost cost) { m_costs.insert_or_assign(rule_key, cost); }

    void set_cost(RuleKey rule_key, NumericKey numeric_key, ygg::ClosedInterval<ygg::float_t> interval, Cost cost)
    {
        m_numeric_transition_costs.insert_or_assign(NumericTransitionCostKey<Kind> { rule_key, numeric_key, interval }, cost);
    }

    const CostMap& get_costs() const noexcept { return m_costs; }
    CostMap& get_costs() noexcept { return m_costs; }
    const NumericTransitionCostMap& get_numeric_transition_costs() const noexcept { return m_numeric_transition_costs; }

protected:
    CostMap m_costs;
    NumericTransitionCostMap m_numeric_transition_costs;
};

template<TaskKind Kind>
class RuleCostOverridePolicy : public RuleCostOverrideStorage<Kind>
{
public:
    using RuleCostOverrideStorage<Kind>::RuleCostOverrideStorage;
};

}

#endif
