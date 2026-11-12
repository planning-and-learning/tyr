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
#include <gtl/phmap.hpp>
#include <ranges>
#include <tuple>
#include <urd/repr/maps/std_map.hpp>
#include <urd/repr/maps/unordered_flat_map.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/repr/values/unordered_flat_set.hpp>
#include <urd/store.hpp>
#include <utility>
#include <yggdrasil/core/closed_interval.hpp>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/formalism/row_key.hpp>

namespace tyr::datalog
{

struct ActionCostLookup
{
    formalism::planning::ActionView<LiftedTag> action;
    formalism::datalog::RuleRowView rule_binding;

    [[nodiscard]] auto get_key() const
    {
        const auto objects = rule_binding.get_key().second;
        assert(objects.size() >= action.get_arity());
        // A subrange of the stored row's iterators, which hold the row: it outlives `objects`.
        return std::make_pair(action.index(), std::ranges::subrange(objects.begin(), objects.begin() + static_cast<std::ptrdiff_t>(action.get_arity())));
    }
};

class RuleCostPolicy
{
public:
    RuleCostPolicy() = default;

    template<typename RuleKey>
    [[nodiscard]] Cost get_cost(RuleKey) const noexcept
    { return static_cast<Cost>(0); }

    void clear() noexcept {}
    template<typename RuleKey>
    void set_cost(RuleKey, Cost) noexcept
    {
    }
};

class RuleCostOverrideStorage
{
    using RuleKey = formalism::datalog::RuleRowIndex;
    using CostMap = urd::Store<urd::map<RuleKey, Cost>>;

public:
    RuleCostOverrideStorage() = default;
    explicit RuleCostOverrideStorage(CostMap costs) : m_costs(std::move(costs)) {}

    [[nodiscard]] Cost get_cost(RuleKey rule_key) const
    {
        if (const auto it = m_costs.find(rule_key); it != m_costs.end())
            return it->second;
        return static_cast<Cost>(0);
    }

    void clear() noexcept { m_costs.clear(); }

    void set_cost(RuleKey rule_key, Cost cost) { m_costs.insert_or_assign(rule_key, cost); }

    [[nodiscard]] const CostMap& get_costs() const noexcept { return m_costs; }
    CostMap& get_costs() noexcept { return m_costs; }

private:
    CostMap m_costs;
};

template<TaskKind Kind>
class RuleCostOverridePolicy
{
public:
    using Rule = std::conditional_t<std::same_as<Kind, GroundTag>, formalism::datalog::RuleRowIndex, urd::index<formalism::datalog::Rule<LiftedTag>>>;
    using Action = std::conditional_t<std::same_as<Kind, GroundTag>, formalism::planning::ActionView<GroundTag>, formalism::planning::ActionView<LiftedTag>>;

private:
    using RuleToActionMapping = urd::Store<urd::map<Rule, urd::Store<urd::sequence<Action>>>>;

public:
    RuleCostOverridePolicy() = default;
    explicit RuleCostOverridePolicy(const RuleToActionMapping& predicate_rule_to_action) noexcept : m_predicate_rule_to_action(&predicate_rule_to_action) {}

    /// Takes the rule binding as a view: the lifted action cost reads the binding's objects.
    [[nodiscard]] Cost get_cost(formalism::datalog::RuleRowView rule_key) const
    {
        const auto& costs = storage().get_costs();
        if (const auto it = costs.find(rule_key.data()); it != costs.end())
            return it->second;
        return get_action_cost(rule_key);
    }

    void clear() noexcept
    {
        predicate_storage.clear();
        m_action_costs.clear();
    }

    void set_action_cost(formalism::planning::ActionRowView action, Cost cost) { m_action_costs.insert_or_assign(action, cost); }

    void set_cost(formalism::datalog::RuleRowIndex rule_key, Cost cost) { storage().set_cost(rule_key, cost); }

private:
    auto& storage() noexcept { return predicate_storage; }
    [[nodiscard]] const auto& storage() const noexcept { return predicate_storage; }

    [[nodiscard]] const auto* mapping() const noexcept { return m_predicate_rule_to_action; }

    /// The credit of a rule is the largest credit of its actions: the rule is the cheapest of their parallel arcs.
    [[nodiscard]] Cost get_action_cost(formalism::datalog::RuleRowView rule_binding) const
        requires std::same_as<Kind, GroundTag>
    {
        const auto* rule_to_action = mapping();
        if (!rule_to_action)
            return static_cast<Cost>(0);

        const auto actions = rule_to_action->find(rule_binding.data());
        if (actions == rule_to_action->end())
            return static_cast<Cost>(0);
        auto credit = static_cast<Cost>(0);
        for (const auto action : actions->second)
            if (const auto cost = m_action_costs.find(action.get_row()); cost != m_action_costs.end())
                credit = std::max(credit, cost->second);
        return credit;
    }

    [[nodiscard]] Cost get_action_cost(formalism::datalog::RuleRowView rule_binding) const
        requires std::same_as<Kind, LiftedTag>
    {
        const auto* rule_to_action = mapping();
        if (!rule_to_action)
            return static_cast<Cost>(0);

        const auto actions = rule_to_action->find(rule_binding.data().relation);
        if (actions == rule_to_action->end())
            return static_cast<Cost>(0);
        auto credit = static_cast<Cost>(0);
        for (const auto action : actions->second)
            if (rule_binding.size() >= action.get_arity())
                if (const auto cost = m_action_costs.find(ActionCostLookup { action, rule_binding }); cost != m_action_costs.end())
                    credit = std::max(credit, cost->second);
        return credit;
    }

    RuleCostOverrideStorage predicate_storage;
    const RuleToActionMapping* m_predicate_rule_to_action { nullptr };
    gtl::flat_hash_map<formalism::planning::ActionRowView, Cost, ygg::RowKeyHash, ygg::RowKeyEqualTo> m_action_costs;
};

}

#endif
