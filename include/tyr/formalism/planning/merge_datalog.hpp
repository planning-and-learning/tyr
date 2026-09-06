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

#ifndef TYR_FORMALISM_PLANNING_MERGE_DATALOG_HPP_
#define TYR_FORMALISM_PLANNING_MERGE_DATALOG_HPP_

#include "tyr/formalism/datalog/canonicalization.hpp"
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/merge_datalog_decl.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <optional>
#include <yggdrasil/containers/associative_containers.hpp>
#include <yggdrasil/containers/tuple.hpp>
#include <yggdrasil/core/concepts.hpp>
#include <yggdrasil/semantics/equal_to.hpp>
#include <yggdrasil/semantics/hash.hpp>

namespace tyr::formalism::planning
{

// Common

std::pair<datalog::VariableView, bool> merge_p2d(VariableView element, MergeDatalogContext& context);

std::pair<datalog::ObjectView, bool> merge_p2d(ObjectView element, MergeDatalogContext& context);

ygg::Data<Term> merge_p2d(TermView element, MergeDatalogContext& context);

// Propositional

template<FactKind T_SRC, FactKind T_DST = T_SRC>
std::pair<datalog::PredicateView<T_DST>, bool> merge_p2d(PredicateView<T_SRC> element, MergeDatalogContext& context);

template<FactKind T_SRC, FactKind T_DST = T_SRC>
std::pair<datalog::AtomView<LiftedTag, T_DST>, bool> merge_p2d(AtomView<LiftedTag, T_SRC> element,  //
                                                               const ygg::UnorderedMap<PredicateView<T_SRC>, datalog::PredicateView<T_DST>>& predicate_mapping,
                                                               MergeDatalogContext& context);

template<FactKind T_SRC, FactKind T_DST = T_SRC>
std::pair<datalog::PredicateBindingView<T_DST>, bool> merge_p2d(PredicateBindingView<T_SRC> element,  //
                                                                const ygg::UnorderedMap<PredicateView<T_SRC>, datalog::PredicateView<T_DST>>& predicate_mapping,
                                                                MergeDatalogContext& context);

template<FactKind T_SRC, FactKind T_DST = T_SRC>
std::pair<datalog::AtomView<GroundTag, T_DST>, bool> merge_p2d(AtomView<GroundTag, T_SRC> element,  //
                                                               const ygg::UnorderedMap<PredicateView<T_SRC>, datalog::PredicateView<T_DST>>& predicate_mapping,
                                                               MergeDatalogContext& context);

template<FactKind T_SRC, FactKind T_DST = T_SRC>
std::pair<datalog::AtomView<GroundTag, T_DST>, bool> merge_p2d(AtomView<GroundTag, T_SRC> element,  //
                                                               ygg::UnorderedMap<AtomView<GroundTag, T_SRC>, datalog::AtomView<GroundTag, T_DST>>& atom_mapping,
                                                               const ygg::UnorderedMap<PredicateView<T_SRC>, datalog::PredicateView<T_DST>>& predicate_mapping,
                                                               MergeDatalogContext& context);

template<FactKind T_SRC, FactKind T_DST = T_SRC>
std::pair<datalog::LiteralView<LiftedTag, T_DST>, bool>
merge_p2d(LiteralView<LiftedTag, T_SRC> element,  //
          const ygg::UnorderedMap<PredicateView<T_SRC>, datalog::PredicateView<T_DST>>& predicate_mapping,
          MergeDatalogContext& context);

template<FactKind T_SRC, FactKind T_DST = T_SRC>
std::pair<datalog::LiteralView<GroundTag, T_DST>, bool>
merge_p2d(LiteralView<GroundTag, T_SRC> element,  //
          const ygg::UnorderedMap<PredicateView<T_SRC>, datalog::PredicateView<T_DST>>& predicate_mapping,
          MergeDatalogContext& context);

template<FactKind T_SRC, FactKind T_DST = T_SRC>
std::pair<datalog::LiteralView<GroundTag, T_DST>, bool>
merge_p2d(LiteralView<GroundTag, T_SRC> element,  //
          ygg::UnorderedMap<AtomView<GroundTag, T_SRC>, datalog::AtomView<GroundTag, T_DST>>& atom_mapping,
          const ygg::UnorderedMap<PredicateView<T_SRC>, datalog::PredicateView<T_DST>>& predicate_mapping,
          MergeDatalogContext& context);

std::optional<datalog::LiteralView<GroundTag, FluentTag>>
merge_p2d(FDRFactView<FluentTag> element,
          bool polarity,
          const ygg::UnorderedMap<PredicateView<FluentTag>, datalog::PredicateView<FluentTag>>& predicate_mapping,
          MergeDatalogContext& context);

std::optional<datalog::LiteralView<GroundTag, FluentTag>>
merge_p2d(FDRFactView<FluentTag> element,
          bool polarity,
          ygg::UnorderedMap<AtomView<GroundTag, FluentTag>, datalog::AtomView<GroundTag, FluentTag>>& atom_mapping,
          const ygg::UnorderedMap<PredicateView<FluentTag>, datalog::PredicateView<FluentTag>>& predicate_mapping,
          MergeDatalogContext& context);

std::pair<datalog::ConjunctiveConditionView<GroundTag>, bool>
merge_p2d(ConjunctiveConditionView<GroundTag> element,
          const ygg::UnorderedMap<PredicateView<FluentTag>, datalog::PredicateView<FluentTag>>& fluent_predicate_mapping,
          const ygg::UnorderedMap<PredicateView<DerivedTag>, datalog::PredicateView<FluentTag>>& derived_predicate_mapping,
          MergeDatalogContext& context);

std::pair<datalog::ConjunctiveConditionView<GroundTag>, bool>
merge_p2d(ConjunctiveConditionView<GroundTag> element,
          ygg::UnorderedMap<AtomView<GroundTag, FluentTag>, datalog::AtomView<GroundTag, FluentTag>>& fluent_atom_mapping,
          const ygg::UnorderedMap<PredicateView<FluentTag>, datalog::PredicateView<FluentTag>>& fluent_predicate_mapping,
          ygg::UnorderedMap<AtomView<GroundTag, DerivedTag>, datalog::AtomView<GroundTag, FluentTag>>& derived_atom_mapping,
          const ygg::UnorderedMap<PredicateView<DerivedTag>, datalog::PredicateView<FluentTag>>& derived_predicate_mapping,
          MergeDatalogContext& context);

std::pair<datalog::ConjunctiveConditionView<GroundTag>, bool>
merge_p2d(ConjunctiveConditionView<GroundTag> element,
          ygg::UnorderedMap<AtomView<GroundTag, FluentTag>, datalog::AtomView<GroundTag, FluentTag>>& fluent_atom_mapping,
          const ygg::UnorderedMap<PredicateView<FluentTag>, datalog::PredicateView<FluentTag>>& fluent_predicate_mapping,
          MergeDatalogContext& context);

// Numeric

template<FactKind T_SRC, FactKind T_DST = T_SRC>
std::pair<datalog::FunctionView<T_DST>, bool> merge_p2d(FunctionView<T_SRC> element, MergeDatalogContext& context);

template<FactKind T_SRC, FactKind T_DST = T_SRC>
std::pair<datalog::FunctionTermView<LiftedTag, T_DST>, bool> merge_p2d(FunctionTermView<LiftedTag, T_SRC> element, MergeDatalogContext& context);

template<FactKind T_SRC, FactKind T_DST = T_SRC>
std::pair<datalog::FunctionBindingView<T_DST>, bool> merge_p2d(FunctionBindingView<T_SRC> element, MergeDatalogContext& context);

template<FactKind T_SRC, FactKind T_DST = T_SRC>
std::pair<datalog::FunctionTermView<GroundTag, T_DST>, bool> merge_p2d(FunctionTermView<GroundTag, T_SRC> element, MergeDatalogContext& context);

template<FactKind T_SRC, FactKind T_DST = T_SRC>
std::pair<datalog::FunctionTermValueView<GroundTag, T_DST>, bool> merge_p2d(FunctionTermValueView<GroundTag, T_SRC> element, MergeDatalogContext& context);

template<FactKind T_SRC,
         FactKind T_DST = T_SRC,
         typename = std::enable_if_t<(std::same_as<T_SRC, FluentTag> || std::same_as<T_SRC, AuxiliaryTag>) && std::same_as<T_DST, FluentTag>>>
std::pair<datalog::NumericEffectView<LiftedTag, T_DST>, bool> merge_p2d(NumericEffectView<LiftedTag, T_SRC> element, MergeDatalogContext& context);

template<FactKind T_SRC,
         FactKind T_DST = T_SRC,
         typename = std::enable_if_t<(std::same_as<T_SRC, FluentTag> || std::same_as<T_SRC, AuxiliaryTag>) && std::same_as<T_DST, FluentTag>>>
ygg::Data<datalog::NumericEffectOperator<LiftedTag, T_DST>> merge_p2d(NumericEffectOperatorView<LiftedTag, T_SRC> element, MergeDatalogContext& context);

ygg::Data<datalog::FunctionExpression<LiftedTag>> merge_p2d(FunctionExpressionView<LiftedTag> element, MergeDatalogContext& context);

ygg::Data<datalog::FunctionExpression<GroundTag>> merge_p2d(FunctionExpressionView<GroundTag> element, MergeDatalogContext& context);

std::pair<datalog::MetricView, bool> merge_p2d(MetricView element, MergeDatalogContext& context);

template<FactKind T_SRC,
         FactKind T_DST = T_SRC,
         typename = std::enable_if_t<(std::same_as<T_SRC, FluentTag> || std::same_as<T_SRC, AuxiliaryTag>) && std::same_as<T_DST, FluentTag>>>
std::pair<datalog::NumericEffectView<GroundTag, T_DST>, bool> merge_p2d(NumericEffectView<GroundTag, T_SRC> element, MergeDatalogContext& context);

template<FactKind T_SRC,
         FactKind T_DST = T_SRC,
         typename = std::enable_if_t<(std::same_as<T_SRC, FluentTag> || std::same_as<T_SRC, AuxiliaryTag>) && std::same_as<T_DST, FluentTag>>>
ygg::Data<datalog::NumericEffectOperator<GroundTag, T_DST>> merge_p2d(NumericEffectOperatorView<GroundTag, T_SRC> element, MergeDatalogContext& context);

template<TaskKind T>
std::pair<datalog::UnaryOperatorView<T>, bool> merge_p2d(UnaryOperatorView<T> element, MergeDatalogContext& context);

template<TaskKind T, BinaryOperatorKind O>
std::pair<datalog::BinaryOperatorView<T, O>, bool> merge_p2d(BinaryOperatorView<T, O> element, MergeDatalogContext& context);

template<TaskKind T>
std::pair<datalog::MultiOperatorView<T>, bool> merge_p2d(MultiOperatorView<T> element, MergeDatalogContext& context);

template<TaskKind T>
ygg::Data<datalog::ArithmeticOperator<T>> merge_p2d(ArithmeticOperatorView<T> element, MergeDatalogContext& context);

template<TaskKind T>
ygg::Data<datalog::BooleanOperator<T>> merge_p2d(BooleanOperatorView<T> element, MergeDatalogContext& context);

// Common

inline std::pair<datalog::VariableView, bool> merge_p2d(VariableView element, MergeDatalogContext& context)
{
    auto variable = datalog::checkout<Variable>(context.builder);

    variable->name = element.get_name();

    return datalog::get_or_create(context.destination, *variable);
}

inline std::pair<datalog::ObjectView, bool> merge_p2d(ObjectView element, MergeDatalogContext& context)
{
    auto object = datalog::checkout<Object>(context.builder);

    object->name = element.get_name();

    return datalog::get_or_create(context.destination, *object);
}

inline ygg::Data<Term> merge_p2d(TermView element, MergeDatalogContext& context)
{
    return visit(
        [&](auto&& arg)
        {
            using Alternative = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<Alternative, ParameterIndex>)
                return ygg::Data<Term>(arg);
            else if constexpr (std::is_same_v<Alternative, ObjectView>)
                return ygg::Data<Term>(merge_p2d(arg, context).first.get_index());
            else
                static_assert(ygg::dependent_false<Alternative>::value, "Missing case");
        },
        element.get_variant());
}

