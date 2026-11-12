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
#include "../../programs/translated_program.hpp"
#include "tyr/formalism/atom_data.hpp"
#include "tyr/formalism/atom_view.hpp"
#include "tyr/formalism/binary_operator_data.hpp"
#include "tyr/formalism/binary_operator_view.hpp"
#include "tyr/formalism/datalog/conjunctive_condition_data.hpp"
#include "tyr/formalism/datalog/conjunctive_condition_view.hpp"
#include "tyr/formalism/datalog/conjunctive_effect_data.hpp"
#include "tyr/formalism/datalog/conjunctive_effect_view.hpp"
#include "tyr/formalism/datalog/copy.hpp"
#include "tyr/formalism/datalog/formatter.hpp"
#include "tyr/formalism/datalog/interning.hpp"
#include "tyr/formalism/datalog/program_data.hpp"
#include "tyr/formalism/datalog/program_view.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/datalog/rule_data.hpp"
#include "tyr/formalism/datalog/rule_view.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/function_data.hpp"
#include "tyr/formalism/function_term_data.hpp"
#include "tyr/formalism/function_term_value_data.hpp"
#include "tyr/formalism/function_term_value_view.hpp"
#include "tyr/formalism/function_term_view.hpp"
#include "tyr/formalism/function_view.hpp"
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
#include "tyr/formalism/planning/conjunctive_condition_view.hpp"
#include "tyr/formalism/planning/conjunctive_effect_view.hpp"
#include "tyr/formalism/planning/domain_view.hpp"
#include "tyr/formalism/planning/fdr_fact_view.hpp"
#include "tyr/formalism/planning/fdr_variable_view.hpp"
#include "tyr/formalism/planning/formatter.hpp"
#include "tyr/formalism/planning/invariant_view.hpp"
#include "tyr/formalism/planning/merge_datalog.hpp"
#include "tyr/formalism/planning/metric_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/planning/task_view.hpp"
#include "tyr/formalism/predicate_data.hpp"
#include "tyr/formalism/predicate_view.hpp"
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
void process_axiom_body(formalism::planning::ConjunctiveConditionView<LiftedTag> axiom_body,
                        const TranslationContext<LiftedTag>& translation_context,
                        formalism::WriteContext<formalism::datalog::Builder, formalism::datalog::Repository>& context,
                        urd::data<formalism::datalog::ConjunctiveCondition<LiftedTag>>& conj_cond)
{
    for (const auto literal : axiom_body.get_literals<formalism::StaticTag>())
        conj_cond.static_literals.push_back(formalism::planning::merge_p2d(literal, translation_context.p2d.static_to_static_predicate, context).first.index());

    for (const auto literal : axiom_body.get_literals<formalism::FluentTag>())
        conj_cond.fluent_literals.push_back(formalism::planning::merge_p2d(literal, translation_context.p2d.fluent_to_fluent_predicate, context).first.index());

    for (const auto literal : axiom_body.get_literals<formalism::DerivedTag>())
        conj_cond.fluent_literals.push_back(
            formalism::planning::merge_p2d<formalism::DerivedTag, formalism::FluentTag>(literal, translation_context.p2d.derived_to_fluent_predicate, context)
                .first.index());

    for (const auto numeric_constraint : axiom_body.get_numeric_constraints())
        conj_cond.numeric_constraints.push_back(formalism::planning::merge_p2d(numeric_constraint, context).first.index());
}

auto create_axiom_rule(formalism::planning::AxiomView<LiftedTag> axiom,
                       const TranslationContext<LiftedTag>& translation_context,
                       formalism::WriteContext<formalism::datalog::Builder, formalism::datalog::Repository>& context)
{
    auto rule = formalism::datalog::checkout<formalism::datalog::Rule<LiftedTag>>(context.builder());

    auto conj_cond = formalism::datalog::checkout<formalism::datalog::ConjunctiveCondition<LiftedTag>>(context.builder());

    for (const auto variable : axiom.get_variables())
        conj_cond->variables.push_back(formalism::copy(variable, context).first.index());

    process_axiom_body(axiom.get_body(), translation_context, context, *conj_cond);

    const auto new_conj_cond = formalism::datalog::insert(context.destination(), *conj_cond).first.index();

    rule->body = new_conj_cond;

    const auto new_head = formalism::planning::merge_p2d<formalism::DerivedTag, formalism::FluentTag>(axiom.get_head(),
                                                                                                      translation_context.p2d.derived_to_fluent_predicate,
                                                                                                      context)
                              .first.index();

    rule->heads.push_back(new_head);

    return formalism::datalog::insert(context.destination(), *rule);
}

auto create_program(formalism::planning::TaskView<LiftedTag> task,
                    TranslationContext<LiftedTag>& translation_context,
                    formalism::datalog::Repository& repository)
{
    auto builder = formalism::datalog::Builder();
    auto context = formalism::WriteContext(builder, repository);
    auto program = formalism::datalog::checkout<formalism::datalog::Program<LiftedTag>>(builder);

    translate_task_symbols(task, true, translation_context, context, *program);

    for (const auto axiom : task.get_domain().get_axioms())
        program->rules.push_back(create_axiom_rule(axiom, translation_context, context).first.index());
    for (const auto axiom : task.get_axioms())
        program->rules.push_back(create_axiom_rule(axiom, translation_context, context).first.index());

    return formalism::datalog::insert(repository, *program).first;
}

struct AxiomTables
{
    TranslationContext<LiftedTag> translation_context;

    AxiomTables remap(SymbolRemapper& remapper) const { return { remapper.translation_context(translation_context) }; }
};

auto create_datalog_program(formalism::planning::TaskView<LiftedTag> task, TranslationContext<LiftedTag>& translation_context)
{
    const auto num_objects = task.get_num_objects();
    auto repository = std::make_shared<formalism::datalog::Repository>(num_objects);
    auto tables = AxiomTables {};
    const auto program = create_program(task, tables.translation_context, *repository);
    auto source = TranslatedProgram<AxiomTables> { program, std::move(repository), std::move(tables) };

    auto result = transform(source, num_objects, decompose_static_literals_pass());
    translation_context = std::move(result.tables.translation_context);
    return datalog::Program<LiftedTag>(result.program, std::move(result.repository));
}
}

AxiomEvaluatorProgram::AxiomEvaluatorProgram(formalism::planning::TaskView<LiftedTag> task) :
    m_translation_context(),
    m_datalog_program(create_datalog_program(task, m_translation_context))
{
    // std::cout << m_datalog_program.get_program() << std::endl;
}

const TranslationContext<LiftedTag>& AxiomEvaluatorProgram::get_translation_context() const noexcept { return m_translation_context; }

datalog::Program<LiftedTag>& AxiomEvaluatorProgram::get_datalog_program() noexcept { return m_datalog_program; }

const datalog::Program<LiftedTag>& AxiomEvaluatorProgram::get_datalog_program() const noexcept { return m_datalog_program; }

const datalog::ConstProgramWorkspace<LiftedTag>& AxiomEvaluatorProgram::get_const_program_workspace() const noexcept
{ return m_datalog_program.get_const_program_workspace(); }
}
