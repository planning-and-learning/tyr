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

std::pair<::tyr::formalism::datalog::VariableView, bool> merge_p2d(VariableView element, MergeDatalogContext& context);

std::pair<::tyr::formalism::datalog::ObjectView, bool> merge_p2d(ObjectView element, MergeDatalogContext& context);

ygg::Data<Term> merge_p2d(TermView element, MergeDatalogContext& context);

// Propositional

template<FactKind T_SRC, FactKind T_DST = T_SRC>
std::pair<::tyr::formalism::datalog::PredicateView<T_DST>, bool> merge_p2d(PredicateView<T_SRC> element, MergeDatalogContext& context);

template<FactKind T_SRC, FactKind T_DST = T_SRC>
std::pair<::tyr::formalism::datalog::AtomView<::tyr::LiftedTag, T_DST>, bool>
merge_p2d(AtomView<::tyr::LiftedTag, T_SRC> element,  //
          const ygg::UnorderedMap<PredicateView<T_SRC>, ::tyr::formalism::datalog::PredicateView<T_DST>>& predicate_mapping,
          MergeDatalogContext& context);

template<FactKind T_SRC, FactKind T_DST = T_SRC>
std::pair<::tyr::formalism::datalog::PredicateBindingView<T_DST>, bool>
merge_p2d(PredicateBindingView<T_SRC> element,  //
          const ygg::UnorderedMap<PredicateView<T_SRC>, ::tyr::formalism::datalog::PredicateView<T_DST>>& predicate_mapping,
          MergeDatalogContext& context);

template<FactKind T_SRC, FactKind T_DST = T_SRC>
std::pair<::tyr::formalism::datalog::AtomView<::tyr::GroundTag, T_DST>, bool>
merge_p2d(AtomView<::tyr::GroundTag, T_SRC> element,  //
          const ygg::UnorderedMap<PredicateView<T_SRC>, ::tyr::formalism::datalog::PredicateView<T_DST>>& predicate_mapping,
          MergeDatalogContext& context);

template<FactKind T_SRC, FactKind T_DST = T_SRC>
std::pair<::tyr::formalism::datalog::AtomView<::tyr::GroundTag, T_DST>, bool>
merge_p2d(AtomView<::tyr::GroundTag, T_SRC> element,  //
          ygg::UnorderedMap<AtomView<::tyr::GroundTag, T_SRC>, ::tyr::formalism::datalog::AtomView<::tyr::GroundTag, T_DST>>& atom_mapping,
          const ygg::UnorderedMap<PredicateView<T_SRC>, ::tyr::formalism::datalog::PredicateView<T_DST>>& predicate_mapping,
          MergeDatalogContext& context);

template<FactKind T_SRC, FactKind T_DST = T_SRC>
std::pair<::tyr::formalism::datalog::LiteralView<::tyr::LiftedTag, T_DST>, bool>
merge_p2d(LiteralView<::tyr::LiftedTag, T_SRC> element,  //
          const ygg::UnorderedMap<PredicateView<T_SRC>, ::tyr::formalism::datalog::PredicateView<T_DST>>& predicate_mapping,
          MergeDatalogContext& context);

template<FactKind T_SRC, FactKind T_DST = T_SRC>
std::pair<::tyr::formalism::datalog::LiteralView<::tyr::GroundTag, T_DST>, bool>
merge_p2d(LiteralView<::tyr::GroundTag, T_SRC> element,  //
          const ygg::UnorderedMap<PredicateView<T_SRC>, ::tyr::formalism::datalog::PredicateView<T_DST>>& predicate_mapping,
          MergeDatalogContext& context);

template<FactKind T_SRC, FactKind T_DST = T_SRC>
std::pair<::tyr::formalism::datalog::LiteralView<::tyr::GroundTag, T_DST>, bool>
merge_p2d(LiteralView<::tyr::GroundTag, T_SRC> element,  //
          ygg::UnorderedMap<AtomView<::tyr::GroundTag, T_SRC>, ::tyr::formalism::datalog::AtomView<::tyr::GroundTag, T_DST>>& atom_mapping,
          const ygg::UnorderedMap<PredicateView<T_SRC>, ::tyr::formalism::datalog::PredicateView<T_DST>>& predicate_mapping,
          MergeDatalogContext& context);

std::optional<::tyr::formalism::datalog::LiteralView<::tyr::GroundTag, FluentTag>>
merge_p2d(FDRFactView<FluentTag> element,
          bool polarity,
          const ygg::UnorderedMap<PredicateView<FluentTag>, ::tyr::formalism::datalog::PredicateView<FluentTag>>& predicate_mapping,
          MergeDatalogContext& context);

std::optional<::tyr::formalism::datalog::LiteralView<::tyr::GroundTag, FluentTag>>
merge_p2d(FDRFactView<FluentTag> element,
          bool polarity,
          ygg::UnorderedMap<AtomView<::tyr::GroundTag, FluentTag>, ::tyr::formalism::datalog::AtomView<::tyr::GroundTag, FluentTag>>& atom_mapping,
          const ygg::UnorderedMap<PredicateView<FluentTag>, ::tyr::formalism::datalog::PredicateView<FluentTag>>& predicate_mapping,
          MergeDatalogContext& context);

std::pair<::tyr::formalism::datalog::ConjunctiveConditionView<::tyr::GroundTag>, bool>
merge_p2d(ConjunctiveConditionView<::tyr::GroundTag> element,
          const ygg::UnorderedMap<PredicateView<FluentTag>, ::tyr::formalism::datalog::PredicateView<FluentTag>>& fluent_predicate_mapping,
          const ygg::UnorderedMap<PredicateView<DerivedTag>, ::tyr::formalism::datalog::PredicateView<FluentTag>>& derived_predicate_mapping,
          MergeDatalogContext& context);

std::pair<::tyr::formalism::datalog::ConjunctiveConditionView<::tyr::GroundTag>, bool>
merge_p2d(ConjunctiveConditionView<::tyr::GroundTag> element,
          ygg::UnorderedMap<AtomView<::tyr::GroundTag, FluentTag>, ::tyr::formalism::datalog::AtomView<::tyr::GroundTag, FluentTag>>& fluent_atom_mapping,
          const ygg::UnorderedMap<PredicateView<FluentTag>, ::tyr::formalism::datalog::PredicateView<FluentTag>>& fluent_predicate_mapping,
          ygg::UnorderedMap<AtomView<::tyr::GroundTag, DerivedTag>, ::tyr::formalism::datalog::AtomView<::tyr::GroundTag, FluentTag>>& derived_atom_mapping,
          const ygg::UnorderedMap<PredicateView<DerivedTag>, ::tyr::formalism::datalog::PredicateView<FluentTag>>& derived_predicate_mapping,
          MergeDatalogContext& context);

std::pair<::tyr::formalism::datalog::ConjunctiveConditionView<::tyr::GroundTag>, bool>
merge_p2d(ConjunctiveConditionView<::tyr::GroundTag> element,
          ygg::UnorderedMap<AtomView<::tyr::GroundTag, FluentTag>, ::tyr::formalism::datalog::AtomView<::tyr::GroundTag, FluentTag>>& fluent_atom_mapping,
          const ygg::UnorderedMap<PredicateView<FluentTag>, ::tyr::formalism::datalog::PredicateView<FluentTag>>& fluent_predicate_mapping,
          MergeDatalogContext& context);

