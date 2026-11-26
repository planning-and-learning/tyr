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
#include "tyr/formalism/conjunctive_condition_view.hpp"
#include "tyr/formalism/datalog/program_view.hpp"
#include "tyr/formalism/datalog/rule_body_view.hpp"
#include "tyr/formalism/datalog/rule_view.hpp"
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
#include "tyr/formalism/planning/task_view.hpp"
#include "tyr/formalism/predicate_view.hpp"
#include "tyr/formalism/repository.hpp"
#include "tyr/formalism/unary_operator_view.hpp"
#include "tyr/formalism/variable_view.hpp"

#include <cassert>
#include <cstddef>
#include <memory>
#include <utility>
#include <yggdrasil/formalism/row_view.hpp>

namespace tyr::planning
{

urd::flat_string create_applicability_name(formalism::ActionView<LiftedTag> action)
{ return urd::flat_string { std::string { "@" } + action.get_name().str() + std::string("_") + std::to_string(action.index().value) }; }

urd::flat_string create_triggered_name(formalism::ActionView<LiftedTag> action, formalism::ConditionalEffectView<LiftedTag> cond_eff)
{ return urd::flat_string { create_applicability_name(action).str() + std::string("_") + std::to_string(cond_eff.index().value) }; }

std::pair<formalism::PredicateView<formalism::DerivedTag>, bool>
create_applicability_predicate(formalism::ActionView<LiftedTag> action, formalism::GroundingContext<formalism::Builder, formalism::Repository>& context)
{
    auto predicate = formalism::checkout<formalism::Predicate<formalism::DerivedTag>>(context.builder());

    predicate->name = create_applicability_name(action);
    predicate->arity = action.get_arity();

    return context.destination().insert(*predicate, context.builder());
}

std::pair<formalism::AtomView<LiftedTag, formalism::DerivedTag>, bool>
create_applicability_atom(formalism::ActionView<LiftedTag> action, formalism::GroundingContext<formalism::Builder, formalism::Repository>& context)
{
    auto atom = formalism::checkout<formalism::Atom<LiftedTag, formalism::DerivedTag>>(context.builder());

    const auto applicability_predicate = create_applicability_predicate(action, context).first;

    // Its terms are the action's variables, in order: they are its parameters.
    auto parameters = formalism::checkout<formalism::Parameters>(context.builder());
    for (const auto variable : action.get_condition().get_variables())
        urd::append(parameters->variables, variable);
    urd::store(atom->parameters, context.destination().insert(*parameters, context.builder()).first);
    urd::store(atom->predicate, applicability_predicate);
    for (ygg::uint_t i = 0; i < applicability_predicate.get_arity(); ++i)
        urd::build_member(atom->terms.emplace_back(), formalism::Term(formalism::ParameterIndex(i)), context.destination());

    return context.destination().insert(*atom, context.builder());
}

std::pair<formalism::PredicateView<formalism::DerivedTag>, bool>
create_applicability_predicate(formalism::AxiomView<LiftedTag> axiom, formalism::GroundingContext<formalism::Builder, formalism::Repository>& context)
{
    auto predicate = formalism::checkout<formalism::Predicate<formalism::DerivedTag>>(context.builder());

    predicate->name = create_applicability_name(axiom);
    predicate->arity = axiom.get_body().get_arity();

    return context.destination().insert(*predicate, context.builder());
}

std::pair<formalism::AtomView<LiftedTag, formalism::DerivedTag>, bool>
create_applicability_atom(formalism::AxiomView<LiftedTag> axiom, formalism::GroundingContext<formalism::Builder, formalism::Repository>& context)
{
    auto atom = formalism::checkout<formalism::Atom<LiftedTag, formalism::DerivedTag>>(context.builder());

    const auto applicability_predicate = create_applicability_predicate(axiom, context).first;

    // Its terms are the axiom's variables, in order: they are its parameters.
    auto parameters = formalism::checkout<formalism::Parameters>(context.builder());
    for (const auto variable : axiom.get_body().get_variables())
        urd::append(parameters->variables, variable);
    urd::store(atom->parameters, context.destination().insert(*parameters, context.builder()).first);
    urd::store(atom->predicate, applicability_predicate);
    for (ygg::uint_t i = 0; i < applicability_predicate.get_arity(); ++i)
        urd::build_member(atom->terms.emplace_back(), formalism::Term(formalism::ParameterIndex(i)), context.destination());

    return context.destination().insert(*atom, context.builder());
}

namespace
{
/// A layer and the parent it stands on, held together.
struct Layer
{
    std::shared_ptr<const formalism::Repository> parent;
    formalism::Repository repository;

