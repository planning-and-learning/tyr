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

#include "tyr/planning/programs/rpg.hpp"

#include "../../programs/common.hpp"
#include "../../programs/layered_program.hpp"
#include "tyr/datalog/rule_splitting.hpp"
#include "tyr/datalog/static_literal_decomposition.hpp"
#include "tyr/formalism/atom_view.hpp"
#include "tyr/formalism/binary_operator_view.hpp"
#include "tyr/formalism/conjunctive_condition_view.hpp"
#include "tyr/formalism/datalog/program_view.hpp"
#include "tyr/formalism/datalog/rule_body_view.hpp"
#include "tyr/formalism/datalog/rule_view.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/detail/repository_instantiations.hpp"
#include "tyr/formalism/function_term_value_view.hpp"
#include "tyr/formalism/function_term_view.hpp"
#include "tyr/formalism/function_view.hpp"
#include "tyr/formalism/interning.hpp"
#include "tyr/formalism/literal_view.hpp"
#include "tyr/formalism/multi_operator_view.hpp"
#include "tyr/formalism/numeric_effect_view.hpp"
#include "tyr/formalism/object_view.hpp"
#include "tyr/formalism/parameter_index.hpp"
#include "tyr/formalism/planning/action_view.hpp"
#include "tyr/formalism/planning/axiom_view.hpp"
#include "tyr/formalism/planning/conditional_effect_view.hpp"
#include "tyr/formalism/planning/conjunctive_effect_view.hpp"
#include "tyr/formalism/planning/domain_view.hpp"
#include "tyr/formalism/planning/fdr_fact_view.hpp"
#include "tyr/formalism/planning/fdr_variable_view.hpp"
#include "tyr/formalism/planning/invariant_view.hpp"
#include "tyr/formalism/planning/metric_view.hpp"
#include "tyr/formalism/planning/planning_task.hpp"
#include "tyr/formalism/planning/task_view.hpp"
#include "tyr/formalism/predicate_view.hpp"
#include "tyr/formalism/repository.hpp"
#include "tyr/formalism/unary_operator_view.hpp"
#include "tyr/formalism/variable_view.hpp"

#include <algorithm>
#include <optional>
#include <utility>
#include <yggdrasil/formalism/row_view.hpp>

