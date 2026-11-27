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

#include "tyr/planning/programs/axiom.hpp"

#include "../common.hpp"
#include "tyr/datalog/applicability.hpp"
#include "tyr/datalog/static_rule_filter.hpp"
#include "tyr/formalism/atom_view.hpp"
#include "tyr/formalism/binary_operator_view.hpp"
#include "tyr/formalism/conjunctive_condition_view.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/function_term_value_view.hpp"
#include "tyr/formalism/function_term_view.hpp"
#include "tyr/formalism/function_view.hpp"
#include "tyr/formalism/grounding_context.hpp"
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

#include <optional>
#include <stdexcept>
#include <urd/store.hpp>
#include <utility>
#include <yggdrasil/formalism/row_view.hpp>

namespace tyr::planning
{
namespace
{
/// The literal of a fact of an axiom's condition: an FDR fact of a condition names an atom.
formalism::LiteralView<GroundTag, formalism::FluentTag> create_condition_literal(formalism::FDRFactView<formalism::FluentTag> fact,
                                                                                 bool polarity,
                                                                                 formalism::GroundingContext<formalism::Builder, formalism::Repository>& context)
{
    const auto literal = create_fact_literal(fact, polarity, context);
    if (!literal)
        throw std::logic_error("create_condition_literal: an axiom's condition holds an FDR fact without an atom.");
    return *literal;
}

/// The rule of a ground axiom: its body the axiom's body, its head the axiom's. Identical rules are one rule: the second
/// is not inserted.
std::pair<formalism::RuleView<GroundTag>, bool> create_axiom_rule(formalism::AxiomView<GroundTag> axiom,
                                                                  formalism::GroundingContext<formalism::Builder, formalism::Repository>& context)
{
    const auto condition = axiom.get_body();
    auto body = formalism::checkout<formalism::RuleBody<GroundTag>>(context.builder());
    for (const auto literal : condition.get_literals<formalism::StaticTag>())
        urd::append(body->static_literals, literal);
    for (const auto fact : condition.get_facts<formalism::PositiveTag>())
        urd::append(body->fluent_literals, create_condition_literal(fact, true, context));
    for (const auto fact : condition.get_facts<formalism::NegativeTag>())
        urd::append(body->fluent_literals, create_condition_literal(fact, false, context));
    for (const auto literal : condition.get_literals<formalism::DerivedTag>())
        urd::append(body->derived_literals, literal);
    for (const auto numeric_constraint : condition.get_numeric_constraints())
        urd::append(body->numeric_constraints, numeric_constraint);

    auto rule = formalism::checkout<formalism::Rule<GroundTag>>(context.builder());
    urd::store(rule->body, context.destination().insert(*body, context.builder()).first);
    urd::append(rule->derived_heads, axiom.get_head());
    return context.destination().insert(*rule, context.builder());
}

formalism::ProgramView<GroundTag> create_axiom_program(formalism::TaskView<GroundTag> task, formalism::Repository& repository)
{
    auto builder = formalism::Builder();
    auto context = formalism::GroundingContext(builder, repository);
    auto program = formalism::checkout<formalism::Program<GroundTag>>(builder);

    translate_task_symbols(task, *program);

    for (const auto axiom : task.get_ground_axioms())
        if (const auto [rule, inserted] = create_axiom_rule(axiom, context); inserted)
            urd::append(program->rules, rule);

    return repository.insert(*program, builder).first;
}

datalog::Program<GroundTag> create_axiom_datalog_program(const formalism::PlanningTask<GroundTag>& planning_task)
{
    auto source_repository = layer_over(planning_task.get_repository());
    const auto source_program = create_axiom_program(planning_task.get_task(), *source_repository);

    auto repository = layer_over(source_repository);
    const auto program = datalog::remove_statically_inapplicable_rules(source_program, *repository);
    return datalog::Program<GroundTag>(program, std::move(repository));
}
}

template<>
AxiomEvaluatorProgram<GroundTag>::AxiomEvaluatorProgram(const formalism::PlanningTask<GroundTag>& task) : m_datalog_program(create_axiom_datalog_program(task))
{
}

}