// Numeric

template<FactKind T_SRC, FactKind T_DST = T_SRC>
std::pair<::tyr::formalism::datalog::FunctionView<T_DST>, bool> merge_p2d(FunctionView<T_SRC> element, MergeDatalogContext& context);

template<FactKind T_SRC, FactKind T_DST = T_SRC>
std::pair<::tyr::formalism::datalog::FunctionTermView<::tyr::LiftedTag, T_DST>, bool> merge_p2d(FunctionTermView<::tyr::LiftedTag, T_SRC> element,
                                                                                                MergeDatalogContext& context);

template<FactKind T_SRC, FactKind T_DST = T_SRC>
std::pair<::tyr::formalism::datalog::FunctionBindingView<T_DST>, bool> merge_p2d(FunctionBindingView<T_SRC> element, MergeDatalogContext& context);

template<FactKind T_SRC, FactKind T_DST = T_SRC>
std::pair<::tyr::formalism::datalog::FunctionTermView<::tyr::GroundTag, T_DST>, bool> merge_p2d(FunctionTermView<::tyr::GroundTag, T_SRC> element,
                                                                                                MergeDatalogContext& context);

template<FactKind T_SRC, FactKind T_DST = T_SRC>
std::pair<::tyr::formalism::datalog::FunctionTermValueView<::tyr::GroundTag, T_DST>, bool> merge_p2d(FunctionTermValueView<::tyr::GroundTag, T_SRC> element,
                                                                                                     MergeDatalogContext& context);

template<FactKind T_SRC,
         FactKind T_DST = T_SRC,
         typename = std::enable_if_t<(std::same_as<T_SRC, FluentTag> || std::same_as<T_SRC, AuxiliaryTag>) && std::same_as<T_DST, FluentTag>>>
std::pair<::tyr::formalism::datalog::NumericEffectView<::tyr::LiftedTag, T_DST>, bool> merge_p2d(NumericEffectView<::tyr::LiftedTag, T_SRC> element,
                                                                                                 MergeDatalogContext& context);

template<FactKind T_SRC,
         FactKind T_DST = T_SRC,
         typename = std::enable_if_t<(std::same_as<T_SRC, FluentTag> || std::same_as<T_SRC, AuxiliaryTag>) && std::same_as<T_DST, FluentTag>>>
ygg::Data<::tyr::formalism::datalog::NumericEffectOperator<::tyr::LiftedTag, T_DST>> merge_p2d(NumericEffectOperatorView<::tyr::LiftedTag, T_SRC> element,
                                                                                               MergeDatalogContext& context);

ygg::Data<::tyr::formalism::datalog::FunctionExpression<::tyr::LiftedTag>> merge_p2d(FunctionExpressionView<::tyr::LiftedTag> element,
                                                                                     MergeDatalogContext& context);

ygg::Data<::tyr::formalism::datalog::FunctionExpression<::tyr::GroundTag>> merge_p2d(FunctionExpressionView<::tyr::GroundTag> element,
                                                                                     MergeDatalogContext& context);

std::pair<::tyr::formalism::datalog::MetricView, bool> merge_p2d(MetricView element, MergeDatalogContext& context);

template<FactKind T_SRC,
         FactKind T_DST = T_SRC,
         typename = std::enable_if_t<(std::same_as<T_SRC, FluentTag> || std::same_as<T_SRC, AuxiliaryTag>) && std::same_as<T_DST, FluentTag>>>
std::pair<::tyr::formalism::datalog::NumericEffectView<::tyr::GroundTag, T_DST>, bool> merge_p2d(NumericEffectView<::tyr::GroundTag, T_SRC> element,
                                                                                                 MergeDatalogContext& context);

template<FactKind T_SRC,
         FactKind T_DST = T_SRC,
         typename = std::enable_if_t<(std::same_as<T_SRC, FluentTag> || std::same_as<T_SRC, AuxiliaryTag>) && std::same_as<T_DST, FluentTag>>>
ygg::Data<::tyr::formalism::datalog::NumericEffectOperator<::tyr::GroundTag, T_DST>> merge_p2d(NumericEffectOperatorView<::tyr::GroundTag, T_SRC> element,
                                                                                               MergeDatalogContext& context);

template<typename T>
std::pair<::tyr::formalism::datalog::UnaryOperatorView<to_datalog_payload_t<T>>, bool> merge_p2d(UnaryOperatorView<T> element, MergeDatalogContext& context);

template<BinaryOperatorKind O, typename T>
std::pair<::tyr::formalism::datalog::BinaryOperatorView<O, to_datalog_payload_t<T>>, bool> merge_p2d(BinaryOperatorView<O, T> element,
                                                                                                     MergeDatalogContext& context);

template<typename T>
std::pair<::tyr::formalism::datalog::MultiOperatorView<to_datalog_payload_t<T>>, bool> merge_p2d(MultiOperatorView<T> element, MergeDatalogContext& context);

template<typename T>
ygg::Data<::tyr::formalism::datalog::ArithmeticOperator<to_datalog_payload_t<T>>> merge_p2d(ArithmeticOperatorView<T> element, MergeDatalogContext& context);

template<typename T>
ygg::Data<::tyr::formalism::datalog::BooleanOperator<to_datalog_payload_t<T>>> merge_p2d(BooleanOperatorView<T> element, MergeDatalogContext& context);

// Common

inline std::pair<::tyr::formalism::datalog::VariableView, bool> merge_p2d(VariableView element, MergeDatalogContext& context)
{
    auto variable = ::tyr::formalism::datalog::checkout<Variable>(context.builder);

    variable->name = element.get_name();

    return ::tyr::formalism::datalog::get_or_create(context.destination, *variable);
}

