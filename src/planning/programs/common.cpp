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

#include "common.hpp"

#include "tyr/formalism/atom_view.hpp"
#include "tyr/formalism/binary_operator_view.hpp"
#include "tyr/formalism/datalog/conjunctive_condition_view.hpp"
#include "tyr/formalism/datalog/copy.hpp"
#include "tyr/formalism/datalog/interning.hpp"
#include "tyr/formalism/datalog/program_view.hpp"
#include "tyr/formalism/datalog/repository.hpp"
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
#include "tyr/formalism/planning/conjunctive_condition_view.hpp"
#include "tyr/formalism/planning/conjunctive_effect_view.hpp"
#include "tyr/formalism/planning/domain_view.hpp"
#include "tyr/formalism/planning/fdr_fact_view.hpp"
#include "tyr/formalism/planning/fdr_variable_view.hpp"
#include "tyr/formalism/planning/invariant_view.hpp"
#include "tyr/formalism/planning/merge_datalog.hpp"
#include "tyr/formalism/planning/metric_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/planning/task_view.hpp"
#include "tyr/formalism/predicate_view.hpp"
#include "tyr/formalism/unary_operator_view.hpp"
#include "tyr/formalism/variable_view.hpp"

#include <yggdrasil/formalism/row_view.hpp>

