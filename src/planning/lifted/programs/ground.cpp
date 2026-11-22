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

#include "tyr/formalism/grounding_context.hpp"
#include "tyr/planning/lifted/programs/ground.hpp"

#include "../../programs/common.hpp"
#include "tyr/formalism/atom_view.hpp"
#include "tyr/formalism/binary_operator_view.hpp"
#include "tyr/formalism/conjunctive_condition_view.hpp"
#include "tyr/formalism/datalog/program_view.hpp"
#include "tyr/formalism/datalog/rule_body_view.hpp"
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
#include <yggdrasil/formalism/row_view.hpp>

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

void append_from_condition(formalism::ConjunctiveConditionView<LiftedTag> cond,
                           urd::builder<formalism::ConjunctiveCondition<LiftedTag>, formalism::Repository>& conj_cond)
{
    // Keep negated static atoms because they are monotonic
    for (const auto literal : cond.template get_literals<formalism::StaticTag>())
        urd::append(conj_cond.static_literals, literal);
    for (const auto literal : cond.template get_literals<formalism::FluentTag>())
        if (literal.get_polarity())
            urd::append(conj_cond.fluent_literals, literal);

    for (const auto literal : cond.template get_literals<formalism::DerivedTag>())
        if (literal.get_polarity())
            urd::append(conj_cond.derived_literals, literal);
};

auto create_applicability_literal(formalism::ActionView<LiftedTag> action, formalism::GroundingContext<formalism::Builder, formalism::Repository>& context)
{
    auto literal = formalism::checkout<formalism::Literal<LiftedTag, formalism::DerivedTag>>(context.builder());

    literal->polarity = true;
    urd::store(literal->atom, create_applicability_atom(action, context).first);

    return context.destination().insert(*literal, context.builder());
}

auto create_applicability_rule(formalism::ActionView<LiftedTag> action, formalism::GroundingContext<formalism::Builder, formalism::Repository>& context)
{
    auto rule = formalism::checkout<formalism::Rule<LiftedTag>>(context.builder());

    auto conj_cond = formalism::checkout<formalism::ConjunctiveCondition<LiftedTag>>(context.builder());

    for (const auto variable : action.get_variables())
        urd::append(conj_cond->variables, variable);
    append_from_condition(action.get_condition(), *conj_cond);

    const auto new_conj_cond = context.destination().insert(*conj_cond, context.builder()).first;

    for (const auto variable : new_conj_cond.get_variables())
        urd::append(rule->variables, variable);
    urd::store(rule->body, new_conj_cond);
    urd::append(rule->derived_heads, create_applicability_atom(action, context).first);

    return context.destination().insert(*rule, context.builder());
}

auto create_applicability_literal(formalism::AxiomView<LiftedTag> axiom, formalism::GroundingContext<formalism::Builder, formalism::Repository>& context)
{
    auto literal = formalism::checkout<formalism::Literal<LiftedTag, formalism::DerivedTag>>(context.builder());

    literal->polarity = true;
    urd::store(literal->atom, create_applicability_atom(axiom, context).first);

    return context.destination().insert(*literal, context.builder());
}

auto create_applicability_rule(formalism::AxiomView<LiftedTag> axiom, formalism::GroundingContext<formalism::Builder, formalism::Repository>& context)
{
    auto rule = formalism::checkout<formalism::Rule<LiftedTag>>(context.builder());

    auto conj_cond = formalism::checkout<formalism::ConjunctiveCondition<LiftedTag>>(context.builder());

    for (const auto variable : axiom.get_variables())
        urd::append(conj_cond->variables, variable);
    append_from_condition(axiom.get_body(), *conj_cond);

    const auto new_conj_cond = context.destination().insert(*conj_cond, context.builder()).first;

    for (const auto variable : new_conj_cond.get_variables())
        urd::append(rule->variables, variable);
    urd::store(rule->body, new_conj_cond);
    urd::append(rule->derived_heads, create_applicability_atom(axiom, context).first);

    return context.destination().insert(*rule, context.builder());
}