// Propositional

template<FactKind T_SRC, FactKind T_DST>
std::pair<datalog::PredicateView<T_DST>, bool> merge_p2d(PredicateView<T_SRC> element, MergeDatalogContext& context)
{
    auto predicate = datalog::checkout<Predicate<T_DST>>(context.builder);

    predicate->name = element.get_name();
    predicate->arity = element.get_arity();

    return datalog::get_or_create(context.destination, *predicate);
}

template<FactKind T_SRC, FactKind T_DST>
std::pair<datalog::AtomView<LiftedTag, T_DST>, bool> merge_p2d(AtomView<LiftedTag, T_SRC> element,  //
                                                               const ygg::UnorderedMap<PredicateView<T_SRC>, datalog::PredicateView<T_DST>>& predicate_mapping,
                                                               MergeDatalogContext& context)
{
    auto atom = datalog::checkout<datalog::Atom<LiftedTag, T_DST>>(context.builder);

    atom->predicate = predicate_mapping.at(element.get_predicate()).get_index();
    for (const auto term : element.get_terms())
        atom->terms.push_back(merge_p2d(term, context));

    return datalog::get_or_create(context.destination, *atom);
}

template<FactKind T_SRC, FactKind T_DST>
std::pair<datalog::PredicateBindingView<T_DST>, bool> merge_p2d(PredicateBindingView<T_SRC> element,  //
                                                                const ygg::UnorderedMap<PredicateView<T_SRC>, datalog::PredicateView<T_DST>>& predicate_mapping,
                                                                MergeDatalogContext& context)
{
    auto binding = datalog::checkout<RelationBinding<Predicate<T_DST>>>(context.builder);

    binding->relation = predicate_mapping.at(element.get_relation()).get_index();
    for (const auto object : element.get_objects())
        binding->objects.push_back(object.get_index());

    return datalog::get_or_create(context.destination, *binding);
}