namespace tyr::planning
{

urd::flat_string create_applicability_name(formalism::planning::ActionView<LiftedTag> action)
{ return urd::flat_string { std::string { "@" } + action.get_name().str() + std::string("_") + std::to_string(action.index().value) }; }

urd::flat_string create_triggered_name(formalism::planning::ActionView<LiftedTag> action, formalism::planning::ConditionalEffectView<LiftedTag> cond_eff)
{ return urd::flat_string { create_applicability_name(action).str() + std::string("_") + std::to_string(cond_eff.index().value) }; }

std::pair<formalism::datalog::PredicateView<formalism::FluentTag>, bool>
create_applicability_predicate(formalism::planning::ActionView<LiftedTag> action,
                               formalism::WriteContext<formalism::datalog::Builder, formalism::datalog::Repository>& context)
{
    auto predicate = formalism::datalog::checkout<formalism::Predicate<formalism::FluentTag>>(context.builder());

    predicate->name = create_applicability_name(action);
    predicate->arity = action.get_arity();

    return context.destination().insert(*predicate, context.builder());
}

std::pair<formalism::datalog::AtomView<LiftedTag, formalism::FluentTag>, bool>
create_applicability_atom(formalism::planning::ActionView<LiftedTag> action,
                          formalism::WriteContext<formalism::datalog::Builder, formalism::datalog::Repository>& context)
{
    auto atom = formalism::datalog::checkout<formalism::Atom<LiftedTag, formalism::FluentTag>>(context.builder());

    const auto applicability_predicate = create_applicability_predicate(action, context).first;

    // Its terms are the action's variables, in order: they are its parameters.
    auto parameters = formalism::datalog::checkout<formalism::Parameters>(context.builder());
    for (const auto variable : action.get_condition().get_variables())
        urd::append(parameters->variables, formalism::copy(variable, context).first);
    urd::store(atom->parameters, context.destination().insert(*parameters, context.builder()).first);
    urd::store(atom->predicate, applicability_predicate);
    for (ygg::uint_t i = 0; i < applicability_predicate.get_arity(); ++i)
        urd::build_member(atom->terms.emplace_back(), formalism::Term(formalism::ParameterIndex(i)), context.destination());

    return context.destination().insert(*atom, context.builder());
}

std::pair<formalism::datalog::PredicateView<formalism::FluentTag>, bool>
create_applicability_predicate(formalism::planning::AxiomView<LiftedTag> axiom,
                               formalism::WriteContext<formalism::datalog::Builder, formalism::datalog::Repository>& context)
{
    auto predicate = formalism::datalog::checkout<formalism::Predicate<formalism::FluentTag>>(context.builder());

    predicate->name = create_applicability_name(axiom);
    predicate->arity = axiom.get_body().get_arity();

    return context.destination().insert(*predicate, context.builder());
}

std::pair<formalism::datalog::AtomView<LiftedTag, formalism::FluentTag>, bool>
create_applicability_atom(formalism::planning::AxiomView<LiftedTag> axiom,
                          formalism::WriteContext<formalism::datalog::Builder, formalism::datalog::Repository>& context)
{
    auto atom = formalism::datalog::checkout<formalism::Atom<LiftedTag, formalism::FluentTag>>(context.builder());

    const auto applicability_predicate = create_applicability_predicate(axiom, context).first;

    // Its terms are the axiom's variables, in order: they are its parameters.
    auto parameters = formalism::datalog::checkout<formalism::Parameters>(context.builder());
    for (const auto variable : axiom.get_body().get_variables())
        urd::append(parameters->variables, formalism::copy(variable, context).first);
    urd::store(atom->parameters, context.destination().insert(*parameters, context.builder()).first);
    urd::store(atom->predicate, applicability_predicate);
    for (ygg::uint_t i = 0; i < applicability_predicate.get_arity(); ++i)
        urd::build_member(atom->terms.emplace_back(), formalism::Term(formalism::ParameterIndex(i)), context.destination());

    return context.destination().insert(*atom, context.builder());
}

TranslationContext<LiftedTag> remap_translation_context(const TranslationContext<LiftedTag>& source,
                                                        formalism::WriteContext<formalism::datalog::Builder, formalism::datalog::Repository>& context)
{
    auto result = TranslationContext<LiftedTag> {};
    const auto remap_keys = [&](const auto& source_mapping, auto& result_mapping)
    {
        for (const auto& [key, value] : source_mapping)
            result_mapping.emplace(formalism::copy(key, context).first, value);
    };
    const auto remap_values = [&](const auto& source_mapping, auto& result_mapping)
    {
        for (const auto& [key, value] : source_mapping)
            result_mapping.emplace(key, formalism::copy(value, context).first);
    };

    remap_keys(source.d2p.static_to_static_predicate, result.d2p.static_to_static_predicate);
    remap_keys(source.d2p.fluent_to_fluent_predicate, result.d2p.fluent_to_fluent_predicate);
    remap_keys(source.d2p.fluent_to_derived_predicate, result.d2p.fluent_to_derived_predicate);
    remap_values(source.p2d.static_to_static_predicate, result.p2d.static_to_static_predicate);
    remap_values(source.p2d.fluent_to_fluent_predicate, result.p2d.fluent_to_fluent_predicate);
    remap_values(source.p2d.derived_to_fluent_predicate, result.p2d.derived_to_fluent_predicate);
    remap_values(source.p2d.fluent_to_fluent_function, result.p2d.fluent_to_fluent_function);
    return result;
}

urd::flat_string create_applicability_name(formalism::planning::AxiomView<LiftedTag> axiom)
{
    return urd::flat_string { std::string { "@" } + axiom.get_head().get_predicate().get_name().str() + std::string("_")
                              + std::to_string(axiom.index().value) };
}

void translate_task_symbols(formalism::planning::TaskView<LiftedTag> task,
                            bool with_derived,
                            TranslationContext<LiftedTag>& translation_context,
                            formalism::WriteContext<formalism::datalog::Builder, formalism::datalog::Repository>& context,
                            urd::builder<formalism::datalog::Program<LiftedTag>, formalism::datalog::Repository>& program)
{
    for (const auto predicate : task.get_domain().get_predicates<formalism::StaticTag>())
    {
        const auto new_predicate = formalism::copy(predicate, context).first;
        translation_context.d2p.static_to_static_predicate.emplace(new_predicate, predicate);
        translation_context.p2d.static_to_static_predicate.emplace(predicate, new_predicate);
        urd::append(program.static_predicates, new_predicate);
    }
    for (const auto predicate : task.get_domain().get_predicates<formalism::FluentTag>())
    {
        const auto new_predicate = formalism::copy(predicate, context).first;
        translation_context.d2p.fluent_to_fluent_predicate.emplace(new_predicate, predicate);
        translation_context.p2d.fluent_to_fluent_predicate.emplace(predicate, new_predicate);
        urd::append(program.fluent_predicates, new_predicate);
    }
    if (with_derived)
    {
        for (const auto predicate : task.get_domain().get_predicates<formalism::DerivedTag>())
        {
            const auto new_predicate = formalism::planning::merge_p2d<formalism::DerivedTag, formalism::FluentTag>(predicate, context).first;
            translation_context.d2p.fluent_to_derived_predicate.emplace(new_predicate, predicate);
            translation_context.p2d.derived_to_fluent_predicate.emplace(predicate, new_predicate);
            urd::append(program.fluent_predicates, new_predicate);
        }
        for (const auto predicate : task.get_derived_predicates())
        {
            const auto new_predicate = formalism::planning::merge_p2d<formalism::DerivedTag, formalism::FluentTag>(predicate, context).first;
            translation_context.d2p.fluent_to_derived_predicate.emplace(new_predicate, predicate);
            translation_context.p2d.derived_to_fluent_predicate.emplace(predicate, new_predicate);
            urd::append(program.fluent_predicates, new_predicate);
        }
    }

    for (const auto function : task.get_domain().get_functions<formalism::StaticTag>())
        urd::append(program.static_functions, formalism::copy(function, context).first);
    for (const auto function : task.get_domain().get_functions<formalism::FluentTag>())
    {
        const auto new_function = formalism::copy(function, context).first;
        translation_context.p2d.fluent_to_fluent_function.emplace(function, new_function);
        urd::append(program.fluent_functions, new_function);
    }

    for (const auto object : task.get_domain().get_constants())
        urd::append(program.objects, formalism::copy(object, context).first);
    for (const auto object : task.get_objects())
        urd::append(program.objects, formalism::copy(object, context).first);

    for (const auto atom : task.get_atoms<formalism::StaticTag>())
        urd::append(program.static_atoms, formalism::planning::merge_p2d(atom, translation_context.p2d.static_to_static_predicate, context).first);
    for (const auto atom : task.get_atoms<formalism::FluentTag>())
        urd::append(program.fluent_atoms, formalism::planning::merge_p2d(atom, translation_context.p2d.fluent_to_fluent_predicate, context).first);

    for (const auto fterm_value : task.get_fterm_values<formalism::StaticTag>())
        urd::append(program.static_fterm_values, formalism::copy(fterm_value, context).first);
    for (const auto fterm_value : task.get_fterm_values<formalism::FluentTag>())
        urd::append(program.fluent_fterm_values, formalism::copy(fterm_value, context).first);
}

}
