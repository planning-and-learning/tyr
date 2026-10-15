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
#include "tyr/formalism/datalog/expression_properties.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/datalog/views.hpp"
#include "tyr/formalism/planning/merge_datalog.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/planning/views.hpp"

#include <algorithm>
#include <optional>
#include <stdexcept>
#include <yggdrasil/containers/unordered_set.hpp>

namespace tyr::planning
{
namespace
{
using MetricFunctionSet = ygg::UnorderedSet<formalism::datalog::FunctionView<formalism::FluentTag>>;

template<formalism::FactKind T>
bool targets_metric_function(formalism::planning::NumericEffectOperatorView<LiftedTag, T> effect,
                             const MetricFunctionSet& metric_functions,
                             formalism::planning::MergeDatalogContext& context)
{
    return ygg::visit(
        [&](auto&& arg)
        {
            const auto fterm = merge_p2d<T, formalism::FluentTag>(arg.get_fterm(), context).first;
            return metric_functions.find(fterm.get_function()) != metric_functions.end();
        },
        effect.get_variant());
}

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

bool is_real_conditional_effect(formalism::planning::ConditionalEffectView<LiftedTag> cond_eff)
{
    const auto condition = cond_eff.get_condition();
    return !cond_eff.get_variables().empty() || !condition.get_literals<formalism::StaticTag>().empty()
           || !condition.get_literals<formalism::FluentTag>().empty() || !condition.get_literals<formalism::DerivedTag>().empty()
           || !condition.get_numeric_constraints().empty();
}

ygg::Data<formalism::datalog::NumericEffectOperator<LiftedTag, formalism::FluentTag>>
create_unit_metric_effect(formalism::datalog::FunctionTermView<LiftedTag, formalism::FluentTag> term, formalism::planning::MergeDatalogContext& context)
{
    auto effect = formalism::datalog::checkout<formalism::datalog::NumericEffect<LiftedTag, formalism::FluentTag>>(context.builder);
    effect->operator_kind = formalism::NumericEffectOperatorKind::Increase;
    effect->fterm = term.get_index();
    effect->fexpr = ygg::Data<formalism::datalog::FunctionExpression<LiftedTag>>(ygg::float_t(1));
    return ygg::Data<formalism::datalog::NumericEffectOperator<LiftedTag, formalism::FluentTag>>(
        formalism::NumericEffectOperatorKind::Increase,
        formalism::datalog::insert(context.destination, *effect).first.get_index());
}

ygg::DataList<formalism::datalog::NumericEffectOperator<LiftedTag, formalism::FluentTag>>
create_unit_metric(ygg::Data<formalism::datalog::Program<LiftedTag>>& program, formalism::planning::MergeDatalogContext& context)
{
    auto function = formalism::datalog::checkout<formalism::Function<formalism::FluentTag>>(context.builder);
    function->name = "__tyr_unit_cost";
    function->arity = 0;
    const auto unit_function = formalism::datalog::insert(context.destination, *function).first;
    program.fluent_functions.push_back(unit_function.get_index());

    auto lifted_term = formalism::datalog::checkout<formalism::datalog::FunctionTerm<LiftedTag, formalism::FluentTag>>(context.builder);
    lifted_term->function = unit_function.get_index();
    const auto unit_term = formalism::datalog::insert(context.destination, *lifted_term).first;

    auto binding = formalism::datalog::checkout<formalism::RelationBinding<formalism::Function<formalism::FluentTag>>>(context.builder);
    binding->relation = unit_function.get_index();
    const auto unit_binding = formalism::datalog::insert(context.destination, *binding).first;

    auto ground_term = formalism::datalog::checkout<formalism::datalog::FunctionTerm<GroundTag, formalism::FluentTag>>(context.builder);
    ground_term->binding = unit_binding.get_index();
    const auto unit_ground_term = formalism::datalog::insert(context.destination, *ground_term).first;

    auto metric = formalism::datalog::checkout<formalism::datalog::Metric>(context.builder);
    metric->fexpr = ygg::Data<formalism::datalog::FunctionExpression<GroundTag>>(unit_ground_term.get_index());
    program.metric = formalism::datalog::insert(context.destination, *metric).first.get_index();

    auto result = ygg::DataList<formalism::datalog::NumericEffectOperator<LiftedTag, formalism::FluentTag>> {};
    result.push_back(create_unit_metric_effect(unit_term, context));
    return result;
}

ygg::DataList<formalism::datalog::NumericEffectOperator<LiftedTag, formalism::FluentTag>>
create_metric_effects(formalism::planning::ActionView<LiftedTag> action,
                      CostMode cost_mode,
                      const ygg::DataList<formalism::datalog::NumericEffectOperator<LiftedTag, formalism::FluentTag>>& unit_metric_effects,
                      const MetricFunctionSet& metric_functions,
                      formalism::planning::MergeDatalogContext& context)
{
    if (cost_mode == CostMode::UNIT)
        return unit_metric_effects;

    auto result = ygg::DataList<formalism::datalog::NumericEffectOperator<LiftedTag, formalism::FluentTag>> {};
    if (metric_functions.empty())
        return result;

    for (const auto cond_eff : action.get_effects())
    {
        for (const auto numeric_effect : cond_eff.get_effect().get_numeric_effects())
            if (targets_metric_function(numeric_effect, metric_functions, context))
            {
                if (is_real_conditional_effect(cond_eff))
                    throw std::invalid_argument("GENERAL action costs with :conditional-effects are unsupported; compile conditional effects away first.");
                result.push_back(merge_p2d(numeric_effect, context));
            }

        if (const auto auxiliary_numeric_effect = cond_eff.get_effect().get_auxiliary_numeric_effect())
            if (targets_metric_function(auxiliary_numeric_effect.value(), metric_functions, context))
            {
                if (is_real_conditional_effect(cond_eff))
                    throw std::invalid_argument("GENERAL action costs with :conditional-effects are unsupported; compile conditional effects away first.");
                result.push_back(merge_p2d<formalism::AuxiliaryTag, formalism::FluentTag>(auxiliary_numeric_effect.value(), context));
            }
    }

    return result;
}

auto create_cond_effect_rule(formalism::planning::ActionView<LiftedTag> action,
                             formalism::planning::ConditionalEffectView<LiftedTag> cond_eff,
                             const ygg::DataList<formalism::datalog::NumericEffectOperator<LiftedTag, formalism::FluentTag>>& metric_effects,
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
    rule->metric_effects.insert(rule->metric_effects.end(), metric_effects.begin(), metric_effects.end());

    return formalism::datalog::insert(context.destination, *rule);
}

auto create_cond_numeric_effect_rule(formalism::planning::ActionView<LiftedTag> action,
                                     formalism::planning::ConditionalEffectView<LiftedTag> cond_eff,
                                     formalism::planning::NumericEffectOperatorView<LiftedTag, formalism::FluentTag> effect,
                                     const ygg::DataList<formalism::datalog::NumericEffectOperator<LiftedTag, formalism::FluentTag>>& metric_effects,
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
    rule->metric_effects.insert(rule->metric_effects.end(), metric_effects.begin(), metric_effects.end());

    return formalism::datalog::insert(context.destination, *rule);
}

void translate_action_to_delete_free_rules(formalism::planning::ActionView<LiftedTag> action,
                                           ygg::Data<formalism::datalog::Program<LiftedTag>>& program,
                                           CostMode cost_mode,
                                           const ygg::DataList<formalism::datalog::NumericEffectOperator<LiftedTag, formalism::FluentTag>>& unit_metric_effects,
                                           const MetricFunctionSet& metric_functions,
                                           TranslationContext<LiftedTag>& translation_context,
                                           formalism::planning::MergeDatalogContext& context,
                                           RPGProgram<LiftedTag>::RuleToActionMappings& rule_to_action)
{
    const auto metric_effects = create_metric_effects(action, cost_mode, unit_metric_effects, metric_functions, context);

    for (const auto cond_eff : action.get_effects())
    {
        if (std::ranges::any_of(cond_eff.get_effect().get_literals(), [](const auto literal) { return literal.get_polarity(); }))
        {
            const auto rule = create_cond_effect_rule(action, cond_eff, metric_effects, translation_context, context).first;

            program.predicate_rules.push_back(rule.get_index());
            rule_to_action.predicate.emplace(rule.get_index(), action);
        }

        for (const auto numeric_effect : cond_eff.get_effect().get_numeric_effects())
        {
            const auto rule = create_cond_numeric_effect_rule(action, cond_eff, numeric_effect, metric_effects, translation_context, context).first;

            program.function_rules.push_back(rule.get_index());
            rule_to_action.function.emplace(rule.get_index(), action);
        }
    }
}

auto create_program(formalism::planning::TaskView<LiftedTag> task,
                    CostMode cost_mode,
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
    if (const auto function = task.get_domain().get_auxiliary_function())
        program->fluent_functions.push_back(
            formalism::planning::merge_p2d<formalism::AuxiliaryTag, formalism::FluentTag>(function.value(), context).first.get_index());
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
    if (const auto fterm_value = task.get_auxiliary_fterm_value())
        program->fluent_fterm_values.push_back(
            formalism::planning::merge_p2d<formalism::AuxiliaryTag, formalism::FluentTag>(fterm_value.value(), context).first.get_index());

    program->goal = create_delete_free_goal(task.get_goal(), translation_context, context).first.get_index();
    auto metric_functions = MetricFunctionSet {};
    auto unit_metric_effects = ygg::DataList<formalism::datalog::NumericEffectOperator<LiftedTag, formalism::FluentTag>> {};
    if (cost_mode == CostMode::UNIT)
    {
        unit_metric_effects = create_unit_metric(*program, context);
    }
    else if (task.get_metric())
    {
        const auto metric = formalism::planning::merge_p2d(task.get_metric().value(), context).first;
        program->metric = metric.get_index();

        auto metric_fterms = ygg::UnorderedSet<formalism::datalog::FunctionTermView<GroundTag, formalism::FluentTag>> {};
        formalism::datalog::collect_fterms(metric.get_fexpr(), metric_fterms);
        for (const auto fterm : metric_fterms)
            metric_functions.insert(fterm.get_function());
    }

    for (const auto action : task.get_domain().get_actions())
        translate_action_to_delete_free_rules(action, *program, cost_mode, unit_metric_effects, metric_functions, translation_context, context, rule_to_action);

    return formalism::datalog::insert(destination, *program).first;
}

auto create_datalog_program(formalism::planning::TaskView<LiftedTag> task,
                            CostMode cost_mode,
                            TranslationContext<LiftedTag>& translation_context,
                            RPGProgram<LiftedTag>::RuleToActionMappings& rule_to_action)
{
    auto factory = std::make_shared<formalism::datalog::RepositoryFactory>();
    auto repository = factory->create_shared(task.get_num_objects());
    auto program = create_program(task, cost_mode, translation_context, rule_to_action, *repository);
    return datalog::Program<LiftedTag>(program, std::move(repository), std::move(factory));
}

}

template<>
RPGProgram<LiftedTag>::RPGProgram(formalism::planning::TaskView<LiftedTag> task, CostMode cost_mode) :
    m_translation_context(),
    m_rule_to_action(),
    m_datalog_program(create_datalog_program(task, cost_mode, m_translation_context, m_rule_to_action))
{
    // std::cout << m_datalog_program.get_program() << std::endl;
}

}
