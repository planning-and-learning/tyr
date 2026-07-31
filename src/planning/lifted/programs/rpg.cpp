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

#include "tyr/planning/lifted/programs/rpg.hpp"

#include "../../programs/common.hpp"
#include "tyr/formalism/datalog/expression_properties.hpp"
#include "tyr/formalism/datalog/formatter.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/datalog/views.hpp"
#include "tyr/formalism/planning/formatter.hpp"
#include "tyr/formalism/planning/merge_datalog.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/planning/views.hpp"

#include <algorithm>
#include <optional>
#include <stdexcept>
#include <yggdrasil/containers/unordered_set.hpp>

namespace f = tyr::formalism;
namespace d = tyr::datalog;
namespace fp = tyr::formalism::planning;
namespace fd = tyr::formalism::datalog;

namespace tyr::planning
{
namespace
{
using MetricFunctionSet = ygg::UnorderedSet<fd::FunctionView<f::FluentTag>>;

template<f::FactKind T>
bool targets_metric_function(fp::NumericEffectOperatorView<T> effect, const MetricFunctionSet& metric_functions, fp::MergeDatalogContext& context)
{
    return ygg::visit(
        [&](auto&& arg)
        {
            const auto fterm = merge_p2d<T, f::FluentTag>(arg.get_fterm(), context).first;
            return metric_functions.find(fterm.get_function()) != metric_functions.end();
        },
        effect.get_variant());
}

void append_variable(fp::VariableView variable, fp::MergeDatalogContext& context, ygg::Data<fd::ConjunctiveCondition>& conj_cond)
{
    const auto index = merge_p2d(variable, context).first.get_index();
    if (std::ranges::find(conj_cond.variables, index) == conj_cond.variables.end())
        conj_cond.variables.push_back(index);
}

void fill_delete_free_condition(fp::ActionView action,
                                fp::ConditionalEffectView cond_eff,
                                TranslationContext<LiftedTag>& translation_context,
                                ::tyr::formalism::planning::MergeDatalogContext& context,
                                ygg::Data<::tyr::formalism::datalog::ConjunctiveCondition>& conj_cond)
{
    // Action parameter may get deleted.
    for (const auto& variable : action.get_variables())
        append_variable(variable, context, conj_cond);
    for (const auto literal : action.get_condition().get_literals<::tyr::formalism::StaticTag>())
        conj_cond.static_literals.push_back(merge_p2d(literal, translation_context.p2d.static_to_static_predicate, context).first.get_index());
    for (const auto literal : action.get_condition().get_literals<::tyr::formalism::FluentTag>())
        if (literal.get_polarity())
            conj_cond.fluent_literals.push_back(merge_p2d(literal, translation_context.p2d.fluent_to_fluent_predicate, context).first.get_index());
    for (const auto numeric_constraint : action.get_condition().get_numeric_constraints())
        conj_cond.numeric_constraints.push_back(merge_p2d(numeric_constraint, context));

    for (const auto variable : cond_eff.get_variables())
        append_variable(variable, context, conj_cond);
    for (const auto literal : cond_eff.get_condition().template get_literals<::tyr::formalism::StaticTag>())
        conj_cond.static_literals.push_back(merge_p2d(literal, translation_context.p2d.static_to_static_predicate, context).first.get_index());
    for (const auto literal : cond_eff.get_condition().template get_literals<::tyr::formalism::FluentTag>())
        if (literal.get_polarity())
            conj_cond.fluent_literals.push_back(merge_p2d(literal, translation_context.p2d.fluent_to_fluent_predicate, context).first.get_index());
    for (const auto numeric_constraint : cond_eff.get_condition().get_numeric_constraints())
        conj_cond.numeric_constraints.push_back(merge_p2d(numeric_constraint, context));
}

fd::MetricView create_metric(fp::MetricView metric, fp::MergeDatalogContext& context)
{
    auto metric_ptr = context.builder.get_builder<fd::Metric>();
    auto& result = *metric_ptr;
    result.clear();
    result.fexpr = fp::merge_p2d(metric.get_fexpr(), context);
    canonicalize(result);
    return context.destination.get_or_create(result).first;
}

fd::MetricView create_metric(fd::GroundFunctionTermView<f::FluentTag> term, fp::MergeDatalogContext& context)
{
    auto metric_ptr = context.builder.get_builder<fd::Metric>();
    auto& result = *metric_ptr;
    result.clear();
    result.fexpr = ygg::Data<fd::GroundFunctionExpression>(term.get_index());
    canonicalize(result);
    return context.destination.get_or_create(result).first;
}

auto create_delete_free_goal(fp::GroundConjunctiveConditionView goal,
                             TranslationContext<LiftedTag>& translation_context,
                             ::tyr::formalism::planning::MergeDatalogContext& context)
{
    auto conj_cond_ptr = context.builder.get_builder<::tyr::formalism::datalog::GroundConjunctiveCondition>();
    auto& conj_cond = *conj_cond_ptr;
    conj_cond.clear();

    for (const auto fact : goal.get_facts<::tyr::formalism::PositiveTag>())
        if (const auto literal = merge_p2d(fact, true, translation_context.p2d.fluent_to_fluent_predicate, context))
            conj_cond.fluent_literals.push_back(literal->get_index());

    for (const auto numeric_constraint : goal.get_numeric_constraints())
        conj_cond.numeric_constraints.push_back(merge_p2d(numeric_constraint, context));

    canonicalize(conj_cond);
    return context.destination.get_or_create(conj_cond);
}

bool is_real_conditional_effect(fp::ConditionalEffectView cond_eff)
{
    const auto condition = cond_eff.get_condition();
    return !cond_eff.get_variables().empty() || !condition.get_literals<f::StaticTag>().empty() || !condition.get_literals<f::FluentTag>().empty()
           || !condition.get_literals<f::DerivedTag>().empty() || !condition.get_numeric_constraints().empty();
}

ygg::Data<fd::NumericEffectOperator<f::FluentTag>> create_unit_metric_effect(fd::FunctionTermView<f::FluentTag> term, fp::MergeDatalogContext& context)
{
    auto effect_ptr = context.builder.get_builder<fd::NumericEffect<f::FluentTag>>();
    auto& effect = *effect_ptr;
    effect.clear();
    effect.operator_kind = f::NumericEffectOperatorKind::Increase;
    effect.fterm = term.get_index();
    effect.fexpr = ygg::Data<fd::FunctionExpression>(ygg::float_t(1));
    canonicalize(effect);
    return ygg::Data<fd::NumericEffectOperator<f::FluentTag>>(context.destination.get_or_create(effect).first.get_index());
}

ygg::DataList<fd::NumericEffectOperator<f::FluentTag>> create_unit_metric(ygg::Data<fd::Program>& program, fp::MergeDatalogContext& context)
{
    auto function_ptr = context.builder.get_builder<f::Function<f::FluentTag>>();
    auto& function = *function_ptr;
    function.clear();
    function.name = "__tyr_unit_cost";
    function.arity = 0;
    canonicalize(function);
    const auto unit_function = context.destination.get_or_create(function).first;
    program.fluent_functions.push_back(unit_function.get_index());

    auto lifted_term_ptr = context.builder.get_builder<fd::FunctionTerm<f::FluentTag>>();
    auto& lifted_term = *lifted_term_ptr;
    lifted_term.clear();
    lifted_term.function = unit_function.get_index();
    canonicalize(lifted_term);
    const auto unit_term = context.destination.get_or_create(lifted_term).first;

    auto binding_ptr = context.builder.get_builder<f::RelationBinding<f::Function<f::FluentTag>>>();
    auto& binding = *binding_ptr;
    binding.clear();
    binding.relation = unit_function.get_index();
    canonicalize(binding);
    const auto unit_binding = context.destination.get_or_create(binding).first;

    auto ground_term_ptr = context.builder.get_builder<fd::GroundFunctionTerm<f::FluentTag>>();
    auto& ground_term = *ground_term_ptr;
    ground_term.clear();
    ground_term.binding = unit_binding.get_index();
    canonicalize(ground_term);
    const auto unit_ground_term = context.destination.get_or_create(ground_term).first;

    auto metric_ptr = context.builder.get_builder<fd::Metric>();
    auto& metric = *metric_ptr;
    metric.clear();
    metric.fexpr = ygg::Data<fd::GroundFunctionExpression>(unit_ground_term.get_index());
    canonicalize(metric);
    program.metric = context.destination.get_or_create(metric).first.get_index();

    auto result = ygg::DataList<fd::NumericEffectOperator<f::FluentTag>> {};
    result.push_back(create_unit_metric_effect(unit_term, context));
    return result;
}

ygg::DataList<fd::NumericEffectOperator<f::FluentTag>> create_metric_effects(fp::ActionView action,
                                                                             CostMode cost_mode,
                                                                             const ygg::DataList<fd::NumericEffectOperator<f::FluentTag>>& unit_metric_effects,
                                                                             const MetricFunctionSet& metric_functions,
                                                                             TranslationContext<LiftedTag>& translation_context,
                                                                             fp::MergeDatalogContext& context)
{
    if (cost_mode == CostMode::UNIT)
        return unit_metric_effects;

    auto result = ygg::DataList<fd::NumericEffectOperator<f::FluentTag>> {};
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
                result.push_back(merge_p2d<f::AuxiliaryTag, f::FluentTag>(auxiliary_numeric_effect.value(), context));
            }
    }

