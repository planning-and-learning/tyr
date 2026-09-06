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
#include "tyr/formalism/datalog/datas.hpp"
#include "tyr/formalism/datalog/formatter.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/datalog/views.hpp"
#include "tyr/formalism/planning/formatter.hpp"
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
void process_axiom_body(fp::ConjunctiveConditionView<LiftedTag> axiom_body,
                        const TranslationContext<LiftedTag>& translation_context,
                        fp::MergeDatalogContext& context,
                        ygg::Data<fd::ConjunctiveCondition<LiftedTag>>& conj_cond)
{
    for (const auto literal : axiom_body.get_literals<f::StaticTag>())
        conj_cond.static_literals.push_back(fp::merge_p2d(literal, translation_context.p2d.static_to_static_predicate, context).first.get_index());

    for (const auto literal : axiom_body.get_literals<f::FluentTag>())
        conj_cond.fluent_literals.push_back(fp::merge_p2d(literal, translation_context.p2d.fluent_to_fluent_predicate, context).first.get_index());

    for (const auto literal : axiom_body.get_literals<f::DerivedTag>())
        conj_cond.fluent_literals.push_back(
            fp::merge_p2d<f::DerivedTag, f::FluentTag>(literal, translation_context.p2d.derived_to_fluent_predicate, context).first.get_index());

    for (const auto numeric_constraint : axiom_body.get_numeric_constraints())
        conj_cond.numeric_constraints.push_back(fp::merge_p2d(numeric_constraint, context));
}

auto create_axiom_rule(fp::AxiomView<LiftedTag> axiom, const TranslationContext<LiftedTag>& translation_context, fp::MergeDatalogContext& context)
{
    auto rule = fd::checkout<fd::Rule<LiftedTag, f::PredicateTag>>(context.builder);

    auto conj_cond = fd::checkout<fd::ConjunctiveCondition<LiftedTag>>(context.builder);

    for (const auto variable : axiom.get_variables())
        conj_cond->variables.push_back(fp::merge_p2d(variable, context).first.get_index());

    process_axiom_body(axiom.get_body(), translation_context, context, *conj_cond);

    const auto new_conj_cond = fd::get_or_create(context.destination, *conj_cond).first.get_index();

    rule->body = new_conj_cond;

    const auto new_head =
        fp::merge_p2d<f::DerivedTag, f::FluentTag>(axiom.get_head(), translation_context.p2d.derived_to_fluent_predicate, context).first.get_index();

    rule->head = new_head;

    return fd::get_or_create(context.destination, *rule);
}

auto create_program(fp::TaskView task, TranslationContext<LiftedTag>& translation_context, fd::Repository& repository)
{
    auto builder = fd::Builder();
    auto context = fp::MergeDatalogContext(builder, repository);
    auto program = fd::checkout<fd::Program<LiftedTag>>(builder);

    for (const auto predicate : task.get_domain().get_predicates<f::StaticTag>())
    {
        const auto new_predicate = fp::merge_p2d(predicate, context).first;
        translation_context.d2p.static_to_static_predicate.emplace(new_predicate, predicate);
        translation_context.p2d.static_to_static_predicate.emplace(predicate, new_predicate);
        program->static_predicates.push_back(new_predicate.get_index());
    }
    for (const auto predicate : task.get_domain().get_predicates<f::FluentTag>())
    {
        const auto new_predicate = fp::merge_p2d(predicate, context).first;
        translation_context.d2p.fluent_to_fluent_predicate.emplace(new_predicate, predicate);
        translation_context.p2d.fluent_to_fluent_predicate.emplace(predicate, new_predicate);
        program->fluent_predicates.push_back(new_predicate.get_index());
    }
    for (const auto predicate : task.get_domain().get_predicates<f::DerivedTag>())
    {
        const auto new_predicate = fp::merge_p2d<f::DerivedTag, f::FluentTag>(predicate, context).first;
        translation_context.d2p.fluent_to_derived_predicate.emplace(new_predicate, predicate);
        translation_context.p2d.derived_to_fluent_predicate.emplace(predicate, new_predicate);
        program->fluent_predicates.push_back(new_predicate.get_index());
    }
    for (const auto predicate : task.get_derived_predicates())
    {
        const auto new_predicate = fp::merge_p2d<f::DerivedTag, f::FluentTag>(predicate, context).first;
        translation_context.d2p.fluent_to_derived_predicate.emplace(new_predicate, predicate);
        translation_context.p2d.derived_to_fluent_predicate.emplace(predicate, new_predicate);
        program->fluent_predicates.push_back(new_predicate.get_index());
    }

    for (const auto function : task.get_domain().get_functions<f::StaticTag>())
        program->static_functions.push_back(fp::merge_p2d(function, context).first.get_index());
    for (const auto function : task.get_domain().get_functions<f::FluentTag>())
        program->fluent_functions.push_back(fp::merge_p2d(function, context).first.get_index());

    for (const auto object : task.get_domain().get_constants())
        program->objects.push_back(fp::merge_p2d(object, context).first.get_index());
    for (const auto object : task.get_objects())
        program->objects.push_back(fp::merge_p2d(object, context).first.get_index());

    for (const auto atom : task.get_atoms<f::StaticTag>())
        program->static_atoms.push_back(fp::merge_p2d(atom, translation_context.p2d.static_to_static_predicate, context).first.get_index());
    for (const auto atom : task.get_atoms<f::FluentTag>())
        program->fluent_atoms.push_back(fp::merge_p2d(atom, translation_context.p2d.fluent_to_fluent_predicate, context).first.get_index());

    for (const auto fterm_value : task.get_fterm_values<f::StaticTag>())
        program->static_fterm_values.push_back(fp::merge_p2d(fterm_value, context).first.get_index());
    for (const auto fterm_value : task.get_fterm_values<f::FluentTag>())
        program->fluent_fterm_values.push_back(fp::merge_p2d(fterm_value, context).first.get_index());

    for (const auto axiom : task.get_domain().get_axioms())
        program->predicate_rules.push_back(create_axiom_rule(axiom, translation_context, context).first.get_index());
    for (const auto axiom : task.get_axioms())
        program->predicate_rules.push_back(create_axiom_rule(axiom, translation_context, context).first.get_index());

    return fd::get_or_create(repository, *program).first;
}

auto create_datalog_program(fp::TaskView task, TranslationContext<LiftedTag>& translation_context)
{
    auto factory = std::make_shared<fd::RepositoryFactory>();
    auto repository = factory->create_shared(task.get_domain().get_constants().size() + task.get_objects().size());
    auto program = create_program(task, translation_context, *repository);
    return datalog::Program<LiftedTag>(program, std::move(repository), std::move(factory));
}
}

AxiomEvaluatorProgram<LiftedTag>::AxiomEvaluatorProgram(fp::TaskView task) :
    m_translation_context(),
    m_datalog_program(create_datalog_program(task, m_translation_context))
{
    // std::cout << m_datalog_program.get_program() << std::endl;
}

const TranslationContext<LiftedTag>& AxiomEvaluatorProgram<LiftedTag>::get_translation_context() const noexcept { return m_translation_context; }

datalog::Program<LiftedTag>& AxiomEvaluatorProgram<LiftedTag>::get_datalog_program() noexcept { return m_datalog_program; }

const datalog::Program<LiftedTag>& AxiomEvaluatorProgram<LiftedTag>::get_datalog_program() const noexcept { return m_datalog_program; }

const datalog::ConstProgramWorkspace<LiftedTag>& AxiomEvaluatorProgram<LiftedTag>::get_const_program_workspace() const noexcept
{
    return m_datalog_program.get_const_program_workspace();
}
}