inline std::pair<::tyr::formalism::datalog::ObjectView, bool> merge_p2d(ObjectView element, MergeDatalogContext& context)
{
    auto object = ::tyr::formalism::datalog::checkout<Object>(context.builder);

    object->name = element.get_name();

    return ::tyr::formalism::datalog::get_or_create(context.destination, *object);
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
std::pair<::tyr::formalism::datalog::PredicateView<T_DST>, bool> merge_p2d(PredicateView<T_SRC> element, MergeDatalogContext& context)
{
    auto predicate = ::tyr::formalism::datalog::checkout<Predicate<T_DST>>(context.builder);

    predicate->name = element.get_name();
    predicate->arity = element.get_arity();

    return ::tyr::formalism::datalog::get_or_create(context.destination, *predicate);
}

template<FactKind T_SRC, FactKind T_DST>
std::pair<::tyr::formalism::datalog::AtomView<::tyr::LiftedTag, T_DST>, bool>
merge_p2d(AtomView<::tyr::LiftedTag, T_SRC> element,  //
          const ygg::UnorderedMap<PredicateView<T_SRC>, ::tyr::formalism::datalog::PredicateView<T_DST>>& predicate_mapping,
          MergeDatalogContext& context)
{
    auto atom = ::tyr::formalism::datalog::checkout<::tyr::formalism::datalog::Atom<::tyr::LiftedTag, T_DST>>(context.builder);

    atom->predicate = predicate_mapping.at(element.get_predicate()).get_index();
    for (const auto term : element.get_terms())
        atom->terms.push_back(merge_p2d(term, context));

    return ::tyr::formalism::datalog::get_or_create(context.destination, *atom);
}

template<FactKind T_SRC, FactKind T_DST>
std::pair<::tyr::formalism::datalog::PredicateBindingView<T_DST>, bool>
merge_p2d(PredicateBindingView<T_SRC> element,  //
          const ygg::UnorderedMap<PredicateView<T_SRC>, ::tyr::formalism::datalog::PredicateView<T_DST>>& predicate_mapping,
          MergeDatalogContext& context)
{
    auto binding = ::tyr::formalism::datalog::checkout<RelationBinding<Predicate<T_DST>>>(context.builder);

    binding->relation = predicate_mapping.at(element.get_relation()).get_index();
    for (const auto object : element.get_objects())
        binding->objects.push_back(object.get_index());

    return ::tyr::formalism::datalog::get_or_create(context.destination, *binding);
}

template<FactKind T_SRC, FactKind T_DST>
std::pair<::tyr::formalism::datalog::AtomView<::tyr::GroundTag, T_DST>, bool>
merge_p2d(AtomView<::tyr::GroundTag, T_SRC> element,  //
          const ygg::UnorderedMap<PredicateView<T_SRC>, ::tyr::formalism::datalog::PredicateView<T_DST>>& predicate_mapping,
          MergeDatalogContext& context)
{
    auto atom = ::tyr::formalism::datalog::checkout<::tyr::formalism::datalog::Atom<::tyr::GroundTag, T_DST>>(context.builder);

    atom->binding = merge_p2d<T_SRC, T_DST>(element.get_row(), predicate_mapping, context).first.get_index();

    return ::tyr::formalism::datalog::get_or_create(context.destination, *atom);
}

template<FactKind T_SRC, FactKind T_DST>
std::pair<::tyr::formalism::datalog::AtomView<::tyr::GroundTag, T_DST>, bool>
merge_p2d(AtomView<::tyr::GroundTag, T_SRC> element,  //
          ygg::UnorderedMap<AtomView<::tyr::GroundTag, T_SRC>, ::tyr::formalism::datalog::AtomView<::tyr::GroundTag, T_DST>>& atom_mapping,
          const ygg::UnorderedMap<PredicateView<T_SRC>, ::tyr::formalism::datalog::PredicateView<T_DST>>& predicate_mapping,
          MergeDatalogContext& context)
{
    if (const auto it = atom_mapping.find(element); it != atom_mapping.end())
        return std::make_pair(it->second, false);

    auto [atom, inserted] = merge_p2d<T_SRC, T_DST>(element, predicate_mapping, context);
    atom_mapping.emplace(element, atom);
    return std::make_pair(atom, inserted);
}

template<FactKind T_SRC, FactKind T_DST>
std::pair<::tyr::formalism::datalog::LiteralView<::tyr::LiftedTag, T_DST>, bool>
merge_p2d(LiteralView<::tyr::LiftedTag, T_SRC> element,  //
          const ygg::UnorderedMap<PredicateView<T_SRC>, ::tyr::formalism::datalog::PredicateView<T_DST>>& predicate_mapping,
          MergeDatalogContext& context)
{
    auto literal = ::tyr::formalism::datalog::checkout<::tyr::formalism::datalog::Literal<::tyr::LiftedTag, T_DST>>(context.builder);

    literal->polarity = element.get_polarity();
    literal->atom = merge_p2d<T_SRC, T_DST>(element.get_atom(), predicate_mapping, context).first.get_index();

    return ::tyr::formalism::datalog::get_or_create(context.destination, *literal);
}

template<FactKind T_SRC, FactKind T_DST>
std::pair<::tyr::formalism::datalog::LiteralView<::tyr::GroundTag, T_DST>, bool>
merge_p2d(LiteralView<::tyr::GroundTag, T_SRC> element,  //
          const ygg::UnorderedMap<PredicateView<T_SRC>, ::tyr::formalism::datalog::PredicateView<T_DST>>& predicate_mapping,
          MergeDatalogContext& context)
{
    auto literal = ::tyr::formalism::datalog::checkout<::tyr::formalism::datalog::Literal<::tyr::GroundTag, T_DST>>(context.builder);

    literal->polarity = element.get_polarity();
    literal->atom = merge_p2d<T_SRC, T_DST>(element.get_atom(), predicate_mapping, context).first.get_index();

    return ::tyr::formalism::datalog::get_or_create(context.destination, *literal);
}

template<FactKind T_SRC, FactKind T_DST>
std::pair<::tyr::formalism::datalog::LiteralView<::tyr::GroundTag, T_DST>, bool>
merge_p2d(LiteralView<::tyr::GroundTag, T_SRC> element,  //
          ygg::UnorderedMap<AtomView<::tyr::GroundTag, T_SRC>, ::tyr::formalism::datalog::AtomView<::tyr::GroundTag, T_DST>>& atom_mapping,
          const ygg::UnorderedMap<PredicateView<T_SRC>, ::tyr::formalism::datalog::PredicateView<T_DST>>& predicate_mapping,
          MergeDatalogContext& context)
{
    auto literal = ::tyr::formalism::datalog::checkout<::tyr::formalism::datalog::Literal<::tyr::GroundTag, T_DST>>(context.builder);

    literal->polarity = element.get_polarity();
    literal->atom = merge_p2d<T_SRC, T_DST>(element.get_atom(), atom_mapping, predicate_mapping, context).first.get_index();

    return ::tyr::formalism::datalog::get_or_create(context.destination, *literal);
}

inline std::optional<::tyr::formalism::datalog::LiteralView<::tyr::GroundTag, FluentTag>>
merge_p2d(FDRFactView<FluentTag> element,
          bool polarity,
          const ygg::UnorderedMap<PredicateView<FluentTag>, ::tyr::formalism::datalog::PredicateView<FluentTag>>& predicate_mapping,
          MergeDatalogContext& context)
{
    if (!element.has_value())
        return std::nullopt;

    auto literal = ::tyr::formalism::datalog::checkout<::tyr::formalism::datalog::Literal<::tyr::GroundTag, FluentTag>>(context.builder);
    literal->polarity = polarity;
    literal->atom = merge_p2d(element.get_atom().value(), predicate_mapping, context).first.get_index();

    return ::tyr::formalism::datalog::get_or_create(context.destination, *literal).first;
}

inline std::optional<::tyr::formalism::datalog::LiteralView<::tyr::GroundTag, FluentTag>>
merge_p2d(FDRFactView<FluentTag> element,
          bool polarity,
          ygg::UnorderedMap<AtomView<::tyr::GroundTag, FluentTag>, ::tyr::formalism::datalog::AtomView<::tyr::GroundTag, FluentTag>>& atom_mapping,
          const ygg::UnorderedMap<PredicateView<FluentTag>, ::tyr::formalism::datalog::PredicateView<FluentTag>>& predicate_mapping,
          MergeDatalogContext& context)
{
    if (!element.has_value())
        return std::nullopt;

    auto literal = ::tyr::formalism::datalog::checkout<::tyr::formalism::datalog::Literal<::tyr::GroundTag, FluentTag>>(context.builder);
    literal->polarity = polarity;
    literal->atom = merge_p2d(element.get_atom().value(), atom_mapping, predicate_mapping, context).first.get_index();

    return ::tyr::formalism::datalog::get_or_create(context.destination, *literal).first;
}

inline std::pair<::tyr::formalism::datalog::ConjunctiveConditionView<::tyr::GroundTag>, bool>
merge_p2d(ConjunctiveConditionView<::tyr::GroundTag> element,
          const ygg::UnorderedMap<PredicateView<FluentTag>, ::tyr::formalism::datalog::PredicateView<FluentTag>>& fluent_predicate_mapping,
          const ygg::UnorderedMap<PredicateView<DerivedTag>, ::tyr::formalism::datalog::PredicateView<FluentTag>>& derived_predicate_mapping,
          MergeDatalogContext& context)
{
    auto condition = ::tyr::formalism::datalog::checkout<::tyr::formalism::datalog::ConjunctiveCondition<::tyr::GroundTag>>(context.builder);

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

    return ::tyr::formalism::datalog::get_or_create(context.destination, *condition);
}

inline std::pair<::tyr::formalism::datalog::ConjunctiveConditionView<::tyr::GroundTag>, bool>
merge_p2d(ConjunctiveConditionView<::tyr::GroundTag> element,
          ygg::UnorderedMap<AtomView<::tyr::GroundTag, FluentTag>, ::tyr::formalism::datalog::AtomView<::tyr::GroundTag, FluentTag>>& fluent_atom_mapping,
          const ygg::UnorderedMap<PredicateView<FluentTag>, ::tyr::formalism::datalog::PredicateView<FluentTag>>& fluent_predicate_mapping,
          ygg::UnorderedMap<AtomView<::tyr::GroundTag, DerivedTag>, ::tyr::formalism::datalog::AtomView<::tyr::GroundTag, FluentTag>>& derived_atom_mapping,
          const ygg::UnorderedMap<PredicateView<DerivedTag>, ::tyr::formalism::datalog::PredicateView<FluentTag>>& derived_predicate_mapping,
          MergeDatalogContext& context)
{
    auto condition = ::tyr::formalism::datalog::checkout<::tyr::formalism::datalog::ConjunctiveCondition<::tyr::GroundTag>>(context.builder);

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

    return ::tyr::formalism::datalog::get_or_create(context.destination, *condition);
}

inline std::pair<::tyr::formalism::datalog::ConjunctiveConditionView<::tyr::GroundTag>, bool>
merge_p2d(ConjunctiveConditionView<::tyr::GroundTag> element,
          ygg::UnorderedMap<AtomView<::tyr::GroundTag, FluentTag>, ::tyr::formalism::datalog::AtomView<::tyr::GroundTag, FluentTag>>& fluent_atom_mapping,
          const ygg::UnorderedMap<PredicateView<FluentTag>, ::tyr::formalism::datalog::PredicateView<FluentTag>>& fluent_predicate_mapping,
          MergeDatalogContext& context)
{
    auto condition = ::tyr::formalism::datalog::checkout<::tyr::formalism::datalog::ConjunctiveCondition<::tyr::GroundTag>>(context.builder);

    for (const auto fact : element.template get_facts<PositiveTag>())
        if (const auto literal = merge_p2d(fact, true, fluent_atom_mapping, fluent_predicate_mapping, context))
            condition->fluent_literals.push_back(literal->get_index());

    for (const auto fact : element.template get_facts<NegativeTag>())
        if (const auto literal = merge_p2d(fact, false, fluent_atom_mapping, fluent_predicate_mapping, context))
            condition->fluent_literals.push_back(literal->get_index());

    for (const auto numeric_constraint : element.get_numeric_constraints())
        condition->numeric_constraints.push_back(merge_p2d(numeric_constraint, context));

    return ::tyr::formalism::datalog::get_or_create(context.destination, *condition);
}

// Numeric

template<FactKind T_SRC, FactKind T_DST>
std::pair<::tyr::formalism::datalog::FunctionView<T_DST>, bool> merge_p2d(FunctionView<T_SRC> element, MergeDatalogContext& context)
{
    auto function = ::tyr::formalism::datalog::checkout<::tyr::formalism::Function<T_DST>>(context.builder);

    function->name = element.get_name();
    function->arity = element.get_arity();

    return ::tyr::formalism::datalog::get_or_create(context.destination, *function);
}

template<FactKind T_SRC, FactKind T_DST>
std::pair<::tyr::formalism::datalog::FunctionTermView<::tyr::LiftedTag, T_DST>, bool> merge_p2d(FunctionTermView<::tyr::LiftedTag, T_SRC> element,
                                                                                                MergeDatalogContext& context)
{
    auto fterm = ::tyr::formalism::datalog::checkout<::tyr::formalism::datalog::FunctionTerm<::tyr::LiftedTag, T_DST>>(context.builder);

    fterm->function = merge_p2d<T_SRC, T_DST>(element.get_function(), context).first.get_index();
    for (const auto term : element.get_terms())
        fterm->terms.push_back(merge_p2d(term, context));

    return ::tyr::formalism::datalog::get_or_create(context.destination, *fterm);
}

template<FactKind T_SRC, FactKind T_DST>
std::pair<::tyr::formalism::datalog::FunctionBindingView<T_DST>, bool> merge_p2d(FunctionBindingView<T_SRC> element, MergeDatalogContext& context)
{
    auto binding = ::tyr::formalism::datalog::checkout<RelationBinding<Function<T_DST>>>(context.builder);

    binding->relation = merge_p2d<T_SRC, T_DST>(element.get_relation(), context).first.get_index();
    for (const auto object : element.get_objects())
        binding->objects.push_back(object.get_index());

    return ::tyr::formalism::datalog::get_or_create(context.destination, *binding);
}

template<FactKind T_SRC, FactKind T_DST>
std::pair<::tyr::formalism::datalog::FunctionTermView<::tyr::GroundTag, T_DST>, bool> merge_p2d(FunctionTermView<::tyr::GroundTag, T_SRC> element,
                                                                                                MergeDatalogContext& context)
{
    auto fterm = ::tyr::formalism::datalog::checkout<::tyr::formalism::datalog::FunctionTerm<::tyr::GroundTag, T_DST>>(context.builder);

    fterm->binding = merge_p2d<T_SRC, T_DST>(element.get_row(), context).first.get_index();

    return ::tyr::formalism::datalog::get_or_create(context.destination, *fterm);
}

template<FactKind T_SRC, FactKind T_DST>
std::pair<::tyr::formalism::datalog::FunctionTermValueView<::tyr::GroundTag, T_DST>, bool> merge_p2d(FunctionTermValueView<::tyr::GroundTag, T_SRC> element,
                                                                                                     MergeDatalogContext& context)
{
    auto fterm_value = ::tyr::formalism::datalog::checkout<::tyr::formalism::datalog::FunctionTermValue<::tyr::GroundTag, T_DST>>(context.builder);

    fterm_value->fterm = merge_p2d<T_SRC, T_DST>(element.get_fterm(), context).first.get_index();
    fterm_value->value = element.get_value();

    return ::tyr::formalism::datalog::get_or_create(context.destination, *fterm_value);
}

template<FactKind T_SRC, FactKind T_DST, typename>
std::pair<::tyr::formalism::datalog::NumericEffectView<::tyr::LiftedTag, T_DST>, bool> merge_p2d(NumericEffectView<::tyr::LiftedTag, T_SRC> element,
                                                                                                 MergeDatalogContext& context)
{
    auto numeric_effect = ::tyr::formalism::datalog::checkout<::tyr::formalism::datalog::NumericEffect<::tyr::LiftedTag, T_DST>>(context.builder);

    numeric_effect->operator_kind = element.get_operator();
    numeric_effect->fterm = merge_p2d<T_SRC, T_DST>(element.get_fterm(), context).first.get_index();
    numeric_effect->fexpr = merge_p2d(element.get_fexpr(), context);

    return ::tyr::formalism::datalog::get_or_create(context.destination, *numeric_effect);
}

template<FactKind T_SRC, FactKind T_DST, typename>
ygg::Data<::tyr::formalism::datalog::NumericEffectOperator<::tyr::LiftedTag, T_DST>> merge_p2d(NumericEffectOperatorView<::tyr::LiftedTag, T_SRC> element,
                                                                                               MergeDatalogContext& context)
{
    using OperatorData = ygg::Data<::tyr::formalism::datalog::NumericEffectOperator<::tyr::LiftedTag, T_DST>>;

    return visit([&](auto&& arg)
                 { return OperatorData(arg.get_operator(), typename OperatorData::Variant(merge_p2d<T_SRC, T_DST>(arg, context).first.get_index())); },
                 element.get_variant());
}

template<FactKind T_SRC, FactKind T_DST, typename>
std::pair<::tyr::formalism::datalog::NumericEffectView<::tyr::GroundTag, T_DST>, bool> merge_p2d(NumericEffectView<::tyr::GroundTag, T_SRC> element,
                                                                                                 MergeDatalogContext& context)
{
    auto numeric_effect = ::tyr::formalism::datalog::checkout<::tyr::formalism::datalog::NumericEffect<::tyr::GroundTag, T_DST>>(context.builder);

    numeric_effect->operator_kind = element.get_operator();
    numeric_effect->fterm = merge_p2d<T_SRC, T_DST>(element.get_fterm(), context).first.get_index();
    numeric_effect->fexpr = merge_p2d(element.get_fexpr(), context);

    return ::tyr::formalism::datalog::get_or_create(context.destination, *numeric_effect);
}

template<FactKind T_SRC, FactKind T_DST, typename>
ygg::Data<::tyr::formalism::datalog::NumericEffectOperator<::tyr::GroundTag, T_DST>> merge_p2d(NumericEffectOperatorView<::tyr::GroundTag, T_SRC> element,
                                                                                               MergeDatalogContext& context)
{
    using OperatorData = ygg::Data<::tyr::formalism::datalog::NumericEffectOperator<::tyr::GroundTag, T_DST>>;

    return visit([&](auto&& arg)
                 { return OperatorData(arg.get_operator(), typename OperatorData::Variant(merge_p2d<T_SRC, T_DST>(arg, context).first.get_index())); },
                 element.get_variant());
}

inline ygg::Data<::tyr::formalism::datalog::FunctionExpression<::tyr::LiftedTag>> merge_p2d(FunctionExpressionView<::tyr::LiftedTag> element,
                                                                                            MergeDatalogContext& context)
{
    return visit(
        [&](auto&& arg) -> ygg::Data<::tyr::formalism::datalog::FunctionExpression<::tyr::LiftedTag>>
        {
            using Alternative = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<Alternative, ygg::float_t>)
                return ygg::Data<::tyr::formalism::datalog::FunctionExpression<::tyr::LiftedTag>>(arg);
            else if constexpr (std::is_same_v<Alternative, LiftedArithmeticOperatorView>)
                return ygg::Data<::tyr::formalism::datalog::FunctionExpression<::tyr::LiftedTag>>(merge_p2d(arg, context));
            else if constexpr (std::is_same_v<Alternative, FunctionTermView<::tyr::LiftedTag, AuxiliaryTag>>)
                return ygg::Data<::tyr::formalism::datalog::FunctionExpression<::tyr::LiftedTag>>(
                    merge_p2d<AuxiliaryTag, FluentTag>(arg, context).first.get_index());
            else
                return ygg::Data<::tyr::formalism::datalog::FunctionExpression<::tyr::LiftedTag>>(merge_p2d(arg, context).first.get_index());
        },
        element.get_variant());
}

