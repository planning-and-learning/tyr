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

#include "tyr/datalog/lifted/scheduler.hpp"

#include "tyr/formalism/datalog/formatter.hpp"
#include "tyr/formalism/datalog/views.hpp"

#include <algorithm>
#include <assert.h>
#include <gtl/phmap.hpp>
#include <type_traits>
#include <utility>
#include <yggdrasil/core/config.hpp>

namespace f = tyr::formalism;
namespace fd = tyr::formalism::datalog;

namespace tyr::datalog
{
namespace
{
template<typename Relation>
void activate_relation(boost::dynamic_bitset<>& bitset, ygg::Index<Relation> relation)
{
    assert(ygg::uint_t(relation) < bitset.size());
    bitset.set(ygg::uint_t(relation));
}

template<typename Relation, f::RelationKind R>
void collect_active_rules(const boost::dynamic_bitset<>& bitset,
                          const ygg::UnorderedMap<ygg::Index<Relation>, analysis::RuleIndexSet<R>>& listeners,
                          ygg::UnorderedSet<ygg::Index<fd::Rule<R>>>& active_rules)
{
    for (auto i = bitset.find_first(); i != boost::dynamic_bitset<>::npos; i = bitset.find_next(i))
        if (const auto it = listeners.find(ygg::Index<Relation>(i)); it != listeners.end())
            active_rules.insert(it->second.begin(), it->second.end());
}
}

template<f::RelationKind R>
TypedRuleSchedulerStratum<R>::TypedRuleSchedulerStratum(const analysis::TypedRuleStratum<R>& rules,
                                                        const analysis::TypedListenerStratum<R>& listeners,
                                                        const fd::Repository& context,
                                                        size_t num_fluent_predicates,
                                                        size_t num_fluent_functions) :
    m_rules(rules),
    m_listeners(listeners),
    m_context(context),
    m_active_predicates(num_fluent_predicates),
    m_active_functions(num_fluent_functions),
    m_active_rules(),
    m_full_enumeration_rules()
{
}

template<f::RelationKind R>
void TypedRuleSchedulerStratum<R>::activate_all()
{
    m_active_rules.clear();
    m_full_enumeration_rules.clear();
    for (const auto rule : m_rules)
        m_active_rules.insert(rule);
    rebuild_sorted_active_rules();
}

template<f::RelationKind R>
void TypedRuleSchedulerStratum<R>::rebuild_sorted_active_rules()
{
    m_sorted_active_rules.set(m_active_rules.begin(), m_active_rules.end());
    std::sort(m_sorted_active_rules.begin(), m_sorted_active_rules.end());
}

template<f::RelationKind R>
void TypedRuleSchedulerStratum<R>::on_start_iteration() noexcept
{
    m_active_predicates.reset();
    m_active_functions.reset();
}

template<f::RelationKind R>
void TypedRuleSchedulerStratum<R>::on_generate(ygg::Index<f::Predicate<f::FluentTag>> predicate)
{
    activate_relation(m_active_predicates, predicate);
}

template<f::RelationKind R>
void TypedRuleSchedulerStratum<R>::on_generate(ygg::Index<f::Function<f::FluentTag>> function)
{
    activate_relation(m_active_functions, function);
}

template<f::RelationKind R>
void TypedRuleSchedulerStratum<R>::on_finish_iteration()
{
    m_active_rules.clear();
    m_full_enumeration_rules.clear();
    collect_active_rules(m_active_predicates, m_listeners.predicates, m_active_rules);
    collect_active_rules(m_active_functions, m_listeners.functions, m_full_enumeration_rules);
    m_active_rules.insert(m_full_enumeration_rules.begin(), m_full_enumeration_rules.end());
    rebuild_sorted_active_rules();
}

Scheduler<LiftedTag>::Scheduler(const analysis::RuleStratum& rules,
                                const analysis::ListenerStratum& listeners,
                                const fd::Repository& context,
                                size_t num_fluent_predicates,
                                size_t num_fluent_functions) :
    predicate_rules(rules.get<f::PredicateTag>(), listeners.get<f::PredicateTag>(), context, num_fluent_predicates, num_fluent_functions),
    function_rules(rules.get<f::FunctionTag>(), listeners.get<f::FunctionTag>(), context, num_fluent_predicates, num_fluent_functions)
{
}

void Scheduler<LiftedTag>::activate_all()
{
    predicate_rules.activate_all();
    function_rules.activate_all();
}

void Scheduler<LiftedTag>::on_start_iteration() noexcept
{
    predicate_rules.on_start_iteration();
    function_rules.on_start_iteration();
}

void Scheduler<LiftedTag>::activate(ygg::Index<f::Predicate<f::FluentTag>> predicate)
{
    predicate_rules.on_generate(predicate);
    function_rules.on_generate(predicate);
}

void Scheduler<LiftedTag>::activate(ygg::Index<f::Function<f::FluentTag>> function)
{
    predicate_rules.on_generate(function);
    function_rules.on_generate(function);
}

void Scheduler<LiftedTag>::on_finish_iteration()
{
    predicate_rules.on_finish_iteration();
    function_rules.on_finish_iteration();
}

std::vector<Scheduler<LiftedTag>> create_schedulers(const analysis::RuleStrata& rules,
                                                    const analysis::ListenerStrata& listeners,
                                                    const fd::Repository& context,
                                                    size_t num_fluent_predicates,
                                                    size_t num_fluent_functions)
{
    assert(rules.data.size() == listeners.data.size());

    auto result = std::vector<Scheduler<LiftedTag>> {};
    for (ygg::uint_t i = 0; i < rules.data.size(); ++i)
        result.emplace_back(rules.data[i], listeners.data[i], context, num_fluent_predicates, num_fluent_functions);

    return result;
}

template class TypedRuleSchedulerStratum<f::PredicateTag>;
template class TypedRuleSchedulerStratum<f::FunctionTag>;
}
