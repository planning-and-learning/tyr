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
#include <yggdrasil/formalism/binding_key.hpp>
#include <yggdrasil/semantics/comparison.hpp>

namespace tyr::datalog
{

template<formalism::RelationKind R>
struct ActionCostLookup
{
    formalism::planning::ActionView<LiftedTag> action;
    formalism::datalog::RuleBindingView<R> rule_binding;

    [[nodiscard]] auto get_key() const
    {
        assert(rule_binding.get_data().size() >= action.get_arity());
        return std::make_pair(action.get_index(), std::views::take(rule_binding.get_data(), action.get_arity()));
    }
};

template<formalism::RelationKind R>
struct NumericTransitionCostKey : ygg::comparison::Mixin<NumericTransitionCostKey<R>>
{
    formalism::datalog::RuleBindingIndex<R> rule_key;
    formalism::datalog::FunctionBindingIndex<formalism::FluentTag> numeric_key;
    ygg::ClosedInterval<ygg::float_t> interval;

    NumericTransitionCostKey() = default;
    NumericTransitionCostKey(formalism::datalog::RuleBindingIndex<R> rule_key,
                             formalism::datalog::FunctionBindingIndex<formalism::FluentTag> numeric_key,
                             ygg::ClosedInterval<ygg::float_t> interval) :
        rule_key(rule_key),
        numeric_key(numeric_key),
        interval(std::move(interval))
    {
    }

    [[nodiscard]] auto identifying_members() const noexcept { return std::make_tuple(rule_key, numeric_key, lower(interval), upper(interval)); }
};

class RuleCostPolicy
{
public:
    RuleCostPolicy() = default;

    template<typename RuleKey>
    [[nodiscard]] Cost get_cost(RuleKey) const noexcept
    { return static_cast<Cost>(0); }
    template<typename RuleKey, typename NumericKey>
    [[nodiscard]] Cost get_cost(RuleKey, NumericKey, const ygg::ClosedInterval<ygg::float_t>&) const noexcept
    { return static_cast<Cost>(0); }

    void clear() noexcept {}
    template<typename RuleKey>
    void set_cost(RuleKey, Cost) noexcept
    {
    }
    template<typename RuleKey, typename NumericKey>
    void set_cost(RuleKey, NumericKey, const ygg::ClosedInterval<ygg::float_t>&, Cost) noexcept
    {
    }
};

template<formalism::RelationKind R>
class RuleCostOverrideStorage
{
    using RuleKey = formalism::datalog::RuleBindingIndex<R>;
    using NumericKey = formalism::datalog::FunctionBindingIndex<formalism::FluentTag>;
    using CostMap = ygg::UnorderedMap<RuleKey, Cost>;
    using NumericTransitionCostMap = ygg::UnorderedMap<NumericTransitionCostKey<R>, Cost>;

public:
    RuleCostOverrideStorage() = default;
    explicit RuleCostOverrideStorage(CostMap costs) : m_costs(std::move(costs)), m_numeric_transition_costs() {}

    [[nodiscard]] Cost get_cost(RuleKey rule_key) const
    {
        if (const auto it = m_costs.find(rule_key); it != m_costs.end())
            return it->second;
        return static_cast<Cost>(0);
    }

    [[nodiscard]] Cost get_cost(RuleKey rule_key, NumericKey numeric_key, ygg::ClosedInterval<ygg::float_t> interval) const
    {
        if (const auto it = m_numeric_transition_costs.find(NumericTransitionCostKey<R> { rule_key, numeric_key, std::move(interval) });
            it != m_numeric_transition_costs.end())
            return it->second;
        return static_cast<Cost>(0);
    }

    void clear() noexcept
    {
        m_costs.clear();
        m_numeric_transition_costs.clear();
    }

    void set_cost(RuleKey rule_key, Cost cost) { m_costs.insert_or_assign(rule_key, cost); }

    void set_cost(RuleKey rule_key, NumericKey numeric_key, ygg::ClosedInterval<ygg::float_t> interval, Cost cost)
    { m_numeric_transition_costs.insert_or_assign(NumericTransitionCostKey<R> { rule_key, numeric_key, std::move(interval) }, cost); }

    [[nodiscard]] const CostMap& get_costs() const noexcept { return m_costs; }
    CostMap& get_costs() noexcept { return m_costs; }
    [[nodiscard]] const NumericTransitionCostMap& get_numeric_transition_costs() const noexcept { return m_numeric_transition_costs; }

private:
    CostMap m_costs;
    NumericTransitionCostMap m_numeric_transition_costs;
};

template<TaskKind Kind>
class RuleCostOverridePolicy
{
public:
    template<formalism::RelationKind R>
    using Rule = std::conditional_t<std::same_as<Kind, GroundTag>, formalism::datalog::RuleBindingIndex<R>, ygg::Index<formalism::datalog::Rule<LiftedTag, R>>>;
    using Action = std::conditional_t<std::same_as<Kind, GroundTag>, formalism::planning::ActionView<GroundTag>, formalism::planning::ActionView<LiftedTag>>;

private:
    template<formalism::RelationKind R>
    using RuleToActionMapping = ygg::UnorderedMap<Rule<R>, Action>;

public:
    RuleCostOverridePolicy() = default;
    RuleCostOverridePolicy(const RuleToActionMapping<formalism::PredicateTag>& predicate_rule_to_action,
                           const RuleToActionMapping<formalism::FunctionTag>& function_rule_to_action) noexcept :
        m_predicate_rule_to_action(&predicate_rule_to_action),
        m_function_rule_to_action(&function_rule_to_action)
    {
    }

