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
#include "packed_layouts.hpp"
#include "tyr/datalog/rule_splitting.hpp"
#include "tyr/formalism/datalog/copy.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/datalog/views.hpp"
#include "tyr/formalism/planning/merge_datalog.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/planning/views.hpp"

#include <algorithm>
#include <optional>

namespace tyr::planning
{
namespace
{
void append_variable(formalism::planning::VariableView variable,
                     formalism::planning::MergeDatalogContext& context,
                     ygg::Data<formalism::datalog::ConjunctiveCondition<LiftedTag>>& conj_cond)
{
    const auto index = merge_p2d(variable, context).first.get_index();
    if (std::ranges::find(conj_cond.variables, index) == conj_cond.variables.end())
        conj_cond.variables.push_back(index);
}

void fill_delete_free_condition(formalism::planning::ActionView<LiftedTag> action,
                                formalism::planning::ConditionalEffectView<LiftedTag> cond_eff,
                                TranslationContext<LiftedTag>& translation_context,
                                formalism::planning::MergeDatalogContext& context,
                                ygg::Data<formalism::datalog::ConjunctiveCondition<LiftedTag>>& conj_cond)
{
    // Action parameter may get deleted.
    for (const auto& variable : action.get_variables())
        append_variable(variable, context, conj_cond);
    for (const auto literal : action.get_condition().get_literals<formalism::StaticTag>())
        conj_cond.static_literals.push_back(merge_p2d(literal, translation_context.p2d.static_to_static_predicate, context).first.get_index());
    for (const auto literal : action.get_condition().get_literals<formalism::FluentTag>())
        if (literal.get_polarity())
            conj_cond.fluent_literals.push_back(merge_p2d(literal, translation_context.p2d.fluent_to_fluent_predicate, context).first.get_index());
    for (const auto numeric_constraint : action.get_condition().get_numeric_constraints())
        conj_cond.numeric_constraints.push_back(merge_p2d(numeric_constraint, context));

    for (const auto variable : cond_eff.get_variables())
        append_variable(variable, context, conj_cond);
    for (const auto literal : cond_eff.get_condition().template get_literals<formalism::StaticTag>())
        conj_cond.static_literals.push_back(merge_p2d(literal, translation_context.p2d.static_to_static_predicate, context).first.get_index());
    for (const auto literal : cond_eff.get_condition().template get_literals<formalism::FluentTag>())
        if (literal.get_polarity())
            conj_cond.fluent_literals.push_back(merge_p2d(literal, translation_context.p2d.fluent_to_fluent_predicate, context).first.get_index());
    for (const auto numeric_constraint : cond_eff.get_condition().get_numeric_constraints())
        conj_cond.numeric_constraints.push_back(merge_p2d(numeric_constraint, context));
}

auto create_delete_free_goal(formalism::planning::ConjunctiveConditionView<GroundTag> goal,
                             TranslationContext<LiftedTag>& translation_context,
                             formalism::planning::MergeDatalogContext& context)
{
    auto conj_cond = formalism::datalog::checkout<formalism::datalog::ConjunctiveCondition<GroundTag>>(context.builder);

    for (const auto fact : goal.get_facts<formalism::PositiveTag>())
        if (const auto literal = merge_p2d(fact, true, translation_context.p2d.fluent_to_fluent_predicate, context))
            conj_cond->fluent_literals.push_back(literal->get_index());

    for (const auto numeric_constraint : goal.get_numeric_constraints())
        conj_cond->numeric_constraints.push_back(merge_p2d(numeric_constraint, context));

    return formalism::datalog::insert(context.destination, *conj_cond);
}

auto create_cond_effect_rule(formalism::planning::ActionView<LiftedTag> action,
                             formalism::planning::ConditionalEffectView<LiftedTag> cond_eff,
                             TranslationContext<LiftedTag>& translation_context,
                             formalism::planning::MergeDatalogContext& context)
{
    auto rule = formalism::datalog::checkout<formalism::datalog::Rule<LiftedTag, formalism::PredicateTag>>(context.builder);

    auto conj_cond = formalism::datalog::checkout<formalism::datalog::ConjunctiveCondition<LiftedTag>>(context.builder);

    fill_delete_free_condition(action, cond_eff, translation_context, context, *conj_cond);

    const auto new_conj_cond = formalism::datalog::insert(context.destination, *conj_cond).first;

    ygg::extend(new_conj_cond.get_variables(), rule->variables);
    rule->body = new_conj_cond.get_index();
    for (const auto literal : cond_eff.get_effect().get_literals())
        if (literal.get_polarity())  /// ignore delete effects
            rule->heads.push_back(merge_p2d(literal.get_atom(), translation_context.p2d.fluent_to_fluent_predicate, context).first.get_index());
    rule->cost = 1;

    return formalism::datalog::insert(context.destination, *rule);
}

auto create_cond_numeric_effect_rule(formalism::planning::ActionView<LiftedTag> action,
                                     formalism::planning::ConditionalEffectView<LiftedTag> cond_eff,
                                     formalism::planning::NumericEffectOperatorView<LiftedTag, formalism::FluentTag> effect,
                                     TranslationContext<LiftedTag>& translation_context,
                                     formalism::planning::MergeDatalogContext& context)
{
    auto rule = formalism::datalog::checkout<formalism::datalog::Rule<LiftedTag, formalism::FunctionTag>>(context.builder);

    auto conj_cond = formalism::datalog::checkout<formalism::datalog::ConjunctiveCondition<LiftedTag>>(context.builder);

    fill_delete_free_condition(action, cond_eff, translation_context, context, *conj_cond);

    const auto new_conj_cond = formalism::datalog::insert(context.destination, *conj_cond).first;

    ygg::extend(new_conj_cond.get_variables(), rule->variables);
    rule->body = new_conj_cond.get_index();
    rule->head = merge_p2d(effect, context);
    rule->cost = 1;

    return formalism::datalog::insert(context.destination, *rule);
}

void translate_action_to_delete_free_rules(formalism::planning::ActionView<LiftedTag> action,
                                           ygg::Data<formalism::datalog::Program<LiftedTag>>& program,
                                           TranslationContext<LiftedTag>& translation_context,
                                           formalism::planning::MergeDatalogContext& context,
                                           RPGProgram<LiftedTag>::RuleToActionMappings& rule_to_action)
{
    for (const auto cond_eff : action.get_effects())
    {
        if (std::ranges::any_of(cond_eff.get_effect().get_literals(), [](const auto literal) { return literal.get_polarity(); }))
        {
            const auto rule = create_cond_effect_rule(action, cond_eff, translation_context, context).first;

            program.predicate_rules.push_back(rule.get_index());
            rule_to_action.predicate.emplace(rule.get_index(), action);
        }

        for (const auto numeric_effect : cond_eff.get_effect().get_numeric_effects())
        {
            const auto rule = create_cond_numeric_effect_rule(action, cond_eff, numeric_effect, translation_context, context).first;

            program.function_rules.push_back(rule.get_index());
            rule_to_action.function.emplace(rule.get_index(), action);
        }
    }
}

auto create_program(formalism::planning::TaskView<LiftedTag> task,
                    TranslationContext<LiftedTag>& translation_context,
                    RPGProgram<LiftedTag>::RuleToActionMappings& rule_to_action,
                    formalism::datalog::Repository& destination)
{
    auto builder = formalism::datalog::Builder();
    auto context = formalism::planning::MergeDatalogContext(builder, destination);
    auto program = formalism::datalog::checkout<formalism::datalog::Program<LiftedTag>>(builder);
    auto domain_storage = std::optional<analysis::TaskVariableDomains> {};
    const auto* domains = packed_layout_domains(task, domain_storage);

    for (const auto predicate : task.get_domain().get_predicates<formalism::StaticTag>())
    {
        const auto new_predicate = formalism::planning::merge_p2d(predicate, context).first;
        translation_context.d2p.static_to_static_predicate.emplace(new_predicate, predicate);
        translation_context.p2d.static_to_static_predicate.emplace(predicate, new_predicate);
        program->static_predicates.push_back(new_predicate.get_index());
    }
    for (const auto predicate : task.get_domain().get_predicates<formalism::FluentTag>())
    {
        const auto new_predicate = formalism::planning::merge_p2d(predicate, context).first;
        translation_context.d2p.fluent_to_fluent_predicate.emplace(new_predicate, predicate);
        translation_context.p2d.fluent_to_fluent_predicate.emplace(predicate, new_predicate);
        program->fluent_predicates.push_back(new_predicate.get_index());
    }

    for (const auto function : task.get_domain().get_functions<formalism::StaticTag>())
        program->static_functions.push_back(formalism::planning::merge_p2d(function, context).first.get_index());
    for (const auto function : task.get_domain().get_functions<formalism::FluentTag>())
    {
        const auto new_function = formalism::planning::merge_p2d(function, context).first;
        translation_context.p2d.fluent_to_fluent_function.emplace(function, new_function);
        program->fluent_functions.push_back(new_function.get_index());
    }
    set_packed_binding_layouts(task, domains, translation_context.p2d, destination);

    for (const auto object : task.get_domain().get_constants())
        program->objects.push_back(formalism::planning::merge_p2d(object, context).first.get_index());
    for (const auto object : task.get_objects())
        program->objects.push_back(formalism::planning::merge_p2d(object, context).first.get_index());

    for (const auto atom : task.get_atoms<formalism::StaticTag>())
        program->static_atoms.push_back(formalism::planning::merge_p2d(atom, translation_context.p2d.static_to_static_predicate, context).first.get_index());
    for (const auto atom : task.get_atoms<formalism::FluentTag>())
        program->fluent_atoms.push_back(formalism::planning::merge_p2d(atom, translation_context.p2d.fluent_to_fluent_predicate, context).first.get_index());

    for (const auto fterm_value : task.get_fterm_values<formalism::StaticTag>())
        program->static_fterm_values.push_back(formalism::planning::merge_p2d(fterm_value, context).first.get_index());
    for (const auto fterm_value : task.get_fterm_values<formalism::FluentTag>())
        program->fluent_fterm_values.push_back(formalism::planning::merge_p2d(fterm_value, context).first.get_index());

    program->goal = create_delete_free_goal(task.get_goal(), translation_context, context).first.get_index();

    for (const auto action : task.get_domain().get_actions())
        translate_action_to_delete_free_rules(action, *program, translation_context, context, rule_to_action);

    return formalism::datalog::insert(destination, *program).first;
}

TranslationContext<LiftedTag> remap_translation_context(const TranslationContext<LiftedTag>& source, formalism::datalog::CopyContext& context)
{
    auto result = TranslationContext<LiftedTag> {};
    const auto remap_keys = [&](const auto& source_mapping, auto& result_mapping)
    {
        for (const auto& [key, value] : source_mapping)
            result_mapping.emplace(formalism::datalog::copy(key, context).first, value);
    };
    const auto remap_values = [&](const auto& source_mapping, auto& result_mapping)
    {
        for (const auto& [key, value] : source_mapping)
            result_mapping.emplace(key, formalism::datalog::copy(value, context).first);
    };

    remap_keys(source.d2p.static_to_static_predicate, result.d2p.static_to_static_predicate);
    remap_keys(source.d2p.fluent_to_fluent_predicate, result.d2p.fluent_to_fluent_predicate);
    remap_keys(source.d2p.fluent_to_derived_predicate, result.d2p.fluent_to_derived_predicate);
    remap_values(source.p2d.static_to_static_predicate, result.p2d.static_to_static_predicate);
    remap_values(source.p2d.fluent_to_fluent_predicate, result.p2d.fluent_to_fluent_predicate);
    remap_values(source.p2d.derived_to_fluent_predicate, result.p2d.derived_to_fluent_predicate);
    remap_values(source.p2d.fluent_to_fluent_function, result.p2d.fluent_to_fluent_function);
    return result;
}

/// The rules of the split program that stand for the actions of the source rules.
RPGProgram<LiftedTag>::RuleToActionMappings remap_rule_to_action(const RPGProgram<LiftedTag>::RuleToActionMappings& source,
                                                                 const formalism::datalog::Repository& source_repository,
                                                                 const datalog::RuleSplitting& splitting,
                                                                 formalism::datalog::CopyContext& context)
{
    auto result = RPGProgram<LiftedTag>::RuleToActionMappings {};
    for (const auto& [rule, action] : source.predicate)
    {
        const auto it = splitting.main_rules.find(rule);
        result.predicate.emplace(
            it != splitting.main_rules.end() ? it->second : formalism::datalog::copy(ygg::make_view(rule, source_repository), context).first.get_index(),
            action);
    }
    for (const auto& [rule, action] : source.function)
        result.function.emplace(formalism::datalog::copy(ygg::make_view(rule, source_repository), context).first.get_index(), action);
    return result;
}

auto create_datalog_program(formalism::planning::TaskView<LiftedTag> task,
                            CostMode cost_mode,
                            bool split_rules,
                            TranslationContext<LiftedTag>& translation_context,
                            RPGProgram<LiftedTag>::RuleToActionMappings& rule_to_action,
                            datalog::RuleSplitting& rule_splitting)
{
    require_unit_cost(cost_mode);

    auto factory = std::make_shared<formalism::datalog::RepositoryFactory>();
    if (!split_rules)
    {
        auto repository = factory->create_shared(task.get_num_objects());
        auto program = create_program(task, translation_context, rule_to_action, *repository);
        return datalog::Program<LiftedTag>(program, std::move(repository), std::move(factory));
    }

    auto source_repository = factory->create(task.get_num_objects());
    auto source_translation_context = TranslationContext<LiftedTag> {};
    auto source_rule_to_action = RPGProgram<LiftedTag>::RuleToActionMappings {};
    const auto source_program = create_program(task, source_translation_context, source_rule_to_action, source_repository);

    auto repository = factory->create_shared(task.get_num_objects());
    const auto program = datalog::split_rules(source_program, *repository, rule_splitting);
    auto builder = formalism::datalog::Builder {};
    auto context = formalism::datalog::CopyContext { builder, *repository };
    translation_context = remap_translation_context(source_translation_context, context);
    rule_to_action = remap_rule_to_action(source_rule_to_action, source_repository, rule_splitting, context);
    return datalog::Program<LiftedTag>(program, std::move(repository), std::move(factory));
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