inline ygg::Data<::tyr::formalism::datalog::FunctionExpression<::tyr::GroundTag>> merge_p2d(FunctionExpressionView<::tyr::GroundTag> element,
                                                                                            MergeDatalogContext& context)
{
    return visit(
        [&](auto&& arg) -> ygg::Data<::tyr::formalism::datalog::FunctionExpression<::tyr::GroundTag>>
        {
            using Alternative = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<Alternative, ygg::float_t>)
                return ygg::Data<::tyr::formalism::datalog::FunctionExpression<::tyr::GroundTag>>(arg);
            else if constexpr (std::is_same_v<Alternative, GroundArithmeticOperatorView>)
                return ygg::Data<::tyr::formalism::datalog::FunctionExpression<::tyr::GroundTag>>(merge_p2d(arg, context));
            else if constexpr (std::is_same_v<Alternative, FunctionTermView<::tyr::GroundTag, AuxiliaryTag>>)
                return ygg::Data<::tyr::formalism::datalog::FunctionExpression<::tyr::GroundTag>>(
                    merge_p2d<AuxiliaryTag, FluentTag>(arg, context).first.get_index());
            else
                return ygg::Data<::tyr::formalism::datalog::FunctionExpression<::tyr::GroundTag>>(merge_p2d(arg, context).first.get_index());
        },
        element.get_variant());
}