namespace tyr::planning
{
namespace
{
void append_variable(formalism::VariableView variable, urd::builder<formalism::ConjunctiveCondition<LiftedTag>, formalism::Repository>& conj_cond)
{
    const auto child = urd::child<formalism::Variable, formalism::Repository>(variable.index());
    if (std::ranges::find(conj_cond.variables, child) == conj_cond.variables.end())
        urd::append(conj_cond.variables, variable);
}

/// The relaxation holds no values of functions: the numeric constraints of a condition are taken to hold.
void fill_delete_free_condition(formalism::ActionView<LiftedTag> action,
                                formalism::ConditionalEffectView<LiftedTag> cond_eff,
                                urd::builder<formalism::ConjunctiveCondition<LiftedTag>, formalism::Repository>& conj_cond)
{
    // Action parameter may get deleted.
    for (const auto& variable : action.get_variables())
        append_variable(variable, conj_cond);
    for (const auto literal : action.get_condition().get_literals<formalism::StaticTag>())
        urd::append(conj_cond.static_literals, literal);
    for (const auto literal : action.get_condition().get_literals<formalism::FluentTag>())
        if (literal.get_polarity())
            urd::append(conj_cond.fluent_literals, literal);

    for (const auto variable : cond_eff.get_variables())
        append_variable(variable, conj_cond);
    for (const auto literal : cond_eff.get_condition().template get_literals<formalism::StaticTag>())
        urd::append(conj_cond.static_literals, literal);
    for (const auto literal : cond_eff.get_condition().template get_literals<formalism::FluentTag>())
        if (literal.get_polarity())
            urd::append(conj_cond.fluent_literals, literal);
}

auto create_delete_free_goal(formalism::ConjunctiveConditionView<GroundTag> goal,
                             formalism::GroundingContext<formalism::Builder, formalism::Repository>& context)
{
    auto conj_cond = formalism::checkout<formalism::RuleBody<GroundTag>>(context.builder());

    for (const auto fact : goal.get_facts<formalism::PositiveTag>())
        if (const auto literal = create_fact_literal(fact, true, context))
            urd::append(conj_cond->fluent_literals, *literal);

    return context.destination().insert(*conj_cond, context.builder());
}

auto create_rpg_cond_effect_rule(formalism::ActionView<LiftedTag> action,
                                 formalism::ConditionalEffectView<LiftedTag> cond_eff,
                                 formalism::GroundingContext<formalism::Builder, formalism::Repository>& context)
{
    auto rule = formalism::checkout<formalism::Rule<LiftedTag>>(context.builder());

    auto conj_cond = formalism::checkout<formalism::ConjunctiveCondition<LiftedTag>>(context.builder());

    fill_delete_free_condition(action, cond_eff, *conj_cond);

    const auto new_conj_cond = context.destination().insert(*conj_cond, context.builder()).first;

    for (const auto variable : new_conj_cond.get_variables())
        urd::append(rule->variables, variable);
    urd::store(rule->body, new_conj_cond);
    for (const auto literal : cond_eff.get_effect().get_literals())
        if (literal.get_polarity())  /// ignore delete effects
            urd::append(rule->fluent_heads, literal.get_atom());
    rule->cost = 1;

    return context.destination().insert(*rule, context.builder());
}

void translate_action_to_rpg_rules(formalism::ActionView<LiftedTag> action,
                                   urd::builder<formalism::Program<LiftedTag>, formalism::Repository>& program,
                                   formalism::GroundingContext<formalism::Builder, formalism::Repository>& context,
                                   RPGProgram<LiftedTag>::RuleToActionMapping& rule_to_action)
{
    for (const auto cond_eff : action.get_effects())
    {
        if (std::ranges::any_of(cond_eff.get_effect().get_literals(), [](const auto literal) { return literal.get_polarity(); }))
        {
            const auto rule = create_rpg_cond_effect_rule(action, cond_eff, context).first;

            urd::append(program.rules, rule);
            rule_to_action[rule.index()].push_back(action);
        }
    }
}

auto create_rpg_program(formalism::TaskView<LiftedTag> task, RPGProgram<LiftedTag>::RuleToActionMapping& rule_to_action, formalism::Repository& destination)
{
    auto builder = formalism::Builder();
    auto context = formalism::GroundingContext(builder, destination);
    auto program = formalism::checkout<formalism::Program<LiftedTag>>(builder);

    translate_task_symbols(task, *program);

    urd::store(program->goal, create_delete_free_goal(task.get_goal(), context).first);

    for (const auto action : task.get_domain().get_actions())
        translate_action_to_rpg_rules(action, *program, context, rule_to_action);

    return destination.insert(*program, builder).first;
}

auto create_rpg_datalog_program(const formalism::PlanningTask<LiftedTag>& planning_task,
                                CostMode cost_mode,
                                bool split_rules,
                                RPGProgram<LiftedTag>::RuleToActionMapping& rule_to_action,
                                datalog::RuleSplitting& rule_splitting)
{
    require_unit_cost(cost_mode);

    const auto task = planning_task.get_task();
    const auto num_objects = task.get_num_objects();
    auto repository = layer_over(planning_task.get_repository(), num_objects);
    const auto program = create_rpg_program(task, rule_to_action, *repository);
    auto result = LayeredProgram { program, std::move(repository) };

    if (split_rules)
    {
        // Each transform maps the rules of its source to its output: the actions follow their rules.
        const auto map_rules = [&](const datalog::RuleMapping& rules)
        {
            auto mapped = RPGProgram<LiftedTag>::RuleToActionMapping {};
            for (const auto& [rule, actions] : rule_to_action)
            {
                auto& mapped_actions = mapped[rules.at(rule)];
                mapped_actions.insert(mapped_actions.end(), actions.begin(), actions.end());
            }
            rule_to_action = std::move(mapped);
        };
        auto rules = datalog::RuleMapping {};
        result = transform(result, num_objects, rules, decompose_static_literals_pass());
        map_rules(rules);
        result = transform(result,
                           num_objects,
                           rules,
                           [&](formalism::ProgramView<LiftedTag> source, formalism::Repository& destination, datalog::RuleMapping& output_rules)
                           {
                               const auto output = datalog::split_rules(source, destination, rule_splitting);
                               output_rules = rule_splitting.rules;
                               return output;
                           });
        map_rules(rules);
    }

    return datalog::Program<LiftedTag>(result.program, std::move(result.repository));
}

}

template<>
RPGProgram<LiftedTag>::RPGProgram(const formalism::PlanningTask<LiftedTag>& task, CostMode cost_mode, bool split_rules) :
    m_rule_to_action(),
    m_rule_splitting(),
    m_datalog_program(create_rpg_datalog_program(task, cost_mode, split_rules, m_rule_to_action, m_rule_splitting))
{
    // std::cout << m_datalog_program.get_program() << std::endl;
}

}
