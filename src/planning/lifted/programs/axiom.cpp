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

#include "tyr/planning/lifted/programs/axiom.hpp"

#include "../../programs/common.hpp"
#include "../../programs/layered_program.hpp"
#include "tyr/formalism/atom_data.hpp"
#include "tyr/formalism/atom_view.hpp"
#include "tyr/formalism/binary_operator_data.hpp"
#include "tyr/formalism/binary_operator_view.hpp"
#include "tyr/formalism/conjunctive_condition_data.hpp"
#include "tyr/formalism/conjunctive_condition_view.hpp"
#include "tyr/formalism/datalog/formatter.hpp"
#include "tyr/formalism/datalog/program_data.hpp"
#include "tyr/formalism/datalog/program_view.hpp"
#include "tyr/formalism/datalog/rule_body_data.hpp"
#include "tyr/formalism/datalog/rule_body_view.hpp"
#include "tyr/formalism/datalog/rule_data.hpp"
#include "tyr/formalism/datalog/rule_view.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/function_data.hpp"
#include "tyr/formalism/function_term_data.hpp"
#include "tyr/formalism/function_term_value_data.hpp"
#include "tyr/formalism/function_term_value_view.hpp"
#include "tyr/formalism/function_term_view.hpp"
#include "tyr/formalism/function_view.hpp"
#include "tyr/formalism/grounding_context.hpp"
#include "tyr/formalism/literal_data.hpp"
#include "tyr/formalism/literal_view.hpp"
#include "tyr/formalism/multi_operator_data.hpp"
#include "tyr/formalism/multi_operator_view.hpp"
#include "tyr/formalism/numeric_effect_data.hpp"
#include "tyr/formalism/numeric_effect_view.hpp"
#include "tyr/formalism/object_data.hpp"
#include "tyr/formalism/object_view.hpp"
#include "tyr/formalism/parameter_index.hpp"
#include "tyr/formalism/planning/action_view.hpp"
#include "tyr/formalism/planning/axiom_view.hpp"
#include "tyr/formalism/planning/conditional_effect_view.hpp"
#include "tyr/formalism/planning/conjunctive_effect_view.hpp"
#include "tyr/formalism/planning/domain_view.hpp"
#include "tyr/formalism/planning/fdr_fact_view.hpp"
#include "tyr/formalism/planning/fdr_variable_view.hpp"
#include "tyr/formalism/planning/formatter.hpp"
#include "tyr/formalism/planning/invariant_view.hpp"
#include "tyr/formalism/planning/metric_view.hpp"
#include "tyr/formalism/planning/planning_task.hpp"
#include "tyr/formalism/planning/task_view.hpp"
#include "tyr/formalism/predicate_data.hpp"
#include "tyr/formalism/predicate_view.hpp"
#include "tyr/formalism/repository.hpp"
#include "tyr/formalism/unary_operator_data.hpp"
#include "tyr/formalism/unary_operator_view.hpp"
#include "tyr/formalism/variable_data.hpp"
#include "tyr/formalism/variable_view.hpp"

#include <yggdrasil/formalism/row_data.hpp>
#include <yggdrasil/formalism/row_view.hpp>

namespace tyr::planning
{
namespace
{
/// The rule of an axiom: its body the axiom's body over the axiom's variables (the task's literals), its head the axiom's.
auto create_axiom_rule(formalism::AxiomView<LiftedTag> axiom, formalism::GroundingContext<formalism::Builder, formalism::Repository>& context)
{
    auto rule = formalism::checkout<formalism::Rule<LiftedTag>>(context.builder());
    auto conj_cond = formalism::checkout<formalism::ConjunctiveCondition<LiftedTag>>(context.builder());

    const auto body = axiom.get_body();
    for (const auto variable : axiom.get_variables())
        urd::append(conj_cond->variables, variable);
    for (const auto literal : body.get_literals<formalism::StaticTag>())
        urd::append(conj_cond->static_literals, literal);
    for (const auto literal : body.get_literals<formalism::FluentTag>())
        urd::append(conj_cond->fluent_literals, literal);
    for (const auto literal : body.get_literals<formalism::DerivedTag>())
        urd::append(conj_cond->derived_literals, literal);
    for (const auto numeric_constraint : body.get_numeric_constraints())
        urd::append(conj_cond->numeric_constraints, numeric_constraint);

    urd::store(rule->body, context.destination().insert(*conj_cond, context.builder()).first);
    urd::append(rule->derived_heads, axiom.get_head());

    return context.destination().insert(*rule, context.builder());
}

auto create_axiom_program(formalism::TaskView<LiftedTag> task, formalism::Repository& repository)
{
    auto builder = formalism::Builder();
    auto context = formalism::GroundingContext(builder, repository);
    auto program = formalism::checkout<formalism::Program<LiftedTag>>(builder);

    translate_task_symbols(task, *program);

    for (const auto axiom : task.get_domain().get_axioms())
        urd::append(program->rules, create_axiom_rule(axiom, context).first);
    for (const auto axiom : task.get_axioms())
        urd::append(program->rules, create_axiom_rule(axiom, context).first);

    return repository.insert(*program, builder).first;
}

auto create_axiom_datalog_program(const formalism::PlanningTask<LiftedTag>& planning_task)
{
    const auto task = planning_task.get_task();
    auto repository = layer_over(planning_task.get_repository());
    const auto program = create_axiom_program(task, *repository);

    auto rules = datalog::RuleMapping {};
    auto result = transform(LayeredProgram { program, std::move(repository) }, rules, decompose_static_literals_pass());
    return datalog::Program<LiftedTag>(result.program, std::move(result.repository));
}
}

AxiomEvaluatorProgram::AxiomEvaluatorProgram(const formalism::PlanningTask<LiftedTag>& task) : m_datalog_program(create_axiom_datalog_program(task))
{
    // std::cout << m_datalog_program.get_program() << std::endl;
}

datalog::Program<LiftedTag>& AxiomEvaluatorProgram::get_datalog_program() noexcept { return m_datalog_program; }

const datalog::Program<LiftedTag>& AxiomEvaluatorProgram::get_datalog_program() const noexcept { return m_datalog_program; }

const datalog::ConstProgramWorkspace<LiftedTag>& AxiomEvaluatorProgram::get_const_program_workspace() const noexcept
{ return m_datalog_program.get_const_program_workspace(); }
}