inline std::pair<::tyr::formalism::datalog::MetricView, bool> merge_p2d(MetricView element, MergeDatalogContext& context)
{
    auto metric = ::tyr::formalism::datalog::checkout<::tyr::formalism::datalog::Metric>(context.builder);
    metric->fexpr = merge_p2d(element.get_fexpr(), context);
    return ::tyr::formalism::datalog::get_or_create(context.destination, *metric);
}

template<typename T>
std::pair<::tyr::formalism::datalog::UnaryOperatorView<to_datalog_payload_t<T>>, bool> merge_p2d(UnaryOperatorView<T> element, MergeDatalogContext& context)
{
    using T_DST = to_datalog_payload_t<T>;

    auto unary = ::tyr::formalism::datalog::checkout<::tyr::formalism::datalog::UnaryOperator<T_DST>>(context.builder);

    unary->operator_kind = element.get_operator();
    unary->arg = merge_p2d(element.get_arg(), context);

    return ::tyr::formalism::datalog::get_or_create(context.destination, *unary);
}

template<BinaryOperatorKind O, typename T>
std::pair<::tyr::formalism::datalog::BinaryOperatorView<O, to_datalog_payload_t<T>>, bool> merge_p2d(BinaryOperatorView<O, T> element,
                                                                                                     MergeDatalogContext& context)
{
    using T_DST = to_datalog_payload_t<T>;

    auto binary = ::tyr::formalism::datalog::checkout<::tyr::formalism::datalog::BinaryOperator<O, T_DST>>(context.builder);

    binary->operator_kind = element.get_operator();
    binary->lhs = merge_p2d(element.get_lhs(), context);
    binary->rhs = merge_p2d(element.get_rhs(), context);

    return ::tyr::formalism::datalog::get_or_create(context.destination, *binary);
}