/// The rule that derives the add atoms of a conditional effect and, given its effect predicate, the atoms for which it fires.
auto create_ground_cond_effect_rule(formalism::ActionView<LiftedTag> action,
                                    formalism::ConditionalEffectView<LiftedTag> cond_eff,
                                    std::optional<formalism::PredicateView<formalism::DerivedTag>> effect_predicate,
                                    formalism::GroundingContext<formalism::Builder, formalism::Repository>& context)
{
    auto rule = formalism::checkout<formalism::Rule<LiftedTag>>(context.builder());

    auto conj_cond = formalism::checkout<formalism::ConjunctiveCondition<LiftedTag>>(context.builder());

    for (const auto variable : action.get_variables())
        urd::append(conj_cond->variables, variable);
    for (const auto literal : action.get_condition().get_literals<formalism::StaticTag>())
        urd::append(conj_cond->static_literals, literal);
    urd::append(conj_cond->derived_literals, create_applicability_literal(action, context).first);

    for (const auto variable : cond_eff.get_variables())
        urd::append(conj_cond->variables, variable);
    append_from_condition(cond_eff.get_condition(), *conj_cond);

    const auto new_conj_cond = context.destination().insert(*conj_cond, context.builder()).first;

    for (const auto variable : new_conj_cond.get_variables())
        urd::append(rule->variables, variable);
    urd::store(rule->body, new_conj_cond);
    for (const auto literal : cond_eff.get_effect().get_literals())
        if (literal.get_polarity())  /// ignore delete effects
            urd::append(rule->fluent_heads, literal.get_atom());
    if (effect_predicate)
    {
        auto effect_parameters = formalism::checkout<formalism::Parameters>(context.builder());
        urd::build_member(effect_parameters->variables, rule->variables, context.destination());
        auto effect_atom = formalism::checkout<formalism::Atom<LiftedTag, formalism::DerivedTag>>(context.builder());
        urd::store(effect_atom->parameters, context.destination().insert(*effect_parameters, context.builder()).first);
        urd::store(effect_atom->predicate, *effect_predicate);
        for (ygg::uint_t i = 0; i < effect_predicate->get_arity(); ++i)
            urd::build_member(effect_atom->terms.emplace_back(), formalism::Term(formalism::ParameterIndex(i)), context.destination());
        urd::append(rule->derived_heads, context.destination().insert(*effect_atom, context.builder()).first);
    }

    return context.destination().insert(*rule, context.builder());
}

auto create_effect_rule(formalism::AxiomView<LiftedTag> axiom,
                        formalism::AtomView<LiftedTag, formalism::DerivedTag> effect,
                        formalism::GroundingContext<formalism::Builder, formalism::Repository>& context)
{
    auto rule = formalism::checkout<formalism::Rule<LiftedTag>>(context.builder());

    auto conj_cond = formalism::checkout<formalism::ConjunctiveCondition<LiftedTag>>(context.builder());

    for (const auto variable : axiom.get_variables())
        urd::append(conj_cond->variables, variable);
    for (const auto literal : axiom.get_body().get_literals<formalism::StaticTag>())
        urd::append(conj_cond->static_literals, literal);
    urd::append(conj_cond->derived_literals, create_applicability_literal(axiom, context).first);

    const auto new_conj_cond = context.destination().insert(*conj_cond, context.builder()).first;

    for (const auto variable : new_conj_cond.get_variables())
        urd::append(rule->variables, variable);
    urd::store(rule->body, new_conj_cond);
    urd::append(rule->derived_heads, effect);

    return context.destination().insert(*rule, context.builder());
}

void translate_action_to_ground_rules(formalism::ActionView<LiftedTag> action,
                                      urd::builder<formalism::Program<LiftedTag>, formalism::Repository>& program,
                                      formalism::GroundingContext<formalism::Builder, formalism::Repository>& context,
                                      AppPredicateToActionMapping& predicate_to_actions,
                                      ConditionalEffectToPredicateMapping& effect_to_predicate)
{
    const auto applicability_predicate = create_applicability_predicate(action, context).first;

    [[maybe_unused]] const auto [it, inserted] = predicate_to_actions.emplace(applicability_predicate, action);
    assert(inserted);

    urd::append(program.derived_predicates, applicability_predicate);

    const auto applicability_rule = create_applicability_rule(action, context).first;

    urd::append(program.rules, applicability_rule);

    for (const auto cond_eff : action.get_effects())
    {
        auto effect_predicate = std::optional<formalism::PredicateView<formalism::DerivedTag>> {};
        // An effect without variables and condition fires with its action.
        if (!cond_eff.get_variables().empty() || !cond_eff.get_condition().empty())
        {
            auto predicate = formalism::checkout<formalism::Predicate<formalism::DerivedTag>>(context.builder());
            predicate->name = create_triggered_name(action, cond_eff);
            predicate->arity = action.get_arity() + cond_eff.get_arity();
            effect_predicate = context.destination().insert(*predicate, context.builder()).first;
            urd::append(program.derived_predicates, *effect_predicate);
        }
        if (!effect_predicate && std::ranges::none_of(cond_eff.get_effect().get_literals(), [](const auto literal) { return literal.get_polarity(); }))
            continue;

        urd::append(program.rules, create_ground_cond_effect_rule(action, cond_eff, effect_predicate, context).first);
        if (effect_predicate)
            effect_to_predicate.emplace(std::pair(action, cond_eff), *effect_predicate);
    }
}