    return result;
}

auto create_cond_effect_rule(fp::ActionView action,
                             fp::ConditionalEffectView cond_eff,
                             fp::AtomView<::tyr::formalism::FluentTag> effect,
                             const ygg::DataList<fd::NumericEffectOperator<f::FluentTag>>& metric_effects,
                             TranslationContext<LiftedTag>& translation_context,
                             ::tyr::formalism::planning::MergeDatalogContext& context)
{
    auto rule_ptr = context.builder.get_builder<::tyr::formalism::datalog::Rule<f::PredicateTag>>();
    auto& rule = *rule_ptr;
    rule.clear();

    auto conj_cond_ptr = context.builder.get_builder<::tyr::formalism::datalog::ConjunctiveCondition>();
    auto& conj_cond = *conj_cond_ptr;
    conj_cond.clear();

    fill_delete_free_condition(action, cond_eff, translation_context, context, conj_cond);

    canonicalize(conj_cond);
    const auto new_conj_cond = context.destination.get_or_create(conj_cond).first;

    rule.variables = new_conj_cond.get_variables().get_data();
    rule.body = new_conj_cond.get_index();
    rule.head = merge_p2d(effect, translation_context.p2d.fluent_to_fluent_predicate, context).first.get_index();
    rule.metric_effects = metric_effects;

    canonicalize(rule);
    return context.destination.get_or_create(rule);
}

