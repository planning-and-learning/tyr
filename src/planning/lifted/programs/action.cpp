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
#include "packed_layouts.hpp"
#include "tyr/formalism/datalog/expression_properties.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/datalog/views.hpp"
#include "tyr/formalism/planning/merge_datalog.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/planning/views.hpp"

#include <optional>
#include <algorithm>

namespace tyr::planning
{
namespace
{

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

auto create_program(formalism::planning::TaskView<LiftedTag> task,
                    TranslationContext<LiftedTag>& translation_context,
                    ApplicableActionProgram<LiftedTag>::AppPredicateToActionMapping& predicate_to_actions,
                    ApplicableActionProgram<LiftedTag>::SchemaPrograms& schema_programs,
                    formalism::datalog::Repository& repository)
{
    auto builder = formalism::datalog::Builder();
    auto context = formalism::planning::MergeDatalogContext(builder, repository);
    auto program = formalism::datalog::checkout<formalism::datalog::Program<LiftedTag>>(builder);
    auto domain_storage = std::optional<analysis::TaskVariableDomains> {};
    const auto* domains = packed_layout_domains(task, domain_storage);

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
    set_packed_binding_layouts(task, domains, translation_context.p2d, repository);

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
    {
        const auto applicability_predicate = create_applicability_predicate(action, context).first;

        [[maybe_unused]] const auto [it, inserted] = predicate_to_actions.emplace(applicability_predicate, action);
        assert(inserted);

        program->fluent_predicates.push_back(applicability_predicate.get_index());

        auto rule = formalism::datalog::checkout<formalism::datalog::Rule<LiftedTag, formalism::PredicateTag>>(builder);

        auto conj_cond = formalism::datalog::checkout<formalism::datalog::ConjunctiveCondition<LiftedTag>>(builder);

        for (const auto variable : action.get_variables())
            rule->variables.push_back(formalism::planning::merge_p2d(variable, context).first.get_index());

        for (const auto variable : action.get_condition().get_variables())
            conj_cond->variables.push_back(formalism::planning::merge_p2d(variable, context).first.get_index());

        for (const auto literal : action.get_condition().get_literals<formalism::StaticTag>())
            conj_cond->static_literals.push_back(
                formalism::planning::merge_p2d(literal, translation_context.p2d.static_to_static_predicate, context).first.get_index());

        for (const auto literal : action.get_condition().get_literals<formalism::FluentTag>())
            conj_cond->fluent_literals.push_back(
                formalism::planning::merge_p2d(literal, translation_context.p2d.fluent_to_fluent_predicate, context).first.get_index());

        for (const auto literal : action.get_condition().get_literals<formalism::DerivedTag>())
            conj_cond->fluent_literals.push_back(
                formalism::planning::merge_p2d<formalism::DerivedTag, formalism::FluentTag>(literal,
                                                                                            translation_context.p2d.derived_to_fluent_predicate,
                                                                                            context)
                    .first.get_index());

        for (const auto numeric_constraint : action.get_condition().get_numeric_constraints())
            conj_cond->numeric_constraints.push_back(formalism::planning::merge_p2d(numeric_constraint, context));

        const auto new_conj_cond = formalism::datalog::insert(repository, *conj_cond).first.get_index();

        rule->body = new_conj_cond;

        const auto applicability_atom = create_applicability_atom(action, context).first.get_index();

        rule->heads.push_back(applicability_atom);

        const auto new_rule = formalism::datalog::insert(repository, *rule).first.get_index();

        program->predicate_rules.push_back(new_rule);
    }

    set_packed_binding_layouts(domains, predicate_to_actions, repository);

    const auto all_actions = formalism::datalog::insert(repository, *program).first;
    for (const auto rule : all_actions.get_rules<formalism::PredicateTag>())
    {
        program->predicate_rules.clear();
        program->predicate_rules.push_back(rule.get_index());
        schema_programs.try_emplace(predicate_to_actions.at(rule.get_heads().front().get_predicate()),
                                    formalism::datalog::insert(repository, *program).first,
                                    translation_context.p2d);
    }
    return all_actions;
}

auto create_datalog_program(formalism::planning::TaskView<LiftedTag> task,
                            TranslationContext<LiftedTag>& translation_context,
                            ApplicableActionProgram<LiftedTag>::AppPredicateToActionMapping& mapping,
                            ApplicableActionProgram<LiftedTag>::SchemaPrograms& schema_programs)
{
    auto factory = std::make_shared<formalism::datalog::RepositoryFactory>();
    auto repository = factory->create_shared(task.get_num_objects());
    auto program = create_program(task, translation_context, mapping, schema_programs, *repository);
    return datalog::Program<LiftedTag>(program, std::move(repository), std::move(factory));
}
}

ApplicableActionProgram<LiftedTag>::SchemaProgram::SchemaProgram(formalism::datalog::ProgramView<LiftedTag> program_,
                                                                 const P2DTranslationContext<LiftedTag>& translation_context) :
    program(program_),
    input_translation(),
    strata(analysis::compute_rule_stratification(program)),
    listeners(analysis::compute_listeners(strata, program.get_context()))
{
    const auto rule = program.get_rules<formalism::PredicateTag>().front();
    const auto literals = rule.get_body().get_literals<formalism::FluentTag>();
    const auto reads_predicate = [&](auto predicate)
    {
        // Negative literals also need the complete relation to check absence.
        return std::ranges::any_of(literals, [&](auto literal) { return literal.get_atom().get_predicate() == predicate; });
    };
    for (const auto& [source, target] : translation_context.fluent_to_fluent_predicate)
        if (reads_predicate(target))
            input_translation.fluent_to_fluent_predicate.emplace(source, target);
    for (const auto& [source, target] : translation_context.derived_to_fluent_predicate)
        if (reads_predicate(target))
            input_translation.derived_to_fluent_predicate.emplace(source, target);

    const auto function_reads = formalism::datalog::collect_fluent_reads(rule);
    for (const auto& [source, target] : translation_context.fluent_to_fluent_function)
        if (std::ranges::any_of(function_reads, [&](auto term) { return term.get_function() == target; }))
            input_translation.fluent_to_fluent_function.emplace(source, target);
}

ApplicableActionProgram<LiftedTag>::ApplicableActionProgram(formalism::planning::TaskView<LiftedTag> task) :
    m_translation_context(),
    m_predicate_to_actions(),
    m_schema_programs(),
    m_datalog_program(create_datalog_program(task, m_translation_context, m_predicate_to_actions, m_schema_programs))
{
    // std::cout << m_datalog_program.get_program() << std::endl;
}

const TranslationContext<LiftedTag>& ApplicableActionProgram<LiftedTag>::get_translation_context() const noexcept { return m_translation_context; }

const ApplicableActionProgram<LiftedTag>::AppPredicateToActionMapping& ApplicableActionProgram<LiftedTag>::get_predicate_to_action_mapping() const noexcept
{
    return m_predicate_to_actions;
}

const ApplicableActionProgram<LiftedTag>::SchemaPrograms& ApplicableActionProgram<LiftedTag>::get_schema_programs() const noexcept { return m_schema_programs; }

datalog::Program<LiftedTag>& ApplicableActionProgram<LiftedTag>::get_datalog_program() noexcept { return m_datalog_program; }

const datalog::Program<LiftedTag>& ApplicableActionProgram<LiftedTag>::get_datalog_program() const noexcept { return m_datalog_program; }

const datalog::ConstProgramWorkspace<LiftedTag>& ApplicableActionProgram<LiftedTag>::get_const_program_workspace() const noexcept
{
    return m_datalog_program.get_const_program_workspace();
}
}