template<FactKind T_SRC, FactKind T_DST>
std::pair<datalog::AtomView<GroundTag, T_DST>, bool> merge_p2d(AtomView<GroundTag, T_SRC> element,  //
                                                               const ygg::UnorderedMap<PredicateView<T_SRC>, datalog::PredicateView<T_DST>>& predicate_mapping,
                                                               MergeDatalogContext& context)
{
    auto atom = datalog::checkout<datalog::Atom<GroundTag, T_DST>>(context.builder);

    atom->binding = merge_p2d<T_SRC, T_DST>(element.get_row(), predicate_mapping, context).first.get_index();

    return datalog::get_or_create(context.destination, *atom);
}

template<FactKind T_SRC, FactKind T_DST>
std::pair<datalog::AtomView<GroundTag, T_DST>, bool> merge_p2d(AtomView<GroundTag, T_SRC> element,  //
                                                               ygg::UnorderedMap<AtomView<GroundTag, T_SRC>, datalog::AtomView<GroundTag, T_DST>>& atom_mapping,
                                                               const ygg::UnorderedMap<PredicateView<T_SRC>, datalog::PredicateView<T_DST>>& predicate_mapping,
                                                               MergeDatalogContext& context)
{
    if (const auto it = atom_mapping.find(element); it != atom_mapping.end())
        return std::make_pair(it->second, false);

    auto [atom, inserted] = merge_p2d<T_SRC, T_DST>(element, predicate_mapping, context);
    atom_mapping.emplace(element, atom);
    return std::make_pair(atom, inserted);
}

template<FactKind T_SRC, FactKind T_DST>
std::pair<datalog::LiteralView<LiftedTag, T_DST>, bool>
merge_p2d(LiteralView<LiftedTag, T_SRC> element,  //
          const ygg::UnorderedMap<PredicateView<T_SRC>, datalog::PredicateView<T_DST>>& predicate_mapping,
          MergeDatalogContext& context)
{
    auto literal = datalog::checkout<datalog::Literal<LiftedTag, T_DST>>(context.builder);

    literal->polarity = element.get_polarity();
    literal->atom = merge_p2d<T_SRC, T_DST>(element.get_atom(), predicate_mapping, context).first.get_index();

    return datalog::get_or_create(context.destination, *literal);
}

template<FactKind T_SRC, FactKind T_DST>
std::pair<datalog::LiteralView<GroundTag, T_DST>, bool>
merge_p2d(LiteralView<GroundTag, T_SRC> element,  //
          const ygg::UnorderedMap<PredicateView<T_SRC>, datalog::PredicateView<T_DST>>& predicate_mapping,
          MergeDatalogContext& context)
{
    auto literal = datalog::checkout<datalog::Literal<GroundTag, T_DST>>(context.builder);

    literal->polarity = element.get_polarity();
    literal->atom = merge_p2d<T_SRC, T_DST>(element.get_atom(), predicate_mapping, context).first.get_index();

    return datalog::get_or_create(context.destination, *literal);
}

template<FactKind T_SRC, FactKind T_DST>
std::pair<datalog::LiteralView<GroundTag, T_DST>, bool>
merge_p2d(LiteralView<GroundTag, T_SRC> element,  //
          ygg::UnorderedMap<AtomView<GroundTag, T_SRC>, datalog::AtomView<GroundTag, T_DST>>& atom_mapping,
          const ygg::UnorderedMap<PredicateView<T_SRC>, datalog::PredicateView<T_DST>>& predicate_mapping,
          MergeDatalogContext& context)
{
    auto literal = datalog::checkout<datalog::Literal<GroundTag, T_DST>>(context.builder);

    literal->polarity = element.get_polarity();
    literal->atom = merge_p2d<T_SRC, T_DST>(element.get_atom(), atom_mapping, predicate_mapping, context).first.get_index();

    return datalog::get_or_create(context.destination, *literal);
}

inline std::optional<datalog::LiteralView<GroundTag, FluentTag>>
merge_p2d(FDRFactView<FluentTag> element,
          bool polarity,
          const ygg::UnorderedMap<PredicateView<FluentTag>, datalog::PredicateView<FluentTag>>& predicate_mapping,
          MergeDatalogContext& context)
{
    if (!element.has_value())
        return std::nullopt;

    auto literal = datalog::checkout<datalog::Literal<GroundTag, FluentTag>>(context.builder);
    literal->polarity = polarity;
    literal->atom = merge_p2d(element.get_atom().value(), predicate_mapping, context).first.get_index();

    return datalog::get_or_create(context.destination, *literal).first;
}

