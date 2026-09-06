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
#include "tyr/datalog/applicability.hpp"
#include "tyr/datalog/static_rule_filter.hpp"
#include "tyr/formalism/datalog/expression_properties.hpp"
#include "tyr/formalism/datalog/merge.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/planning/merge_datalog.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/planning/views.hpp"

#include <optional>
#include <stdexcept>
#include <yggdrasil/containers/unique_object_pool.hpp>
#include <yggdrasil/containers/unordered_set.hpp>

namespace f = tyr::formalism;
namespace d = tyr::datalog;
namespace fp = tyr::formalism::planning;
namespace fd = tyr::formalism::datalog;

namespace tyr::planning
{
namespace
{
using MetricGroundFunctionTermSet = ygg::UnorderedSet<fd::FunctionTermView<::tyr::GroundTag, f::FluentTag>>;

template<f::FactKind T>
bool targets_metric_fterm(fp::NumericEffectOperatorView<::tyr::GroundTag, T> effect, const MetricGroundFunctionTermSet& metric_fterms, fp::MergeDatalogContext& context)
{
    return ygg::visit(
        [&](auto&& arg)
        {
            const auto fterm = merge_p2d<T, f::FluentTag>(arg.get_fterm(), context).first;
            return metric_fterms.find(fterm) != metric_fterms.end();
        },
        effect.get_variant());
}

struct GroundProgramBuildContext
{
    using StaticPredicateMapping = ygg::UnorderedMap<fp::PredicateView<f::StaticTag>, fd::PredicateView<f::StaticTag>>;
    using FluentPredicateMapping = ygg::UnorderedMap<fp::PredicateView<f::FluentTag>, fd::PredicateView<f::FluentTag>>;
    fd::Builder builder;
    fp::MergeDatalogContext merge_context;
    ygg::UniqueObjectPoolPtr<ygg::Data<fd::Program<::tyr::GroundTag>>> program;
    StaticPredicateMapping static_predicates;
    FluentPredicateMapping fluent_predicates;
    ygg::uint_t next_rule_id = 0;