    /// Takes the rule binding as a view: the lifted action cost reads the binding's objects.
    template<formalism::RelationKind R>
    [[nodiscard]] Cost get_cost(formalism::datalog::RuleBindingView<R> rule_key) const
    {
        const auto& costs = storage(R {}).get_costs();
        if (const auto it = costs.find(rule_key.get_index()); it != costs.end())
            return it->second;
        return get_action_cost(rule_key);
    }

    template<formalism::RelationKind R>
    [[nodiscard]] Cost get_cost(formalism::datalog::RuleBindingIndex<R> rule_key,
                                formalism::datalog::FunctionBindingIndex<formalism::FluentTag> numeric_key,
                                ygg::ClosedInterval<ygg::float_t> interval) const
    { return storage(R {}).get_cost(rule_key, numeric_key, std::move(interval)); }

    void clear() noexcept
    {
        predicate_storage.clear();
        function_storage.clear();
        m_action_costs.clear();
    }

    void set_action_cost(formalism::planning::ActionBindingView action, Cost cost) { m_action_costs.insert_or_assign(action, cost); }

    template<formalism::RelationKind R>
    void set_cost(formalism::datalog::RuleBindingIndex<R> rule_key, Cost cost)
    { storage(R {}).set_cost(rule_key, cost); }

    template<formalism::RelationKind R>
    void set_cost(formalism::datalog::RuleBindingIndex<R> rule_key,
                  formalism::datalog::FunctionBindingIndex<formalism::FluentTag> numeric_key,
                  ygg::ClosedInterval<ygg::float_t> interval,
                  Cost cost)
    { storage(R {}).set_cost(rule_key, numeric_key, std::move(interval), cost); }

private:
    auto& storage(formalism::PredicateTag) noexcept { return predicate_storage; }
    auto& storage(formalism::FunctionTag) noexcept { return function_storage; }
    [[nodiscard]] const auto& storage(formalism::PredicateTag) const noexcept { return predicate_storage; }
    [[nodiscard]] const auto& storage(formalism::FunctionTag) const noexcept { return function_storage; }

    [[nodiscard]] const auto* mapping(formalism::PredicateTag) const noexcept { return m_predicate_rule_to_action; }
    [[nodiscard]] const auto* mapping(formalism::FunctionTag) const noexcept { return m_function_rule_to_action; }

    template<formalism::RelationKind R>
    [[nodiscard]] Cost get_action_cost(formalism::datalog::RuleBindingView<R> rule_binding) const
        requires std::same_as<Kind, GroundTag>
    {
        const auto* rule_to_action = mapping(R {});
        if (!rule_to_action)
            return static_cast<Cost>(0);

        const auto action = rule_to_action->find(rule_binding.get_index());
        if (action == rule_to_action->end())
            return static_cast<Cost>(0);
        const auto cost = m_action_costs.find(action->second.get_row());
        return cost == m_action_costs.end() ? static_cast<Cost>(0) : cost->second;
    }

    template<formalism::RelationKind R>
    [[nodiscard]] Cost get_action_cost(formalism::datalog::RuleBindingView<R> rule_binding) const
        requires std::same_as<Kind, LiftedTag>
    {
        const auto* rule_to_action = mapping(R {});
        if (!rule_to_action)
            return static_cast<Cost>(0);

        const auto action = rule_to_action->find(rule_binding.get_index().relation);
        if (action == rule_to_action->end() || rule_binding.get_data().size() < action->second.get_arity())
            return static_cast<Cost>(0);
        const auto cost = m_action_costs.find(ActionCostLookup<R> { action->second, rule_binding });
        return cost == m_action_costs.end() ? static_cast<Cost>(0) : cost->second;
    }

    RuleCostOverrideStorage<formalism::PredicateTag> predicate_storage;
    RuleCostOverrideStorage<formalism::FunctionTag> function_storage;
    const RuleToActionMapping<formalism::PredicateTag>* m_predicate_rule_to_action { nullptr };
    const RuleToActionMapping<formalism::FunctionTag>* m_function_rule_to_action { nullptr };
    ygg::UnorderedMap<formalism::planning::ActionBindingView, Cost, ygg::formalism::BindingKeyHash, ygg::formalism::BindingKeyEqualTo> m_action_costs;
};

}

#endif