template<typename T>
std::pair<::tyr::formalism::datalog::MultiOperatorView<to_datalog_payload_t<T>>, bool> merge_p2d(MultiOperatorView<T> element, MergeDatalogContext& context)
{
    using T_DST = to_datalog_payload_t<T>;

    auto multi = ::tyr::formalism::datalog::checkout<::tyr::formalism::datalog::MultiOperator<T_DST>>(context.builder);

    multi->operator_kind = element.get_operator();
    for (const auto arg : element.get_args())
        multi->args.push_back(merge_p2d(arg, context));

    return ::tyr::formalism::datalog::get_or_create(context.destination, *multi);
}

template<typename T>
ygg::Data<::tyr::formalism::datalog::ArithmeticOperator<to_datalog_payload_t<T>>> merge_p2d(ArithmeticOperatorView<T> element, MergeDatalogContext& context)
{
    using T_DST = to_datalog_payload_t<T>;

    return visit([&](auto&& arg)
                 { return ygg::Data<::tyr::formalism::datalog::ArithmeticOperator<T_DST>>(arg.get_operator(), merge_p2d(arg, context).first.get_index()); },
                 element.get_variant());
}

template<typename T>
ygg::Data<::tyr::formalism::datalog::BooleanOperator<to_datalog_payload_t<T>>> merge_p2d(BooleanOperatorView<T> element, MergeDatalogContext& context)
{
    using T_DST = to_datalog_payload_t<T>;

    return visit([&](auto&& arg)
                 { return ygg::Data<::tyr::formalism::datalog::BooleanOperator<T_DST>>(arg.get_operator(), merge_p2d(arg, context).first.get_index()); },
                 element.get_variant());
}

}

#ifndef TYR_HEADER_INSTANTIATION

