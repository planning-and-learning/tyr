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
#include "tyr/formalism/planning/repository.hpp"

#include <cassert>
#include <ranges>
#include <tuple>
#include <utility>
#include <yggdrasil/containers/associative_containers.hpp>
#include <yggdrasil/containers/unordered_set.hpp>
#include <yggdrasil/core/closed_interval.hpp>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/semantics/comparison.hpp>

namespace tyr::datalog
{

template<::tyr::formalism::RelationKind R>
struct ActionCostLookup
{
    ::tyr::formalism::planning::ActionView<::tyr::LiftedTag> action;
    ::tyr::formalism::datalog::RuleBindingView<R> rule_binding;

    auto get_key() const noexcept
    {
        assert(rule_binding.get_data().size() >= action.get_arity());
        return std::make_pair(action.get_index(), std::views::take(rule_binding.get_data(), action.get_arity()));
    }
};

struct ActionCostHash
{
    using is_transparent = void;

    template<typename Key>
    size_t operator()(const Key& key) const noexcept
    {
        const auto [relation, objects] = key.get_key();
        return ygg::hash_combine(relation, ygg::hash_range(objects));
    }
};

struct ActionCostEqual
{
    using is_transparent = void;

    template<typename Lhs, typename Rhs>
    bool operator()(const Lhs& lhs, const Rhs& rhs) const noexcept
    {
        const auto lhs_key = lhs.get_key();
        const auto rhs_key = rhs.get_key();
        return ygg::EqualTo<> {}(lhs_key.first, rhs_key.first) && ygg::equal_range(lhs_key.second, rhs_key.second);
    }
};

template<::tyr::formalism::RelationKind R>
struct NumericTransitionCostKey : ygg::comparison::Mixin<NumericTransitionCostKey<R>>
{
    ::tyr::formalism::datalog::RuleBindingView<R> rule_key;
    ::tyr::formalism::datalog::FunctionBindingView<::tyr::formalism::FluentTag> numeric_key;
    ygg::ClosedInterval<ygg::float_t> interval;

    NumericTransitionCostKey() = default;
    NumericTransitionCostKey(::tyr::formalism::datalog::RuleBindingView<R> rule_key,
                             ::tyr::formalism::datalog::FunctionBindingView<::tyr::formalism::FluentTag> numeric_key,
                             ygg::ClosedInterval<ygg::float_t> interval) :
        rule_key(rule_key),
        numeric_key(numeric_key),
        interval(interval)
    {
    }

    auto identifying_members() const noexcept { return std::make_tuple(rule_key, numeric_key, lower(interval), upper(interval)); }
};

class RuleCostPolicy
{
public:
    using MetricTarget = ygg::Index<::tyr::formalism::RelationBinding<::tyr::formalism::Function<::tyr::formalism::FluentTag>>>;
    using MetricTargets = ygg::UnorderedSet<MetricTarget>;

    RuleCostPolicy() = default;
    explicit RuleCostPolicy(const MetricTargets* metric_targets) noexcept : m_metric_targets(metric_targets) {}

    template<typename RuleKey>
    Cost get_cost(RuleKey) const noexcept
    {
        return Cost(0);
    }
    template<typename RuleKey>
    Cost get_cost(RuleKey, ::tyr::formalism::datalog::FunctionBindingView<::tyr::formalism::FluentTag>, ygg::ClosedInterval<ygg::float_t>) const noexcept
    {
        return Cost(0);
    }

    bool is_metric_target(::tyr::formalism::datalog::FunctionBindingView<::tyr::formalism::FluentTag> binding) const noexcept
    {
        return !m_metric_targets || m_metric_targets->contains(binding.get_index());
    }