    GroundProgramBuildContext(fd::Repository& repository) : builder(), merge_context(builder, repository), program(fd::checkout<fd::Program<::tyr::GroundTag>>(builder)) {}
};

ygg::Data<fd::NumericEffectOperator<::tyr::LiftedTag, f::FluentTag>> create_rule_binding_numeric_head(GroundProgramBuildContext& context)
{
    auto function = fd::checkout<f::Function<f::FluentTag>>(context.builder);
    function->name = "__tyr_ground_rule_binding";
    function->arity = 0;
    const auto function_view = fd::get_or_create(context.merge_context.destination, *function).first;

    auto term = fd::checkout<fd::FunctionTerm<::tyr::LiftedTag, f::FluentTag>>(context.builder);
    term->function = function_view.get_index();
    const auto term_view = fd::get_or_create(context.merge_context.destination, *term).first;

    auto effect = fd::checkout<fd::NumericEffect<::tyr::LiftedTag, f::FluentTag>>(context.builder);
    effect->operator_kind = f::NumericEffectOperatorKind::Assign;
    effect->fterm = term_view.get_index();
    effect->fexpr = ygg::Data<fd::FunctionExpression<::tyr::LiftedTag>>(ygg::float_t(0));
    return ygg::Data<fd::NumericEffectOperator<::tyr::LiftedTag, f::FluentTag>>(f::NumericEffectOperatorKind::Assign,
                                                              fd::get_or_create(context.merge_context.destination, *effect).first.get_index());
}

template<f::RelationKind R, typename CreateHead>
fd::RuleBindingView<R> create_rule_binding(GroundProgramBuildContext& context, CreateHead&& create_head)
{
    auto predicate = fd::checkout<f::Predicate<f::FluentTag>>(context.builder);
    predicate->name = "ground_rule_" + std::to_string(context.next_rule_id++);
    predicate->arity = 0;
    const auto new_predicate = fd::get_or_create(context.merge_context.destination, *predicate).first;
    context.program->fluent_predicates.push_back(new_predicate.get_index());

    auto atom = fd::checkout<fd::Atom<::tyr::LiftedTag, f::FluentTag>>(context.builder);
    atom->predicate = new_predicate.get_index();
    const auto new_atom = fd::get_or_create(context.merge_context.destination, *atom).first;

    auto literal = fd::checkout<fd::Literal<::tyr::LiftedTag, f::FluentTag>>(context.builder);
    literal->atom = new_atom.get_index();
    literal->polarity = false;
    const auto new_literal = fd::get_or_create(context.merge_context.destination, *literal).first;

    auto condition = fd::checkout<fd::ConjunctiveCondition<::tyr::LiftedTag>>(context.builder);
    // Keep synthetic function-rule keys distinct without adding a positive witness precondition.
    condition->fluent_literals.push_back(new_literal.get_index());
    const auto new_condition = fd::get_or_create(context.merge_context.destination, *condition).first;

    auto rule = fd::checkout<fd::Rule<::tyr::LiftedTag, R>>(context.builder);
    rule->body = new_condition.get_index();
    rule->head = create_head(new_atom);
    const auto new_rule = fd::get_or_create(context.merge_context.destination, *rule).first;

    auto binding = fd::checkout<f::RelationBinding<fd::Rule<::tyr::LiftedTag, R>>>(context.builder);
    binding->relation = new_rule.get_index();
    return fd::get_or_create(context.merge_context.destination, *binding).first;
}

fd::AtomView<::tyr::GroundTag, f::FluentTag> create_applicability_atom(fp::ActionView<::tyr::GroundTag> action, GroundProgramBuildContext& context)
{
    auto predicate = fd::checkout<f::Predicate<f::FluentTag>>(context.builder);
    predicate->name = create_applicability_name(action.get_action());
    predicate->arity = action.get_objects().size();
    const auto new_predicate = fd::get_or_create(context.merge_context.destination, *predicate).first;

    context.program->fluent_predicates.push_back(new_predicate.get_index());

    auto binding = fd::checkout<f::RelationBinding<f::Predicate<f::FluentTag>>>(context.builder);
    binding->relation = new_predicate.get_index();
    for (const auto object : action.get_objects())
        binding->objects.push_back(object.get_index());
    const auto new_binding = fd::get_or_create(context.merge_context.destination, *binding).first;

    auto atom = fd::checkout<fd::Atom<::tyr::GroundTag, f::FluentTag>>(context.builder);
    atom->binding = new_binding.get_index();
    return fd::get_or_create(context.merge_context.destination, *atom).first;
}

fd::LiteralView<::tyr::GroundTag, f::FluentTag> create_positive_literal(fd::AtomView<::tyr::GroundTag, f::FluentTag> atom, GroundProgramBuildContext& context)
{
    auto literal = fd::checkout<fd::Literal<::tyr::GroundTag, f::FluentTag>>(context.builder);
    literal->atom = atom.get_index();
    literal->polarity = true;
    return fd::get_or_create(context.merge_context.destination, *literal).first;
}

void fill_delete_free_condition(fp::ConjunctiveConditionView<::tyr::GroundTag> condition,
                                TranslationContext<GroundTag>& translation_context,
                                GroundProgramBuildContext& context,
                                ygg::Data<fd::ConjunctiveCondition<::tyr::GroundTag>>& result)
{
    for (const auto fact : condition.template get_facts<f::PositiveTag>())
        if (const auto literal = fp::merge_p2d(fact, true, translation_context.p2d.fluent_to_fluent_atom, context.fluent_predicates, context.merge_context))
            result.fluent_literals.push_back(literal->get_index());

    for (const auto numeric_constraint : condition.get_numeric_constraints())
        result.numeric_constraints.push_back(fp::merge_p2d(numeric_constraint, context.merge_context));
}

fd::ConjunctiveConditionView<::tyr::GroundTag> create_delete_free_condition(fp::ConjunctiveConditionView<::tyr::GroundTag> condition,
                                                                TranslationContext<GroundTag>& translation_context,
                                                                GroundProgramBuildContext& context)
{
    auto result = fd::checkout<fd::ConjunctiveCondition<::tyr::GroundTag>>(context.builder);
    fill_delete_free_condition(condition, translation_context, context, *result);

    return fd::get_or_create(context.merge_context.destination, *result).first;
}

fd::ConjunctiveConditionView<::tyr::GroundTag>
create_delete_free_goal(fp::ConjunctiveConditionView<::tyr::GroundTag> goal, TranslationContext<GroundTag>& translation_context, GroundProgramBuildContext& context)
{
    return create_delete_free_condition(goal, translation_context, context);
}

fd::ConjunctiveConditionView<::tyr::GroundTag> create_delete_free_effect_condition(fd::AtomView<::tyr::GroundTag, f::FluentTag> applicability_atom,
                                                                       fp::ConjunctiveConditionView<::tyr::GroundTag> effect_condition,
                                                                       TranslationContext<GroundTag>& translation_context,
                                                                       GroundProgramBuildContext& context)
{
    auto result = fd::checkout<fd::ConjunctiveCondition<::tyr::GroundTag>>(context.builder);
    fill_delete_free_condition(effect_condition, translation_context, context, *result);
    result->fluent_literals.push_back(create_positive_literal(applicability_atom, context).get_index());
    return fd::get_or_create(context.merge_context.destination, *result).first;
}

fd::ConjunctiveConditionView<::tyr::GroundTag> create_delete_free_numeric_effect_condition(fp::ConjunctiveConditionView<::tyr::GroundTag> action_condition,
                                                                               fp::ConjunctiveConditionView<::tyr::GroundTag> effect_condition,
                                                                               TranslationContext<GroundTag>& translation_context,
                                                                               GroundProgramBuildContext& context)
{
    auto result = fd::checkout<fd::ConjunctiveCondition<::tyr::GroundTag>>(context.builder);
    fill_delete_free_condition(action_condition, translation_context, context, *result);
    fill_delete_free_condition(effect_condition, translation_context, context, *result);
    return fd::get_or_create(context.merge_context.destination, *result).first;
}

bool is_real_conditional_effect(fp::ConditionalEffectView<::tyr::GroundTag> cond_eff)
{
    const auto condition = cond_eff.get_condition();
    return !condition.get_literals<f::StaticTag>().empty() || !condition.get_facts<f::PositiveTag>().empty() || !condition.get_facts<f::NegativeTag>().empty()
           || !condition.get_literals<f::DerivedTag>().empty() || !condition.get_numeric_constraints().empty();
}

ygg::Data<fd::NumericEffectOperator<::tyr::GroundTag, f::FluentTag>> create_unit_metric_effect(fd::FunctionTermView<::tyr::GroundTag, f::FluentTag> term,
                                                                                   GroundProgramBuildContext& context)
{
    auto effect = fd::checkout<fd::NumericEffect<::tyr::GroundTag, f::FluentTag>>(context.builder);
    effect->operator_kind = f::NumericEffectOperatorKind::Increase;
    effect->fterm = term.get_index();
    effect->fexpr = ygg::Data<fd::FunctionExpression<::tyr::GroundTag>>(ygg::float_t(1));
    return ygg::Data<fd::NumericEffectOperator<::tyr::GroundTag, f::FluentTag>>(f::NumericEffectOperatorKind::Increase,
                                                                    fd::get_or_create(context.merge_context.destination, *effect).first.get_index());
}

ygg::DataList<fd::NumericEffectOperator<::tyr::GroundTag, f::FluentTag>> create_unit_metric(GroundProgramBuildContext& context)
{
    auto function = fd::checkout<f::Function<f::FluentTag>>(context.builder);
    function->name = "__tyr_unit_cost";
    function->arity = 0;
    const auto unit_function = fd::get_or_create(context.merge_context.destination, *function).first;
    context.program->fluent_functions.push_back(unit_function.get_index());

    auto binding = fd::checkout<f::RelationBinding<f::Function<f::FluentTag>>>(context.builder);
    binding->relation = unit_function.get_index();
    const auto unit_binding = fd::get_or_create(context.merge_context.destination, *binding).first;

    auto ground_term = fd::checkout<fd::FunctionTerm<::tyr::GroundTag, f::FluentTag>>(context.builder);
    ground_term->binding = unit_binding.get_index();
    const auto unit_ground_term = fd::get_or_create(context.merge_context.destination, *ground_term).first;

    auto metric = fd::checkout<fd::Metric>(context.builder);
    metric->fexpr = ygg::Data<fd::FunctionExpression<::tyr::GroundTag>>(unit_ground_term.get_index());
    context.program->metric = fd::get_or_create(context.merge_context.destination, *metric).first.get_index();

    auto result = ygg::DataList<fd::NumericEffectOperator<::tyr::GroundTag, f::FluentTag>> {};
    result.push_back(create_unit_metric_effect(unit_ground_term, context));
    return result;
}

ygg::DataList<fd::NumericEffectOperator<::tyr::GroundTag, f::FluentTag>>
create_metric_effects(fp::ActionView<::tyr::GroundTag> action,
                      CostMode cost_mode,
                      const ygg::DataList<fd::NumericEffectOperator<::tyr::GroundTag, f::FluentTag>>& unit_metric_effects,
                      const MetricGroundFunctionTermSet& metric_fterms,
                      GroundProgramBuildContext& context)
{
    if (cost_mode == CostMode::UNIT)
        return unit_metric_effects;

    auto result = ygg::DataList<fd::NumericEffectOperator<::tyr::GroundTag, f::FluentTag>> {};
    if (metric_fterms.empty())
        return result;

    for (const auto cond_eff : action.get_effects())
    {
        for (const auto numeric_effect : cond_eff.get_effect().get_numeric_effects())
            if (targets_metric_fterm(numeric_effect, metric_fterms, context.merge_context))
            {
                if (is_real_conditional_effect(cond_eff))
                    throw std::invalid_argument("GENERAL action costs with :conditional-effects are unsupported; compile conditional effects away first.");
                result.push_back(fp::merge_p2d(numeric_effect, context.merge_context));
            }

        if (const auto auxiliary_numeric_effect = cond_eff.get_effect().get_auxiliary_numeric_effect())
            if (targets_metric_fterm(auxiliary_numeric_effect.value(), metric_fterms, context.merge_context))
            {
                if (is_real_conditional_effect(cond_eff))
                    throw std::invalid_argument("GENERAL action costs with :conditional-effects are unsupported; compile conditional effects away first.");
                result.push_back(fp::merge_p2d<f::AuxiliaryTag, f::FluentTag>(auxiliary_numeric_effect.value(), context.merge_context));
            }
    }

    return result;
}

fd::RuleView<::tyr::GroundTag, f::PredicateTag> create_ground_atom_rule(fd::ConjunctiveConditionView<::tyr::GroundTag> body,
                                                            fd::AtomView<::tyr::GroundTag, f::FluentTag> head,
                                                            GroundProgramBuildContext& context,
                                                            ygg::DataList<fd::NumericEffectOperator<::tyr::GroundTag, f::FluentTag>> metric_effects = {})
{
    auto rule = fd::checkout<fd::Rule<::tyr::GroundTag, f::PredicateTag>>(context.builder);
    rule->binding = create_rule_binding<f::PredicateTag>(context, [](auto atom) { return atom.get_index(); }).get_index();
    rule->body = body.get_index();
    rule->head = head.get_index();
    rule->metric_effects.insert(rule->metric_effects.end(), metric_effects.begin(), metric_effects.end());
    return fd::get_or_create(context.merge_context.destination, *rule).first;
}

fd::RuleView<::tyr::GroundTag, f::FunctionTag> create_ground_numeric_effect_rule(fd::ConjunctiveConditionView<::tyr::GroundTag> body,
                                                                     fp::NumericEffectOperatorView<::tyr::GroundTag, f::FluentTag> head,
                                                                     GroundProgramBuildContext& context,
                                                                     ygg::DataList<fd::NumericEffectOperator<::tyr::GroundTag, f::FluentTag>> metric_effects = {})
{
    const auto datalog_head = fp::merge_p2d(head, context.merge_context);
    auto rule = fd::checkout<fd::Rule<::tyr::GroundTag, f::FunctionTag>>(context.builder);
    rule->binding = create_rule_binding<f::FunctionTag>(context, [&](auto) { return create_rule_binding_numeric_head(context); }).get_index();
    rule->body = body.get_index();
    rule->head = datalog_head;
    rule->metric_effects.insert(rule->metric_effects.end(), metric_effects.begin(), metric_effects.end());
    return fd::get_or_create(context.merge_context.destination, *rule).first;
}

fd::RuleView<::tyr::GroundTag, f::PredicateTag> create_applicability_rule(fp::ActionView<::tyr::GroundTag> action,
                                                              fd::AtomView<::tyr::GroundTag, f::FluentTag> applicability_atom,
                                                              TranslationContext<GroundTag>& translation_context,
                                                              GroundProgramBuildContext& context)
{
    return create_ground_atom_rule(create_delete_free_condition(action.get_condition(), translation_context, context), applicability_atom, context);
}

fd::ProgramView<GroundTag> finish_program(GroundProgramBuildContext& context)
{
    return fd::get_or_create(context.merge_context.destination, *context.program).first;
}

void translate_action_to_delete_free_rules(fp::ActionView<::tyr::GroundTag> action,
                                           ygg::Data<fd::Program<::tyr::GroundTag>>& program,
                                           CostMode cost_mode,
                                           const ygg::DataList<fd::NumericEffectOperator<::tyr::GroundTag, f::FluentTag>>& unit_metric_effects,
                                           const MetricGroundFunctionTermSet& metric_fterms,
                                           TranslationContext<GroundTag>& translation_context,
                                           GroundProgramBuildContext& context,
                                           RPGProgram<GroundTag>::RuleToActionMappings& rule_to_action)
{
    const auto applicability_atom = create_applicability_atom(action, context);
    const auto applicability_rule = create_applicability_rule(action, applicability_atom, translation_context, context);
    program.predicate_rules.push_back(applicability_rule.get_index());
    const auto metric_effects = create_metric_effects(action, cost_mode, unit_metric_effects, metric_fterms, context);

    for (const auto cond_eff : action.get_effects())
    {
        const auto body = create_delete_free_effect_condition(applicability_atom, cond_eff.get_condition(), translation_context, context);

        for (const auto fact : cond_eff.get_effect().get_facts<f::PositiveTag>())
        {
            if (const auto literal = fp::merge_p2d(fact, true, translation_context.p2d.fluent_to_fluent_atom, context.fluent_predicates, context.merge_context))
            {
                const auto rule = create_ground_atom_rule(body, literal->get_atom(), context, metric_effects);
                program.predicate_rules.push_back(rule.get_index());
                rule_to_action.predicate.emplace(rule.get_row(), action);
            }
        }

        const auto numeric_body = create_delete_free_numeric_effect_condition(action.get_condition(), cond_eff.get_condition(), translation_context, context);
        for (const auto numeric_effect : cond_eff.get_effect().get_numeric_effects())
        {
            const auto rule = create_ground_numeric_effect_rule(numeric_body, numeric_effect, context, metric_effects);
            program.function_rules.push_back(rule.get_index());
            rule_to_action.function.emplace(rule.get_row(), action);
        }
    }
}

fd::ProgramView<GroundTag> create_rpg_ground_program(fp::FDRTaskView task,
                                                     CostMode cost_mode,
                                                     TranslationContext<GroundTag>& translation_context,
                                                     RPGProgram<GroundTag>::RuleToActionMappings& mapping,
                                                     fd::Repository& repository)
{
    auto context = GroundProgramBuildContext(repository);
    auto& merge_context = context.merge_context;

    for (const auto predicate : task.get_domain().get_predicates<f::StaticTag>())
    {
        const auto new_predicate = fp::merge_p2d(predicate, merge_context).first;
        context.static_predicates.emplace(predicate, new_predicate);
        context.program->static_predicates.push_back(new_predicate.get_index());
    }
    for (const auto predicate : task.get_domain().get_predicates<f::FluentTag>())
    {
        const auto new_predicate = fp::merge_p2d(predicate, merge_context).first;
        context.fluent_predicates.emplace(predicate, new_predicate);
        context.program->fluent_predicates.push_back(new_predicate.get_index());
    }

    for (const auto function : task.get_domain().get_functions<f::StaticTag>())
        context.program->static_functions.push_back(fp::merge_p2d(function, merge_context).first.get_index());
    for (const auto function : task.get_domain().get_functions<f::FluentTag>())
        context.program->fluent_functions.push_back(fp::merge_p2d(function, merge_context).first.get_index());
    if (const auto function = task.get_domain().get_auxiliary_function())
        context.program->fluent_functions.push_back(fp::merge_p2d<f::AuxiliaryTag, f::FluentTag>(function.value(), merge_context).first.get_index());

    for (const auto object : task.get_domain().get_constants())
        context.program->objects.push_back(fp::merge_p2d(object, merge_context).first.get_index());
    for (const auto object : task.get_objects())
        context.program->objects.push_back(fp::merge_p2d(object, merge_context).first.get_index());

    for (const auto atom : task.get_atoms<f::StaticTag>())
    {
        const auto new_atom = fp::merge_p2d(atom, context.static_predicates, merge_context).first;
        translation_context.p2d.static_to_static_atom.emplace(atom, new_atom);
        translation_context.d2p.static_to_static_atom.emplace(new_atom, atom);
        context.program->static_atoms.push_back(new_atom.get_index());
    }
    for (const auto atom : task.get_atoms<f::FluentTag>())
    {
        const auto new_atom = fp::merge_p2d(atom, translation_context.p2d.fluent_to_fluent_atom, context.fluent_predicates, merge_context).first;
        translation_context.d2p.fluent_to_fluent_atom.emplace(new_atom.get_row(), atom);
        context.program->fluent_atoms.push_back(new_atom.get_index());
    }
    for (const auto fact : task.get_fluent_facts())
    {
        if (const auto atom = fact.get_atom())
        {
            const auto [new_atom, inserted] = fp::merge_p2d(*atom, translation_context.p2d.fluent_to_fluent_atom, context.fluent_predicates, merge_context);
            translation_context.d2p.fluent_to_fluent_atom.emplace(new_atom.get_row(), *atom);
            if (inserted)
                context.program->fluent_atoms.push_back(new_atom.get_index());
        }
    }

    for (const auto fterm_value : task.get_fterm_values<f::StaticTag>())
    {
        const auto new_fterm_value = fp::merge_p2d(fterm_value, merge_context).first;
        translation_context.p2d.static_to_static_fterm.emplace(fterm_value.get_fterm(), new_fterm_value.get_fterm());
        translation_context.d2p.static_to_static_fterm.emplace(new_fterm_value.get_fterm(), fterm_value.get_fterm());
        context.program->static_fterm_values.push_back(new_fterm_value.get_index());
    }
    for (const auto fterm_value : task.get_fterm_values<f::FluentTag>())
    {
        const auto new_fterm_value = fp::merge_p2d(fterm_value, merge_context).first;
        translation_context.p2d.fluent_to_fluent_fterm.emplace(fterm_value.get_fterm(), new_fterm_value.get_fterm());
        translation_context.d2p.fluent_to_fluent_fterm.emplace(new_fterm_value.get_fterm(), fterm_value.get_fterm());
        context.program->fluent_fterm_values.push_back(new_fterm_value.get_index());
    }
    if (const auto fterm_value = task.get_auxiliary_fterm_value())
    {
        const auto new_fterm_value = fp::merge_p2d<f::AuxiliaryTag, f::FluentTag>(fterm_value.value(), merge_context).first;
        context.program->fluent_fterm_values.push_back(new_fterm_value.get_index());
    }

    context.program->goal = create_delete_free_goal(task.get_goal(), translation_context, context).get_index();
    auto metric_fterms = MetricGroundFunctionTermSet {};
    auto unit_metric_effects = ygg::DataList<fd::NumericEffectOperator<::tyr::GroundTag, f::FluentTag>> {};
    if (cost_mode == CostMode::UNIT)
    {
        unit_metric_effects = create_unit_metric(context);
    }
    else if (task.get_metric())
    {
        const auto metric = fp::merge_p2d(task.get_metric().value(), context.merge_context).first;
        context.program->metric = metric.get_index();
        fd::collect_fterms(metric.get_fexpr(), metric_fterms);
    }

    for (const auto action : task.get_ground_actions())
        translate_action_to_delete_free_rules(action, *context.program, cost_mode, unit_metric_effects, metric_fterms, translation_context, context, mapping);

    return finish_program(context);
}

TranslationContext<GroundTag> remap_translation_context(const TranslationContext<GroundTag>& source, fd::MergeContext& context)
{
    auto result = TranslationContext<GroundTag> {};
    const auto remap_keys = [&](const auto& source_mapping, auto& result_mapping)
    {
        for (const auto& [key, value] : source_mapping)
            result_mapping.emplace(fd::merge_d2d(key, context).first, value);
    };
    const auto remap_values = [&](const auto& source_mapping, auto& result_mapping)
    {
        for (const auto& [key, value] : source_mapping)
            result_mapping.emplace(key, fd::merge_d2d(value, context).first);
    };

    remap_keys(source.d2p.static_to_static_atom, result.d2p.static_to_static_atom);
    remap_keys(source.d2p.fluent_to_fluent_atom, result.d2p.fluent_to_fluent_atom);
    remap_keys(source.d2p.fluent_to_derived_atom, result.d2p.fluent_to_derived_atom);
    remap_keys(source.d2p.static_to_static_fterm, result.d2p.static_to_static_fterm);
    remap_keys(source.d2p.fluent_to_fluent_fterm, result.d2p.fluent_to_fluent_fterm);
    remap_values(source.p2d.static_to_static_atom, result.p2d.static_to_static_atom);
    remap_values(source.p2d.fluent_to_fluent_atom, result.p2d.fluent_to_fluent_atom);
    remap_values(source.p2d.derived_to_fluent_atom, result.p2d.derived_to_fluent_atom);
    remap_values(source.p2d.static_to_static_fterm, result.p2d.static_to_static_fterm);
    remap_values(source.p2d.fluent_to_fluent_fterm, result.p2d.fluent_to_fluent_fterm);
    return result;
}

template<f::RelationKind R>
void remap_rule_to_action(const RPGProgram<GroundTag>::RuleToActionMapping<R>& source_mapping,
                          RPGProgram<GroundTag>::RuleToActionMapping<R>& result_mapping,
                          fd::MergeContext& context,
                          const d::FactSets& fact_sets)
{
    for (const auto& [source_binding, action] : source_mapping)
    {
        const auto source_rule = fd::find_ground_rule(source_binding);
        if (!source_rule)
            throw std::logic_error("Ground rule binding has no rule");
        if (!d::is_statically_applicable(*source_rule, fact_sets))
            continue;

        const auto [result_rule, inserted] = fd::merge_d2d(*source_rule, context);
        if (inserted)
            throw std::logic_error("Static rule filtering omitted a retained rule");
        result_mapping.emplace(result_rule.get_row(), action);
    }
}

d::Program<GroundTag> create_rpg_datalog_program(fp::FDRTaskView task,
                                                 CostMode cost_mode,
                                                 TranslationContext<GroundTag>& translation_context,
                                                 RPGProgram<GroundTag>::RuleToActionMappings& mapping)
{
    auto factory = std::make_shared<fd::RepositoryFactory>();
    const auto num_objects = task.get_domain().get_constants().size() + task.get_objects().size();
    auto source_repository = factory->create(num_objects);
    auto source_translation_context = TranslationContext<GroundTag> {};
    auto source_mapping = RPGProgram<GroundTag>::RuleToActionMappings {};
    const auto source_program = create_rpg_ground_program(task, cost_mode, source_translation_context, source_mapping, source_repository);
    const auto source_static_fact_sets = d::TaggedFactSets<f::StaticTag>(source_program.get_predicates<f::StaticTag>(),
                                                                         source_program.get_functions<f::StaticTag>(),
                                                                         source_program.get_atoms<f::StaticTag>(),
                                                                         source_program.get_fterm_values<f::StaticTag>(),
                                                                         source_program.get_context());
    const auto source_fluent_fact_sets = d::TaggedFactSets<f::FluentTag>(source_program.get_predicates<f::FluentTag>(),
                                                                         source_program.get_functions<f::FluentTag>(),
                                                                         source_program.get_atoms<f::FluentTag>(),
                                                                         source_program.get_fterm_values<f::FluentTag>(),
                                                                         source_program.get_context());
    const auto source_fact_sets = d::FactSets { source_static_fact_sets, source_fluent_fact_sets };

    auto repository = factory->create_shared(num_objects);
    const auto program = d::remove_statically_inapplicable_rules(source_program, *repository);
    auto builder = fd::Builder {};
    auto merge_context = fd::MergeContext { builder, *repository };
    translation_context = remap_translation_context(source_translation_context, merge_context);
    remap_rule_to_action<f::PredicateTag>(source_mapping.predicate, mapping.predicate, merge_context, source_fact_sets);
    remap_rule_to_action<f::FunctionTag>(source_mapping.function, mapping.function, merge_context, source_fact_sets);
    return d::Program<GroundTag>(program, std::move(repository), std::move(factory));
}

}  // namespace

template<>
RPGProgram<GroundTag>::RPGProgram(fp::FDRTaskView task, CostMode cost_mode) :
    m_translation_context(),
    m_rule_to_action(),
    m_datalog_program(create_rpg_datalog_program(task, cost_mode, m_translation_context, m_rule_to_action))
{
}

}  // namespace tyr::planning
