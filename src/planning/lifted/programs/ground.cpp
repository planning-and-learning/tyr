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

#include "tyr/planning/lifted/programs/ground.hpp"

#include "../../programs/common.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/datalog/views.hpp"
#include "tyr/formalism/planning/merge_datalog.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/planning/views.hpp"

namespace f = tyr::formalism;
namespace fp = tyr::formalism::planning;
namespace fd = tyr::formalism::datalog;

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

auto create_applicability_predicate(fp::ActionView action, fp::MergeDatalogContext& context)
{
    auto predicate_ptr = context.builder.get_builder<f::Predicate<f::FluentTag>>();
    auto& predicate = *predicate_ptr;
    predicate.clear();

    predicate.name = create_applicability_name(action);
    predicate.arity = action.get_arity();

    canonicalize(predicate);
    return context.destination.get_or_create(predicate);
}

auto create_applicability_atom(fp::ActionView action, fp::MergeDatalogContext& context)
{
    auto atom_ptr = context.builder.get_builder<fd::Atom<f::FluentTag>>();
    auto& atom = *atom_ptr;
    atom.clear();

    const auto applicability_predicate = create_applicability_predicate(action, context).first;

    atom.predicate = applicability_predicate.get_index();
    for (ygg::uint_t i = 0; i < applicability_predicate.get_arity(); ++i)
        atom.terms.push_back(ygg::Data<f::Term>(f::ParameterIndex(i)));

    canonicalize(atom);
    return context.destination.get_or_create(atom);
}

auto create_applicability_predicate(fp::AxiomView axiom, fp::MergeDatalogContext& context)
{
    auto predicate_ptr = context.builder.get_builder<f::Predicate<f::FluentTag>>();
    auto& predicate = *predicate_ptr;
    predicate.clear();

    predicate.name = create_applicability_name(axiom);
    predicate.arity = axiom.get_arity();

    canonicalize(predicate);
    return context.destination.get_or_create(predicate);
}

auto create_applicability_atom(fp::AxiomView axiom, fp::MergeDatalogContext& context)
{
    auto atom_ptr = context.builder.get_builder<fd::Atom<f::FluentTag>>();
    auto& atom = *atom_ptr;
    atom.clear();

    const auto applicability_predicate = create_applicability_predicate(axiom, context).first;

    atom.predicate = applicability_predicate.get_index();
    for (ygg::uint_t i = 0; i < applicability_predicate.get_arity(); ++i)
        atom.terms.push_back(ygg::Data<f::Term>(f::ParameterIndex(i)));

    canonicalize(atom);
    return context.destination.get_or_create(atom);
}

void append_from_condition(fp::ConjunctiveConditionView cond,
                           const TranslationContext<LiftedTag>& translation_context,
                           fp::MergeDatalogContext& context,
                           ygg::Data<fd::ConjunctiveCondition>& conj_cond)
{
    // Keep negated static atoms because they are monotonic
    for (const auto literal : cond.template get_literals<f::StaticTag>())
        conj_cond.static_literals.push_back(merge_p2d(literal, translation_context.p2d.static_to_static_predicate, context).first.get_index());
    for (const auto literal : cond.template get_literals<f::FluentTag>())
        if (literal.get_polarity())
            conj_cond.fluent_literals.push_back(merge_p2d(literal, translation_context.p2d.fluent_to_fluent_predicate, context).first.get_index());

    for (const auto literal : cond.template get_literals<f::DerivedTag>())
        if (literal.get_polarity())
            conj_cond.fluent_literals.push_back(
                merge_p2d<f::DerivedTag, f::FluentTag>(literal, translation_context.p2d.derived_to_fluent_predicate, context).first.get_index());
};

auto create_applicability_literal(fp::ActionView action, const TranslationContext<LiftedTag>& translation_context, fp::MergeDatalogContext& context)
{
    auto literal_ptr = context.builder.get_builder<fd::Literal<f::FluentTag>>();
    auto& literal = *literal_ptr;
    literal.clear();

    literal.polarity = true;
    literal.atom = create_applicability_atom(action, context).first.get_index();

    canonicalize(literal);
    return context.destination.get_or_create(literal);
}

