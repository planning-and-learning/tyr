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
#include "tyr/formalism/atom_view.hpp"
#include "tyr/formalism/binary_operator_view.hpp"
#include "tyr/formalism/datalog/copy.hpp"
#include "tyr/formalism/datalog/interning.hpp"
#include "tyr/formalism/datalog/repository.hpp"
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

#include <optional>
#include <stdexcept>
#include <urd/repr/maps/std_map.hpp>
#include <urd/repr/maps/unordered_flat_map.hpp>
#include <urd/repr/slots/slots.hpp>
#include <urd/repr/values/unordered_flat_set.hpp>
#include <urd/store.hpp>
#include <utility>
#include <yggdrasil/formalism/row_view.hpp>

namespace tyr::planning
{
namespace
{
struct GroundProgramBuildContext
{
    using StaticPredicateMapping =
        urd::Store<urd::map<formalism::planning::PredicateView<formalism::StaticTag>, formalism::datalog::PredicateView<formalism::StaticTag>>>;
    using FluentPredicateMapping =
        urd::Store<urd::map<formalism::planning::PredicateView<formalism::FluentTag>, formalism::datalog::PredicateView<formalism::FluentTag>>>;
    formalism::datalog::Builder builder;
    formalism::WriteContext<formalism::datalog::Builder, formalism::datalog::Repository> merge_context;
    urd::unique_slot_ptr<urd::data<formalism::datalog::Program<GroundTag>>> program;
    StaticPredicateMapping static_predicates;
    FluentPredicateMapping fluent_predicates;
    ygg::uint_t next_rule_id = 0;