auto create_cond_numeric_effect_rule(fp::ActionView action,
                                     fp::ConditionalEffectView cond_eff,
                                     fp::NumericEffectOperatorView<::tyr::formalism::FluentTag> effect,
                                     const ygg::DataList<fd::NumericEffectOperator<f::FluentTag>>& metric_effects,
                                     TranslationContext<LiftedTag>& translation_context,
                                     ::tyr::formalism::planning::MergeDatalogContext& context)
{
    auto rule_ptr = context.builder.get_builder<::tyr::formalism::datalog::Rule<f::FunctionTag>>();
    auto& rule = *rule_ptr;
    rule.clear();

    auto conj_cond_ptr = context.builder.get_builder<::tyr::formalism::datalog::ConjunctiveCondition>();
    auto& conj_cond = *conj_cond_ptr;
    conj_cond.clear();

    fill_delete_free_condition(action, cond_eff, translation_context, context, conj_cond);

    canonicalize(conj_cond);
    const auto new_conj_cond = context.destination.get_or_create(conj_cond).first;

    rule.variables = new_conj_cond.get_variables().get_data();
    rule.body = new_conj_cond.get_index();
    rule.head = merge_p2d(effect, context);
    rule.metric_effects = metric_effects;

    canonicalize(rule);
    return context.destination.get_or_create(rule);
}

