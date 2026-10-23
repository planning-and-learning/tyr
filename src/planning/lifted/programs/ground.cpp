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

#include "tyr/formalism/datalog/interning.hpp"
#include "tyr/planning/lifted/programs/ground.hpp"

#include "../../programs/common.hpp"
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
/**
 * Assume each pre and eff is a set of literals
 *
 * Action = [pre, [<c1_pre,c1_eff>,...,cn_pre,cn_eff>]]
 * App_pre :- pre
 * App_c1_pre :- App_pre and ci_pre    forall i=1,...,n
 * e :- App_ci_pre                     forall i=1,...,n forall e in ci_eff
 */

auto create_applicability_predicate(formalism::planning::ActionView<LiftedTag> action, formalism::planning::MergeDatalogContext& context)
{
    auto predicate = formalism::datalog::checkout<formalism::Predicate<formalism::FluentTag>>(context.builder);

    predicate->name = create_applicability_name(action);
    predicate->arity = action.get_arity();

    return formalism::datalog::insert(context.destination, *predicate);
}

auto create_applicability_atom(formalism::planning::ActionView<LiftedTag> action, formalism::planning::MergeDatalogContext& context)
{
    auto atom = formalism::datalog::checkout<formalism::datalog::Atom<LiftedTag, formalism::FluentTag>>(context.builder);

    const auto applicability_predicate = create_applicability_predicate(action, context).first;

    atom->predicate = applicability_predicate.get_index();
    for (ygg::uint_t i = 0; i < applicability_predicate.get_arity(); ++i)
        atom->terms.push_back(ygg::Data<formalism::Term>(formalism::ParameterIndex(i)));

    return formalism::datalog::insert(context.destination, *atom);
}

auto create_applicability_predicate(formalism::planning::AxiomView<LiftedTag> axiom, formalism::planning::MergeDatalogContext& context)
{
    auto predicate = formalism::datalog::checkout<formalism::Predicate<formalism::FluentTag>>(context.builder);

    predicate->name = create_applicability_name(axiom);
    predicate->arity = axiom.get_arity();

    return formalism::datalog::insert(context.destination, *predicate);
}

auto create_applicability_atom(formalism::planning::AxiomView<LiftedTag> axiom, formalism::planning::MergeDatalogContext& context)
{
    auto atom = formalism::datalog::checkout<formalism::datalog::Atom<LiftedTag, formalism::FluentTag>>(context.builder);

    const auto applicability_predicate = create_applicability_predicate(axiom, context).first;

    atom->predicate = applicability_predicate.get_index();
    for (ygg::uint_t i = 0; i < applicability_predicate.get_arity(); ++i)
        atom->terms.push_back(ygg::Data<formalism::Term>(formalism::ParameterIndex(i)));

    return formalism::datalog::insert(context.destination, *atom);
}

void append_from_condition(formalism::planning::ConjunctiveConditionView<LiftedTag> cond,
                           const TranslationContext<LiftedTag>& translation_context,
                           formalism::planning::MergeDatalogContext& context,
                           ygg::Data<formalism::datalog::ConjunctiveCondition<LiftedTag>>& conj_cond)
{
    // Keep negated static atoms because they are monotonic
    for (const auto literal : cond.template get_literals<formalism::StaticTag>())
        conj_cond.static_literals.push_back(merge_p2d(literal, translation_context.p2d.static_to_static_predicate, context).first.get_index());
    for (const auto literal : cond.template get_literals<formalism::FluentTag>())
        if (literal.get_polarity())
            conj_cond.fluent_literals.push_back(merge_p2d(literal, translation_context.p2d.fluent_to_fluent_predicate, context).first.get_index());

    for (const auto literal : cond.template get_literals<formalism::DerivedTag>())
        if (literal.get_polarity())
            conj_cond.fluent_literals.push_back(
                merge_p2d<formalism::DerivedTag, formalism::FluentTag>(literal, translation_context.p2d.derived_to_fluent_predicate, context)
                    .first.get_index());
};

auto create_applicability_literal(formalism::planning::ActionView<LiftedTag> action, formalism::planning::MergeDatalogContext& context)
{
    auto literal = formalism::datalog::checkout<formalism::datalog::Literal<LiftedTag, formalism::FluentTag>>(context.builder);

    literal->polarity = true;
    literal->atom = create_applicability_atom(action, context).first.get_index();

    return formalism::datalog::insert(context.destination, *literal);
}

