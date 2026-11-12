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

#include "tyr/formalism/atom_view.hpp"
#include "tyr/formalism/binary_operator_view.hpp"
#include "tyr/formalism/datalog/conjunctive_condition_view.hpp"
#include "tyr/formalism/datalog/conjunctive_effect_view.hpp"
#include "tyr/formalism/datalog/formatter.hpp"
#include "tyr/formalism/datalog/program_view.hpp"
#include "tyr/formalism/datalog/rule_view.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/function_term_value_view.hpp"
#include "tyr/formalism/function_term_view.hpp"
#include "tyr/formalism/function_view.hpp"
#include "tyr/formalism/literal_view.hpp"
#include "tyr/formalism/multi_operator_view.hpp"
#include "tyr/formalism/numeric_effect_view.hpp"
#include "tyr/formalism/object_view.hpp"
#include "tyr/formalism/parameter_index.hpp"
#include "tyr/formalism/predicate_view.hpp"
#include "tyr/formalism/unary_operator_view.hpp"
#include "tyr/formalism/variable_view.hpp"

#include <assert.h>
#include <gtl/phmap.hpp>
#include <type_traits>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <utility>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/formalism/row_view.hpp>

namespace tyr::datalog
{
namespace
{
template<typename Relation>
void collect_active_rules(const urd::Store<urd::set<urd::index<Relation>>, urd::dense>& relations,
                          const urd::Store<urd::map<urd::index<Relation>, analysis::RuleIndexSet>>& listeners,
                          urd::Store<urd::set<urd::index<formalism::datalog::Rule<LiftedTag>>>, urd::dense>& active_rules)
{
    for (const auto relation : relations)
        if (const auto it = listeners.find(relation); it != listeners.end())
            active_rules.insert(it->second.begin(), it->second.end());
}
}

TypedRuleSchedulerStratum::TypedRuleSchedulerStratum(const analysis::TypedRuleStratum& rules,
                                                     const analysis::TypedListenerStratum& listeners,
                                                     const formalism::datalog::Repository& context,
                                                     size_t num_fluent_predicates,
                                                     size_t num_fluent_functions) :
    m_rules(&rules),
    m_listeners(&listeners),
    m_context(&context),
    m_active_rules(),
    m_full_enumeration_rules()
{
    m_active_predicates.reserve(num_fluent_predicates);
    m_active_functions.reserve(num_fluent_functions);
}

void TypedRuleSchedulerStratum::activate_all()
{
    m_active_rules.clear();
    m_full_enumeration_rules.clear();
    for (const auto rule : *m_rules)
        m_active_rules.insert(rule);
    rebuild_sorted_active_rules();
}

void TypedRuleSchedulerStratum::rebuild_sorted_active_rules() { m_sorted_active_rules.set(m_active_rules.begin(), m_active_rules.end()); }

void TypedRuleSchedulerStratum::on_start_iteration() noexcept
{
    m_active_predicates.clear();
    m_active_functions.clear();
}

void TypedRuleSchedulerStratum::on_generate(urd::index<formalism::Predicate<formalism::FluentTag>> predicate) { m_active_predicates.insert(predicate); }

void TypedRuleSchedulerStratum::on_generate(urd::index<formalism::Function<formalism::FluentTag>> function) { m_active_functions.insert(function); }

void TypedRuleSchedulerStratum::on_finish_iteration()
{
    m_active_rules.clear();
    m_full_enumeration_rules.clear();
    collect_active_rules(m_active_predicates, m_listeners->predicates, m_active_rules);
    collect_active_rules(m_active_functions, m_listeners->functions, m_full_enumeration_rules);
    m_active_rules.insert(m_full_enumeration_rules.begin(), m_full_enumeration_rules.end());
    rebuild_sorted_active_rules();
}

Scheduler<LiftedTag>::Scheduler(const analysis::RuleStratum& rules,
                                const analysis::ListenerStratum& listeners,
                                const formalism::datalog::Repository& context,
                                size_t num_fluent_predicates,
                                size_t num_fluent_functions) :
    predicate_rules(rules, listeners, context, num_fluent_predicates, num_fluent_functions)
{
}

void Scheduler<LiftedTag>::activate_all() { predicate_rules.activate_all(); }

void Scheduler<LiftedTag>::on_start_iteration() noexcept { predicate_rules.on_start_iteration(); }

void Scheduler<LiftedTag>::activate(urd::index<formalism::Predicate<formalism::FluentTag>> predicate) { predicate_rules.on_generate(predicate); }

void Scheduler<LiftedTag>::on_finish_iteration() { predicate_rules.on_finish_iteration(); }

urd::Store<urd::sequence<Scheduler<LiftedTag>>> create_schedulers(const analysis::RuleStrata& rules,
                                                                  const analysis::ListenerStrata& listeners,
                                                                  const formalism::datalog::Repository& context,
                                                                  size_t num_fluent_predicates,
                                                                  size_t num_fluent_functions)
{
    assert(rules.data.size() == listeners.data.size());

    auto result = urd::Store<urd::sequence<Scheduler<LiftedTag>>> {};
    for (ygg::uint_t i = 0; i < rules.data.size(); ++i)
        result.emplace_back(rules.data[i], listeners.data[i], context, num_fluent_predicates, num_fluent_functions);

    return result;
}

}