inline std::optional<datalog::LiteralView<GroundTag, FluentTag>>
merge_p2d(FDRFactView<FluentTag> element,
          bool polarity,
          ygg::UnorderedMap<AtomView<GroundTag, FluentTag>, datalog::AtomView<GroundTag, FluentTag>>& atom_mapping,
          const ygg::UnorderedMap<PredicateView<FluentTag>, datalog::PredicateView<FluentTag>>& predicate_mapping,
          MergeDatalogContext& context)
{
    if (!element.has_value())
        return std::nullopt;

    auto literal = datalog::checkout<datalog::Literal<GroundTag, FluentTag>>(context.builder);
    literal->polarity = polarity;
    literal->atom = merge_p2d(element.get_atom().value(), atom_mapping, predicate_mapping, context).first.get_index();

    return datalog::get_or_create(context.destination, *literal).first;
}

inline std::pair<datalog::ConjunctiveConditionView<GroundTag>, bool>
merge_p2d(ConjunctiveConditionView<GroundTag> element,
          const ygg::UnorderedMap<PredicateView<FluentTag>, datalog::PredicateView<FluentTag>>& fluent_predicate_mapping,
          const ygg::UnorderedMap<PredicateView<DerivedTag>, datalog::PredicateView<FluentTag>>& derived_predicate_mapping,
          MergeDatalogContext& context)
{
    auto condition = datalog::checkout<datalog::ConjunctiveCondition<GroundTag>>(context.builder);

    for (const auto fact : element.template get_facts<PositiveTag>())
        if (const auto literal = merge_p2d(fact, true, fluent_predicate_mapping, context))
            condition->fluent_literals.push_back(literal->get_index());

    for (const auto fact : element.template get_facts<NegativeTag>())
        if (const auto literal = merge_p2d(fact, false, fluent_predicate_mapping, context))
            condition->fluent_literals.push_back(literal->get_index());

    for (const auto literal : element.template get_literals<DerivedTag>())
        condition->fluent_literals.push_back(merge_p2d(literal, derived_predicate_mapping, context).first.get_index());

    for (const auto numeric_constraint : element.get_numeric_constraints())
        condition->numeric_constraints.push_back(merge_p2d(numeric_constraint, context));

    return datalog::get_or_create(context.destination, *condition);
}

inline std::pair<datalog::ConjunctiveConditionView<GroundTag>, bool>
merge_p2d(ConjunctiveConditionView<GroundTag> element,
          ygg::UnorderedMap<AtomView<GroundTag, FluentTag>, datalog::AtomView<GroundTag, FluentTag>>& fluent_atom_mapping,
          const ygg::UnorderedMap<PredicateView<FluentTag>, datalog::PredicateView<FluentTag>>& fluent_predicate_mapping,
          ygg::UnorderedMap<AtomView<GroundTag, DerivedTag>, datalog::AtomView<GroundTag, FluentTag>>& derived_atom_mapping,
          const ygg::UnorderedMap<PredicateView<DerivedTag>, datalog::PredicateView<FluentTag>>& derived_predicate_mapping,
          MergeDatalogContext& context)
{
    auto condition = datalog::checkout<datalog::ConjunctiveCondition<GroundTag>>(context.builder);

    for (const auto fact : element.template get_facts<PositiveTag>())
        if (const auto literal = merge_p2d(fact, true, fluent_atom_mapping, fluent_predicate_mapping, context))
            condition->fluent_literals.push_back(literal->get_index());

    for (const auto fact : element.template get_facts<NegativeTag>())
        if (const auto literal = merge_p2d(fact, false, fluent_atom_mapping, fluent_predicate_mapping, context))
            condition->fluent_literals.push_back(literal->get_index());

    for (const auto literal : element.template get_literals<DerivedTag>())
        condition->fluent_literals.push_back(merge_p2d(literal, derived_atom_mapping, derived_predicate_mapping, context).first.get_index());

    for (const auto numeric_constraint : element.get_numeric_constraints())
        condition->numeric_constraints.push_back(merge_p2d(numeric_constraint, context));

    return datalog::get_or_create(context.destination, *condition);
}

inline std::pair<datalog::ConjunctiveConditionView<GroundTag>, bool>
merge_p2d(ConjunctiveConditionView<GroundTag> element,
          ygg::UnorderedMap<AtomView<GroundTag, FluentTag>, datalog::AtomView<GroundTag, FluentTag>>& fluent_atom_mapping,
          const ygg::UnorderedMap<PredicateView<FluentTag>, datalog::PredicateView<FluentTag>>& fluent_predicate_mapping,
          MergeDatalogContext& context)
{
    auto condition = datalog::checkout<datalog::ConjunctiveCondition<GroundTag>>(context.builder);

    for (const auto fact : element.template get_facts<PositiveTag>())
        if (const auto literal = merge_p2d(fact, true, fluent_atom_mapping, fluent_predicate_mapping, context))
            condition->fluent_literals.push_back(literal->get_index());

    for (const auto fact : element.template get_facts<NegativeTag>())
        if (const auto literal = merge_p2d(fact, false, fluent_atom_mapping, fluent_predicate_mapping, context))
            condition->fluent_literals.push_back(literal->get_index());

    for (const auto numeric_constraint : element.get_numeric_constraints())
        condition->numeric_constraints.push_back(merge_p2d(numeric_constraint, context));

    return datalog::get_or_create(context.destination, *condition);
}

// Numeric

template<FactKind T_SRC, FactKind T_DST>
std::pair<datalog::FunctionView<T_DST>, bool> merge_p2d(FunctionView<T_SRC> element, MergeDatalogContext& context)
{
    auto function = datalog::checkout<Function<T_DST>>(context.builder);

    function->name = element.get_name();
    function->arity = element.get_arity();

    return datalog::get_or_create(context.destination, *function);
}

template<FactKind T_SRC, FactKind T_DST>
std::pair<datalog::FunctionTermView<LiftedTag, T_DST>, bool> merge_p2d(FunctionTermView<LiftedTag, T_SRC> element, MergeDatalogContext& context)
{
    auto fterm = datalog::checkout<datalog::FunctionTerm<LiftedTag, T_DST>>(context.builder);

    fterm->function = merge_p2d<T_SRC, T_DST>(element.get_function(), context).first.get_index();
    for (const auto term : element.get_terms())
        fterm->terms.push_back(merge_p2d(term, context));

    return datalog::get_or_create(context.destination, *fterm);
}