auto create_applicability_rule(fp::ActionView action, const TranslationContext<LiftedTag>& translation_context, fp::MergeDatalogContext& context)
{
    auto rule_ptr = context.builder.get_builder<fd::Rule<f::PredicateTag>>();
    auto& rule = *rule_ptr;
    rule.clear();

    auto conj_cond_ptr = context.builder.get_builder<fd::ConjunctiveCondition>();
    auto& conj_cond = *conj_cond_ptr;
    conj_cond.clear();

    for (const auto variable : action.get_variables())
        conj_cond.variables.push_back(merge_p2d(variable, context).first.get_index());
    append_from_condition(action.get_condition(), translation_context, context, conj_cond);

    canonicalize(conj_cond);
    const auto new_conj_cond = context.destination.get_or_create(conj_cond).first;

    rule.variables = new_conj_cond.get_variables().get_data();
    rule.body = new_conj_cond.get_index();
    rule.head = create_applicability_atom(action, context).first.get_index();

    canonicalize(rule);
    return context.destination.get_or_create(rule);
}

auto create_applicability_literal(fp::AxiomView axiom, const TranslationContext<LiftedTag>& translation_context, fp::MergeDatalogContext& context)
{
    auto literal_ptr = context.builder.get_builder<fd::Literal<f::FluentTag>>();
    auto& literal = *literal_ptr;
    literal.clear();

    literal.polarity = true;
    literal.atom = create_applicability_atom(axiom, context).first.get_index();

    canonicalize(literal);
    return context.destination.get_or_create(literal);
}

auto create_applicability_rule(fp::AxiomView axiom, const TranslationContext<LiftedTag>& translation_context, fp::MergeDatalogContext& context)
{
    auto rule_ptr = context.builder.get_builder<fd::Rule<f::PredicateTag>>();
    auto& rule = *rule_ptr;
    rule.clear();

    auto conj_cond_ptr = context.builder.get_builder<fd::ConjunctiveCondition>();
    auto& conj_cond = *conj_cond_ptr;
    conj_cond.clear();

    for (const auto variable : axiom.get_variables())
        conj_cond.variables.push_back(merge_p2d(variable, context).first.get_index());
    append_from_condition(axiom.get_body(), translation_context, context, conj_cond);

    canonicalize(conj_cond);
    const auto new_conj_cond = context.destination.get_or_create(conj_cond).first;

    rule.variables = new_conj_cond.get_variables().get_data();
    rule.body = new_conj_cond.get_index();
    rule.head = create_applicability_atom(axiom, context).first.get_index();

    canonicalize(rule);
    return context.destination.get_or_create(rule);
}

auto create_cond_effect_rule(fp::ActionView action,
                             fp::ConditionalEffectView cond_eff,
                             fd::AtomView<f::FluentTag> effect,
                             const TranslationContext<LiftedTag>& translation_context,
                             fp::MergeDatalogContext& context)
{
    auto rule_ptr = context.builder.get_builder<fd::Rule<f::PredicateTag>>();
    auto& rule = *rule_ptr;
    rule.clear();

    auto conj_cond_ptr = context.builder.get_builder<fd::ConjunctiveCondition>();
    auto& conj_cond = *conj_cond_ptr;
    conj_cond.clear();

    for (const auto variable : action.get_variables())
        conj_cond.variables.push_back(merge_p2d(variable, context).first.get_index());
    for (const auto literal : action.get_condition().get_literals<f::StaticTag>())
        conj_cond.static_literals.push_back(merge_p2d(literal, translation_context.p2d.static_to_static_predicate, context).first.get_index());
    conj_cond.fluent_literals.push_back(create_applicability_literal(action, translation_context, context).first.get_index());

    for (const auto variable : cond_eff.get_variables())
        conj_cond.variables.push_back(merge_p2d(variable, context).first.get_index());
    append_from_condition(cond_eff.get_condition(), translation_context, context, conj_cond);

    canonicalize(conj_cond);
    const auto new_conj_cond = context.destination.get_or_create(conj_cond).first;

    rule.variables = new_conj_cond.get_variables().get_data();
    rule.body = new_conj_cond.get_index();
    rule.head = effect.get_index();

    canonicalize(rule);
    return context.destination.get_or_create(rule);
}

