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
#include "../../programs/translated_program.hpp"
#include "tyr/datalog/rule_splitting.hpp"
#include "tyr/datalog/static_literal_decomposition.hpp"
#include "tyr/formalism/atom_view.hpp"
#include "tyr/formalism/binary_operator_view.hpp"
#include "tyr/formalism/datalog/conjunctive_condition_view.hpp"
#include "tyr/formalism/datalog/conjunctive_effect_view.hpp"
#include "tyr/formalism/datalog/copy.hpp"
#include "tyr/formalism/datalog/interning.hpp"
#include "tyr/formalism/datalog/program_view.hpp"
#include "tyr/formalism/datalog/repository.hpp"
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
#include "tyr/formalism/planning/action_view.hpp"
#include "tyr/formalism/planning/axiom_view.hpp"
#include "tyr/formalism/planning/conditional_effect_view.hpp"
#include "tyr/formalism/planning/conjunctive_condition_view.hpp"
#include "tyr/formalism/planning/conjunctive_effect_view.hpp"
#include "tyr/formalism/planning/domain_view.hpp"
#include "tyr/formalism/planning/fdr_fact_view.hpp"
#include "tyr/formalism/planning/fdr_variable_view.hpp"
#include "tyr/formalism/planning/invariant_view.hpp"
#include "tyr/formalism/planning/merge_datalog.hpp"
#include "tyr/formalism/planning/metric_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/planning/task_view.hpp"
#include "tyr/formalism/predicate_view.hpp"
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
void append_variable(formalism::planning::VariableView variable,
                     formalism::WriteContext<formalism::datalog::Builder, formalism::datalog::Repository>& context,
                     urd::data<formalism::datalog::ConjunctiveCondition<LiftedTag>>& conj_cond)
{
    const auto index = formalism::copy(variable, context).first.index();
    if (std::ranges::find(conj_cond.variables, index) == conj_cond.variables.end())
        conj_cond.variables.push_back(index);
}

/// The relaxation holds no values of functions: the numeric constraints of a condition are taken to hold.
void fill_delete_free_condition(formalism::planning::ActionView<LiftedTag> action,
                                formalism::planning::ConditionalEffectView<LiftedTag> cond_eff,
                                TranslationContext<LiftedTag>& translation_context,
                                formalism::WriteContext<formalism::datalog::Builder, formalism::datalog::Repository>& context,
                                urd::data<formalism::datalog::ConjunctiveCondition<LiftedTag>>& conj_cond)
{
    // Action parameter may get deleted.
    for (const auto& variable : action.get_variables())
        append_variable(variable, context, conj_cond);
    for (const auto literal : action.get_condition().get_literals<formalism::StaticTag>())
        conj_cond.static_literals.push_back(merge_p2d(literal, translation_context.p2d.static_to_static_predicate, context).first.index());
    for (const auto literal : action.get_condition().get_literals<formalism::FluentTag>())
        if (literal.get_polarity())
            conj_cond.fluent_literals.push_back(merge_p2d(literal, translation_context.p2d.fluent_to_fluent_predicate, context).first.index());

    for (const auto variable : cond_eff.get_variables())
        append_variable(variable, context, conj_cond);
    for (const auto literal : cond_eff.get_condition().template get_literals<formalism::StaticTag>())
        conj_cond.static_literals.push_back(merge_p2d(literal, translation_context.p2d.static_to_static_predicate, context).first.index());
    for (const auto literal : cond_eff.get_condition().template get_literals<formalism::FluentTag>())
        if (literal.get_polarity())
            conj_cond.fluent_literals.push_back(merge_p2d(literal, translation_context.p2d.fluent_to_fluent_predicate, context).first.index());
}

auto create_delete_free_goal(formalism::planning::ConjunctiveConditionView<GroundTag> goal,
                             TranslationContext<LiftedTag>& translation_context,
                             formalism::WriteContext<formalism::datalog::Builder, formalism::datalog::Repository>& context)
{
    auto conj_cond = formalism::datalog::checkout<formalism::datalog::ConjunctiveCondition<GroundTag>>(context.builder());

    for (const auto fact : goal.get_facts<formalism::PositiveTag>())
        if (const auto literal = merge_p2d(fact, true, translation_context.p2d.fluent_to_fluent_predicate, context))
            conj_cond->fluent_literals.push_back(literal->index());

    return formalism::datalog::insert(context.destination(), *conj_cond);
}