template<FactKind T_SRC, FactKind T_DST>
std::pair<datalog::FunctionBindingView<T_DST>, bool> merge_p2d(FunctionBindingView<T_SRC> element, MergeDatalogContext& context)
{
    auto binding = datalog::checkout<RelationBinding<Function<T_DST>>>(context.builder);

    binding->relation = merge_p2d<T_SRC, T_DST>(element.get_relation(), context).first.get_index();
    for (const auto object : element.get_objects())
        binding->objects.push_back(object.get_index());

    return datalog::get_or_create(context.destination, *binding);
}

template<FactKind T_SRC, FactKind T_DST>
std::pair<datalog::FunctionTermView<GroundTag, T_DST>, bool> merge_p2d(FunctionTermView<GroundTag, T_SRC> element, MergeDatalogContext& context)
{
    auto fterm = datalog::checkout<datalog::FunctionTerm<GroundTag, T_DST>>(context.builder);

    fterm->binding = merge_p2d<T_SRC, T_DST>(element.get_row(), context).first.get_index();

    return datalog::get_or_create(context.destination, *fterm);
}

template<FactKind T_SRC, FactKind T_DST>
std::pair<datalog::FunctionTermValueView<GroundTag, T_DST>, bool> merge_p2d(FunctionTermValueView<GroundTag, T_SRC> element, MergeDatalogContext& context)
{
    auto fterm_value = datalog::checkout<datalog::FunctionTermValue<GroundTag, T_DST>>(context.builder);

    fterm_value->fterm = merge_p2d<T_SRC, T_DST>(element.get_fterm(), context).first.get_index();
    fterm_value->value = element.get_value();

    return datalog::get_or_create(context.destination, *fterm_value);
}

template<FactKind T_SRC, FactKind T_DST, typename>
std::pair<datalog::NumericEffectView<LiftedTag, T_DST>, bool> merge_p2d(NumericEffectView<LiftedTag, T_SRC> element, MergeDatalogContext& context)
{
    auto numeric_effect = datalog::checkout<datalog::NumericEffect<LiftedTag, T_DST>>(context.builder);

    numeric_effect->operator_kind = element.get_operator();
    numeric_effect->fterm = merge_p2d<T_SRC, T_DST>(element.get_fterm(), context).first.get_index();
    numeric_effect->fexpr = merge_p2d(element.get_fexpr(), context);

    return datalog::get_or_create(context.destination, *numeric_effect);
}

template<FactKind T_SRC, FactKind T_DST, typename>
ygg::Data<datalog::NumericEffectOperator<LiftedTag, T_DST>> merge_p2d(NumericEffectOperatorView<LiftedTag, T_SRC> element, MergeDatalogContext& context)
{
    using OperatorData = ygg::Data<datalog::NumericEffectOperator<LiftedTag, T_DST>>;

    return visit([&](auto&& arg)
                 { return OperatorData(arg.get_operator(), typename OperatorData::Variant(merge_p2d<T_SRC, T_DST>(arg, context).first.get_index())); },
                 element.get_variant());
}

template<FactKind T_SRC, FactKind T_DST, typename>
std::pair<datalog::NumericEffectView<GroundTag, T_DST>, bool> merge_p2d(NumericEffectView<GroundTag, T_SRC> element, MergeDatalogContext& context)
{
    auto numeric_effect = datalog::checkout<datalog::NumericEffect<GroundTag, T_DST>>(context.builder);

    numeric_effect->operator_kind = element.get_operator();
    numeric_effect->fterm = merge_p2d<T_SRC, T_DST>(element.get_fterm(), context).first.get_index();
    numeric_effect->fexpr = merge_p2d(element.get_fexpr(), context);

    return datalog::get_or_create(context.destination, *numeric_effect);
}

template<FactKind T_SRC, FactKind T_DST, typename>
ygg::Data<datalog::NumericEffectOperator<GroundTag, T_DST>> merge_p2d(NumericEffectOperatorView<GroundTag, T_SRC> element, MergeDatalogContext& context)
{
    using OperatorData = ygg::Data<datalog::NumericEffectOperator<GroundTag, T_DST>>;

    return visit([&](auto&& arg)
                 { return OperatorData(arg.get_operator(), typename OperatorData::Variant(merge_p2d<T_SRC, T_DST>(arg, context).first.get_index())); },
                 element.get_variant());
}

inline ygg::Data<datalog::FunctionExpression<LiftedTag>> merge_p2d(FunctionExpressionView<LiftedTag> element, MergeDatalogContext& context)
{
    return visit(
        [&](auto&& arg) -> ygg::Data<datalog::FunctionExpression<LiftedTag>>
        {
            using Alternative = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<Alternative, ygg::float_t>)
                return ygg::Data<datalog::FunctionExpression<LiftedTag>>(arg);
            else if constexpr (std::is_same_v<Alternative, ArithmeticOperatorView<LiftedTag>>)
                return ygg::Data<datalog::FunctionExpression<LiftedTag>>(merge_p2d(arg, context));
            else if constexpr (std::is_same_v<Alternative, FunctionTermView<LiftedTag, AuxiliaryTag>>)
                return ygg::Data<datalog::FunctionExpression<LiftedTag>>(merge_p2d<AuxiliaryTag, FluentTag>(arg, context).first.get_index());
            else
                return ygg::Data<datalog::FunctionExpression<LiftedTag>>(merge_p2d(arg, context).first.get_index());
        },
        element.get_variant());
}