auto create_effect_rule(fp::AxiomView axiom,
                        fd::AtomView<f::FluentTag> effect,
                        const TranslationContext<LiftedTag>& translation_context,
                        fp::MergeDatalogContext& context)
{
    auto rule_ptr = context.builder.get_builder<fd::Rule<f::PredicateTag>>();
    auto& rule = *rule_ptr;
    rule.clear();

    auto conj_cond_ptr = context.builder.get_builder<fd::ConjunctiveCondition>();
    auto& conj_cond = *conj_cond_ptr;
    conj_cond.clear();

    for (const auto variable : axiom.get_variables())
        conj_cond.variables.push_back(merge_p2d(variable, context).first.get_index());
    for (const auto literal : axiom.get_body().get_literals<f::StaticTag>())
        conj_cond.static_literals.push_back(merge_p2d(literal, translation_context.p2d.static_to_static_predicate, context).first.get_index());
    conj_cond.fluent_literals.push_back(create_applicability_literal(axiom, translation_context, context).first.get_index());

    canonicalize(conj_cond);
    const auto new_conj_cond = context.destination.get_or_create(conj_cond).first;

    rule.variables = new_conj_cond.get_variables().get_data();
    rule.body = new_conj_cond.get_index();
    rule.head = effect.get_index();

    canonicalize(rule);
    return context.destination.get_or_create(rule);
}

void translate_action_to_delete_free_rules(fp::ActionView action,
                                           ygg::Data<fd::Program>& program,
                                           const TranslationContext<LiftedTag>& translation_context,
                                           fp::MergeDatalogContext& context,
                                           GroundTaskProgram::AppPredicateToActionMapping& predicate_to_actions)
{
    const auto applicability_predicate = create_applicability_predicate(action, context).first;

    [[maybe_unused]] const auto [it, inserted] = predicate_to_actions.emplace(applicability_predicate, action);
    assert(inserted);

    program.fluent_predicates.push_back(applicability_predicate.get_index());

    const auto applicability_rule = create_applicability_rule(action, translation_context, context).first.get_index();

    program.predicate_rules.push_back(applicability_rule);

    for (const auto cond_eff : action.get_effects())
    {
        for (const auto literal : cond_eff.get_effect().get_literals())
        {
            if (!literal.get_polarity())
                continue;  /// ignore delete effects

            program.predicate_rules.push_back(
                create_cond_effect_rule(action,
                                        cond_eff,
                                        fp::merge_p2d(literal.get_atom(), translation_context.p2d.fluent_to_fluent_predicate, context).first,
                                        translation_context,
                                        context)
                    .first.get_index());
        }
    }
}

void translate_axiom_to_delete_free_axiom_rules(fp::AxiomView axiom,
                                                ygg::Data<fd::Program>& program,
                                                const TranslationContext<LiftedTag>& translation_context,
                                                fp::MergeDatalogContext& context,
                                                GroundTaskProgram::AppPredicateToAxiomMapping& predicate_to_axioms)
{
    const auto applicability_predicate = create_applicability_predicate(axiom, context).first;

    program.fluent_predicates.push_back(applicability_predicate.get_index());

    predicate_to_axioms[applicability_predicate].push_back(axiom);

    const auto applicability_rule = create_applicability_rule(axiom, translation_context, context).first.get_index();

    program.predicate_rules.push_back(applicability_rule);

    program.predicate_rules.push_back(
        create_effect_rule(axiom,
                           fp::merge_p2d<f::DerivedTag, f::FluentTag>(axiom.get_head(), translation_context.p2d.derived_to_fluent_predicate, context).first,
                           translation_context,
                           context)
            .first.get_index());
}