    void clear() noexcept {}
    template<typename RuleKey>
    void set_cost(RuleKey, Cost) noexcept
    {
    }
    template<typename RuleKey>
    void set_cost(RuleKey, ::tyr::formalism::datalog::FunctionBindingView<::tyr::formalism::FluentTag>, ygg::ClosedInterval<ygg::float_t>, Cost) noexcept
    {
    }

private:
    const MetricTargets* m_metric_targets { nullptr };
};

template<::tyr::formalism::RelationKind R>
class RuleCostOverrideStorage
{
public:
    using RuleKey = ::tyr::formalism::datalog::RuleBindingView<R>;
    using NumericKey = ::tyr::formalism::datalog::FunctionBindingView<::tyr::formalism::FluentTag>;
    using CostMap = ygg::UnorderedMap<RuleKey, Cost>;
    using NumericTransitionCostMap = ygg::UnorderedMap<NumericTransitionCostKey<R>, Cost>;

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
        if (const auto it = m_numeric_transition_costs.find(NumericTransitionCostKey<R> { rule_key, numeric_key, interval });
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
        m_numeric_transition_costs.insert_or_assign(NumericTransitionCostKey<R> { rule_key, numeric_key, interval }, cost);
    }

    const CostMap& get_costs() const noexcept { return m_costs; }
    CostMap& get_costs() noexcept { return m_costs; }
    const NumericTransitionCostMap& get_numeric_transition_costs() const noexcept { return m_numeric_transition_costs; }

protected:
    CostMap m_costs;
    NumericTransitionCostMap m_numeric_transition_costs;
};

template<TaskKind Kind>
class RuleCostOverridePolicy : public RuleCostPolicy
{
public:
    using PredicateRuleKey = ::tyr::formalism::datalog::RuleBindingView<::tyr::formalism::PredicateTag>;
    using FunctionRuleKey = ::tyr::formalism::datalog::RuleBindingView<::tyr::formalism::FunctionTag>;

    template<::tyr::formalism::RelationKind R>
    using Rule = std::conditional_t<std::same_as<Kind, GroundTag>, ::tyr::formalism::datalog::RuleBindingView<R>, ::tyr::formalism::datalog::RuleView<::tyr::LiftedTag, R>>;
    using Action = std::conditional_t<std::same_as<Kind, GroundTag>, ::tyr::formalism::planning::ActionView<::tyr::GroundTag>, ::tyr::formalism::planning::ActionView<::tyr::LiftedTag>>;
    template<::tyr::formalism::RelationKind R>
    using RuleToActionMapping = ygg::UnorderedMap<Rule<R>, Action>;

    RuleCostOverridePolicy() = default;
    using RuleCostPolicy::RuleCostPolicy;
    RuleCostOverridePolicy(const RuleToActionMapping<::tyr::formalism::PredicateTag>& predicate_rule_to_action,
                           const RuleToActionMapping<::tyr::formalism::FunctionTag>& function_rule_to_action) noexcept :
        m_predicate_rule_to_action(&predicate_rule_to_action),
        m_function_rule_to_action(&function_rule_to_action)
    {
    }
    RuleCostOverridePolicy(const RuleToActionMapping<::tyr::formalism::PredicateTag>& predicate_rule_to_action,
                           const RuleToActionMapping<::tyr::formalism::FunctionTag>& function_rule_to_action,
                           const MetricTargets* metric_targets) noexcept :
        RuleCostPolicy(metric_targets),
        m_predicate_rule_to_action(&predicate_rule_to_action),
        m_function_rule_to_action(&function_rule_to_action)
    {
    }

    template<typename RuleKey>
    Cost get_cost(RuleKey rule_key) const
    {
        const auto& costs = storage_for(rule_key).get_costs();
        if (const auto it = costs.find(rule_key); it != costs.end())
            return it->second;
        return get_action_cost(rule_key);
    }

    template<typename RuleKey>
    Cost get_cost(RuleKey rule_key,
                  ::tyr::formalism::datalog::FunctionBindingView<::tyr::formalism::FluentTag> numeric_key,
                  ygg::ClosedInterval<ygg::float_t> interval) const
    {
        return storage_for(rule_key).get_cost(rule_key, numeric_key, interval);
    }