    explicit Layer(std::shared_ptr<const formalism::Repository> parent_) : parent(std::move(parent_)), repository(parent.get()) {}
};

/// Appends the records to `list`, asserting that each sits at the position of its index.
template<typename List, typename Range>
void append_dense(List& list, const Range& records)
{
    for (const auto record : records)
    {
        assert(record.index().value == list.size());
        urd::append(list, record);
    }
}
}

formalism::RepositoryPtr layer_over(std::shared_ptr<const formalism::Repository> parent)
{
    auto layer = std::make_shared<Layer>(std::move(parent));
    return formalism::RepositoryPtr(layer, &layer->repository);
}

urd::flat_string create_applicability_name(formalism::AxiomView<LiftedTag> axiom)
{
    return urd::flat_string { std::string { "@" } + axiom.get_head().get_predicate().get_name().str() + std::string("_")
                              + std::to_string(axiom.index().value) };
}

std::optional<formalism::LiteralView<GroundTag, formalism::FluentTag>>
create_fact_literal(formalism::FDRFactView<formalism::FluentTag> fact,
                    bool polarity,
                    formalism::GroundingContext<formalism::Builder, formalism::Repository>& context)
{
    const auto atom = fact.get_atom();
    if (!atom.has_value())
        return std::nullopt;

    auto literal = formalism::checkout<formalism::Literal<GroundTag, formalism::FluentTag>>(context.builder());
    literal->polarity = polarity;
    urd::store(literal->atom, *atom);
    return context.destination().insert(*literal, context.builder()).first;
}

template<TaskKind Kind>
void translate_task_symbols(formalism::TaskView<Kind> task, urd::builder<formalism::Program<Kind>, formalism::Repository>& program)
{
    append_dense(program.static_predicates, task.get_domain().template get_predicates<formalism::StaticTag>());
    append_dense(program.fluent_predicates, task.get_domain().template get_predicates<formalism::FluentTag>());
    append_dense(program.derived_predicates, task.get_domain().template get_predicates<formalism::DerivedTag>());
    append_dense(program.derived_predicates, task.get_derived_predicates());

    append_dense(program.static_functions, task.get_domain().template get_functions<formalism::StaticTag>());
    append_dense(program.fluent_functions, task.get_domain().template get_functions<formalism::FluentTag>());

    append_dense(program.objects, task.get_domain().get_constants());
    append_dense(program.objects, task.get_objects());

    for (const auto atom : task.template get_atoms<formalism::StaticTag>())
        urd::append(program.static_atoms, atom);
    for (const auto atom : task.template get_atoms<formalism::FluentTag>())
        urd::append(program.fluent_atoms, atom);
    // An atom a timed initial literal adds becomes true at its time, whatever the plan does: reachable as an initial one.
    for (const auto timed_literal : task.get_timed_literals())
        if (timed_literal.get_literal().get_polarity())
            urd::append(program.fluent_atoms, timed_literal.get_literal().get_atom());

    for (const auto fterm_value : task.template get_fterm_values<formalism::StaticTag>())
        urd::append(program.static_fterm_values, fterm_value);
    for (const auto fterm_value : task.template get_fterm_values<formalism::FluentTag>())
        urd::append(program.fluent_fterm_values, fterm_value);
}

template void translate_task_symbols(formalism::TaskView<LiftedTag> task, urd::builder<formalism::Program<LiftedTag>, formalism::Repository>& program);
template void translate_task_symbols(formalism::TaskView<GroundTag> task, urd::builder<formalism::Program<GroundTag>, formalism::Repository>& program);

}