auto create_applicability_rule(formalism::planning::ActionView<LiftedTag> action,
                               const TranslationContext<LiftedTag>& translation_context,
                               formalism::planning::MergeDatalogContext& context)
{
    auto rule = formalism::datalog::checkout<formalism::datalog::Rule<LiftedTag, formalism::PredicateTag>>(context.builder);

    auto conj_cond = formalism::datalog::checkout<formalism::datalog::ConjunctiveCondition<LiftedTag>>(context.builder);

    for (const auto variable : action.get_variables())
        conj_cond->variables.push_back(merge_p2d(variable, context).first.get_index());
    append_from_condition(action.get_condition(), translation_context, context, *conj_cond);

    const auto new_conj_cond = formalism::datalog::insert(context.destination, *conj_cond).first;

    ygg::extend(new_conj_cond.get_variables(), rule->variables);
    rule->body = new_conj_cond.get_index();
    rule->heads.push_back(create_applicability_atom(action, context).first.get_index());

    return formalism::datalog::insert(context.destination, *rule);
}

auto create_applicability_literal(formalism::planning::AxiomView<LiftedTag> axiom, formalism::planning::MergeDatalogContext& context)
{
    auto literal = formalism::datalog::checkout<formalism::datalog::Literal<LiftedTag, formalism::FluentTag>>(context.builder);

    literal->polarity = true;
    literal->atom = create_applicability_atom(axiom, context).first.get_index();

    return formalism::datalog::insert(context.destination, *literal);
}

auto create_applicability_rule(formalism::planning::AxiomView<LiftedTag> axiom,
                               const TranslationContext<LiftedTag>& translation_context,
                               formalism::planning::MergeDatalogContext& context)
{
    auto rule = formalism::datalog::checkout<formalism::datalog::Rule<LiftedTag, formalism::PredicateTag>>(context.builder);

    auto conj_cond = formalism::datalog::checkout<formalism::datalog::ConjunctiveCondition<LiftedTag>>(context.builder);

    for (const auto variable : axiom.get_variables())
        conj_cond->variables.push_back(merge_p2d(variable, context).first.get_index());
    append_from_condition(axiom.get_body(), translation_context, context, *conj_cond);

    const auto new_conj_cond = formalism::datalog::insert(context.destination, *conj_cond).first;

    ygg::extend(new_conj_cond.get_variables(), rule->variables);
    rule->body = new_conj_cond.get_index();
    rule->heads.push_back(create_applicability_atom(axiom, context).first.get_index());

    return formalism::datalog::insert(context.destination, *rule);
}

/// The rule that derives the add atoms of a conditional effect and, given its effect predicate, the atoms for which it fires.
auto create_cond_effect_rule(formalism::planning::ActionView<LiftedTag> action,
                             formalism::planning::ConditionalEffectView<LiftedTag> cond_eff,
                             std::optional<formalism::datalog::PredicateView<formalism::FluentTag>> effect_predicate,
                             const TranslationContext<LiftedTag>& translation_context,
                             formalism::planning::MergeDatalogContext& context)
{
    auto rule = formalism::datalog::checkout<formalism::datalog::Rule<LiftedTag, formalism::PredicateTag>>(context.builder);

    auto conj_cond = formalism::datalog::checkout<formalism::datalog::ConjunctiveCondition<LiftedTag>>(context.builder);

    for (const auto variable : action.get_variables())
        conj_cond->variables.push_back(merge_p2d(variable, context).first.get_index());
    for (const auto literal : action.get_condition().get_literals<formalism::StaticTag>())
        conj_cond->static_literals.push_back(merge_p2d(literal, translation_context.p2d.static_to_static_predicate, context).first.get_index());
    conj_cond->fluent_literals.push_back(create_applicability_literal(action, context).first.get_index());

    for (const auto variable : cond_eff.get_variables())
        conj_cond->variables.push_back(merge_p2d(variable, context).first.get_index());
    append_from_condition(cond_eff.get_condition(), translation_context, context, *conj_cond);

    const auto new_conj_cond = formalism::datalog::insert(context.destination, *conj_cond).first;

    ygg::extend(new_conj_cond.get_variables(), rule->variables);
    rule->body = new_conj_cond.get_index();
    for (const auto literal : cond_eff.get_effect().get_literals())
        if (literal.get_polarity())  /// ignore delete effects
            rule->heads.push_back(
                formalism::planning::merge_p2d(literal.get_atom(), translation_context.p2d.fluent_to_fluent_predicate, context).first.get_index());
    if (effect_predicate)
    {
        auto effect_atom = formalism::datalog::checkout<formalism::datalog::Atom<LiftedTag, formalism::FluentTag>>(context.builder);
        effect_atom->predicate = effect_predicate->get_index();
        for (ygg::uint_t i = 0; i < effect_predicate->get_arity(); ++i)
            effect_atom->terms.push_back(ygg::Data<formalism::Term>(formalism::ParameterIndex(i)));
        rule->heads.push_back(formalism::datalog::insert(context.destination, *effect_atom).first.get_index());
    }

    return formalism::datalog::insert(context.destination, *rule);
}

