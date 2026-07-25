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

template<TaskKind Kind, ::tyr::formalism::RelationKind R>
struct NumericTransitionCostKey : ygg::comparison::Mixin<NumericTransitionCostKey<Kind, R>>
{
    WitnessRuleKeyT<Kind, R> rule_key;
    NumericSupportKeyT<Kind> numeric_key;
    ygg::ClosedInterval<ygg::float_t> interval;

    NumericTransitionCostKey() = default;
    NumericTransitionCostKey(WitnessRuleKeyT<Kind, R> rule_key, NumericSupportKeyT<Kind> numeric_key, ygg::ClosedInterval<ygg::float_t> interval) :
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
    template<typename RuleKey>
    Cost get_cost(RuleKey) const noexcept
    {
        return Cost(0);
    }
    template<typename RuleKey>
    Cost get_cost(RuleKey, NumericSupportKeyT<Kind>, ygg::ClosedInterval<ygg::float_t>) const noexcept
    {
        return Cost(0);
    }

    void clear() noexcept {}
    template<typename RuleKey>
    void set_cost(RuleKey, Cost) noexcept
    {
    }
    template<typename RuleKey>
    void set_cost(RuleKey, NumericSupportKeyT<Kind>, ygg::ClosedInterval<ygg::float_t>, Cost) noexcept
    {
    }
};

template<TaskKind Kind, ::tyr::formalism::RelationKind R>
class RuleCostOverrideStorage
{
public:
    using RuleKey = WitnessRuleKeyT<Kind, R>;
    using NumericKey = NumericSupportKeyT<Kind>;
    using CostMap = ygg::UnorderedMap<RuleKey, Cost>;
    using NumericTransitionCostMap = ygg::UnorderedMap<NumericTransitionCostKey<Kind, R>, Cost>;

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
        if (const auto it = m_numeric_transition_costs.find(NumericTransitionCostKey<Kind, R> { rule_key, numeric_key, interval });
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
        m_numeric_transition_costs.insert_or_assign(NumericTransitionCostKey<Kind, R> { rule_key, numeric_key, interval }, cost);
    }

    const CostMap& get_costs() const noexcept { return m_costs; }
    CostMap& get_costs() noexcept { return m_costs; }
    const NumericTransitionCostMap& get_numeric_transition_costs() const noexcept { return m_numeric_transition_costs; }

protected:
    CostMap m_costs;
    NumericTransitionCostMap m_numeric_transition_costs;
};

template<TaskKind Kind>
class RuleCostOverridePolicy
{
public:
    template<typename RuleKey>
    Cost get_cost(RuleKey rule_key) const
    {
        return storage_for(rule_key).get_cost(rule_key);
    }

    template<typename RuleKey>
    Cost get_cost(RuleKey rule_key, NumericSupportKeyT<Kind> numeric_key, ygg::ClosedInterval<ygg::float_t> interval) const
    {
        return storage_for(rule_key).get_cost(rule_key, numeric_key, interval);
    }

    void clear() noexcept
    {
        predicate_storage.clear();
        function_storage.clear();
    }

    template<typename RuleKey>
    void set_cost(RuleKey rule_key, Cost cost)
    {
        storage_for(rule_key).set_cost(rule_key, cost);
    }

    template<typename RuleKey>
    void set_cost(RuleKey rule_key, NumericSupportKeyT<Kind> numeric_key, ygg::ClosedInterval<ygg::float_t> interval, Cost cost)
    {
        storage_for(rule_key).set_cost(rule_key, numeric_key, interval, cost);
    }

protected:
    auto& storage(::tyr::formalism::PredicateTag) noexcept { return predicate_storage; }
    auto& storage(::tyr::formalism::FunctionTag) noexcept { return function_storage; }
    const auto& storage(::tyr::formalism::PredicateTag) const noexcept { return predicate_storage; }
    const auto& storage(::tyr::formalism::FunctionTag) const noexcept { return function_storage; }

    auto& storage_for(WitnessRuleKeyT<Kind, ::tyr::formalism::PredicateTag>) noexcept { return predicate_storage; }
    auto& storage_for(WitnessRuleKeyT<Kind, ::tyr::formalism::FunctionTag>) noexcept { return function_storage; }
    const auto& storage_for(WitnessRuleKeyT<Kind, ::tyr::formalism::PredicateTag>) const noexcept { return predicate_storage; }
    const auto& storage_for(WitnessRuleKeyT<Kind, ::tyr::formalism::FunctionTag>) const noexcept { return function_storage; }

    RuleCostOverrideStorage<Kind, ::tyr::formalism::PredicateTag> predicate_storage;
    RuleCostOverrideStorage<Kind, ::tyr::formalism::FunctionTag> function_storage;
};

}

#endif