inline ygg::Data<datalog::FunctionExpression<GroundTag>> merge_p2d(FunctionExpressionView<GroundTag> element, MergeDatalogContext& context)
{
    return visit(
        [&](auto&& arg) -> ygg::Data<datalog::FunctionExpression<GroundTag>>
        {
            using Alternative = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<Alternative, ygg::float_t>)
                return ygg::Data<datalog::FunctionExpression<GroundTag>>(arg);
            else if constexpr (std::is_same_v<Alternative, ArithmeticOperatorView<GroundTag>>)
                return ygg::Data<datalog::FunctionExpression<GroundTag>>(merge_p2d(arg, context));
            else if constexpr (std::is_same_v<Alternative, FunctionTermView<GroundTag, AuxiliaryTag>>)
                return ygg::Data<datalog::FunctionExpression<GroundTag>>(merge_p2d<AuxiliaryTag, FluentTag>(arg, context).first.get_index());
            else
                return ygg::Data<datalog::FunctionExpression<GroundTag>>(merge_p2d(arg, context).first.get_index());
        },
        element.get_variant());
}

inline std::pair<datalog::MetricView, bool> merge_p2d(MetricView element, MergeDatalogContext& context)
{
    auto metric = datalog::checkout<datalog::Metric>(context.builder);
    metric->fexpr = merge_p2d(element.get_fexpr(), context);
    return datalog::get_or_create(context.destination, *metric);
}

template<TaskKind T>
std::pair<datalog::UnaryOperatorView<T>, bool> merge_p2d(UnaryOperatorView<T> element, MergeDatalogContext& context)
{
    auto unary = datalog::checkout<datalog::UnaryOperator<T>>(context.builder);

    unary->operator_kind = element.get_operator();
    unary->arg = merge_p2d(element.get_arg(), context);

    return datalog::get_or_create(context.destination, *unary);
}

template<TaskKind T, BinaryOperatorKind O>
std::pair<datalog::BinaryOperatorView<T, O>, bool> merge_p2d(BinaryOperatorView<T, O> element, MergeDatalogContext& context)
{
    auto binary = datalog::checkout<datalog::BinaryOperator<T, O>>(context.builder);

    binary->operator_kind = element.get_operator();
    binary->lhs = merge_p2d(element.get_lhs(), context);
    binary->rhs = merge_p2d(element.get_rhs(), context);

    return datalog::get_or_create(context.destination, *binary);
}

template<TaskKind T>
std::pair<datalog::MultiOperatorView<T>, bool> merge_p2d(MultiOperatorView<T> element, MergeDatalogContext& context)
{
    auto multi = datalog::checkout<datalog::MultiOperator<T>>(context.builder);

    multi->operator_kind = element.get_operator();
    for (const auto arg : element.get_args())
        multi->args.push_back(merge_p2d(arg, context));

    return datalog::get_or_create(context.destination, *multi);
}

template<TaskKind T>
ygg::Data<datalog::ArithmeticOperator<T>> merge_p2d(ArithmeticOperatorView<T> element, MergeDatalogContext& context)
{
    return visit([&](auto&& arg) { return ygg::Data<datalog::ArithmeticOperator<T>>(arg.get_operator(), merge_p2d(arg, context).first.get_index()); },
                 element.get_variant());
}

template<TaskKind T>
ygg::Data<datalog::BooleanOperator<T>> merge_p2d(BooleanOperatorView<T> element, MergeDatalogContext& context)
{
    return visit([&](auto&& arg) { return ygg::Data<datalog::BooleanOperator<T>>(arg.get_operator(), merge_p2d(arg, context).first.get_index()); },
                 element.get_variant());
}

}

#ifndef TYR_HEADER_INSTANTIATION

