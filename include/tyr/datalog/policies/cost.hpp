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
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/planning/action_view.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/repository.hpp"

#include <cassert>
#include <gtl/phmap.hpp>
#include <ranges>
#include <span>
#include <tuple>
#include <urd/elements/equal_to.hpp>
#include <urd/elements/hash.hpp>
#include <urd/repr/maps/std_map.hpp>
#include <urd/repr/maps/unordered_flat_map.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/repr/values/unordered_flat_set.hpp>
#include <urd/store.hpp>
#include <utility>
#include <vector>
#include <yggdrasil/core/closed_interval.hpp>
#include <yggdrasil/core/config.hpp>

namespace tyr::datalog
{

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

template<TaskKind Kind>
class RuleCostOverrideStorage
{
    using CostMap = urd::Store<urd::map<RuleKey<Kind>, Cost>>;

public:
    RuleCostOverrideStorage() = default;

    explicit RuleCostOverrideStorage(CostMap costs) : m_costs(std::move(costs)) {}

    [[nodiscard]] Cost get_cost(RuleKey<Kind> rule_key) const
    {
        if (const auto it = m_costs.find(rule_key); it != m_costs.end())
            return it->second;
        return static_cast<Cost>(0);
    }

    void clear() noexcept { m_costs.clear(); }

    void set_cost(RuleKey<Kind> rule_key, Cost cost) { m_costs.insert_or_assign(rule_key, cost); }

    [[nodiscard]] const CostMap& get_costs() const noexcept { return m_costs; }

    CostMap& get_costs() noexcept { return m_costs; }

private:
    CostMap m_costs;
};

template<TaskKind Kind>
class RuleCostOverridePolicy
{
public:
    /// The rule an action is translated to: actions are mapped by rule, a lifted rule's by its rule alone.
    using Rule = urd::index<formalism::Rule<Kind>>;
    /// What the credit of a rule instance is read from: a ground rule's key; a lifted instance's row as a view, since
    /// its action cost reads the row's objects.
    using RuleInstanceKey = std::conditional_t<std::same_as<Kind, GroundTag>, RuleKey<GroundTag>, formalism::RuleRowView>;
    using Action = std::conditional_t<std::same_as<Kind, GroundTag>, formalism::ActionView<GroundTag>, formalism::ActionView<LiftedTag>>;

private:
    using RuleToActionMapping = urd::Store<urd::map<Rule, urd::Store<urd::sequence<Action>>>>;

public:
    RuleCostOverridePolicy() = default;

    explicit RuleCostOverridePolicy(const RuleToActionMapping& predicate_rule_to_action) noexcept : m_predicate_rule_to_action(&predicate_rule_to_action) {}

    [[nodiscard]] Cost get_cost(RuleInstanceKey rule_key) const
    {
        const auto& costs = storage().get_costs();
        if (const auto it = costs.find(key_of(rule_key)); it != costs.end())
            return it->second;
        return get_action_cost(rule_key);
    }

    void clear() noexcept
    {
        predicate_storage.clear();
        m_action_costs.clear();
        m_action_repository = nullptr;
    }

    /// The cost of an action's binding: a row of the task's repository, which every binding given is a row of.
    void set_action_cost(formalism::ActionRowView action, Cost cost)
    {
        assert(!m_action_repository || m_action_repository == &action.context());
        m_action_repository = &action.context();
        m_action_costs.insert_or_assign(action, cost);
    }

    void set_cost(RuleKey<Kind> rule_key, Cost cost) { storage().set_cost(rule_key, cost); }

private:
    static RuleKey<Kind> key_of(RuleInstanceKey rule_key) noexcept
    {
        if constexpr (std::same_as<Kind, GroundTag>)
            return rule_key;
        else
            return rule_key.index();
    }

    auto& storage() noexcept { return predicate_storage; }

    [[nodiscard]] const auto& storage() const noexcept { return predicate_storage; }

    [[nodiscard]] const auto* mapping() const noexcept { return m_predicate_rule_to_action; }

    /// The credit of a rule is the largest credit of its actions: the rule is the cheapest of their parallel arcs.
    [[nodiscard]] Cost get_action_cost(RuleKey<GroundTag> rule) const
        requires std::same_as<Kind, GroundTag>
    {
        const auto* rule_to_action = mapping();
        if (!rule_to_action)
            return static_cast<Cost>(0);

        const auto actions = rule_to_action->find(rule);
        if (actions == rule_to_action->end())
            return static_cast<Cost>(0);
        auto credit = static_cast<Cost>(0);
        for (const auto action : actions->second)
            if (const auto cost = m_action_costs.find(action.get_row()); cost != m_action_costs.end())
                credit = std::max(credit, cost->second);
        return credit;
    }

    [[nodiscard]] Cost get_action_cost(formalism::RuleRowView rule_binding) const
        requires std::same_as<Kind, LiftedTag>
    {
        const auto* rule_to_action = mapping();
        if (!rule_to_action)
            return static_cast<Cost>(0);

        const auto actions = rule_to_action->find(rule_binding.index().relation);
        if (actions == rule_to_action->end())
            return static_cast<Cost>(0);
        if (m_action_costs.empty())
            return static_cast<Cost>(0);
        // An action's binding is the first values of the rule's row, its arity many: the row of the task's repository
        // that holds them (if it holds them, its cost may be set), by its identity.
        const auto values = rule_binding.get_values();
        auto credit = static_cast<Cost>(0);
        for (const auto action : actions->second)
        {
            const auto arity = action.get_arity();
            if (values.size() < arity)
                continue;
            m_binding.assign(values.begin(), values.begin() + static_cast<std::ptrdiff_t>(arity));
            const auto row = m_action_repository->find_row(action.index(), std::span<const ygg::Value>(m_binding));
            if (row.is_none())
                continue;
            const auto binding = m_action_repository->view(ygg::RowIndex<formalism::Action<LiftedTag>> { action.index(), row });
            if (const auto cost = m_action_costs.find(binding); cost != m_action_costs.end())
                credit = std::max(credit, cost->second);
        }
        return credit;
    }

    RuleCostOverrideStorage<Kind> predicate_storage;
    const RuleToActionMapping* m_predicate_rule_to_action { nullptr };
    /// The costs of action bindings, by their identity; the repository they are rows of (none before the first).
    gtl::flat_hash_map<formalism::ActionRowView, Cost, urd::Hash<formalism::ActionRowView>, urd::EqualTo<formalism::ActionRowView>> m_action_costs;
    const formalism::Repository* m_action_repository { nullptr };
    /// The binding of a lookup, written here (reused: it allocates once grown).
    mutable std::vector<ygg::Value> m_binding;
};

}

#endif