auto create_cond_effect_rule(formalism::planning::ActionView<LiftedTag> action,
                             formalism::planning::ConditionalEffectView<LiftedTag> cond_eff,
                             TranslationContext<LiftedTag>& translation_context,
                             formalism::WriteContext<formalism::datalog::Builder, formalism::datalog::Repository>& context)
{
    auto rule = formalism::datalog::checkout<formalism::datalog::Rule<LiftedTag>>(context.builder());

    auto conj_cond = formalism::datalog::checkout<formalism::datalog::ConjunctiveCondition<LiftedTag>>(context.builder());

    fill_delete_free_condition(action, cond_eff, translation_context, context, *conj_cond);

    const auto new_conj_cond = formalism::datalog::insert(context.destination(), *conj_cond).first;

    for (const auto variable : new_conj_cond.get_variables())
        rule->variables.push_back(urd::to_stored(variable));
    rule->body = new_conj_cond.index();
    for (const auto literal : cond_eff.get_effect().get_literals())
        if (literal.get_polarity())  /// ignore delete effects
            rule->heads.push_back(merge_p2d(literal.get_atom(), translation_context.p2d.fluent_to_fluent_predicate, context).first.index());
    rule->cost = 1;

    return formalism::datalog::insert(context.destination(), *rule);
}

void translate_action_to_delete_free_rules(formalism::planning::ActionView<LiftedTag> action,
                                           urd::data<formalism::datalog::Program<LiftedTag>>& program,
                                           TranslationContext<LiftedTag>& translation_context,
                                           formalism::WriteContext<formalism::datalog::Builder, formalism::datalog::Repository>& context,
                                           RPGProgram<LiftedTag>::RuleToActionMapping& rule_to_action)
{
    for (const auto cond_eff : action.get_effects())
    {
        if (std::ranges::any_of(cond_eff.get_effect().get_literals(), [](const auto literal) { return literal.get_polarity(); }))
        {
            const auto rule = create_cond_effect_rule(action, cond_eff, translation_context, context).first;

            program.rules.push_back(rule.index());
            rule_to_action[rule.index()].push_back(action);
        }
    }
}

auto create_program(formalism::planning::TaskView<LiftedTag> task,
                    TranslationContext<LiftedTag>& translation_context,
                    RPGProgram<LiftedTag>::RuleToActionMapping& rule_to_action,
                    formalism::datalog::Repository& destination)
{
    auto builder = formalism::datalog::Builder();
    auto context = formalism::WriteContext(builder, destination);
    auto program = formalism::datalog::checkout<formalism::datalog::Program<LiftedTag>>(builder);

    translate_task_symbols(task, false, translation_context, context, *program);

    program->goal = create_delete_free_goal(task.get_goal(), translation_context, context).first.index();

    for (const auto action : task.get_domain().get_actions())
        translate_action_to_delete_free_rules(action, *program, translation_context, context, rule_to_action);

    return formalism::datalog::insert(destination, *program).first;
}

struct RPGTables
{
    TranslationContext<LiftedTag> translation_context;
    RPGProgram<LiftedTag>::RuleToActionMapping rule_to_action;

    RPGTables remap(SymbolRemapper& remapper) const
    {
        auto result = RPGTables { remapper.translation_context(translation_context), {} };
        for (const auto& [rule, actions] : rule_to_action)
        {
            auto& result_actions = result.rule_to_action[remapper.rule(rule)];
            result_actions.insert(result_actions.end(), actions.begin(), actions.end());
        }
        return result;
    }
};

auto create_datalog_program(formalism::planning::TaskView<LiftedTag> task,
                            CostMode cost_mode,
                            bool split_rules,
                            TranslationContext<LiftedTag>& translation_context,
                            RPGProgram<LiftedTag>::RuleToActionMapping& rule_to_action,
                            datalog::RuleSplitting& rule_splitting)
{
    require_unit_cost(cost_mode);

    const auto num_objects = task.get_num_objects();
    auto repository = std::make_shared<formalism::datalog::Repository>(num_objects);
    auto tables = RPGTables {};
    const auto program = create_program(task, tables.translation_context, tables.rule_to_action, *repository);
    auto result = TranslatedProgram<RPGTables> { program, std::move(repository), std::move(tables) };

    if (split_rules)
    {
        result = transform(result, num_objects, decompose_static_literals_pass());
        result = transform(result,
                           num_objects,
                           [&](formalism::datalog::ProgramView<LiftedTag> source, formalism::datalog::Repository& destination, datalog::RuleMapping& rules)
                           {
                               const auto output = datalog::split_rules(source, destination, rule_splitting);
                               rules = rule_splitting.rules;
                               return output;
                           });
    }

    translation_context = std::move(result.tables.translation_context);
    rule_to_action = std::move(result.tables.rule_to_action);
    return datalog::Program<LiftedTag>(result.program, std::move(result.repository));
}

}

template<>
RPGProgram<LiftedTag>::RPGProgram(formalism::planning::TaskView<LiftedTag> task, CostMode cost_mode, bool split_rules) :
    m_translation_context(),
    m_rule_to_action(),
    m_rule_splitting(),
    m_datalog_program(create_datalog_program(task, cost_mode, split_rules, m_translation_context, m_rule_to_action, m_rule_splitting))
{
    // std::cout << m_datalog_program.get_program() << std::endl;
}

}