auto create_program(fp::TaskView task,
                    TranslationContext<LiftedTag>& translation_context,
                    GroundTaskProgram::AppPredicateToActionMapping& predicate_to_actions,
                    GroundTaskProgram::AppPredicateToAxiomMapping& predicate_to_axioms,
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
    for (const auto predicate : task.get_domain().get_predicates<f::DerivedTag>())
    {
        const auto new_predicate = fp::merge_p2d<f::DerivedTag, f::FluentTag>(predicate, context).first;
        translation_context.d2p.fluent_to_derived_predicate.emplace(new_predicate, predicate);
        translation_context.p2d.derived_to_fluent_predicate.emplace(predicate, new_predicate);
        program.fluent_predicates.push_back(new_predicate.get_index());
    }
    for (const auto predicate : task.get_derived_predicates())
    {
        const auto new_predicate = fp::merge_p2d<f::DerivedTag, f::FluentTag>(predicate, context).first;
        translation_context.d2p.fluent_to_derived_predicate.emplace(new_predicate, predicate);
        translation_context.p2d.derived_to_fluent_predicate.emplace(predicate, new_predicate);
        program.fluent_predicates.push_back(new_predicate.get_index());
    }

    for (const auto function : task.get_domain().get_functions<f::StaticTag>())
        program.static_functions.push_back(fp::merge_p2d(function, context).first.get_index());
    for (const auto function : task.get_domain().get_functions<f::FluentTag>())
        program.fluent_functions.push_back(fp::merge_p2d(function, context).first.get_index());

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

    for (const auto action : task.get_domain().get_actions())
        translate_action_to_delete_free_rules(action, program, translation_context, context, predicate_to_actions);

    for (const auto axiom : task.get_domain().get_axioms())
        translate_axiom_to_delete_free_axiom_rules(axiom, program, translation_context, context, predicate_to_axioms);

    for (const auto axiom : task.get_axioms())
        translate_axiom_to_delete_free_axiom_rules(axiom, program, translation_context, context, predicate_to_axioms);

    canonicalize(program);
    return destination.get_or_create(program).first;
}

static auto create_datalog_program(fp::TaskView task,
                                   TranslationContext<LiftedTag>& translation_context,
                                   GroundTaskProgram::AppPredicateToActionMapping& predicate_to_actions,
                                   GroundTaskProgram::AppPredicateToAxiomMapping& predicate_to_axioms)
{
    auto factory = std::make_shared<fd::RepositoryFactory>();
    auto repository = factory->create_shared(task.get_domain().get_constants().size() + task.get_objects().size());
    auto program = create_program(task, translation_context, predicate_to_actions, predicate_to_axioms, *repository);
    return datalog::Program<LiftedTag>(program, std::move(repository), std::move(factory));
}

}

GroundTaskProgram::GroundTaskProgram(fp::TaskView task) :
    m_translation_context(),
    m_predicate_to_actions(),
    m_predicate_to_axioms(),
    m_datalog_program(create_datalog_program(task, m_translation_context, m_predicate_to_actions, m_predicate_to_axioms))
{
    // std::cout << m_datalog_program.get_program() << std::endl;
}

const TranslationContext<LiftedTag>& GroundTaskProgram::get_translation_context() const noexcept { return m_translation_context; }

const GroundTaskProgram::AppPredicateToActionMapping& GroundTaskProgram::get_predicate_to_action_mapping() const noexcept { return m_predicate_to_actions; }

const GroundTaskProgram::AppPredicateToAxiomMapping& GroundTaskProgram::get_predicate_to_axiom_mapping() const noexcept { return m_predicate_to_axioms; }

datalog::Program<LiftedTag>& GroundTaskProgram::get_datalog_program() noexcept { return m_datalog_program; }

const datalog::Program<LiftedTag>& GroundTaskProgram::get_datalog_program() const noexcept { return m_datalog_program; }

const datalog::ConstProgramWorkspace<LiftedTag>& GroundTaskProgram::get_const_program_workspace() const noexcept
{
    return m_datalog_program.get_const_program_workspace();
}

}
