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

#ifndef TYR_DATALOG_LIFTED_POLICIES_COST_HPP_
#define TYR_DATALOG_LIFTED_POLICIES_COST_HPP_

#include "tyr/datalog/declarations.hpp"
#include "tyr/datalog/lifted/policies/annotation_types.hpp"
#include "tyr/datalog/policies/cost.hpp"
#include "tyr/datalog/policies/cost_concept.hpp"
#include "tyr/formalism/datalog/grounder.hpp"
#include "tyr/formalism/datalog/repository.hpp"

#include <cassert>
#include <span>
#include <yggdrasil/containers/vector.hpp>
#include <yggdrasil/core/closed_interval.hpp>
#include <yggdrasil/core/config.hpp>

namespace tyr::datalog
{

template<>
class RuleCostOverridePolicy<LiftedTag>
{
public:
    RuleCostOverridePolicy() = default;

    template<::tyr::formalism::RelationKind R>
    Cost get_cost(::tyr::formalism::datalog::RuleBindingView<R> rule_binding) const;

    template<::tyr::formalism::RelationKind R>
    Cost get_cost(::tyr::formalism::datalog::RuleBindingView<R> rule_binding,
                  ::tyr::formalism::datalog::FunctionBindingView<::tyr::formalism::FluentTag> binding,
                  ygg::ClosedInterval<ygg::float_t> interval) const;

    void clear() noexcept;

    template<::tyr::formalism::RelationKind R>
    void set_cost(::tyr::formalism::datalog::RuleBindingView<R> rule_binding, Cost cost)
    {
        storage(R {}).set_cost(rule_binding, cost);
    }

    template<::tyr::formalism::RelationKind R>
    void set_cost(::tyr::formalism::datalog::RuleBindingView<R> rule_binding,
                  ::tyr::formalism::datalog::FunctionBindingView<::tyr::formalism::FluentTag> binding,
                  ygg::ClosedInterval<ygg::float_t> interval,
                  Cost cost)
    {
        storage(R {}).set_cost(rule_binding, binding, interval, cost);
    }

    template<::tyr::formalism::RelationKind R>
    void set_prefix_cost(::tyr::formalism::datalog::RuleView<R> rule, std::span<const ygg::Index<::tyr::formalism::Object>> objects, Cost cost);

    size_t get_num_prefix_costs() const noexcept { return m_predicate_prefix_costs.size() + m_function_prefix_costs.size(); }

private:
    template<::tyr::formalism::RelationKind R>
    struct PrefixCost
    {
        ::tyr::formalism::datalog::RuleView<R> rule;
        ygg::IndexList<::tyr::formalism::Object> objects;
        Cost cost;
    };

    auto& storage(::tyr::formalism::PredicateTag) noexcept { return m_predicate_storage; }
    auto& storage(::tyr::formalism::FunctionTag) noexcept { return m_function_storage; }
    const auto& storage(::tyr::formalism::PredicateTag) const noexcept { return m_predicate_storage; }
    const auto& storage(::tyr::formalism::FunctionTag) const noexcept { return m_function_storage; }

    auto& prefix_costs(::tyr::formalism::PredicateTag) noexcept { return m_predicate_prefix_costs; }
    auto& prefix_costs(::tyr::formalism::FunctionTag) noexcept { return m_function_prefix_costs; }
    const auto& prefix_costs(::tyr::formalism::PredicateTag) const noexcept { return m_predicate_prefix_costs; }
    const auto& prefix_costs(::tyr::formalism::FunctionTag) const noexcept { return m_function_prefix_costs; }

    template<::tyr::formalism::RelationKind R>
    const Cost* find_prefix_override(::tyr::formalism::datalog::RuleBindingView<R> rule_binding) const;

    RuleCostOverrideStorage<LiftedTag, ::tyr::formalism::PredicateTag> m_predicate_storage;
    RuleCostOverrideStorage<LiftedTag, ::tyr::formalism::FunctionTag> m_function_storage;
    std::vector<PrefixCost<::tyr::formalism::PredicateTag>> m_predicate_prefix_costs;
    std::vector<PrefixCost<::tyr::formalism::FunctionTag>> m_function_prefix_costs;
};

template<::tyr::formalism::RelationKind R>
void set_rule_cost(RuleCostPolicy<LiftedTag>& policy,
                   ::tyr::formalism::datalog::RuleView<R> rule,
                   std::span<const ygg::Index<::tyr::formalism::Object>> objects,
                   Cost cost,
                   ::tyr::formalism::datalog::GrounderContext& grounder_context)
{
    assert(rule.get_arity() == objects.size());
    policy.set_cost(::tyr::formalism::datalog::ground_binding(rule, grounder_context).first, cost);
}

template<::tyr::formalism::RelationKind R>
void set_rule_cost(RuleCostOverridePolicy<LiftedTag>& policy,
                   ::tyr::formalism::datalog::RuleView<R> rule,
                   std::span<const ygg::Index<::tyr::formalism::Object>> objects,
                   Cost cost,
                   ::tyr::formalism::datalog::GrounderContext&)
{
    policy.set_prefix_cost(rule, objects, cost);
}

static_assert(RuleCostPolicyConcept<RuleCostPolicy<LiftedTag>, LiftedTag>);
static_assert(RuleCostPolicyConcept<RuleCostOverridePolicy<LiftedTag>, LiftedTag>);
static_assert(MutableRuleCostPolicyConcept<RuleCostPolicy<LiftedTag>, LiftedTag>);
static_assert(MutableRuleCostPolicyConcept<RuleCostOverridePolicy<LiftedTag>, LiftedTag>);

}

#endif
