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

#include "tyr/planning/lifted/programs/action.hpp"

#include "../../programs/common.hpp"
#include "../../programs/layered_program.hpp"
#include "tyr/formalism/atom_view.hpp"
#include "tyr/formalism/binary_operator_view.hpp"
#include "tyr/formalism/conjunctive_condition_view.hpp"
#include "tyr/formalism/datalog/expression_properties.hpp"
#include "tyr/formalism/datalog/interning.hpp"
#include "tyr/formalism/datalog/program_builder.hpp"
#include "tyr/formalism/datalog/program_view.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/datalog/rule_body_view.hpp"
#include "tyr/formalism/datalog/rule_view.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/detail/repository_instantiations.hpp"
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
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/planning/task_view.hpp"
#include "tyr/formalism/predicate_view.hpp"
#include "tyr/formalism/unary_operator_view.hpp"
#include "tyr/formalism/variable_view.hpp"

#include <algorithm>
#include <yggdrasil/formalism/row_view.hpp>

namespace tyr::planning
{
namespace
{

auto create_action_program(formalism::TaskView<LiftedTag> task,
                           AppPredicateToActionMapping& predicate_to_actions,
                           ConditionalEffectToPredicateMapping& effect_to_predicate,
                           formalism::Repository& repository)
{
    auto builder = formalism::Builder();
    auto context = formalism::WriteContext(builder, repository);
    auto program = formalism::checkout<formalism::Program<LiftedTag>>(builder);

    translate_task_symbols(task, *program);

    // Appends the literals and numeric constraints of a planning condition to a rule's body: the task's records.
    const auto append_condition = [&](auto condition, auto& conj_cond)
    {
        for (const auto literal : condition.template get_literals<formalism::StaticTag>())
            urd::append(conj_cond.static_literals, literal);
        for (const auto literal : condition.template get_literals<formalism::FluentTag>())
            urd::append(conj_cond.fluent_literals, literal);
        for (const auto literal : condition.template get_literals<formalism::DerivedTag>())
            urd::append(conj_cond.derived_literals, literal);
        for (const auto numeric_constraint : condition.get_numeric_constraints())
            urd::append(conj_cond.numeric_constraints, numeric_constraint);
    };

    for (const auto action : task.get_domain().get_actions())
    {
        const auto applicability_predicate = create_applicability_predicate(action, context).first;

        [[maybe_unused]] const auto [it, inserted] = predicate_to_actions.emplace(applicability_predicate, action);
        assert(inserted);

        urd::append(program->derived_predicates, applicability_predicate);

        // The action's rule: its body is the action's precondition.
        auto rule = formalism::checkout<formalism::Rule<LiftedTag>>(builder);
        for (const auto variable : action.get_variables())
            urd::append(rule->variables, variable);
        urd::store(rule->body, action.get_condition());
        urd::append(rule->derived_heads, create_applicability_atom(action, context).first);
        urd::append(program->rules, repository.insert(*rule, builder).first);

        // A rule per conditional effect with variables or a condition: its atoms are the bindings for which the effect fires.
        for (const auto cond_eff : action.get_effects())
        {
            // An effect without variables and condition fires with its action.
            if (cond_eff.get_variables().empty() && cond_eff.get_condition().empty())
                continue;
            const auto condition = cond_eff.get_condition();

            auto effect_predicate = formalism::checkout<formalism::Predicate<formalism::DerivedTag>>(builder);
            effect_predicate->name = create_triggered_name(action, cond_eff);
            effect_predicate->arity = action.get_arity() + cond_eff.get_arity();
            const auto new_effect_predicate = repository.insert(*effect_predicate, builder).first;
            urd::append(program->derived_predicates, new_effect_predicate);

            auto effect_rule = formalism::checkout<formalism::Rule<LiftedTag>>(builder);
            auto effect_condition = formalism::checkout<formalism::ConjunctiveCondition<LiftedTag>>(builder);
            for (const auto variable : action.get_variables())
                urd::append(effect_condition->variables, variable);
            for (const auto variable : cond_eff.get_variables())
                urd::append(effect_condition->variables, variable);
            urd::build_member(effect_rule->variables, effect_condition->variables, repository);
            append_condition(action.get_condition(), *effect_condition);
            append_condition(condition, *effect_condition);
            urd::store(effect_rule->body, repository.insert(*effect_condition, builder).first);

            auto effect_parameters = formalism::checkout<formalism::Parameters>(builder);
            urd::build_member(effect_parameters->variables, effect_condition->variables, repository);
            auto effect_atom = formalism::checkout<formalism::Atom<LiftedTag, formalism::DerivedTag>>(builder);
            urd::store(effect_atom->parameters, repository.insert(*effect_parameters, builder).first);
            urd::store(effect_atom->predicate, new_effect_predicate);
            for (ygg::uint_t i = 0; i < new_effect_predicate.get_arity(); ++i)
                urd::build_member(effect_atom->terms.emplace_back(), formalism::Term(formalism::ParameterIndex(i)), repository);
            urd::append(effect_rule->derived_heads, repository.insert(*effect_atom, builder).first);

            urd::append(program->rules, repository.insert(*effect_rule, builder).first);
            effect_to_predicate.emplace(std::pair(action, cond_eff), new_effect_predicate);
        }
    }

    return repository.insert(*program, builder).first;
}

auto create_action_datalog_program(const formalism::PlanningTask<LiftedTag>& planning_task,
                                   AppPredicateToActionMapping& mapping,
                                   ApplicableActionProgram::SchemaPrograms& schema_programs,
                                   ConditionalEffectToPredicateMapping& effect_to_predicate)
{
    const auto task = planning_task.get_task();
    const auto num_objects = task.get_num_objects();
    auto source_repository = layer_over(planning_task.get_repository(), num_objects);
    const auto source_program = create_action_program(task, mapping, effect_to_predicate, *source_repository);

    auto rules = datalog::RuleMapping {};
    const auto result = transform(LayeredProgram { source_program, std::move(source_repository) }, num_objects, rules, decompose_static_literals_pass());
    const auto program = result.program;
    auto repository = result.repository;
    auto builder = formalism::Builder {};
    auto context = formalism::WriteContext { builder, *repository };

    // A program per action schema with the rules that derive its applicability predicate and the predicates of its effects.
    auto rule_of = urd::Store<urd::map<formalism::PredicateView<formalism::DerivedTag>, urd::index<formalism::Rule<LiftedTag>>>> {};
    for (const auto rule : program.get_rules())
        rule_of.emplace(rule.get_derived_heads().front().get_predicate(), rule.index());
    auto schema = formalism::checkout<formalism::Program<LiftedTag>>(builder);
    formalism::list_except_rules(program, *schema);
    for (const auto& [predicate, action] : mapping)
    {
        schema->rules.clear();
        urd::append(schema->rules, repository->view(rule_of.at(predicate)));
        for (const auto effect : action.get_effects())
            if (const auto it = effect_to_predicate.find(std::pair(action, effect)); it != effect_to_predicate.end())
                urd::append(schema->rules, repository->view(rule_of.at(it->second)));
        schema_programs.try_emplace(action, repository->insert(*schema, builder).first);
    }
    return datalog::Program<LiftedTag>(program, std::move(repository));
}
}

ApplicableActionProgram::SchemaProgram::SchemaProgram(formalism::ProgramView<LiftedTag> program_) :
    program(program_),
    inputs(),
    strata(analysis::compute_rule_stratification(program)),
    listeners(analysis::compute_listeners(strata, program.context()))
{
    const auto rules = program.get_rules();
    const auto reads_predicate = [&]<typename F>(formalism::PredicateView<F> predicate)
    {
        // Negative literals also need the complete relation to check absence.
        return std::ranges::any_of(rules,
                                   [&](auto rule)
                                   {
                                       return std::ranges::any_of(rule.get_body().template get_literals<F>(),
                                                                  [&](auto literal) { return literal.get_atom().get_predicate() == predicate; });
                                   });
    };
    // The program lists the task's fluent and derived predicates (and its own derived ones, which no state holds).
    for (const auto predicate : program.get_predicates<formalism::FluentTag>())
        if (reads_predicate(predicate))
            inputs.fluent_predicates.insert(predicate);
    for (const auto predicate : program.get_predicates<formalism::DerivedTag>())
        if (reads_predicate(predicate))
            inputs.derived_predicates.insert(predicate);

    for (const auto rule : rules)
    {
        const auto function_reads = formalism::collect_fluent_reads(rule);
        for (const auto function : program.get_functions<formalism::FluentTag>())
            if (std::ranges::any_of(function_reads, [&](auto term) { return term.get_function() == function; }))
                inputs.fluent_functions.insert(function);
    }
}

ApplicableActionProgram::ApplicableActionProgram(const formalism::PlanningTask<LiftedTag>& task) :
    m_predicate_to_actions(),
    m_schema_programs(),
    m_conditional_effect_to_predicate(),
    m_datalog_program(create_action_datalog_program(task, m_predicate_to_actions, m_schema_programs, m_conditional_effect_to_predicate))
{
    // std::cout << m_datalog_program.get_program() << std::endl;
}

const AppPredicateToActionMapping& ApplicableActionProgram::get_predicate_to_action_mapping() const noexcept { return m_predicate_to_actions; }

const ApplicableActionProgram::SchemaPrograms& ApplicableActionProgram::get_schema_programs() const noexcept { return m_schema_programs; }

const ConditionalEffectToPredicateMapping& ApplicableActionProgram::get_conditional_effect_to_predicate_mapping() const noexcept
{ return m_conditional_effect_to_predicate; }

datalog::Program<LiftedTag>& ApplicableActionProgram::get_datalog_program() noexcept { return m_datalog_program; }

const datalog::Program<LiftedTag>& ApplicableActionProgram::get_datalog_program() const noexcept { return m_datalog_program; }

const datalog::ConstProgramWorkspace<LiftedTag>& ApplicableActionProgram::get_const_program_workspace() const noexcept
{ return m_datalog_program.get_const_program_workspace(); }
}