void translate_action_to_delete_free_rules(fp::ActionView action,
                                           ygg::Data<fd::Program>& program,
                                           CostMode cost_mode,
                                           const ygg::DataList<fd::NumericEffectOperator<f::FluentTag>>& unit_metric_effects,
                                           const MetricFunctionSet& metric_functions,
                                           TranslationContext<LiftedTag>& translation_context,
                                           fp::MergeDatalogContext& context,
                                           RPGProgram<LiftedTag>::RuleToActionMappings& rule_to_action)
{
    const auto metric_effects = create_metric_effects(action, cost_mode, unit_metric_effects, metric_functions, translation_context, context);

    for (const auto cond_eff : action.get_effects())
    {
        for (const auto literal : cond_eff.get_effect().get_literals())
        {
            if (!literal.get_polarity())
                continue;  /// ignore delete effects

            const auto rule = create_cond_effect_rule(action, cond_eff, literal.get_atom(), metric_effects, translation_context, context).first;

            program.predicate_rules.push_back(rule.get_index());
            rule_to_action.predicate.emplace(rule, action);
        }

        for (const auto numeric_effect : cond_eff.get_effect().get_numeric_effects())
        {
            const auto rule = create_cond_numeric_effect_rule(action, cond_eff, numeric_effect, metric_effects, translation_context, context).first;

            program.function_rules.push_back(rule.get_index());
            rule_to_action.function.emplace(rule, action);
        }
    }
}

auto create_program(fp::TaskView task,
                    CostMode cost_mode,
                    TranslationContext<LiftedTag>& translation_context,
                    RPGProgram<LiftedTag>::RuleToActionMappings& rule_to_action,
                    fd::Repository& destination)
{
    auto builder = fd::Builder();
    auto context = fp::MergeDatalogContext(builder, destination);
    auto program_ptr = builder.get_builder<fd::Program>();
    auto& program = *program_ptr;
    program.clear();

    for (const auto predicate : task.get_domain().get_predicates<f::StaticTag>())
    {
        const auto new_predicate = fp::merge_p2d(predicate, context).first;
        translation_context.d2p.static_to_static_predicate.emplace(new_predicate, predicate);
        translation_context.p2d.static_to_static_predicate.emplace(predicate, new_predicate);
        program.static_predicates.push_back(new_predicate.get_index());
    }
    for (const auto predicate : task.get_domain().get_predicates<f::FluentTag>())
    {
        const auto new_predicate = fp::merge_p2d(predicate, context).first;
        translation_context.d2p.fluent_to_fluent_predicate.emplace(new_predicate, predicate);
        translation_context.p2d.fluent_to_fluent_predicate.emplace(predicate, new_predicate);
        program.fluent_predicates.push_back(new_predicate.get_index());
    }

    for (const auto function : task.get_domain().get_functions<f::StaticTag>())
        program.static_functions.push_back(fp::merge_p2d(function, context).first.get_index());
    for (const auto function : task.get_domain().get_functions<f::FluentTag>())
        program.fluent_functions.push_back(fp::merge_p2d(function, context).first.get_index());
    if (const auto function = task.get_domain().get_auxiliary_function())
        program.fluent_functions.push_back(fp::merge_p2d<f::AuxiliaryTag, f::FluentTag>(function.value(), context).first.get_index());

    for (const auto object : task.get_domain().get_constants())
        program.objects.push_back(fp::merge_p2d(object, context).first.get_index());
    for (const auto object : task.get_objects())
        program.objects.push_back(fp::merge_p2d(object, context).first.get_index());

    for (const auto atom : task.get_atoms<f::StaticTag>())
        program.static_atoms.push_back(fp::merge_p2d(atom, translation_context.p2d.static_to_static_predicate, context).first.get_index());
    for (const auto atom : task.get_atoms<f::FluentTag>())
        program.fluent_atoms.push_back(fp::merge_p2d(atom, translation_context.p2d.fluent_to_fluent_predicate, context).first.get_index());

    for (const auto fterm_value : task.get_fterm_values<f::StaticTag>())
        program.static_fterm_values.push_back(fp::merge_p2d(fterm_value, context).first.get_index());
    for (const auto fterm_value : task.get_fterm_values<f::FluentTag>())
        program.fluent_fterm_values.push_back(fp::merge_p2d(fterm_value, context).first.get_index());
    if (const auto fterm_value = task.get_auxiliary_fterm_value())
        program.fluent_fterm_values.push_back(fp::merge_p2d<f::AuxiliaryTag, f::FluentTag>(fterm_value.value(), context).first.get_index());

    program.goal = create_delete_free_goal(task.get_goal(), translation_context, context).first.get_index();
    auto metric_functions = MetricFunctionSet {};
    auto unit_metric_effects = ygg::DataList<fd::NumericEffectOperator<f::FluentTag>> {};
    if (cost_mode == CostMode::UNIT)
    {
        unit_metric_effects = create_unit_metric(program, context);
    }
    else if (task.get_auxiliary_fterm_value())
    {
        const auto fterm_value = fp::merge_p2d<f::AuxiliaryTag, f::FluentTag>(task.get_auxiliary_fterm_value().value(), context).first;
        const auto metric = create_metric(fterm_value.get_fterm(), context);
        program.metric = metric.get_index();
        metric_functions.insert(fterm_value.get_fterm().get_function());
    }
    else if (task.get_metric())
    {
        const auto metric = create_metric(task.get_metric().value(), context);
        program.metric = metric.get_index();

        auto metric_fterms = ygg::UnorderedSet<fd::GroundFunctionTermView<f::FluentTag>> {};
        fd::collect_fterms(metric.get_fexpr(), metric_fterms);
        for (const auto fterm : metric_fterms)
            metric_functions.insert(fterm.get_function());
    }

    for (const auto action : task.get_domain().get_actions())
        translate_action_to_delete_free_rules(action, program, cost_mode, unit_metric_effects, metric_functions, translation_context, context, rule_to_action);

    canonicalize(program);
    return destination.get_or_create(program).first;
}