namespace tyr::formalism::planning
{
extern template std::pair<::tyr::formalism::datalog::PredicateView<StaticTag>, bool> merge_p2d(PredicateView<StaticTag> element, MergeDatalogContext& context);
extern template std::pair<::tyr::formalism::datalog::PredicateView<FluentTag>, bool> merge_p2d(PredicateView<FluentTag> element, MergeDatalogContext& context);
extern template std::pair<::tyr::formalism::datalog::PredicateView<FluentTag>, bool> merge_p2d(PredicateView<DerivedTag> element, MergeDatalogContext& context);

extern template std::pair<::tyr::formalism::datalog::AtomView<::tyr::LiftedTag, StaticTag>, bool>
merge_p2d(AtomView<::tyr::LiftedTag, StaticTag> element,
          const ygg::UnorderedMap<PredicateView<StaticTag>, ::tyr::formalism::datalog::PredicateView<StaticTag>>& predicate_mapping,
          MergeDatalogContext& context);
extern template std::pair<::tyr::formalism::datalog::AtomView<::tyr::LiftedTag, FluentTag>, bool>
merge_p2d(AtomView<::tyr::LiftedTag, FluentTag> element,
          const ygg::UnorderedMap<PredicateView<FluentTag>, ::tyr::formalism::datalog::PredicateView<FluentTag>>& predicate_mapping,
          MergeDatalogContext& context);
extern template std::pair<::tyr::formalism::datalog::AtomView<::tyr::LiftedTag, FluentTag>, bool>
merge_p2d(AtomView<::tyr::LiftedTag, DerivedTag> element,
          const ygg::UnorderedMap<PredicateView<DerivedTag>, ::tyr::formalism::datalog::PredicateView<FluentTag>>& predicate_mapping,
          MergeDatalogContext& context);
extern template std::pair<::tyr::formalism::datalog::PredicateBindingView<StaticTag>, bool>
merge_p2d(PredicateBindingView<StaticTag> element,
          const ygg::UnorderedMap<PredicateView<StaticTag>, ::tyr::formalism::datalog::PredicateView<StaticTag>>& predicate_mapping,
          MergeDatalogContext& context);
extern template std::pair<::tyr::formalism::datalog::PredicateBindingView<FluentTag>, bool>
merge_p2d(PredicateBindingView<FluentTag> element,
          const ygg::UnorderedMap<PredicateView<FluentTag>, ::tyr::formalism::datalog::PredicateView<FluentTag>>& predicate_mapping,
          MergeDatalogContext& context);
extern template std::pair<::tyr::formalism::datalog::PredicateBindingView<FluentTag>, bool>
merge_p2d(PredicateBindingView<DerivedTag> element,
          const ygg::UnorderedMap<PredicateView<DerivedTag>, ::tyr::formalism::datalog::PredicateView<FluentTag>>& predicate_mapping,
          MergeDatalogContext& context);

extern template std::pair<::tyr::formalism::datalog::AtomView<::tyr::GroundTag, StaticTag>, bool>
merge_p2d(AtomView<::tyr::GroundTag, StaticTag> element,
          const ygg::UnorderedMap<PredicateView<StaticTag>, ::tyr::formalism::datalog::PredicateView<StaticTag>>& predicate_mapping,
          MergeDatalogContext& context);
extern template std::pair<::tyr::formalism::datalog::AtomView<::tyr::GroundTag, FluentTag>, bool>
merge_p2d(AtomView<::tyr::GroundTag, FluentTag> element,
          const ygg::UnorderedMap<PredicateView<FluentTag>, ::tyr::formalism::datalog::PredicateView<FluentTag>>& predicate_mapping,
          MergeDatalogContext& context);
extern template std::pair<::tyr::formalism::datalog::AtomView<::tyr::GroundTag, FluentTag>, bool>
merge_p2d(AtomView<::tyr::GroundTag, DerivedTag> element,
          const ygg::UnorderedMap<PredicateView<DerivedTag>, ::tyr::formalism::datalog::PredicateView<FluentTag>>& predicate_mapping,
          MergeDatalogContext& context);

extern template std::pair<::tyr::formalism::datalog::LiteralView<::tyr::LiftedTag, StaticTag>, bool>
merge_p2d(LiteralView<::tyr::LiftedTag, StaticTag> element,
          const ygg::UnorderedMap<PredicateView<StaticTag>, ::tyr::formalism::datalog::PredicateView<StaticTag>>& predicate_mapping,
          MergeDatalogContext& context);
extern template std::pair<::tyr::formalism::datalog::LiteralView<::tyr::LiftedTag, FluentTag>, bool>
merge_p2d(LiteralView<::tyr::LiftedTag, FluentTag> element,
          const ygg::UnorderedMap<PredicateView<FluentTag>, ::tyr::formalism::datalog::PredicateView<FluentTag>>& predicate_mapping,
          MergeDatalogContext& context);
extern template std::pair<::tyr::formalism::datalog::LiteralView<::tyr::LiftedTag, FluentTag>, bool>
merge_p2d(LiteralView<::tyr::LiftedTag, DerivedTag> element,
          const ygg::UnorderedMap<PredicateView<DerivedTag>, ::tyr::formalism::datalog::PredicateView<FluentTag>>& predicate_mapping,
          MergeDatalogContext& context);
extern template std::pair<::tyr::formalism::datalog::LiteralView<::tyr::GroundTag, StaticTag>, bool>
merge_p2d(LiteralView<::tyr::GroundTag, StaticTag> element,
          const ygg::UnorderedMap<PredicateView<StaticTag>, ::tyr::formalism::datalog::PredicateView<StaticTag>>& predicate_mapping,
          MergeDatalogContext& context);
extern template std::pair<::tyr::formalism::datalog::LiteralView<::tyr::GroundTag, FluentTag>, bool>
merge_p2d(LiteralView<::tyr::GroundTag, FluentTag> element,
          const ygg::UnorderedMap<PredicateView<FluentTag>, ::tyr::formalism::datalog::PredicateView<FluentTag>>& predicate_mapping,
          MergeDatalogContext& context);
extern template std::pair<::tyr::formalism::datalog::LiteralView<::tyr::GroundTag, FluentTag>, bool>
merge_p2d(LiteralView<::tyr::GroundTag, DerivedTag> element,
          const ygg::UnorderedMap<PredicateView<DerivedTag>, ::tyr::formalism::datalog::PredicateView<FluentTag>>& predicate_mapping,
          MergeDatalogContext& context);

std::optional<::tyr::formalism::datalog::LiteralView<::tyr::GroundTag, FluentTag>>
merge_p2d(FDRFactView<FluentTag> element,
          bool polarity,
          const ygg::UnorderedMap<PredicateView<FluentTag>, ::tyr::formalism::datalog::PredicateView<FluentTag>>& predicate_mapping,
          MergeDatalogContext& context);

std::optional<::tyr::formalism::datalog::LiteralView<::tyr::GroundTag, FluentTag>>
merge_p2d(FDRFactView<FluentTag> element,
          bool polarity,
          ygg::UnorderedMap<AtomView<::tyr::GroundTag, FluentTag>, ::tyr::formalism::datalog::AtomView<::tyr::GroundTag, FluentTag>>& atom_mapping,
          const ygg::UnorderedMap<PredicateView<FluentTag>, ::tyr::formalism::datalog::PredicateView<FluentTag>>& predicate_mapping,
          MergeDatalogContext& context);

std::pair<::tyr::formalism::datalog::ConjunctiveConditionView<::tyr::GroundTag>, bool>
merge_p2d(ConjunctiveConditionView<::tyr::GroundTag> element,
          const ygg::UnorderedMap<PredicateView<FluentTag>, ::tyr::formalism::datalog::PredicateView<FluentTag>>& fluent_predicate_mapping,
          const ygg::UnorderedMap<PredicateView<DerivedTag>, ::tyr::formalism::datalog::PredicateView<FluentTag>>& derived_predicate_mapping,
          MergeDatalogContext& context);

std::pair<::tyr::formalism::datalog::ConjunctiveConditionView<::tyr::GroundTag>, bool>
merge_p2d(ConjunctiveConditionView<::tyr::GroundTag> element,
          ygg::UnorderedMap<AtomView<::tyr::GroundTag, FluentTag>, ::tyr::formalism::datalog::AtomView<::tyr::GroundTag, FluentTag>>& fluent_atom_mapping,
          const ygg::UnorderedMap<PredicateView<FluentTag>, ::tyr::formalism::datalog::PredicateView<FluentTag>>& fluent_predicate_mapping,
          ygg::UnorderedMap<AtomView<::tyr::GroundTag, DerivedTag>, ::tyr::formalism::datalog::AtomView<::tyr::GroundTag, FluentTag>>& derived_atom_mapping,
          const ygg::UnorderedMap<PredicateView<DerivedTag>, ::tyr::formalism::datalog::PredicateView<FluentTag>>& derived_predicate_mapping,
          MergeDatalogContext& context);

std::pair<::tyr::formalism::datalog::ConjunctiveConditionView<::tyr::GroundTag>, bool>
merge_p2d(ConjunctiveConditionView<::tyr::GroundTag> element,
          ygg::UnorderedMap<AtomView<::tyr::GroundTag, FluentTag>, ::tyr::formalism::datalog::AtomView<::tyr::GroundTag, FluentTag>>& fluent_atom_mapping,
          const ygg::UnorderedMap<PredicateView<FluentTag>, ::tyr::formalism::datalog::PredicateView<FluentTag>>& fluent_predicate_mapping,
          MergeDatalogContext& context);

// Numeric

extern template std::pair<::tyr::formalism::datalog::FunctionView<StaticTag>, bool> merge_p2d(FunctionView<StaticTag> element, MergeDatalogContext& context);
extern template std::pair<::tyr::formalism::datalog::FunctionView<FluentTag>, bool> merge_p2d(FunctionView<FluentTag> element, MergeDatalogContext& context);

extern template std::pair<::tyr::formalism::datalog::FunctionTermView<::tyr::LiftedTag, StaticTag>, bool>
merge_p2d(FunctionTermView<::tyr::LiftedTag, StaticTag> element, MergeDatalogContext& context);
extern template std::pair<::tyr::formalism::datalog::FunctionTermView<::tyr::LiftedTag, FluentTag>, bool>
merge_p2d(FunctionTermView<::tyr::LiftedTag, FluentTag> element, MergeDatalogContext& context);

extern template std::pair<::tyr::formalism::datalog::FunctionBindingView<StaticTag>, bool> merge_p2d(FunctionBindingView<StaticTag> element,
                                                                                                     MergeDatalogContext& context);
extern template std::pair<::tyr::formalism::datalog::FunctionBindingView<FluentTag>, bool> merge_p2d(FunctionBindingView<FluentTag> element,
                                                                                                     MergeDatalogContext& context);

extern template std::pair<::tyr::formalism::datalog::FunctionTermView<::tyr::GroundTag, StaticTag>, bool>
merge_p2d(FunctionTermView<::tyr::GroundTag, StaticTag> element, MergeDatalogContext& context);
extern template std::pair<::tyr::formalism::datalog::FunctionTermView<::tyr::GroundTag, FluentTag>, bool>
merge_p2d(FunctionTermView<::tyr::GroundTag, FluentTag> element, MergeDatalogContext& context);

extern template std::pair<::tyr::formalism::datalog::FunctionTermValueView<::tyr::GroundTag, StaticTag>, bool>
merge_p2d(FunctionTermValueView<::tyr::GroundTag, StaticTag> element, MergeDatalogContext& context);
extern template std::pair<::tyr::formalism::datalog::FunctionTermValueView<::tyr::GroundTag, FluentTag>, bool>
merge_p2d(FunctionTermValueView<::tyr::GroundTag, FluentTag> element, MergeDatalogContext& context);

extern template std::pair<::tyr::formalism::datalog::FunctionView<FluentTag>, bool> merge_p2d<AuxiliaryTag, FluentTag>(FunctionView<AuxiliaryTag> element,
                                                                                                                       MergeDatalogContext& context);
extern template std::pair<::tyr::formalism::datalog::FunctionTermView<::tyr::LiftedTag, FluentTag>, bool>
merge_p2d<AuxiliaryTag, FluentTag>(FunctionTermView<::tyr::LiftedTag, AuxiliaryTag> element, MergeDatalogContext& context);
extern template std::pair<::tyr::formalism::datalog::FunctionBindingView<FluentTag>, bool>
merge_p2d<AuxiliaryTag, FluentTag>(FunctionBindingView<AuxiliaryTag> element, MergeDatalogContext& context);
extern template std::pair<::tyr::formalism::datalog::FunctionTermView<::tyr::GroundTag, FluentTag>, bool>
merge_p2d<AuxiliaryTag, FluentTag>(FunctionTermView<::tyr::GroundTag, AuxiliaryTag> element, MergeDatalogContext& context);
extern template std::pair<::tyr::formalism::datalog::FunctionTermValueView<::tyr::GroundTag, FluentTag>, bool>
merge_p2d<AuxiliaryTag, FluentTag>(FunctionTermValueView<::tyr::GroundTag, AuxiliaryTag> element, MergeDatalogContext& context);

extern template std::pair<::tyr::formalism::datalog::NumericEffectView<::tyr::LiftedTag, FluentTag>, bool>
merge_p2d<FluentTag, FluentTag>(NumericEffectView<::tyr::LiftedTag, FluentTag> element, MergeDatalogContext& context);
extern template std::pair<::tyr::formalism::datalog::NumericEffectView<::tyr::LiftedTag, FluentTag>, bool>
merge_p2d<AuxiliaryTag, FluentTag>(NumericEffectView<::tyr::LiftedTag, AuxiliaryTag> element, MergeDatalogContext& context);
extern template ygg::Data<::tyr::formalism::datalog::NumericEffectOperator<::tyr::LiftedTag, FluentTag>>
merge_p2d<FluentTag, FluentTag>(NumericEffectOperatorView<::tyr::LiftedTag, FluentTag> element, MergeDatalogContext& context);
extern template ygg::Data<::tyr::formalism::datalog::NumericEffectOperator<::tyr::LiftedTag, FluentTag>>
merge_p2d<AuxiliaryTag, FluentTag>(NumericEffectOperatorView<::tyr::LiftedTag, AuxiliaryTag> element, MergeDatalogContext& context);

extern template std::pair<::tyr::formalism::datalog::NumericEffectView<::tyr::GroundTag, FluentTag>, bool>
merge_p2d<FluentTag, FluentTag>(NumericEffectView<::tyr::GroundTag, FluentTag> element, MergeDatalogContext& context);
extern template std::pair<::tyr::formalism::datalog::NumericEffectView<::tyr::GroundTag, FluentTag>, bool>
merge_p2d<AuxiliaryTag, FluentTag>(NumericEffectView<::tyr::GroundTag, AuxiliaryTag> element, MergeDatalogContext& context);
extern template ygg::Data<::tyr::formalism::datalog::NumericEffectOperator<::tyr::GroundTag, FluentTag>>
merge_p2d<FluentTag, FluentTag>(NumericEffectOperatorView<::tyr::GroundTag, FluentTag> element, MergeDatalogContext& context);
extern template ygg::Data<::tyr::formalism::datalog::NumericEffectOperator<::tyr::GroundTag, FluentTag>>
merge_p2d<AuxiliaryTag, FluentTag>(NumericEffectOperatorView<::tyr::GroundTag, AuxiliaryTag> element, MergeDatalogContext& context);

extern template std::pair<::tyr::formalism::datalog::UnaryOperatorView<ygg::Data<::tyr::formalism::datalog::FunctionExpression<::tyr::LiftedTag>>>, bool>
merge_p2d(UnaryOperatorView<ygg::Data<FunctionExpression<::tyr::LiftedTag>>> element, MergeDatalogContext& context);
extern template std::pair<::tyr::formalism::datalog::UnaryOperatorView<ygg::Data<::tyr::formalism::datalog::FunctionExpression<::tyr::GroundTag>>>, bool>
merge_p2d(UnaryOperatorView<ygg::Data<FunctionExpression<::tyr::GroundTag>>> element, MergeDatalogContext& context);

extern template std::
    pair<::tyr::formalism::datalog::BinaryOperatorView<BooleanOperatorKind, ygg::Data<::tyr::formalism::datalog::FunctionExpression<::tyr::LiftedTag>>>, bool>
    merge_p2d(BinaryOperatorView<BooleanOperatorKind, ygg::Data<FunctionExpression<::tyr::LiftedTag>>> element, MergeDatalogContext& context);
extern template std::pair<
    ::tyr::formalism::datalog::BinaryOperatorView<ArithmeticOperatorKind, ygg::Data<::tyr::formalism::datalog::FunctionExpression<::tyr::LiftedTag>>>,
    bool>
merge_p2d(BinaryOperatorView<ArithmeticOperatorKind, ygg::Data<FunctionExpression<::tyr::LiftedTag>>> element, MergeDatalogContext& context);
extern template std::
    pair<::tyr::formalism::datalog::BinaryOperatorView<BooleanOperatorKind, ygg::Data<::tyr::formalism::datalog::FunctionExpression<::tyr::GroundTag>>>, bool>
    merge_p2d(BinaryOperatorView<BooleanOperatorKind, ygg::Data<FunctionExpression<::tyr::GroundTag>>> element, MergeDatalogContext& context);
extern template std::pair<
    ::tyr::formalism::datalog::BinaryOperatorView<ArithmeticOperatorKind, ygg::Data<::tyr::formalism::datalog::FunctionExpression<::tyr::GroundTag>>>,
    bool>
merge_p2d(BinaryOperatorView<ArithmeticOperatorKind, ygg::Data<FunctionExpression<::tyr::GroundTag>>> element, MergeDatalogContext& context);

extern template std::pair<::tyr::formalism::datalog::MultiOperatorView<ygg::Data<::tyr::formalism::datalog::FunctionExpression<::tyr::LiftedTag>>>, bool>
merge_p2d(MultiOperatorView<ygg::Data<FunctionExpression<::tyr::LiftedTag>>> element, MergeDatalogContext& context);
extern template std::pair<::tyr::formalism::datalog::MultiOperatorView<ygg::Data<::tyr::formalism::datalog::FunctionExpression<::tyr::GroundTag>>>, bool>
merge_p2d(MultiOperatorView<ygg::Data<FunctionExpression<::tyr::GroundTag>>> element, MergeDatalogContext& context);

extern template ygg::Data<::tyr::formalism::datalog::ArithmeticOperator<ygg::Data<::tyr::formalism::datalog::FunctionExpression<::tyr::LiftedTag>>>>
merge_p2d(ArithmeticOperatorView<ygg::Data<FunctionExpression<::tyr::LiftedTag>>> element, MergeDatalogContext& context);
extern template ygg::Data<::tyr::formalism::datalog::ArithmeticOperator<ygg::Data<::tyr::formalism::datalog::FunctionExpression<::tyr::GroundTag>>>>
merge_p2d(ArithmeticOperatorView<ygg::Data<FunctionExpression<::tyr::GroundTag>>> element, MergeDatalogContext& context);

extern template ygg::Data<::tyr::formalism::datalog::BooleanOperator<ygg::Data<::tyr::formalism::datalog::FunctionExpression<::tyr::LiftedTag>>>>
merge_p2d(BooleanOperatorView<ygg::Data<FunctionExpression<::tyr::LiftedTag>>> element, MergeDatalogContext& context);
extern template ygg::Data<::tyr::formalism::datalog::BooleanOperator<ygg::Data<::tyr::formalism::datalog::FunctionExpression<::tyr::GroundTag>>>>
merge_p2d(BooleanOperatorView<ygg::Data<FunctionExpression<::tyr::GroundTag>>> element, MergeDatalogContext& context);
}

#endif

#endif