auto create_effect_rule(formalism::planning::AxiomView<LiftedTag> axiom,
                        formalism::datalog::AtomView<LiftedTag, formalism::FluentTag> effect,
                        const TranslationContext<LiftedTag>& translation_context,
                        formalism::planning::MergeDatalogContext& context)
{
    auto rule = formalism::datalog::checkout<formalism::datalog::Rule<LiftedTag, formalism::PredicateTag>>(context.builder);

    auto conj_cond = formalism::datalog::checkout<formalism::datalog::ConjunctiveCondition<LiftedTag>>(context.builder);

    for (const auto variable : axiom.get_variables())
        conj_cond->variables.push_back(merge_p2d(variable, context).first.get_index());
    for (const auto literal : axiom.get_body().get_literals<formalism::StaticTag>())
        conj_cond->static_literals.push_back(merge_p2d(literal, translation_context.p2d.static_to_static_predicate, context).first.get_index());
    conj_cond->fluent_literals.push_back(create_applicability_literal(axiom, context).first.get_index());

    const auto new_conj_cond = formalism::datalog::insert(context.destination, *conj_cond).first;

    ygg::extend(new_conj_cond.get_variables(), rule->variables);
    rule->body = new_conj_cond.get_index();
    rule->heads.push_back(effect.get_index());

    return formalism::datalog::insert(context.destination, *rule);
}

void translate_action_to_delete_free_rules(formalism::planning::ActionView<LiftedTag> action,
                                           ygg::Data<formalism::datalog::Program<LiftedTag>>& program,
                                           const TranslationContext<LiftedTag>& translation_context,
                                           formalism::planning::MergeDatalogContext& context,
                                           GroundTaskProgram::AppPredicateToActionMapping& predicate_to_actions,
                                           GroundTaskProgram::ConditionalEffectToPredicateMapping& effect_to_predicate)
{
    const auto applicability_predicate = create_applicability_predicate(action, context).first;

    [[maybe_unused]] const auto [it, inserted] = predicate_to_actions.emplace(applicability_predicate, action);
    assert(inserted);

    program.fluent_predicates.push_back(applicability_predicate.get_index());

    const auto applicability_rule = create_applicability_rule(action, translation_context, context).first.get_index();

    program.predicate_rules.push_back(applicability_rule);

    for (const auto cond_eff : action.get_effects())
    {
        auto effect_predicate = std::optional<formalism::datalog::PredicateView<formalism::FluentTag>> {};
        if (has_effect_predicate(cond_eff))
        {
            auto predicate = formalism::datalog::checkout<formalism::Predicate<formalism::FluentTag>>(context.builder);
            predicate->name = create_triggered_name(action, cond_eff);
            predicate->arity = action.get_arity() + cond_eff.get_arity();
            effect_predicate = formalism::datalog::insert(context.destination, *predicate).first;
            program.fluent_predicates.push_back(effect_predicate->get_index());
        }
        if (!effect_predicate && std::ranges::none_of(cond_eff.get_effect().get_literals(), [](const auto literal) { return literal.get_polarity(); }))
            continue;

        program.predicate_rules.push_back(create_cond_effect_rule(action, cond_eff, effect_predicate, translation_context, context).first.get_index());
        if (effect_predicate)
            effect_to_predicate.emplace(std::pair(action, cond_eff), *effect_predicate);
    }
}

void translate_axiom_to_delete_free_axiom_rules(formalism::planning::AxiomView<LiftedTag> axiom,
                                                ygg::Data<formalism::datalog::Program<LiftedTag>>& program,
                                                const TranslationContext<LiftedTag>& translation_context,
                                                formalism::planning::MergeDatalogContext& context,
                                                GroundTaskProgram::AppPredicateToAxiomMapping& predicate_to_axioms)
{
    const auto applicability_predicate = create_applicability_predicate(axiom, context).first;

    program.fluent_predicates.push_back(applicability_predicate.get_index());

    predicate_to_axioms[applicability_predicate].push_back(axiom);

    const auto applicability_rule = create_applicability_rule(axiom, translation_context, context).first.get_index();

    program.predicate_rules.push_back(applicability_rule);

    program.predicate_rules.push_back(
        create_effect_rule(axiom,
                           formalism::planning::merge_p2d<formalism::DerivedTag, formalism::FluentTag>(axiom.get_head(),
                                                                                                       translation_context.p2d.derived_to_fluent_predicate,
                                                                                                       context)
                               .first,
                           translation_context,
                           context)
            .first.get_index());
}