    GroundProgramBuildContext(formalism::datalog::Repository& repository) :
        builder(),
        merge_context(builder, repository),
        program(formalism::datalog::checkout<formalism::datalog::Program<GroundTag>>(builder))
    {
    }
};

urd::index<formalism::NumericEffect<LiftedTag, formalism::FluentTag>> create_rule_binding_numeric_head(GroundProgramBuildContext& context)
{
    auto function = formalism::datalog::checkout<formalism::Function<formalism::FluentTag>>(context.builder);
    function->name = "__tyr_ground_rule_binding";
    function->arity = 0;
    const auto function_view = formalism::datalog::insert(context.merge_context.destination(), *function).first;

    auto term = formalism::datalog::checkout<formalism::FunctionTerm<LiftedTag, formalism::FluentTag>>(context.builder);
    term->function = function_view.index();
    const auto term_view = formalism::datalog::insert(context.merge_context.destination(), *term).first;

    auto effect = formalism::datalog::checkout<formalism::NumericEffect<LiftedTag, formalism::FluentTag>>(context.builder);
    effect->operator_kind = formalism::NumericEffectOperatorKind::Assign;
    effect->fterm = term_view.index();
    effect->fexpr = formalism::FunctionExpression<LiftedTag>(static_cast<ygg::float_t>(0));
    return formalism::datalog::insert(context.merge_context.destination(), *effect).first.index();
}

template<formalism::RelationKind R, typename CreateHead>
formalism::datalog::RuleRowView<R> create_rule_binding(GroundProgramBuildContext& context, CreateHead&& create_head)
{
    auto predicate = formalism::datalog::checkout<formalism::Predicate<formalism::FluentTag>>(context.builder);
    predicate->name = "ground_rule_" + std::to_string(context.next_rule_id++);
    predicate->arity = 0;
    const auto new_predicate = formalism::datalog::insert(context.merge_context.destination(), *predicate).first;
    context.program->fluent_predicates.push_back(new_predicate.index());

    auto atom = formalism::datalog::checkout<formalism::Atom<LiftedTag, formalism::FluentTag>>(context.builder);
    atom->predicate = new_predicate.index();
    const auto new_atom = formalism::datalog::insert(context.merge_context.destination(), *atom).first;

    auto literal = formalism::datalog::checkout<formalism::Literal<LiftedTag, formalism::FluentTag>>(context.builder);
    literal->atom = new_atom.index();
    literal->polarity = false;
    const auto new_literal = formalism::datalog::insert(context.merge_context.destination(), *literal).first;

    auto condition = formalism::datalog::checkout<formalism::datalog::ConjunctiveCondition<LiftedTag>>(context.builder);
    // Keep synthetic function-rule keys distinct without adding a positive witness precondition.
    condition->fluent_literals.push_back(new_literal.index());
    const auto new_condition = formalism::datalog::insert(context.merge_context.destination(), *condition).first;

    auto rule = formalism::datalog::checkout<formalism::datalog::Rule<LiftedTag, R>>(context.builder);
    rule->body = new_condition.index();
    rule->head_member() = std::forward<CreateHead>(create_head)(new_atom);
    const auto new_rule = formalism::datalog::insert(context.merge_context.destination(), *rule).first;

    auto binding = formalism::datalog::checkout<ygg::Row<formalism::datalog::Rule<LiftedTag, R>>>(context.builder);
    binding->relation = new_rule.index();
    return formalism::datalog::insert(context.merge_context.destination(), *binding).first;
}

formalism::datalog::AtomView<GroundTag, formalism::FluentTag> create_applicability_atom(formalism::planning::ActionView<GroundTag> action,
                                                                                        GroundProgramBuildContext& context)
{
    auto predicate = formalism::datalog::checkout<formalism::Predicate<formalism::FluentTag>>(context.builder);
    predicate->name = create_applicability_name(action.get_action());
    predicate->arity = action.get_objects().size();
    const auto new_predicate = formalism::datalog::insert(context.merge_context.destination(), *predicate).first;

    context.program->fluent_predicates.push_back(new_predicate.index());

    auto binding = formalism::datalog::checkout<ygg::Row<formalism::Predicate<formalism::FluentTag>>>(context.builder);
    binding->relation = new_predicate.index();
    for (const auto object : action.get_objects())
        binding->values.push_back(object.index().value);
    const auto new_binding = formalism::datalog::insert(context.merge_context.destination(), *binding).first;

    auto atom = formalism::datalog::checkout<formalism::Atom<GroundTag, formalism::FluentTag>>(context.builder);
    atom->row = new_binding.data();
    return formalism::datalog::insert(context.merge_context.destination(), *atom).first;
}

formalism::datalog::LiteralView<GroundTag, formalism::FluentTag> create_positive_literal(formalism::datalog::AtomView<GroundTag, formalism::FluentTag> atom,
                                                                                         GroundProgramBuildContext& context)
{
    auto literal = formalism::datalog::checkout<formalism::Literal<GroundTag, formalism::FluentTag>>(context.builder);
    literal->atom = atom.index();
    literal->polarity = true;
    return formalism::datalog::insert(context.merge_context.destination(), *literal).first;
}

void fill_delete_free_condition(formalism::planning::ConjunctiveConditionView<GroundTag> condition,
                                TranslationContext<GroundTag>& translation_context,
                                GroundProgramBuildContext& context,
                                urd::data<formalism::datalog::ConjunctiveCondition<GroundTag>>& result)
{
    for (const auto fact : condition.template get_facts<formalism::PositiveTag>())
        if (const auto literal =
                formalism::planning::merge_p2d(fact, true, translation_context.p2d.fluent_to_fluent_atom, context.fluent_predicates, context.merge_context))
            result.fluent_literals.push_back(literal->index());

    for (const auto numeric_constraint : condition.get_numeric_constraints())
        result.numeric_constraints.push_back(formalism::planning::merge_p2d(numeric_constraint, context.merge_context).first.index());
}

formalism::datalog::ConjunctiveConditionView<GroundTag> create_delete_free_condition(formalism::planning::ConjunctiveConditionView<GroundTag> condition,
                                                                                     TranslationContext<GroundTag>& translation_context,
                                                                                     GroundProgramBuildContext& context)
{
    auto result = formalism::datalog::checkout<formalism::datalog::ConjunctiveCondition<GroundTag>>(context.builder);
    fill_delete_free_condition(condition, translation_context, context, *result);

    return formalism::datalog::insert(context.merge_context.destination(), *result).first;
}

formalism::datalog::ConjunctiveConditionView<GroundTag> create_delete_free_goal(formalism::planning::ConjunctiveConditionView<GroundTag> goal,
                                                                                TranslationContext<GroundTag>& translation_context,
                                                                                GroundProgramBuildContext& context)
{ return create_delete_free_condition(goal, translation_context, context); }

formalism::datalog::ConjunctiveConditionView<GroundTag>
create_delete_free_effect_condition(formalism::datalog::AtomView<GroundTag, formalism::FluentTag> applicability_atom,
                                    formalism::planning::ConjunctiveConditionView<GroundTag> effect_condition,
                                    TranslationContext<GroundTag>& translation_context,
                                    GroundProgramBuildContext& context)
{
    auto result = formalism::datalog::checkout<formalism::datalog::ConjunctiveCondition<GroundTag>>(context.builder);
    fill_delete_free_condition(effect_condition, translation_context, context, *result);
    result->fluent_literals.push_back(create_positive_literal(applicability_atom, context).index());
    return formalism::datalog::insert(context.merge_context.destination(), *result).first;
}

formalism::datalog::ConjunctiveConditionView<GroundTag>
create_delete_free_numeric_effect_condition(formalism::planning::ConjunctiveConditionView<GroundTag> action_condition,
                                            formalism::planning::ConjunctiveConditionView<GroundTag> effect_condition,
                                            TranslationContext<GroundTag>& translation_context,
                                            GroundProgramBuildContext& context)
{
    auto result = formalism::datalog::checkout<formalism::datalog::ConjunctiveCondition<GroundTag>>(context.builder);
    fill_delete_free_condition(action_condition, translation_context, context, *result);
    fill_delete_free_condition(effect_condition, translation_context, context, *result);
    return formalism::datalog::insert(context.merge_context.destination(), *result).first;
}

formalism::datalog::RuleView<GroundTag, formalism::PredicateTag>
create_ground_atom_rule(formalism::datalog::ConjunctiveConditionView<GroundTag> body,
                        const urd::flat_vector<urd::index<formalism::Atom<GroundTag, formalism::FluentTag>>>& heads,
                        GroundProgramBuildContext& context,
                        ygg::float_t cost = 0)
{
    auto rule = formalism::datalog::checkout<formalism::datalog::Rule<GroundTag, formalism::PredicateTag>>(context.builder);
    rule->row = create_rule_binding<formalism::PredicateTag>(context,
                                                             [](auto atom)
                                                             {
                                                                 auto atoms = urd::flat_vector<urd::index<formalism::Atom<LiftedTag, formalism::FluentTag>>> {};
                                                                 atoms.push_back(atom.index());
                                                                 return atoms;
                                                             })
                    .data();
    rule->body = body.index();
    rule->heads = heads;
    rule->cost = cost;
    return formalism::datalog::insert(context.merge_context.destination(), *rule).first;
}

formalism::datalog::RuleView<GroundTag, formalism::FunctionTag>
create_ground_numeric_effect_rule(formalism::datalog::ConjunctiveConditionView<GroundTag> body,
                                  formalism::planning::NumericEffectOperatorView<GroundTag, formalism::FluentTag> head,
                                  GroundProgramBuildContext& context)
{
    const auto datalog_head = formalism::planning::merge_p2d(head, context.merge_context).first.index();
    auto rule = formalism::datalog::checkout<formalism::datalog::Rule<GroundTag, formalism::FunctionTag>>(context.builder);
    rule->row = create_rule_binding<formalism::FunctionTag>(context, [&](auto) { return create_rule_binding_numeric_head(context); }).data();
    rule->body = body.index();
    rule->head = datalog_head;
    rule->cost = 1;
    return formalism::datalog::insert(context.merge_context.destination(), *rule).first;
}

formalism::datalog::RuleView<GroundTag, formalism::PredicateTag>
create_applicability_rule(formalism::planning::ActionView<GroundTag> action,
                          formalism::datalog::AtomView<GroundTag, formalism::FluentTag> applicability_atom,
                          TranslationContext<GroundTag>& translation_context,
                          GroundProgramBuildContext& context)
{
    auto heads = urd::flat_vector<urd::index<formalism::Atom<GroundTag, formalism::FluentTag>>> {};
    heads.push_back(applicability_atom.index());
    return create_ground_atom_rule(create_delete_free_condition(action.get_condition(), translation_context, context), heads, context);
}

formalism::datalog::ProgramView<GroundTag> finish_program(GroundProgramBuildContext& context)
{ return formalism::datalog::insert(context.merge_context.destination(), *context.program).first; }

void translate_action_to_delete_free_rules(formalism::planning::ActionView<GroundTag> action,
                                           urd::data<formalism::datalog::Program<GroundTag>>& program,
                                           TranslationContext<GroundTag>& translation_context,
                                           GroundProgramBuildContext& context,
                                           RPGProgram<GroundTag>::RuleToActionMappings& rule_to_action)
{
    const auto applicability_atom = create_applicability_atom(action, context);
    const auto applicability_rule = create_applicability_rule(action, applicability_atom, translation_context, context);
    program.predicate_rules.push_back(applicability_rule.index());

    for (const auto cond_eff : action.get_effects())
    {
        const auto body = create_delete_free_effect_condition(applicability_atom, cond_eff.get_condition(), translation_context, context);

        auto heads = urd::flat_vector<urd::index<formalism::Atom<GroundTag, formalism::FluentTag>>> {};
        for (const auto fact : cond_eff.get_effect().get_facts<formalism::PositiveTag>())
            if (const auto literal =
                    formalism::planning::merge_p2d(fact, true, translation_context.p2d.fluent_to_fluent_atom, context.fluent_predicates, context.merge_context))
                heads.push_back(literal->get_atom().index());
        if (!heads.empty())
        {
            const auto rule = create_ground_atom_rule(body, heads, context, 1);
            program.predicate_rules.push_back(rule.index());
            rule_to_action.predicate[rule.get_row().data()].push_back(action);
        }

        const auto numeric_body = create_delete_free_numeric_effect_condition(action.get_condition(), cond_eff.get_condition(), translation_context, context);
        for (const auto numeric_effect : cond_eff.get_effect().get_numeric_effects())
        {
            const auto rule = create_ground_numeric_effect_rule(numeric_body, numeric_effect, context);
            program.function_rules.push_back(rule.index());
            rule_to_action.function[rule.get_row().data()].push_back(action);
        }
    }
}

formalism::datalog::ProgramView<GroundTag> create_rpg_ground_program(formalism::planning::TaskView<GroundTag> task,
                                                                     TranslationContext<GroundTag>& translation_context,
                                                                     RPGProgram<GroundTag>::RuleToActionMappings& mapping,
                                                                     formalism::datalog::Repository& repository)
{
    auto context = GroundProgramBuildContext(repository);
    auto& merge_context = context.merge_context;

    for (const auto predicate : task.get_domain().get_predicates<formalism::StaticTag>())
    {
        const auto new_predicate = formalism::copy(predicate, merge_context).first;
        context.static_predicates.emplace(predicate, new_predicate);
        context.program->static_predicates.push_back(new_predicate.index());
    }
    for (const auto predicate : task.get_domain().get_predicates<formalism::FluentTag>())
    {
        const auto new_predicate = formalism::copy(predicate, merge_context).first;
        context.fluent_predicates.emplace(predicate, new_predicate);
        context.program->fluent_predicates.push_back(new_predicate.index());
    }

    for (const auto function : task.get_domain().get_functions<formalism::StaticTag>())
        context.program->static_functions.push_back(formalism::copy(function, merge_context).first.index());
    for (const auto function : task.get_domain().get_functions<formalism::FluentTag>())
        context.program->fluent_functions.push_back(formalism::copy(function, merge_context).first.index());

    for (const auto object : task.get_domain().get_constants())
        context.program->objects.push_back(formalism::copy(object, merge_context).first.index());
    for (const auto object : task.get_objects())
        context.program->objects.push_back(formalism::copy(object, merge_context).first.index());

    for (const auto atom : task.get_atoms<formalism::StaticTag>())
    {
        const auto new_atom = formalism::planning::merge_p2d(atom, context.static_predicates, merge_context).first;
        translation_context.p2d.static_to_static_atom.emplace(atom, new_atom);
        translation_context.d2p.static_to_static_atom.emplace(new_atom, atom);
        context.program->static_atoms.push_back(new_atom.index());
    }
    for (const auto atom : task.get_atoms<formalism::FluentTag>())
    {
        const auto new_atom =
            formalism::planning::merge_p2d(atom, translation_context.p2d.fluent_to_fluent_atom, context.fluent_predicates, merge_context).first;
        translation_context.d2p.fluent_to_fluent_atom.emplace(new_atom.get_row(), atom);
        context.program->fluent_atoms.push_back(new_atom.index());
    }
    for (const auto fact : task.get_fluent_facts())
    {
        if (const auto atom = fact.get_atom())
        {
            const auto [new_atom, inserted] =
                formalism::planning::merge_p2d(*atom, translation_context.p2d.fluent_to_fluent_atom, context.fluent_predicates, merge_context);
            translation_context.d2p.fluent_to_fluent_atom.emplace(new_atom.get_row(), *atom);
            if (inserted)
                context.program->fluent_atoms.push_back(new_atom.index());
        }
    }

    for (const auto fterm_value : task.get_fterm_values<formalism::StaticTag>())
    {
        const auto new_fterm_value = formalism::copy(fterm_value, merge_context).first;
        translation_context.p2d.static_to_static_fterm.emplace(fterm_value.get_fterm(), new_fterm_value.get_fterm());
        translation_context.d2p.static_to_static_fterm.emplace(new_fterm_value.get_fterm(), fterm_value.get_fterm());
        context.program->static_fterm_values.push_back(new_fterm_value.index());
    }
    for (const auto fterm_value : task.get_fterm_values<formalism::FluentTag>())
    {
        const auto new_fterm_value = formalism::copy(fterm_value, merge_context).first;
        translation_context.p2d.fluent_to_fluent_fterm.emplace(fterm_value.get_fterm(), new_fterm_value.get_fterm());
        translation_context.d2p.fluent_to_fluent_fterm.emplace(new_fterm_value.get_fterm(), fterm_value.get_fterm());
        context.program->fluent_fterm_values.push_back(new_fterm_value.index());
    }

    context.program->goal = create_delete_free_goal(task.get_goal(), translation_context, context).index();

    for (const auto action : task.get_ground_actions())
        translate_action_to_delete_free_rules(action, *context.program, translation_context, context, mapping);

    return finish_program(context);
}

TranslationContext<GroundTag> remap_translation_context(const TranslationContext<GroundTag>& source,
                                                        formalism::WriteContext<formalism::datalog::Builder, formalism::datalog::Repository>& context)
{
    auto result = TranslationContext<GroundTag> {};
    const auto remap_keys = [&](const auto& source_mapping, auto& result_mapping)
    {
        for (const auto& [key, value] : source_mapping)
            result_mapping.emplace(formalism::copy(key, context).first, value);
    };
    const auto remap_values = [&](const auto& source_mapping, auto& result_mapping)
    {
        for (const auto& [key, value] : source_mapping)
            result_mapping.emplace(key, formalism::copy(value, context).first);
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

template<formalism::RelationKind R>
void remap_rule_to_action(const RPGProgram<GroundTag>::RuleToActionMapping<R>& source_mapping,
                          RPGProgram<GroundTag>::RuleToActionMapping<R>& result_mapping,
                          const formalism::datalog::Repository& source_repository,
                          formalism::WriteContext<formalism::datalog::Builder, formalism::datalog::Repository>& context,
                          const datalog::FactSets& fact_sets)
{
    for (const auto& [source_binding, actions] : source_mapping)
    {
        const auto source_rule = formalism::datalog::find_ground_rule(source_binding, source_repository);
        if (!source_rule)
            throw std::logic_error("Ground rule binding has no rule");
        if (!datalog::is_statically_applicable(*source_rule, fact_sets))
            continue;

        const auto [result_rule, inserted] = formalism::copy(*source_rule, context);
        if (inserted)
            throw std::logic_error("Static rule filtering omitted a retained rule");
        auto& result_actions = result_mapping[result_rule.get_row().data()];
        result_actions.insert(result_actions.end(), actions.begin(), actions.end());
    }
}

datalog::Program<GroundTag> create_rpg_datalog_program(formalism::planning::TaskView<GroundTag> task,
                                                       CostMode cost_mode,
                                                       TranslationContext<GroundTag>& translation_context,
                                                       RPGProgram<GroundTag>::RuleToActionMappings& mapping)
{
    require_unit_cost(cost_mode);

    const auto num_objects = task.get_num_objects();
    auto source_repository = formalism::datalog::Repository(num_objects);
    auto source_translation_context = TranslationContext<GroundTag> {};
    auto source_mapping = RPGProgram<GroundTag>::RuleToActionMappings {};
    const auto source_program = create_rpg_ground_program(task, source_translation_context, source_mapping, source_repository);
    const auto source_static_fact_sets = datalog::TaggedFactSets<formalism::StaticTag>(source_program.get_predicates<formalism::StaticTag>(),
                                                                                       source_program.get_functions<formalism::StaticTag>(),
                                                                                       source_program.get_atoms<formalism::StaticTag>(),
                                                                                       source_program.get_fterm_values<formalism::StaticTag>(),
                                                                                       source_program.context());
    const auto source_fluent_fact_sets = datalog::TaggedFactSets<formalism::FluentTag>(source_program.get_predicates<formalism::FluentTag>(),
                                                                                       source_program.get_functions<formalism::FluentTag>(),
                                                                                       source_program.get_atoms<formalism::FluentTag>(),
                                                                                       source_program.get_fterm_values<formalism::FluentTag>(),
                                                                                       source_program.context());
    const auto source_fact_sets = datalog::FactSets { source_static_fact_sets, source_fluent_fact_sets };

    auto repository = std::make_shared<formalism::datalog::Repository>(num_objects);
    const auto program = datalog::remove_statically_inapplicable_rules(source_program, *repository);
    auto builder = formalism::datalog::Builder {};
    auto merge_context = formalism::WriteContext { builder, *repository };
    translation_context = remap_translation_context(source_translation_context, merge_context);
    remap_rule_to_action<formalism::PredicateTag>(source_mapping.predicate, mapping.predicate, source_program.context(), merge_context, source_fact_sets);
    remap_rule_to_action<formalism::FunctionTag>(source_mapping.function, mapping.function, source_program.context(), merge_context, source_fact_sets);
    return datalog::Program<GroundTag>(program, std::move(repository));
}

}  // namespace

template<>
RPGProgram<GroundTag>::RPGProgram(formalism::planning::TaskView<GroundTag> task, CostMode cost_mode, bool) :
    m_translation_context(),
    m_rule_to_action(),
    m_datalog_program(create_rpg_datalog_program(task, cost_mode, m_translation_context, m_rule_to_action))
{
}

}  // namespace tyr::planning