    void clear() noexcept
    {
        predicate_storage.clear();
        function_storage.clear();
        m_action_costs.clear();
    }

    void set_action_cost(::tyr::formalism::planning::ActionBindingView action, Cost cost) { m_action_costs.insert_or_assign(action, cost); }

    template<typename RuleKey>
    void set_cost(RuleKey rule_key, Cost cost)
    {
        storage_for(rule_key).set_cost(rule_key, cost);
    }

    template<typename RuleKey>
    void set_cost(RuleKey rule_key,
                  ::tyr::formalism::datalog::FunctionBindingView<::tyr::formalism::FluentTag> numeric_key,
                  ygg::ClosedInterval<ygg::float_t> interval,
                  Cost cost)
    {
        storage_for(rule_key).set_cost(rule_key, numeric_key, interval, cost);
    }

protected:
    auto& storage(::tyr::formalism::PredicateTag) noexcept { return predicate_storage; }
    auto& storage(::tyr::formalism::FunctionTag) noexcept { return function_storage; }
    const auto& storage(::tyr::formalism::PredicateTag) const noexcept { return predicate_storage; }
    const auto& storage(::tyr::formalism::FunctionTag) const noexcept { return function_storage; }

    auto& storage_for(::tyr::formalism::datalog::RuleBindingView<::tyr::formalism::PredicateTag>) noexcept { return predicate_storage; }
    auto& storage_for(::tyr::formalism::datalog::RuleBindingView<::tyr::formalism::FunctionTag>) noexcept { return function_storage; }
    const auto& storage_for(::tyr::formalism::datalog::RuleBindingView<::tyr::formalism::PredicateTag>) const noexcept { return predicate_storage; }
    const auto& storage_for(::tyr::formalism::datalog::RuleBindingView<::tyr::formalism::FunctionTag>) const noexcept { return function_storage; }

    const auto* mapping(::tyr::formalism::PredicateTag) const noexcept { return m_predicate_rule_to_action; }
    const auto* mapping(::tyr::formalism::FunctionTag) const noexcept { return m_function_rule_to_action; }

    template<::tyr::formalism::RelationKind R>
    Cost get_action_cost(::tyr::formalism::datalog::RuleBindingView<R> rule_binding) const
        requires std::same_as<Kind, GroundTag>
    {
        const auto* rule_to_action = mapping(R {});
        if (!rule_to_action)
            return Cost(0);

        const auto action = rule_to_action->find(rule_binding);
        if (action == rule_to_action->end())
            return Cost(0);
        const auto cost = m_action_costs.find(action->second.get_row());
        return cost == m_action_costs.end() ? Cost(0) : cost->second;
    }

    template<::tyr::formalism::RelationKind R>
    Cost get_action_cost(::tyr::formalism::datalog::RuleBindingView<R> rule_binding) const
        requires std::same_as<Kind, LiftedTag>
    {
        const auto* rule_to_action = mapping(R {});
        if (!rule_to_action)
            return Cost(0);

        const auto action = rule_to_action->find(rule_binding.get_relation());
        if (action == rule_to_action->end() || rule_binding.get_data().size() < action->second.get_arity())
            return Cost(0);
        const auto cost = m_action_costs.find(ActionCostLookup<R> { action->second, rule_binding });
        return cost == m_action_costs.end() ? Cost(0) : cost->second;
    }

    RuleCostOverrideStorage<::tyr::formalism::PredicateTag> predicate_storage;
    RuleCostOverrideStorage<::tyr::formalism::FunctionTag> function_storage;
    const RuleToActionMapping<::tyr::formalism::PredicateTag>* m_predicate_rule_to_action { nullptr };
    const RuleToActionMapping<::tyr::formalism::FunctionTag>* m_function_rule_to_action { nullptr };
    gtl::flat_hash_map<::tyr::formalism::planning::ActionBindingView, Cost, ActionCostHash, ActionCostEqual> m_action_costs;
};

}

#endif