void translate_axiom_to_delete_free_axiom_rules(formalism::AxiomView<LiftedTag> axiom,
                                                urd::builder<formalism::Program<LiftedTag>, formalism::Repository>& program,
                                                formalism::GroundingContext<formalism::Builder, formalism::Repository>& context,
                                                GroundTaskProgram::AppPredicateToAxiomMapping& predicate_to_axioms)
{
    const auto applicability_predicate = create_applicability_predicate(axiom, context).first;

    urd::append(program.derived_predicates, applicability_predicate);

    predicate_to_axioms[applicability_predicate].push_back(axiom);

    const auto applicability_rule = create_applicability_rule(axiom, context).first;

    urd::append(program.rules, applicability_rule);

    urd::append(program.rules, create_effect_rule(axiom, axiom.get_head(), context).first);
}

auto create_ground_program(formalism::TaskView<LiftedTag> task,
                           AppPredicateToActionMapping& predicate_to_actions,
                           GroundTaskProgram::AppPredicateToAxiomMapping& predicate_to_axioms,
                           ConditionalEffectToPredicateMapping& effect_to_predicate,
                           formalism::Repository& destination)
{
    auto builder = formalism::Builder();
    auto context = formalism::GroundingContext(builder, destination);
    auto program = formalism::checkout<formalism::Program<LiftedTag>>(builder);

    translate_task_symbols(task, *program);

    for (const auto action : task.get_domain().get_actions())
        translate_action_to_ground_rules(action, *program, context, predicate_to_actions, effect_to_predicate);

    for (const auto axiom : task.get_domain().get_axioms())
        translate_axiom_to_delete_free_axiom_rules(axiom, *program, context, predicate_to_axioms);

    for (const auto axiom : task.get_axioms())
        translate_axiom_to_delete_free_axiom_rules(axiom, *program, context, predicate_to_axioms);

    return destination.insert(*program, builder).first;
}

static auto create_ground_datalog_program(const formalism::PlanningTask<LiftedTag>& planning_task,
                                          AppPredicateToActionMapping& predicate_to_actions,
                                          GroundTaskProgram::AppPredicateToAxiomMapping& predicate_to_axioms,
                                          ConditionalEffectToPredicateMapping& effect_to_predicate)
{
    const auto task = planning_task.get_task();
    auto repository = layer_over(planning_task.get_repository());
    auto program = create_ground_program(task, predicate_to_actions, predicate_to_axioms, effect_to_predicate, *repository);
    return datalog::Program<LiftedTag>(program, std::move(repository));
}

}

GroundTaskProgram::GroundTaskProgram(const formalism::PlanningTask<LiftedTag>& task) :
    m_predicate_to_actions(),
    m_predicate_to_axioms(),
    m_conditional_effect_to_predicate(),
    m_datalog_program(create_ground_datalog_program(task, m_predicate_to_actions, m_predicate_to_axioms, m_conditional_effect_to_predicate))
{
    // std::cout << m_datalog_program.get_program() << std::endl;
}

const AppPredicateToActionMapping& GroundTaskProgram::get_predicate_to_action_mapping() const noexcept { return m_predicate_to_actions; }

const GroundTaskProgram::AppPredicateToAxiomMapping& GroundTaskProgram::get_predicate_to_axiom_mapping() const noexcept { return m_predicate_to_axioms; }

const ConditionalEffectToPredicateMapping& GroundTaskProgram::get_conditional_effect_to_predicate_mapping() const noexcept
{ return m_conditional_effect_to_predicate; }

datalog::Program<LiftedTag>& GroundTaskProgram::get_datalog_program() noexcept { return m_datalog_program; }

const datalog::Program<LiftedTag>& GroundTaskProgram::get_datalog_program() const noexcept { return m_datalog_program; }

const datalog::ConstProgramWorkspace<LiftedTag>& GroundTaskProgram::get_const_program_workspace() const noexcept
{ return m_datalog_program.get_const_program_workspace(); }

}
