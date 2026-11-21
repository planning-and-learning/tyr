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
#include "tyr/formalism/datalog/interning.hpp"
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
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/planning/task_view.hpp"
#include "tyr/formalism/predicate_view.hpp"
#include "tyr/formalism/unary_operator_view.hpp"
#include "tyr/formalism/variable_view.hpp"

#include <cassert>
#include <cstddef>
#include <memory>
#include <utility>
#include <yggdrasil/formalism/row_view.hpp>

namespace tyr::planning
{

urd::flat_string create_applicability_name(formalism::planning::ActionView<LiftedTag> action)
{ return urd::flat_string { std::string { "@" } + action.get_name().str() + std::string("_") + std::to_string(action.index().value) }; }

urd::flat_string create_triggered_name(formalism::planning::ActionView<LiftedTag> action, formalism::planning::ConditionalEffectView<LiftedTag> cond_eff)
{ return urd::flat_string { create_applicability_name(action).str() + std::string("_") + std::to_string(cond_eff.index().value) }; }

std::pair<formalism::datalog::PredicateView<formalism::DerivedTag>, bool>
create_applicability_predicate(formalism::planning::ActionView<LiftedTag> action,
                               formalism::WriteContext<formalism::datalog::Builder, formalism::datalog::Repository>& context)
{
    auto predicate = formalism::datalog::checkout<formalism::Predicate<formalism::DerivedTag>>(context.builder());

    predicate->name = create_applicability_name(action);
    predicate->arity = action.get_arity();

    return context.destination().insert(*predicate, context.builder());
}

std::pair<formalism::datalog::AtomView<LiftedTag, formalism::DerivedTag>, bool>
create_applicability_atom(formalism::planning::ActionView<LiftedTag> action,
                          formalism::WriteContext<formalism::datalog::Builder, formalism::datalog::Repository>& context)
{
    auto atom = formalism::datalog::checkout<formalism::Atom<LiftedTag, formalism::DerivedTag>>(context.builder());

    const auto applicability_predicate = create_applicability_predicate(action, context).first;

    // Its terms are the action's variables, in order: they are its parameters.
    auto parameters = formalism::datalog::checkout<formalism::Parameters>(context.builder());
    for (const auto variable : action.get_condition().get_variables())
        urd::append(parameters->variables, variable);
    urd::store(atom->parameters, context.destination().insert(*parameters, context.builder()).first);
    urd::store(atom->predicate, applicability_predicate);
    for (ygg::uint_t i = 0; i < applicability_predicate.get_arity(); ++i)
        urd::build_member(atom->terms.emplace_back(), formalism::Term(formalism::ParameterIndex(i)), context.destination());

    return context.destination().insert(*atom, context.builder());
}

std::pair<formalism::datalog::PredicateView<formalism::DerivedTag>, bool>
create_applicability_predicate(formalism::planning::AxiomView<LiftedTag> axiom,
                               formalism::WriteContext<formalism::datalog::Builder, formalism::datalog::Repository>& context)
{
    auto predicate = formalism::datalog::checkout<formalism::Predicate<formalism::DerivedTag>>(context.builder());

    predicate->name = create_applicability_name(axiom);
    predicate->arity = axiom.get_body().get_arity();

    return context.destination().insert(*predicate, context.builder());
}

std::pair<formalism::datalog::AtomView<LiftedTag, formalism::DerivedTag>, bool>
create_applicability_atom(formalism::planning::AxiomView<LiftedTag> axiom,
                          formalism::WriteContext<formalism::datalog::Builder, formalism::datalog::Repository>& context)
{
    auto atom = formalism::datalog::checkout<formalism::Atom<LiftedTag, formalism::DerivedTag>>(context.builder());

    const auto applicability_predicate = create_applicability_predicate(axiom, context).first;

    // Its terms are the axiom's variables, in order: they are its parameters.
    auto parameters = formalism::datalog::checkout<formalism::Parameters>(context.builder());
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
    std::shared_ptr<const formalism::datalog::Repository> parent;
    formalism::datalog::Repository repository;

    Layer(std::shared_ptr<const formalism::datalog::Repository> parent_, std::size_t num_objects) :
        parent(std::move(parent_)),
        repository(num_objects, parent.get())
    {
    }
};

/// Appends the records to `list`, asserting that each sits at the position of its index.
template<typename List, typename Range>
void append_dense(List& list, Range&& records)
{
    for (const auto record : records)
    {
        assert(record.index().value == list.size());
        urd::append(list, record);
    }
}
}

formalism::datalog::RepositoryPtr layer_over(std::shared_ptr<const formalism::datalog::Repository> parent, std::size_t num_objects)
{
    auto layer = std::make_shared<Layer>(std::move(parent), num_objects);
    return formalism::datalog::RepositoryPtr(layer, &layer->repository);
}

urd::flat_string create_applicability_name(formalism::planning::AxiomView<LiftedTag> axiom)
{
    return urd::flat_string { std::string { "@" } + axiom.get_head().get_predicate().get_name().str() + std::string("_")
                              + std::to_string(axiom.index().value) };
}

std::optional<formalism::datalog::LiteralView<GroundTag, formalism::FluentTag>>
create_fact_literal(formalism::planning::FDRFactView<formalism::FluentTag> fact,
                    bool polarity,
                    formalism::WriteContext<formalism::datalog::Builder, formalism::datalog::Repository>& context)
{
    const auto atom = fact.get_atom();
    if (!atom.has_value())
        return std::nullopt;

    auto literal = formalism::datalog::checkout<formalism::Literal<GroundTag, formalism::FluentTag>>(context.builder());
    literal->polarity = polarity;
    urd::store(literal->atom, *atom);
    return context.destination().insert(*literal, context.builder()).first;
}

template<TaskKind Kind>
void translate_task_symbols(formalism::planning::TaskView<Kind> task, urd::builder<formalism::datalog::Program<Kind>, formalism::datalog::Repository>& program)
{
    append_dense(program.static_predicates, task.get_domain().template get_predicates<formalism::StaticTag>());
    append_dense(program.fluent_predicates, task.get_domain().template get_predicates<formalism::FluentTag>());
    append_dense(program.derived_predicates, task.get_domain().template get_predicates<formalism::DerivedTag>());
    append_dense(program.derived_predicates, task.get_derived_predicates());

    append_dense(program.static_functions, task.get_domain().template get_functions<formalism::StaticTag>());
    append_dense(program.fluent_functions, task.get_domain().template get_functions<formalism::FluentTag>());

    append_dense(program.objects, task.get_domain().template get_constants());
    append_dense(program.objects, task.get_objects());

    for (const auto atom : task.template get_atoms<formalism::StaticTag>())
        urd::append(program.static_atoms, atom);
    for (const auto atom : task.template get_atoms<formalism::FluentTag>())
        urd::append(program.fluent_atoms, atom);

    for (const auto fterm_value : task.template get_fterm_values<formalism::StaticTag>())
        urd::append(program.static_fterm_values, fterm_value);
    for (const auto fterm_value : task.template get_fterm_values<formalism::FluentTag>())
        urd::append(program.fluent_fterm_values, fterm_value);
}

template void translate_task_symbols(formalism::planning::TaskView<LiftedTag> task,
                                     urd::builder<formalism::datalog::Program<LiftedTag>, formalism::datalog::Repository>& program);
template void translate_task_symbols(formalism::planning::TaskView<GroundTag> task,
                                     urd::builder<formalism::datalog::Program<GroundTag>, formalism::datalog::Repository>& program);

}
