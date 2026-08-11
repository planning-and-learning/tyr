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

auto create_applicability_predicate(fp::ActionView action, fp::MergeDatalogContext& context)
{
    auto predicate = fd::checkout<f::Predicate<f::FluentTag>>(context.builder);

    predicate->name = create_applicability_name(action);
    predicate->arity = action.get_arity();

    return fd::get_or_create(context.destination, *predicate);
}

auto create_applicability_atom(fp::ActionView action, fp::MergeDatalogContext& context)
{
    auto atom = fd::checkout<f::datalog::Atom<f::FluentTag>>(context.builder);

    const auto applicability_predicate = create_applicability_predicate(action, context).first;

    atom->predicate = applicability_predicate.get_index();
    for (ygg::uint_t i = 0; i < applicability_predicate.get_arity(); ++i)
        atom->terms.push_back(ygg::Data<f::Term>(f::ParameterIndex(i)));

    return fd::get_or_create(context.destination, *atom);
}

auto create_program(fp::TaskView task,
                    TranslationContext<LiftedTag>& translation_context,
                    ApplicableActionProgram<LiftedTag>::AppPredicateToActionMapping& predicate_to_actions,
                    fd::Repository& repository)
{
    auto builder = fd::Builder();
    auto context = fp::MergeDatalogContext(builder, repository);
    auto program = fd::checkout<fd::Program>(builder);

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

    for (const auto action : task.get_domain().get_actions())
    {
        const auto applicability_predicate = create_applicability_predicate(action, context).first;

        [[maybe_unused]] const auto [it, inserted] = predicate_to_actions.emplace(applicability_predicate, action);
        assert(inserted);

        program->fluent_predicates.push_back(applicability_predicate.get_index());

        auto rule = fd::checkout<fd::Rule<f::PredicateTag>>(builder);

        auto conj_cond = fd::checkout<fd::ConjunctiveCondition>(builder);

        for (const auto variable : action.get_variables())
            rule->variables.push_back(fp::merge_p2d(variable, context).first.get_index());

        for (const auto variable : action.get_condition().get_variables())
            conj_cond->variables.push_back(fp::merge_p2d(variable, context).first.get_index());

        for (const auto literal : action.get_condition().get_literals<f::StaticTag>())
            conj_cond->static_literals.push_back(fp::merge_p2d(literal, translation_context.p2d.static_to_static_predicate, context).first.get_index());

        for (const auto literal : action.get_condition().get_literals<f::FluentTag>())
            conj_cond->fluent_literals.push_back(fp::merge_p2d(literal, translation_context.p2d.fluent_to_fluent_predicate, context).first.get_index());

        for (const auto literal : action.get_condition().get_literals<f::DerivedTag>())
            conj_cond->fluent_literals.push_back(
                fp::merge_p2d<f::DerivedTag, f::FluentTag>(literal, translation_context.p2d.derived_to_fluent_predicate, context).first.get_index());

        for (const auto numeric_constraint : action.get_condition().get_numeric_constraints())
            conj_cond->numeric_constraints.push_back(fp::merge_p2d(numeric_constraint, context));

        const auto new_conj_cond = fd::get_or_create(repository, *conj_cond).first.get_index();

        rule->body = new_conj_cond;

        const auto applicability_atom = create_applicability_atom(action, context).first.get_index();

        rule->head = applicability_atom;

        const auto new_rule = fd::get_or_create(repository, *rule).first.get_index();

        program->predicate_rules.push_back(new_rule);
    }

    return fd::get_or_create(repository, *program).first;
}

auto create_datalog_program(fp::TaskView task,
                            TranslationContext<LiftedTag>& translation_context,
                            ApplicableActionProgram<LiftedTag>::AppPredicateToActionMapping& mapping)
{
    auto factory = std::make_shared<fd::RepositoryFactory>();
    auto repository = factory->create_shared(task.get_domain().get_constants().size() + task.get_objects().size());
    auto program = create_program(task, translation_context, mapping, *repository);
    return datalog::Program<LiftedTag>(program, std::move(repository), std::move(factory));
}
}

ApplicableActionProgram<LiftedTag>::ApplicableActionProgram(fp::TaskView task) :
    m_translation_context(),
    m_predicate_to_actions(),
    m_datalog_program(create_datalog_program(task, m_translation_context, m_predicate_to_actions))
{
    // std::cout << m_datalog_program.get_program() << std::endl;
}

const TranslationContext<LiftedTag>& ApplicableActionProgram<LiftedTag>::get_translation_context() const noexcept { return m_translation_context; }

const ApplicableActionProgram<LiftedTag>::AppPredicateToActionMapping& ApplicableActionProgram<LiftedTag>::get_predicate_to_action_mapping() const noexcept
{
    return m_predicate_to_actions;
}

datalog::Program<LiftedTag>& ApplicableActionProgram<LiftedTag>::get_datalog_program() noexcept { return m_datalog_program; }

const datalog::Program<LiftedTag>& ApplicableActionProgram<LiftedTag>::get_datalog_program() const noexcept { return m_datalog_program; }

const datalog::ConstProgramWorkspace<LiftedTag>& ApplicableActionProgram<LiftedTag>::get_const_program_workspace() const noexcept
{
    return m_datalog_program.get_const_program_workspace();
}
}