namespace tyr::formalism::planning
{
extern template std::pair<datalog::PredicateView<StaticTag>, bool> merge_p2d(PredicateView<StaticTag> element, MergeDatalogContext& context);
extern template std::pair<datalog::PredicateView<FluentTag>, bool> merge_p2d(PredicateView<FluentTag> element, MergeDatalogContext& context);
extern template std::pair<datalog::PredicateView<FluentTag>, bool> merge_p2d(PredicateView<DerivedTag> element, MergeDatalogContext& context);

extern template std::pair<datalog::AtomView<LiftedTag, StaticTag>, bool>
merge_p2d(AtomView<LiftedTag, StaticTag> element,
          const ygg::UnorderedMap<PredicateView<StaticTag>, datalog::PredicateView<StaticTag>>& predicate_mapping,
          MergeDatalogContext& context);
extern template std::pair<datalog::AtomView<LiftedTag, FluentTag>, bool>
merge_p2d(AtomView<LiftedTag, FluentTag> element,
          const ygg::UnorderedMap<PredicateView<FluentTag>, datalog::PredicateView<FluentTag>>& predicate_mapping,
          MergeDatalogContext& context);
extern template std::pair<datalog::AtomView<LiftedTag, FluentTag>, bool>
merge_p2d(AtomView<LiftedTag, DerivedTag> element,
          const ygg::UnorderedMap<PredicateView<DerivedTag>, datalog::PredicateView<FluentTag>>& predicate_mapping,
          MergeDatalogContext& context);
extern template std::pair<datalog::PredicateBindingView<StaticTag>, bool>
merge_p2d(PredicateBindingView<StaticTag> element,
          const ygg::UnorderedMap<PredicateView<StaticTag>, datalog::PredicateView<StaticTag>>& predicate_mapping,
          MergeDatalogContext& context);
extern template std::pair<datalog::PredicateBindingView<FluentTag>, bool>
merge_p2d(PredicateBindingView<FluentTag> element,
          const ygg::UnorderedMap<PredicateView<FluentTag>, datalog::PredicateView<FluentTag>>& predicate_mapping,
          MergeDatalogContext& context);
extern template std::pair<datalog::PredicateBindingView<FluentTag>, bool>
merge_p2d(PredicateBindingView<DerivedTag> element,
          const ygg::UnorderedMap<PredicateView<DerivedTag>, datalog::PredicateView<FluentTag>>& predicate_mapping,
          MergeDatalogContext& context);

extern template std::pair<datalog::AtomView<GroundTag, StaticTag>, bool>
merge_p2d(AtomView<GroundTag, StaticTag> element,
          const ygg::UnorderedMap<PredicateView<StaticTag>, datalog::PredicateView<StaticTag>>& predicate_mapping,
          MergeDatalogContext& context);
extern template std::pair<datalog::AtomView<GroundTag, FluentTag>, bool>
merge_p2d(AtomView<GroundTag, FluentTag> element,
          const ygg::UnorderedMap<PredicateView<FluentTag>, datalog::PredicateView<FluentTag>>& predicate_mapping,
          MergeDatalogContext& context);
extern template std::pair<datalog::AtomView<GroundTag, FluentTag>, bool>
merge_p2d(AtomView<GroundTag, DerivedTag> element,
          const ygg::UnorderedMap<PredicateView<DerivedTag>, datalog::PredicateView<FluentTag>>& predicate_mapping,
          MergeDatalogContext& context);

extern template std::pair<datalog::LiteralView<LiftedTag, StaticTag>, bool>
merge_p2d(LiteralView<LiftedTag, StaticTag> element,
          const ygg::UnorderedMap<PredicateView<StaticTag>, datalog::PredicateView<StaticTag>>& predicate_mapping,
          MergeDatalogContext& context);
extern template std::pair<datalog::LiteralView<LiftedTag, FluentTag>, bool>
merge_p2d(LiteralView<LiftedTag, FluentTag> element,
          const ygg::UnorderedMap<PredicateView<FluentTag>, datalog::PredicateView<FluentTag>>& predicate_mapping,
          MergeDatalogContext& context);
extern template std::pair<datalog::LiteralView<LiftedTag, FluentTag>, bool>
merge_p2d(LiteralView<LiftedTag, DerivedTag> element,
          const ygg::UnorderedMap<PredicateView<DerivedTag>, datalog::PredicateView<FluentTag>>& predicate_mapping,
          MergeDatalogContext& context);
extern template std::pair<datalog::LiteralView<GroundTag, StaticTag>, bool>
merge_p2d(LiteralView<GroundTag, StaticTag> element,
          const ygg::UnorderedMap<PredicateView<StaticTag>, datalog::PredicateView<StaticTag>>& predicate_mapping,
          MergeDatalogContext& context);
extern template std::pair<datalog::LiteralView<GroundTag, FluentTag>, bool>
merge_p2d(LiteralView<GroundTag, FluentTag> element,
          const ygg::UnorderedMap<PredicateView<FluentTag>, datalog::PredicateView<FluentTag>>& predicate_mapping,
          MergeDatalogContext& context);
extern template std::pair<datalog::LiteralView<GroundTag, FluentTag>, bool>
merge_p2d(LiteralView<GroundTag, DerivedTag> element,
          const ygg::UnorderedMap<PredicateView<DerivedTag>, datalog::PredicateView<FluentTag>>& predicate_mapping,
          MergeDatalogContext& context);

std::optional<datalog::LiteralView<GroundTag, FluentTag>>
merge_p2d(FDRFactView<FluentTag> element,
          bool polarity,
          const ygg::UnorderedMap<PredicateView<FluentTag>, datalog::PredicateView<FluentTag>>& predicate_mapping,
          MergeDatalogContext& context);

std::optional<datalog::LiteralView<GroundTag, FluentTag>>
merge_p2d(FDRFactView<FluentTag> element,
          bool polarity,
          ygg::UnorderedMap<AtomView<GroundTag, FluentTag>, datalog::AtomView<GroundTag, FluentTag>>& atom_mapping,
          const ygg::UnorderedMap<PredicateView<FluentTag>, datalog::PredicateView<FluentTag>>& predicate_mapping,
          MergeDatalogContext& context);

std::pair<datalog::ConjunctiveConditionView<GroundTag>, bool>
merge_p2d(ConjunctiveConditionView<GroundTag> element,
          const ygg::UnorderedMap<PredicateView<FluentTag>, datalog::PredicateView<FluentTag>>& fluent_predicate_mapping,
          const ygg::UnorderedMap<PredicateView<DerivedTag>, datalog::PredicateView<FluentTag>>& derived_predicate_mapping,
          MergeDatalogContext& context);

std::pair<datalog::ConjunctiveConditionView<GroundTag>, bool>
merge_p2d(ConjunctiveConditionView<GroundTag> element,
          ygg::UnorderedMap<AtomView<GroundTag, FluentTag>, datalog::AtomView<GroundTag, FluentTag>>& fluent_atom_mapping,
          const ygg::UnorderedMap<PredicateView<FluentTag>, datalog::PredicateView<FluentTag>>& fluent_predicate_mapping,
          ygg::UnorderedMap<AtomView<GroundTag, DerivedTag>, datalog::AtomView<GroundTag, FluentTag>>& derived_atom_mapping,
          const ygg::UnorderedMap<PredicateView<DerivedTag>, datalog::PredicateView<FluentTag>>& derived_predicate_mapping,
          MergeDatalogContext& context);

std::pair<datalog::ConjunctiveConditionView<GroundTag>, bool>
merge_p2d(ConjunctiveConditionView<GroundTag> element,
          ygg::UnorderedMap<AtomView<GroundTag, FluentTag>, datalog::AtomView<GroundTag, FluentTag>>& fluent_atom_mapping,
          const ygg::UnorderedMap<PredicateView<FluentTag>, datalog::PredicateView<FluentTag>>& fluent_predicate_mapping,
          MergeDatalogContext& context);

// Numeric

extern template std::pair<datalog::FunctionView<StaticTag>, bool> merge_p2d(FunctionView<StaticTag> element, MergeDatalogContext& context);
extern template std::pair<datalog::FunctionView<FluentTag>, bool> merge_p2d(FunctionView<FluentTag> element, MergeDatalogContext& context);

extern template std::pair<datalog::FunctionTermView<LiftedTag, StaticTag>, bool> merge_p2d(FunctionTermView<LiftedTag, StaticTag> element,
                                                                                           MergeDatalogContext& context);
extern template std::pair<datalog::FunctionTermView<LiftedTag, FluentTag>, bool> merge_p2d(FunctionTermView<LiftedTag, FluentTag> element,
                                                                                           MergeDatalogContext& context);

extern template std::pair<datalog::FunctionBindingView<StaticTag>, bool> merge_p2d(FunctionBindingView<StaticTag> element, MergeDatalogContext& context);
extern template std::pair<datalog::FunctionBindingView<FluentTag>, bool> merge_p2d(FunctionBindingView<FluentTag> element, MergeDatalogContext& context);

extern template std::pair<datalog::FunctionTermView<GroundTag, StaticTag>, bool> merge_p2d(FunctionTermView<GroundTag, StaticTag> element,
                                                                                           MergeDatalogContext& context);
extern template std::pair<datalog::FunctionTermView<GroundTag, FluentTag>, bool> merge_p2d(FunctionTermView<GroundTag, FluentTag> element,
                                                                                           MergeDatalogContext& context);

extern template std::pair<datalog::FunctionTermValueView<GroundTag, StaticTag>, bool> merge_p2d(FunctionTermValueView<GroundTag, StaticTag> element,
                                                                                                MergeDatalogContext& context);
extern template std::pair<datalog::FunctionTermValueView<GroundTag, FluentTag>, bool> merge_p2d(FunctionTermValueView<GroundTag, FluentTag> element,
                                                                                                MergeDatalogContext& context);

extern template std::pair<datalog::FunctionView<FluentTag>, bool> merge_p2d<AuxiliaryTag, FluentTag>(FunctionView<AuxiliaryTag> element,
                                                                                                     MergeDatalogContext& context);
extern template std::pair<datalog::FunctionTermView<LiftedTag, FluentTag>, bool>
merge_p2d<AuxiliaryTag, FluentTag>(FunctionTermView<LiftedTag, AuxiliaryTag> element, MergeDatalogContext& context);
extern template std::pair<datalog::FunctionBindingView<FluentTag>, bool> merge_p2d<AuxiliaryTag, FluentTag>(FunctionBindingView<AuxiliaryTag> element,
                                                                                                            MergeDatalogContext& context);
extern template std::pair<datalog::FunctionTermView<GroundTag, FluentTag>, bool>
merge_p2d<AuxiliaryTag, FluentTag>(FunctionTermView<GroundTag, AuxiliaryTag> element, MergeDatalogContext& context);
extern template std::pair<datalog::FunctionTermValueView<GroundTag, FluentTag>, bool>
merge_p2d<AuxiliaryTag, FluentTag>(FunctionTermValueView<GroundTag, AuxiliaryTag> element, MergeDatalogContext& context);

extern template std::pair<datalog::NumericEffectView<LiftedTag, FluentTag>, bool>
merge_p2d<FluentTag, FluentTag>(NumericEffectView<LiftedTag, FluentTag> element, MergeDatalogContext& context);
extern template std::pair<datalog::NumericEffectView<LiftedTag, FluentTag>, bool>
merge_p2d<AuxiliaryTag, FluentTag>(NumericEffectView<LiftedTag, AuxiliaryTag> element, MergeDatalogContext& context);
extern template ygg::Data<datalog::NumericEffectOperator<LiftedTag, FluentTag>>
merge_p2d<FluentTag, FluentTag>(NumericEffectOperatorView<LiftedTag, FluentTag> element, MergeDatalogContext& context);
extern template ygg::Data<datalog::NumericEffectOperator<LiftedTag, FluentTag>>
merge_p2d<AuxiliaryTag, FluentTag>(NumericEffectOperatorView<LiftedTag, AuxiliaryTag> element, MergeDatalogContext& context);

extern template std::pair<datalog::NumericEffectView<GroundTag, FluentTag>, bool>
merge_p2d<FluentTag, FluentTag>(NumericEffectView<GroundTag, FluentTag> element, MergeDatalogContext& context);
extern template std::pair<datalog::NumericEffectView<GroundTag, FluentTag>, bool>
merge_p2d<AuxiliaryTag, FluentTag>(NumericEffectView<GroundTag, AuxiliaryTag> element, MergeDatalogContext& context);
extern template ygg::Data<datalog::NumericEffectOperator<GroundTag, FluentTag>>
merge_p2d<FluentTag, FluentTag>(NumericEffectOperatorView<GroundTag, FluentTag> element, MergeDatalogContext& context);
extern template ygg::Data<datalog::NumericEffectOperator<GroundTag, FluentTag>>
merge_p2d<AuxiliaryTag, FluentTag>(NumericEffectOperatorView<GroundTag, AuxiliaryTag> element, MergeDatalogContext& context);

extern template std::pair<datalog::UnaryOperatorView<LiftedTag>, bool> merge_p2d(UnaryOperatorView<LiftedTag> element, MergeDatalogContext& context);
extern template std::pair<datalog::UnaryOperatorView<GroundTag>, bool> merge_p2d(UnaryOperatorView<GroundTag> element, MergeDatalogContext& context);

extern template std::pair<datalog::BinaryOperatorView<LiftedTag, BooleanOperatorKind>, bool>
merge_p2d(BinaryOperatorView<LiftedTag, BooleanOperatorKind> element, MergeDatalogContext& context);
extern template std::pair<datalog::BinaryOperatorView<LiftedTag, ArithmeticOperatorKind>, bool>
merge_p2d(BinaryOperatorView<LiftedTag, ArithmeticOperatorKind> element, MergeDatalogContext& context);
extern template std::pair<datalog::BinaryOperatorView<GroundTag, BooleanOperatorKind>, bool>
merge_p2d(BinaryOperatorView<GroundTag, BooleanOperatorKind> element, MergeDatalogContext& context);
extern template std::pair<datalog::BinaryOperatorView<GroundTag, ArithmeticOperatorKind>, bool>
merge_p2d(BinaryOperatorView<GroundTag, ArithmeticOperatorKind> element, MergeDatalogContext& context);

extern template std::pair<datalog::MultiOperatorView<LiftedTag>, bool> merge_p2d(MultiOperatorView<LiftedTag> element, MergeDatalogContext& context);
extern template std::pair<datalog::MultiOperatorView<GroundTag>, bool> merge_p2d(MultiOperatorView<GroundTag> element, MergeDatalogContext& context);

extern template ygg::Data<datalog::ArithmeticOperator<LiftedTag>> merge_p2d(ArithmeticOperatorView<LiftedTag> element, MergeDatalogContext& context);
extern template ygg::Data<datalog::ArithmeticOperator<GroundTag>> merge_p2d(ArithmeticOperatorView<GroundTag> element, MergeDatalogContext& context);

extern template ygg::Data<datalog::BooleanOperator<LiftedTag>> merge_p2d(BooleanOperatorView<LiftedTag> element, MergeDatalogContext& context);
extern template ygg::Data<datalog::BooleanOperator<GroundTag>> merge_p2d(BooleanOperatorView<GroundTag> element, MergeDatalogContext& context);
}

#endif

#endif