auto create_program(formalism::planning::TaskView<LiftedTag> task,
                    TranslationContext<LiftedTag>& translation_context,
                    GroundTaskProgram::AppPredicateToActionMapping& predicate_to_actions,
                    GroundTaskProgram::AppPredicateToAxiomMapping& predicate_to_axioms,
                    GroundTaskProgram::ConditionalEffectToPredicateMapping& effect_to_predicate,
                    formalism::datalog::Repository& destination)
{
    auto builder = formalism::datalog::Builder();
    auto context = formalism::planning::MergeDatalogContext(builder, destination);
    auto program = formalism::datalog::checkout<formalism::datalog::Program<LiftedTag>>(builder);

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
    for (const auto predicate : task.get_domain().get_predicates<formalism::DerivedTag>())
    {
        const auto new_predicate = formalism::planning::merge_p2d<formalism::DerivedTag, formalism::FluentTag>(predicate, context).first;
        translation_context.d2p.fluent_to_derived_predicate.emplace(new_predicate, predicate);
        translation_context.p2d.derived_to_fluent_predicate.emplace(predicate, new_predicate);
        program->fluent_predicates.push_back(new_predicate.get_index());
    }
    for (const auto predicate : task.get_derived_predicates())
    {
        const auto new_predicate = formalism::planning::merge_p2d<formalism::DerivedTag, formalism::FluentTag>(predicate, context).first;
        translation_context.d2p.fluent_to_derived_predicate.emplace(new_predicate, predicate);
        translation_context.p2d.derived_to_fluent_predicate.emplace(predicate, new_predicate);
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

    for (const auto action : task.get_domain().get_actions())
        translate_action_to_delete_free_rules(action, *program, translation_context, context, predicate_to_actions, effect_to_predicate);

    for (const auto axiom : task.get_domain().get_axioms())
        translate_axiom_to_delete_free_axiom_rules(axiom, *program, translation_context, context, predicate_to_axioms);

    for (const auto axiom : task.get_axioms())
        translate_axiom_to_delete_free_axiom_rules(axiom, *program, translation_context, context, predicate_to_axioms);

    return formalism::datalog::insert(destination, *program).first;
}

static auto create_datalog_program(formalism::planning::TaskView<LiftedTag> task,
                                   TranslationContext<LiftedTag>& translation_context,
                                   GroundTaskProgram::AppPredicateToActionMapping& predicate_to_actions,
                                   GroundTaskProgram::AppPredicateToAxiomMapping& predicate_to_axioms,
                                   GroundTaskProgram::ConditionalEffectToPredicateMapping& effect_to_predicate)
{
    auto factory = std::make_shared<formalism::datalog::RepositoryFactory>();
    auto repository = factory->create_shared(task.get_num_objects());
    auto program = create_program(task, translation_context, predicate_to_actions, predicate_to_axioms, effect_to_predicate, *repository);
    return datalog::Program<LiftedTag>(program, std::move(repository), std::move(factory));
}

}

GroundTaskProgram::GroundTaskProgram(formalism::planning::TaskView<LiftedTag> task) :
    m_translation_context(),
    m_predicate_to_actions(),
    m_predicate_to_axioms(),
    m_conditional_effect_to_predicate(),
    m_datalog_program(create_datalog_program(task, m_translation_context, m_predicate_to_actions, m_predicate_to_axioms, m_conditional_effect_to_predicate))
{
    // std::cout << m_datalog_program.get_program() << std::endl;
}

const TranslationContext<LiftedTag>& GroundTaskProgram::get_translation_context() const noexcept { return m_translation_context; }

const GroundTaskProgram::AppPredicateToActionMapping& GroundTaskProgram::get_predicate_to_action_mapping() const noexcept { return m_predicate_to_actions; }

const GroundTaskProgram::AppPredicateToAxiomMapping& GroundTaskProgram::get_predicate_to_axiom_mapping() const noexcept { return m_predicate_to_axioms; }

const GroundTaskProgram::ConditionalEffectToPredicateMapping& GroundTaskProgram::get_conditional_effect_to_predicate_mapping() const noexcept
{
    return m_conditional_effect_to_predicate;
}

datalog::Program<LiftedTag>& GroundTaskProgram::get_datalog_program() noexcept { return m_datalog_program; }

const datalog::Program<LiftedTag>& GroundTaskProgram::get_datalog_program() const noexcept { return m_datalog_program; }

const datalog::ConstProgramWorkspace<LiftedTag>& GroundTaskProgram::get_const_program_workspace() const noexcept
{
    return m_datalog_program.get_const_program_workspace();
}

}