auto create_datalog_program(fp::TaskView task,
                            CostMode cost_mode,
                            TranslationContext<LiftedTag>& translation_context,
                            RPGProgram<LiftedTag>::RuleToActionMappings& rule_to_action)
{
    auto factory = std::make_shared<fd::RepositoryFactory>();
    auto repository = factory->create_shared();
    auto program = create_program(task, cost_mode, translation_context, rule_to_action, *repository);
    return datalog::Program<LiftedTag>(program, std::move(repository), std::move(factory));
}

}

RPGProgram<LiftedTag>::RPGProgram(fp::TaskView task, CostMode cost_mode) :
    m_translation_context(),
    m_rule_to_action(),
    m_datalog_program(create_datalog_program(task, cost_mode, m_translation_context, m_rule_to_action))
{
    // std::cout << m_datalog_program.get_program() << std::endl;
}

const TranslationContext<LiftedTag>& RPGProgram<LiftedTag>::get_translation_context() const noexcept { return m_translation_context; }

template<>
const RPGProgram<LiftedTag>::RuleToActionMapping<f::PredicateTag>& RPGProgram<LiftedTag>::get_rule_to_action_mapping<f::PredicateTag>() const noexcept
{
    return m_rule_to_action.predicate;
}

template<>
const RPGProgram<LiftedTag>::RuleToActionMapping<f::FunctionTag>& RPGProgram<LiftedTag>::get_rule_to_action_mapping<f::FunctionTag>() const noexcept
{
    return m_rule_to_action.function;
}

datalog::Program<LiftedTag>& RPGProgram<LiftedTag>::get_datalog_program() noexcept { return m_datalog_program; }

const datalog::Program<LiftedTag>& RPGProgram<LiftedTag>::get_datalog_program() const noexcept { return m_datalog_program; }

const datalog::ConstProgramWorkspace<LiftedTag>& RPGProgram<LiftedTag>::get_const_program_workspace() const noexcept
{
    return m_datalog_program.get_const_program_workspace();
}

::tyr::formalism::datalog::GroundConjunctiveConditionView RPGProgram<LiftedTag>::get_goal() const noexcept
{
    return m_datalog_program.get_program().get_goal().value();
}

}
