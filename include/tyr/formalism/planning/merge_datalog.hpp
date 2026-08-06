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

#include "tyr/formalism/datalog/builder.hpp"
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
std::pair<::tyr::formalism::datalog::AtomView<T_DST>, bool>
merge_p2d(AtomView<T_SRC> element,  //
          const ygg::UnorderedMap<PredicateView<T_SRC>, ::tyr::formalism::datalog::PredicateView<T_DST>>& predicate_mapping,
          MergeDatalogContext& context);

template<FactKind T_SRC, FactKind T_DST = T_SRC>
std::pair<::tyr::formalism::datalog::PredicateBindingView<T_DST>, bool>
merge_p2d(PredicateBindingView<T_SRC> element,  //
          const ygg::UnorderedMap<PredicateView<T_SRC>, ::tyr::formalism::datalog::PredicateView<T_DST>>& predicate_mapping,
          MergeDatalogContext& context);

template<FactKind T_SRC, FactKind T_DST = T_SRC>
std::pair<::tyr::formalism::datalog::GroundAtomView<T_DST>, bool>
merge_p2d(GroundAtomView<T_SRC> element,  //
          const ygg::UnorderedMap<PredicateView<T_SRC>, ::tyr::formalism::datalog::PredicateView<T_DST>>& predicate_mapping,
          MergeDatalogContext& context);

template<FactKind T_SRC, FactKind T_DST = T_SRC>
std::pair<::tyr::formalism::datalog::GroundAtomView<T_DST>, bool>
merge_p2d(GroundAtomView<T_SRC> element,  //
          ygg::UnorderedMap<GroundAtomView<T_SRC>, ::tyr::formalism::datalog::GroundAtomView<T_DST>>& atom_mapping,
          const ygg::UnorderedMap<PredicateView<T_SRC>, ::tyr::formalism::datalog::PredicateView<T_DST>>& predicate_mapping,
          MergeDatalogContext& context);

template<FactKind T_SRC, FactKind T_DST = T_SRC>
std::pair<::tyr::formalism::datalog::LiteralView<T_DST>, bool>
merge_p2d(LiteralView<T_SRC> element,  //
          const ygg::UnorderedMap<PredicateView<T_SRC>, ::tyr::formalism::datalog::PredicateView<T_DST>>& predicate_mapping,
          MergeDatalogContext& context);

template<FactKind T_SRC, FactKind T_DST = T_SRC>
std::pair<::tyr::formalism::datalog::GroundLiteralView<T_DST>, bool>
merge_p2d(GroundLiteralView<T_SRC> element,  //
          const ygg::UnorderedMap<PredicateView<T_SRC>, ::tyr::formalism::datalog::PredicateView<T_DST>>& predicate_mapping,
          MergeDatalogContext& context);

template<FactKind T_SRC, FactKind T_DST = T_SRC>
std::pair<::tyr::formalism::datalog::GroundLiteralView<T_DST>, bool>
merge_p2d(GroundLiteralView<T_SRC> element,  //
          ygg::UnorderedMap<GroundAtomView<T_SRC>, ::tyr::formalism::datalog::GroundAtomView<T_DST>>& atom_mapping,
          const ygg::UnorderedMap<PredicateView<T_SRC>, ::tyr::formalism::datalog::PredicateView<T_DST>>& predicate_mapping,
          MergeDatalogContext& context);

std::optional<::tyr::formalism::datalog::GroundLiteralView<FluentTag>>
merge_p2d(FDRFactView<FluentTag> element,
          bool polarity,
          const ygg::UnorderedMap<PredicateView<FluentTag>, ::tyr::formalism::datalog::PredicateView<FluentTag>>& predicate_mapping,
          MergeDatalogContext& context);

std::optional<::tyr::formalism::datalog::GroundLiteralView<FluentTag>>
merge_p2d(FDRFactView<FluentTag> element,
          bool polarity,
          ygg::UnorderedMap<GroundAtomView<FluentTag>, ::tyr::formalism::datalog::GroundAtomView<FluentTag>>& atom_mapping,
          const ygg::UnorderedMap<PredicateView<FluentTag>, ::tyr::formalism::datalog::PredicateView<FluentTag>>& predicate_mapping,
          MergeDatalogContext& context);

std::pair<::tyr::formalism::datalog::GroundConjunctiveConditionView, bool>
merge_p2d(GroundConjunctiveConditionView element,
          const ygg::UnorderedMap<PredicateView<FluentTag>, ::tyr::formalism::datalog::PredicateView<FluentTag>>& fluent_predicate_mapping,
          const ygg::UnorderedMap<PredicateView<DerivedTag>, ::tyr::formalism::datalog::PredicateView<FluentTag>>& derived_predicate_mapping,
          MergeDatalogContext& context);

std::pair<::tyr::formalism::datalog::GroundConjunctiveConditionView, bool>
merge_p2d(GroundConjunctiveConditionView element,
          ygg::UnorderedMap<GroundAtomView<FluentTag>, ::tyr::formalism::datalog::GroundAtomView<FluentTag>>& fluent_atom_mapping,
          const ygg::UnorderedMap<PredicateView<FluentTag>, ::tyr::formalism::datalog::PredicateView<FluentTag>>& fluent_predicate_mapping,
          ygg::UnorderedMap<GroundAtomView<DerivedTag>, ::tyr::formalism::datalog::GroundAtomView<FluentTag>>& derived_atom_mapping,
          const ygg::UnorderedMap<PredicateView<DerivedTag>, ::tyr::formalism::datalog::PredicateView<FluentTag>>& derived_predicate_mapping,
          MergeDatalogContext& context);

std::pair<::tyr::formalism::datalog::GroundConjunctiveConditionView, bool>
merge_p2d(GroundConjunctiveConditionView element,
          ygg::UnorderedMap<GroundAtomView<FluentTag>, ::tyr::formalism::datalog::GroundAtomView<FluentTag>>& fluent_atom_mapping,
          const ygg::UnorderedMap<PredicateView<FluentTag>, ::tyr::formalism::datalog::PredicateView<FluentTag>>& fluent_predicate_mapping,
          MergeDatalogContext& context);

// Numeric

template<FactKind T_SRC, FactKind T_DST = T_SRC>
std::pair<::tyr::formalism::datalog::FunctionView<T_DST>, bool> merge_p2d(FunctionView<T_SRC> element, MergeDatalogContext& context);

template<FactKind T_SRC, FactKind T_DST = T_SRC>
std::pair<::tyr::formalism::datalog::FunctionTermView<T_DST>, bool> merge_p2d(FunctionTermView<T_SRC> element, MergeDatalogContext& context);

template<FactKind T_SRC, FactKind T_DST = T_SRC>
std::pair<::tyr::formalism::datalog::FunctionBindingView<T_DST>, bool> merge_p2d(FunctionBindingView<T_SRC> element, MergeDatalogContext& context);

template<FactKind T_SRC, FactKind T_DST = T_SRC>
std::pair<::tyr::formalism::datalog::GroundFunctionTermView<T_DST>, bool> merge_p2d(GroundFunctionTermView<T_SRC> element, MergeDatalogContext& context);

template<FactKind T_SRC, FactKind T_DST = T_SRC>
std::pair<::tyr::formalism::datalog::GroundFunctionTermValueView<T_DST>, bool> merge_p2d(GroundFunctionTermValueView<T_SRC> element,
                                                                                         MergeDatalogContext& context);

template<FactKind T_SRC,
         FactKind T_DST = T_SRC,
         typename = std::enable_if_t<(std::same_as<T_SRC, FluentTag> || std::same_as<T_SRC, AuxiliaryTag>) && std::same_as<T_DST, FluentTag>>>
std::pair<::tyr::formalism::datalog::NumericEffectView<T_DST>, bool> merge_p2d(NumericEffectView<T_SRC> element, MergeDatalogContext& context);

template<FactKind T_SRC,
         FactKind T_DST = T_SRC,
         typename = std::enable_if_t<(std::same_as<T_SRC, FluentTag> || std::same_as<T_SRC, AuxiliaryTag>) && std::same_as<T_DST, FluentTag>>>
ygg::Data<::tyr::formalism::datalog::NumericEffectOperator<T_DST>> merge_p2d(NumericEffectOperatorView<T_SRC> element, MergeDatalogContext& context);

ygg::Data<::tyr::formalism::datalog::FunctionExpression> merge_p2d(FunctionExpressionView element, MergeDatalogContext& context);

ygg::Data<::tyr::formalism::datalog::GroundFunctionExpression> merge_p2d(GroundFunctionExpressionView element, MergeDatalogContext& context);

template<FactKind T_SRC,
         FactKind T_DST = T_SRC,
         typename = std::enable_if_t<(std::same_as<T_SRC, FluentTag> || std::same_as<T_SRC, AuxiliaryTag>) && std::same_as<T_DST, FluentTag>>>
std::pair<::tyr::formalism::datalog::GroundNumericEffectView<T_DST>, bool> merge_p2d(GroundNumericEffectView<T_SRC> element, MergeDatalogContext& context);

template<FactKind T_SRC,
         FactKind T_DST = T_SRC,
         typename = std::enable_if_t<(std::same_as<T_SRC, FluentTag> || std::same_as<T_SRC, AuxiliaryTag>) && std::same_as<T_DST, FluentTag>>>
ygg::Data<::tyr::formalism::datalog::GroundNumericEffectOperator<T_DST>> merge_p2d(GroundNumericEffectOperatorView<T_SRC> element,
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
    auto variable_ptr = context.builder.template get_builder<Variable>();
    auto& variable = *variable_ptr;
    variable.clear();

    variable.name = element.get_name();

    canonicalize(variable);
    return context.destination.get_or_create(variable);
}

inline std::pair<::tyr::formalism::datalog::ObjectView, bool> merge_p2d(ObjectView element, MergeDatalogContext& context)
{
    auto object_ptr = context.builder.template get_builder<Object>();
    auto& object = *object_ptr;
    object.clear();

    object.name = element.get_name();

    canonicalize(object);
    return context.destination.get_or_create(object);
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
    auto predicate_ptr = context.builder.template get_builder<Predicate<T_DST>>();
    auto& predicate = *predicate_ptr;
    predicate.clear();

    predicate.name = element.get_name();
    predicate.arity = element.get_arity();

    canonicalize(predicate);
    return context.destination.get_or_create(predicate);
}

template<FactKind T_SRC, FactKind T_DST>
std::pair<::tyr::formalism::datalog::AtomView<T_DST>, bool>
merge_p2d(AtomView<T_SRC> element,  //
          const ygg::UnorderedMap<PredicateView<T_SRC>, ::tyr::formalism::datalog::PredicateView<T_DST>>& predicate_mapping,
          MergeDatalogContext& context)
{
    auto atom_ptr = context.builder.template get_builder<::tyr::formalism::datalog::Atom<T_DST>>();
    auto& atom = *atom_ptr;
    atom.clear();

    atom.predicate = predicate_mapping.at(element.get_predicate()).get_index();
    for (const auto term : element.get_terms())
        atom.terms.push_back(merge_p2d(term, context));

    canonicalize(atom);
    return context.destination.get_or_create(atom);
}

template<FactKind T_SRC, FactKind T_DST>
std::pair<::tyr::formalism::datalog::PredicateBindingView<T_DST>, bool>
merge_p2d(PredicateBindingView<T_SRC> element,  //
          const ygg::UnorderedMap<PredicateView<T_SRC>, ::tyr::formalism::datalog::PredicateView<T_DST>>& predicate_mapping,
          MergeDatalogContext& context)
{
    auto binding_ptr = context.builder.template get_builder<RelationBinding<Predicate<T_DST>>>();
    auto& binding = *binding_ptr;
    binding.clear();

    binding.relation = predicate_mapping.at(element.get_relation()).get_index();
    for (const auto object : element.get_objects())
        binding.objects.push_back(object.get_index());

    canonicalize(binding);
    return context.destination.get_or_create(binding);
}

template<FactKind T_SRC, FactKind T_DST>
std::pair<::tyr::formalism::datalog::GroundAtomView<T_DST>, bool>
merge_p2d(GroundAtomView<T_SRC> element,  //
          const ygg::UnorderedMap<PredicateView<T_SRC>, ::tyr::formalism::datalog::PredicateView<T_DST>>& predicate_mapping,
          MergeDatalogContext& context)
{
    auto atom_ptr = context.builder.template get_builder<::tyr::formalism::datalog::GroundAtom<T_DST>>();
    auto& atom = *atom_ptr;
    atom.clear();

    atom.binding = merge_p2d<T_SRC, T_DST>(element.get_row(), predicate_mapping, context).first.get_index();

    canonicalize(atom);
    return context.destination.get_or_create(atom);
}

template<FactKind T_SRC, FactKind T_DST>
std::pair<::tyr::formalism::datalog::GroundAtomView<T_DST>, bool>
merge_p2d(GroundAtomView<T_SRC> element,  //
          ygg::UnorderedMap<GroundAtomView<T_SRC>, ::tyr::formalism::datalog::GroundAtomView<T_DST>>& atom_mapping,
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
std::pair<::tyr::formalism::datalog::LiteralView<T_DST>, bool>
merge_p2d(LiteralView<T_SRC> element,  //
          const ygg::UnorderedMap<PredicateView<T_SRC>, ::tyr::formalism::datalog::PredicateView<T_DST>>& predicate_mapping,
          MergeDatalogContext& context)
{
    auto literal_ptr = context.builder.template get_builder<::tyr::formalism::datalog::Literal<T_DST>>();
    auto& literal = *literal_ptr;
    literal.clear();

    literal.polarity = element.get_polarity();
    literal.atom = merge_p2d<T_SRC, T_DST>(element.get_atom(), predicate_mapping, context).first.get_index();

    canonicalize(literal);
    return context.destination.get_or_create(literal);
}

template<FactKind T_SRC, FactKind T_DST>
std::pair<::tyr::formalism::datalog::GroundLiteralView<T_DST>, bool>
merge_p2d(GroundLiteralView<T_SRC> element,  //
          const ygg::UnorderedMap<PredicateView<T_SRC>, ::tyr::formalism::datalog::PredicateView<T_DST>>& predicate_mapping,
          MergeDatalogContext& context)
{
    auto literal_ptr = context.builder.template get_builder<::tyr::formalism::datalog::GroundLiteral<T_DST>>();
    auto& literal = *literal_ptr;
    literal.clear();

    literal.polarity = element.get_polarity();
    literal.atom = merge_p2d<T_SRC, T_DST>(element.get_atom(), predicate_mapping, context).first.get_index();

    canonicalize(literal);
    return context.destination.get_or_create(literal);
}

template<FactKind T_SRC, FactKind T_DST>
std::pair<::tyr::formalism::datalog::GroundLiteralView<T_DST>, bool>
merge_p2d(GroundLiteralView<T_SRC> element,  //
          ygg::UnorderedMap<GroundAtomView<T_SRC>, ::tyr::formalism::datalog::GroundAtomView<T_DST>>& atom_mapping,
          const ygg::UnorderedMap<PredicateView<T_SRC>, ::tyr::formalism::datalog::PredicateView<T_DST>>& predicate_mapping,
          MergeDatalogContext& context)
{
    auto literal_ptr = context.builder.template get_builder<::tyr::formalism::datalog::GroundLiteral<T_DST>>();
    auto& literal = *literal_ptr;
    literal.clear();

    literal.polarity = element.get_polarity();
    literal.atom = merge_p2d<T_SRC, T_DST>(element.get_atom(), atom_mapping, predicate_mapping, context).first.get_index();

    canonicalize(literal);
    return context.destination.get_or_create(literal);
}

inline std::optional<::tyr::formalism::datalog::GroundLiteralView<FluentTag>>
merge_p2d(FDRFactView<FluentTag> element,
          bool polarity,
          const ygg::UnorderedMap<PredicateView<FluentTag>, ::tyr::formalism::datalog::PredicateView<FluentTag>>& predicate_mapping,
          MergeDatalogContext& context)
{
    if (!element.has_value())
        return std::nullopt;

    auto literal_ptr = context.builder.template get_builder<::tyr::formalism::datalog::GroundLiteral<FluentTag>>();
    auto& literal = *literal_ptr;
    literal.clear();
    literal.polarity = polarity;
    literal.atom = merge_p2d(element.get_atom().value(), predicate_mapping, context).first.get_index();

    ::tyr::formalism::datalog::canonicalize(literal);
    return context.destination.get_or_create(literal).first;
}

inline std::optional<::tyr::formalism::datalog::GroundLiteralView<FluentTag>>
merge_p2d(FDRFactView<FluentTag> element,
          bool polarity,
          ygg::UnorderedMap<GroundAtomView<FluentTag>, ::tyr::formalism::datalog::GroundAtomView<FluentTag>>& atom_mapping,
          const ygg::UnorderedMap<PredicateView<FluentTag>, ::tyr::formalism::datalog::PredicateView<FluentTag>>& predicate_mapping,
          MergeDatalogContext& context)
{
    if (!element.has_value())
        return std::nullopt;

    auto literal_ptr = context.builder.template get_builder<::tyr::formalism::datalog::GroundLiteral<FluentTag>>();
    auto& literal = *literal_ptr;
    literal.clear();
    literal.polarity = polarity;
    literal.atom = merge_p2d(element.get_atom().value(), atom_mapping, predicate_mapping, context).first.get_index();

    ::tyr::formalism::datalog::canonicalize(literal);
    return context.destination.get_or_create(literal).first;
}

inline std::pair<::tyr::formalism::datalog::GroundConjunctiveConditionView, bool>
merge_p2d(GroundConjunctiveConditionView element,
          const ygg::UnorderedMap<PredicateView<FluentTag>, ::tyr::formalism::datalog::PredicateView<FluentTag>>& fluent_predicate_mapping,
          const ygg::UnorderedMap<PredicateView<DerivedTag>, ::tyr::formalism::datalog::PredicateView<FluentTag>>& derived_predicate_mapping,
          MergeDatalogContext& context)
{
    auto condition_ptr = context.builder.template get_builder<::tyr::formalism::datalog::GroundConjunctiveCondition>();
    auto& condition = *condition_ptr;
    condition.clear();

    for (const auto fact : element.template get_facts<PositiveTag>())
        if (const auto literal = merge_p2d(fact, true, fluent_predicate_mapping, context))
            condition.fluent_literals.push_back(literal->get_index());

    for (const auto fact : element.template get_facts<NegativeTag>())
        if (const auto literal = merge_p2d(fact, false, fluent_predicate_mapping, context))
            condition.fluent_literals.push_back(literal->get_index());

    for (const auto literal : element.template get_literals<DerivedTag>())
        condition.fluent_literals.push_back(merge_p2d(literal, derived_predicate_mapping, context).first.get_index());

    for (const auto numeric_constraint : element.get_numeric_constraints())
        condition.numeric_constraints.push_back(merge_p2d(numeric_constraint, context));

    ::tyr::formalism::datalog::canonicalize(condition);
    return context.destination.get_or_create(condition);
}

inline std::pair<::tyr::formalism::datalog::GroundConjunctiveConditionView, bool>
merge_p2d(GroundConjunctiveConditionView element,
          ygg::UnorderedMap<GroundAtomView<FluentTag>, ::tyr::formalism::datalog::GroundAtomView<FluentTag>>& fluent_atom_mapping,
          const ygg::UnorderedMap<PredicateView<FluentTag>, ::tyr::formalism::datalog::PredicateView<FluentTag>>& fluent_predicate_mapping,
          ygg::UnorderedMap<GroundAtomView<DerivedTag>, ::tyr::formalism::datalog::GroundAtomView<FluentTag>>& derived_atom_mapping,
          const ygg::UnorderedMap<PredicateView<DerivedTag>, ::tyr::formalism::datalog::PredicateView<FluentTag>>& derived_predicate_mapping,
          MergeDatalogContext& context)
{
    auto condition_ptr = context.builder.template get_builder<::tyr::formalism::datalog::GroundConjunctiveCondition>();
    auto& condition = *condition_ptr;
    condition.clear();

    for (const auto fact : element.template get_facts<PositiveTag>())
        if (const auto literal = merge_p2d(fact, true, fluent_atom_mapping, fluent_predicate_mapping, context))
            condition.fluent_literals.push_back(literal->get_index());

    for (const auto fact : element.template get_facts<NegativeTag>())
        if (const auto literal = merge_p2d(fact, false, fluent_atom_mapping, fluent_predicate_mapping, context))
            condition.fluent_literals.push_back(literal->get_index());

    for (const auto literal : element.template get_literals<DerivedTag>())
        condition.fluent_literals.push_back(merge_p2d(literal, derived_atom_mapping, derived_predicate_mapping, context).first.get_index());

    for (const auto numeric_constraint : element.get_numeric_constraints())
        condition.numeric_constraints.push_back(merge_p2d(numeric_constraint, context));

    ::tyr::formalism::datalog::canonicalize(condition);
    return context.destination.get_or_create(condition);
}

inline std::pair<::tyr::formalism::datalog::GroundConjunctiveConditionView, bool>
merge_p2d(GroundConjunctiveConditionView element,
          ygg::UnorderedMap<GroundAtomView<FluentTag>, ::tyr::formalism::datalog::GroundAtomView<FluentTag>>& fluent_atom_mapping,
          const ygg::UnorderedMap<PredicateView<FluentTag>, ::tyr::formalism::datalog::PredicateView<FluentTag>>& fluent_predicate_mapping,
          MergeDatalogContext& context)
{
    auto condition_ptr = context.builder.template get_builder<::tyr::formalism::datalog::GroundConjunctiveCondition>();
    auto& condition = *condition_ptr;
    condition.clear();

    for (const auto fact : element.template get_facts<PositiveTag>())
        if (const auto literal = merge_p2d(fact, true, fluent_atom_mapping, fluent_predicate_mapping, context))
            condition.fluent_literals.push_back(literal->get_index());

    for (const auto fact : element.template get_facts<NegativeTag>())
        if (const auto literal = merge_p2d(fact, false, fluent_atom_mapping, fluent_predicate_mapping, context))
            condition.fluent_literals.push_back(literal->get_index());

    for (const auto numeric_constraint : element.get_numeric_constraints())
        condition.numeric_constraints.push_back(merge_p2d(numeric_constraint, context));

    ::tyr::formalism::datalog::canonicalize(condition);
    return context.destination.get_or_create(condition);
}

// Numeric

template<FactKind T_SRC, FactKind T_DST>
std::pair<::tyr::formalism::datalog::FunctionView<T_DST>, bool> merge_p2d(FunctionView<T_SRC> element, MergeDatalogContext& context)
{
    auto function_ptr = context.builder.template get_builder<::tyr::formalism::Function<T_DST>>();
    auto& function = *function_ptr;
    function.clear();

    function.name = element.get_name();
    function.arity = element.get_arity();

    canonicalize(function);
    return context.destination.get_or_create(function);
}

template<FactKind T_SRC, FactKind T_DST>
std::pair<::tyr::formalism::datalog::FunctionTermView<T_DST>, bool> merge_p2d(FunctionTermView<T_SRC> element, MergeDatalogContext& context)
{
    auto fterm_ptr = context.builder.template get_builder<::tyr::formalism::datalog::FunctionTerm<T_DST>>();
    auto& fterm = *fterm_ptr;
    fterm.clear();

    fterm.function = merge_p2d<T_SRC, T_DST>(element.get_function(), context).first.get_index();
    for (const auto term : element.get_terms())
        fterm.terms.push_back(merge_p2d(term, context));

    canonicalize(fterm);
    return context.destination.get_or_create(fterm);
}

template<FactKind T_SRC, FactKind T_DST>
std::pair<::tyr::formalism::datalog::FunctionBindingView<T_DST>, bool> merge_p2d(FunctionBindingView<T_SRC> element, MergeDatalogContext& context)
{
    auto binding_ptr = context.builder.template get_builder<RelationBinding<Function<T_DST>>>();
    auto& binding = *binding_ptr;
    binding.clear();

    binding.relation = merge_p2d<T_SRC, T_DST>(element.get_relation(), context).first.get_index();
    for (const auto object : element.get_objects())
        binding.objects.push_back(object.get_index());

    canonicalize(binding);
    return context.destination.get_or_create(binding);
}

template<FactKind T_SRC, FactKind T_DST>
std::pair<::tyr::formalism::datalog::GroundFunctionTermView<T_DST>, bool> merge_p2d(GroundFunctionTermView<T_SRC> element, MergeDatalogContext& context)
{
    auto fterm_ptr = context.builder.template get_builder<::tyr::formalism::datalog::GroundFunctionTerm<T_DST>>();
    auto& fterm = *fterm_ptr;
    fterm.clear();

    fterm.binding = merge_p2d<T_SRC, T_DST>(element.get_row(), context).first.get_index();

    canonicalize(fterm);
    return context.destination.get_or_create(fterm);
}

template<FactKind T_SRC, FactKind T_DST>
std::pair<::tyr::formalism::datalog::GroundFunctionTermValueView<T_DST>, bool> merge_p2d(GroundFunctionTermValueView<T_SRC> element,
                                                                                         MergeDatalogContext& context)
{
    auto fterm_value_ptr = context.builder.template get_builder<::tyr::formalism::datalog::GroundFunctionTermValue<T_DST>>();
    auto& fterm_value = *fterm_value_ptr;
    fterm_value.clear();

    fterm_value.fterm = merge_p2d<T_SRC, T_DST>(element.get_fterm(), context).first.get_index();
    fterm_value.value = element.get_value();

    canonicalize(fterm_value);
    return context.destination.get_or_create(fterm_value);
}

template<FactKind T_SRC, FactKind T_DST, typename>
std::pair<::tyr::formalism::datalog::NumericEffectView<T_DST>, bool> merge_p2d(NumericEffectView<T_SRC> element, MergeDatalogContext& context)
{
    auto numeric_effect_ptr = context.builder.template get_builder<::tyr::formalism::datalog::NumericEffect<T_DST>>();
    auto& numeric_effect = *numeric_effect_ptr;
    numeric_effect.clear();

    numeric_effect.operator_kind = element.get_operator();
    numeric_effect.fterm = merge_p2d<T_SRC, T_DST>(element.get_fterm(), context).first.get_index();
    numeric_effect.fexpr = merge_p2d(element.get_fexpr(), context);

    canonicalize(numeric_effect);
    return context.destination.get_or_create(numeric_effect);
}

template<FactKind T_SRC, FactKind T_DST, typename>
ygg::Data<::tyr::formalism::datalog::NumericEffectOperator<T_DST>> merge_p2d(NumericEffectOperatorView<T_SRC> element, MergeDatalogContext& context)
{
    using OperatorData = ygg::Data<::tyr::formalism::datalog::NumericEffectOperator<T_DST>>;

    return visit([&](auto&& arg)
                 { return OperatorData(arg.get_operator(), typename OperatorData::Variant(merge_p2d<T_SRC, T_DST>(arg, context).first.get_index())); },
                 element.get_variant());
}

template<FactKind T_SRC, FactKind T_DST, typename>
std::pair<::tyr::formalism::datalog::GroundNumericEffectView<T_DST>, bool> merge_p2d(GroundNumericEffectView<T_SRC> element, MergeDatalogContext& context)
{
    auto numeric_effect_ptr = context.builder.template get_builder<::tyr::formalism::datalog::GroundNumericEffect<T_DST>>();
    auto& numeric_effect = *numeric_effect_ptr;
    numeric_effect.clear();

    numeric_effect.operator_kind = element.get_operator();
    numeric_effect.fterm = merge_p2d<T_SRC, T_DST>(element.get_fterm(), context).first.get_index();
    numeric_effect.fexpr = merge_p2d(element.get_fexpr(), context);

    canonicalize(numeric_effect);
    return context.destination.get_or_create(numeric_effect);
}

template<FactKind T_SRC, FactKind T_DST, typename>
ygg::Data<::tyr::formalism::datalog::GroundNumericEffectOperator<T_DST>> merge_p2d(GroundNumericEffectOperatorView<T_SRC> element, MergeDatalogContext& context)
{
    using OperatorData = ygg::Data<::tyr::formalism::datalog::GroundNumericEffectOperator<T_DST>>;

    return visit([&](auto&& arg)
                 { return OperatorData(arg.get_operator(), typename OperatorData::Variant(merge_p2d<T_SRC, T_DST>(arg, context).first.get_index())); },
                 element.get_variant());
}

inline ygg::Data<::tyr::formalism::datalog::FunctionExpression> merge_p2d(FunctionExpressionView element, MergeDatalogContext& context)
{
    return visit(
        [&](auto&& arg) -> ygg::Data<::tyr::formalism::datalog::FunctionExpression>
        {
            using Alternative = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<Alternative, ygg::float_t>)
                return ygg::Data<::tyr::formalism::datalog::FunctionExpression>(arg);
            else if constexpr (std::is_same_v<Alternative, LiftedArithmeticOperatorView>)
                return ygg::Data<::tyr::formalism::datalog::FunctionExpression>(merge_p2d(arg, context));
            else if constexpr (std::is_same_v<Alternative, FunctionTermView<AuxiliaryTag>>)
                return ygg::Data<::tyr::formalism::datalog::FunctionExpression>(merge_p2d<AuxiliaryTag, FluentTag>(arg, context).first.get_index());
            else
                return ygg::Data<::tyr::formalism::datalog::FunctionExpression>(merge_p2d(arg, context).first.get_index());
        },
        element.get_variant());
}

inline ygg::Data<::tyr::formalism::datalog::GroundFunctionExpression> merge_p2d(GroundFunctionExpressionView element, MergeDatalogContext& context)
{
    return visit(
        [&](auto&& arg) -> ygg::Data<::tyr::formalism::datalog::GroundFunctionExpression>
        {
            using Alternative = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<Alternative, ygg::float_t>)
                return ygg::Data<::tyr::formalism::datalog::GroundFunctionExpression>(arg);
            else if constexpr (std::is_same_v<Alternative, GroundArithmeticOperatorView>)
                return ygg::Data<::tyr::formalism::datalog::GroundFunctionExpression>(merge_p2d(arg, context));
            else if constexpr (std::is_same_v<Alternative, GroundFunctionTermView<AuxiliaryTag>>)
                return ygg::Data<::tyr::formalism::datalog::GroundFunctionExpression>(merge_p2d<AuxiliaryTag, FluentTag>(arg, context).first.get_index());
            else
                return ygg::Data<::tyr::formalism::datalog::GroundFunctionExpression>(merge_p2d(arg, context).first.get_index());
        },
        element.get_variant());
}

template<typename T>
std::pair<::tyr::formalism::datalog::UnaryOperatorView<to_datalog_payload_t<T>>, bool> merge_p2d(UnaryOperatorView<T> element, MergeDatalogContext& context)
{
    using T_DST = to_datalog_payload_t<T>;

    auto unary_ptr = context.builder.template get_builder<::tyr::formalism::datalog::UnaryOperator<T_DST>>();
    auto& unary = *unary_ptr;
    unary.clear();

    unary.operator_kind = element.get_operator();
    unary.arg = merge_p2d(element.get_arg(), context);

    canonicalize(unary);
    return context.destination.get_or_create(unary);
}

template<BinaryOperatorKind O, typename T>
std::pair<::tyr::formalism::datalog::BinaryOperatorView<O, to_datalog_payload_t<T>>, bool> merge_p2d(BinaryOperatorView<O, T> element,
                                                                                                     MergeDatalogContext& context)
{
    using T_DST = to_datalog_payload_t<T>;

    auto binary_ptr = context.builder.template get_builder<::tyr::formalism::datalog::BinaryOperator<O, T_DST>>();
    auto& binary = *binary_ptr;
    binary.clear();

    binary.operator_kind = element.get_operator();
    binary.lhs = merge_p2d(element.get_lhs(), context);
    binary.rhs = merge_p2d(element.get_rhs(), context);

    canonicalize(binary);
    return context.destination.get_or_create(binary);
}

template<typename T>
std::pair<::tyr::formalism::datalog::MultiOperatorView<to_datalog_payload_t<T>>, bool> merge_p2d(MultiOperatorView<T> element, MergeDatalogContext& context)
{
    using T_DST = to_datalog_payload_t<T>;

    auto multi_ptr = context.builder.template get_builder<::tyr::formalism::datalog::MultiOperator<T_DST>>();
    auto& multi = *multi_ptr;
    multi.clear();

    multi.operator_kind = element.get_operator();
    for (const auto arg : element.get_args())
        multi.args.push_back(merge_p2d(arg, context));

    canonicalize(multi);
    return context.destination.get_or_create(multi);
}

template<typename T>
ygg::Data<::tyr::formalism::datalog::ArithmeticOperator<to_datalog_payload_t<T>>> merge_p2d(ArithmeticOperatorView<T> element, MergeDatalogContext& context)
{
    using T_DST = to_datalog_payload_t<T>;

    return visit([&](auto&& arg)
                 {
                     return ygg::Data<::tyr::formalism::datalog::ArithmeticOperator<T_DST>>(arg.get_operator(),
                                                                                           merge_p2d(arg, context).first.get_index());
                 },
                 element.get_variant());
}

template<typename T>
ygg::Data<::tyr::formalism::datalog::BooleanOperator<to_datalog_payload_t<T>>> merge_p2d(BooleanOperatorView<T> element, MergeDatalogContext& context)
{
    using T_DST = to_datalog_payload_t<T>;

    return visit([&](auto&& arg)
                 {
                     return ygg::Data<::tyr::formalism::datalog::BooleanOperator<T_DST>>(arg.get_operator(),
                                                                                        merge_p2d(arg, context).first.get_index());
                 },
                 element.get_variant());
}

}

#ifndef TYR_HEADER_INSTANTIATION

namespace tyr::formalism::planning
{
extern template std::pair<::tyr::formalism::datalog::PredicateView<StaticTag>, bool> merge_p2d(PredicateView<StaticTag> element, MergeDatalogContext& context);
extern template std::pair<::tyr::formalism::datalog::PredicateView<FluentTag>, bool> merge_p2d(PredicateView<FluentTag> element, MergeDatalogContext& context);
extern template std::pair<::tyr::formalism::datalog::PredicateView<FluentTag>, bool> merge_p2d(PredicateView<DerivedTag> element, MergeDatalogContext& context);

extern template std::pair<::tyr::formalism::datalog::AtomView<StaticTag>, bool>
merge_p2d(AtomView<StaticTag> element,
          const ygg::UnorderedMap<PredicateView<StaticTag>, ::tyr::formalism::datalog::PredicateView<StaticTag>>& predicate_mapping,
          MergeDatalogContext& context);
extern template std::pair<::tyr::formalism::datalog::AtomView<FluentTag>, bool>
merge_p2d(AtomView<FluentTag> element,
          const ygg::UnorderedMap<PredicateView<FluentTag>, ::tyr::formalism::datalog::PredicateView<FluentTag>>& predicate_mapping,
          MergeDatalogContext& context);
extern template std::pair<::tyr::formalism::datalog::AtomView<FluentTag>, bool>
merge_p2d(AtomView<DerivedTag> element,
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

extern template std::pair<::tyr::formalism::datalog::GroundAtomView<StaticTag>, bool>
merge_p2d(GroundAtomView<StaticTag> element,
          const ygg::UnorderedMap<PredicateView<StaticTag>, ::tyr::formalism::datalog::PredicateView<StaticTag>>& predicate_mapping,
          MergeDatalogContext& context);
extern template std::pair<::tyr::formalism::datalog::GroundAtomView<FluentTag>, bool>
merge_p2d(GroundAtomView<FluentTag> element,
          const ygg::UnorderedMap<PredicateView<FluentTag>, ::tyr::formalism::datalog::PredicateView<FluentTag>>& predicate_mapping,
          MergeDatalogContext& context);
extern template std::pair<::tyr::formalism::datalog::GroundAtomView<FluentTag>, bool>
merge_p2d(GroundAtomView<DerivedTag> element,
          const ygg::UnorderedMap<PredicateView<DerivedTag>, ::tyr::formalism::datalog::PredicateView<FluentTag>>& predicate_mapping,
          MergeDatalogContext& context);

extern template std::pair<::tyr::formalism::datalog::LiteralView<StaticTag>, bool>
merge_p2d(LiteralView<StaticTag> element,
          const ygg::UnorderedMap<PredicateView<StaticTag>, ::tyr::formalism::datalog::PredicateView<StaticTag>>& predicate_mapping,
          MergeDatalogContext& context);
extern template std::pair<::tyr::formalism::datalog::LiteralView<FluentTag>, bool>
merge_p2d(LiteralView<FluentTag> element,
          const ygg::UnorderedMap<PredicateView<FluentTag>, ::tyr::formalism::datalog::PredicateView<FluentTag>>& predicate_mapping,
          MergeDatalogContext& context);
extern template std::pair<::tyr::formalism::datalog::LiteralView<FluentTag>, bool>
merge_p2d(LiteralView<DerivedTag> element,
          const ygg::UnorderedMap<PredicateView<DerivedTag>, ::tyr::formalism::datalog::PredicateView<FluentTag>>& predicate_mapping,
          MergeDatalogContext& context);
extern template std::pair<::tyr::formalism::datalog::GroundLiteralView<StaticTag>, bool>
merge_p2d(GroundLiteralView<StaticTag> element,
          const ygg::UnorderedMap<PredicateView<StaticTag>, ::tyr::formalism::datalog::PredicateView<StaticTag>>& predicate_mapping,
          MergeDatalogContext& context);
extern template std::pair<::tyr::formalism::datalog::GroundLiteralView<FluentTag>, bool>
merge_p2d(GroundLiteralView<FluentTag> element,
          const ygg::UnorderedMap<PredicateView<FluentTag>, ::tyr::formalism::datalog::PredicateView<FluentTag>>& predicate_mapping,
          MergeDatalogContext& context);
extern template std::pair<::tyr::formalism::datalog::GroundLiteralView<FluentTag>, bool>
merge_p2d(GroundLiteralView<DerivedTag> element,
          const ygg::UnorderedMap<PredicateView<DerivedTag>, ::tyr::formalism::datalog::PredicateView<FluentTag>>& predicate_mapping,
          MergeDatalogContext& context);

std::optional<::tyr::formalism::datalog::GroundLiteralView<FluentTag>>
merge_p2d(FDRFactView<FluentTag> element,
          bool polarity,
          const ygg::UnorderedMap<PredicateView<FluentTag>, ::tyr::formalism::datalog::PredicateView<FluentTag>>& predicate_mapping,
          MergeDatalogContext& context);

std::optional<::tyr::formalism::datalog::GroundLiteralView<FluentTag>>
merge_p2d(FDRFactView<FluentTag> element,
          bool polarity,
          ygg::UnorderedMap<GroundAtomView<FluentTag>, ::tyr::formalism::datalog::GroundAtomView<FluentTag>>& atom_mapping,
          const ygg::UnorderedMap<PredicateView<FluentTag>, ::tyr::formalism::datalog::PredicateView<FluentTag>>& predicate_mapping,
          MergeDatalogContext& context);

std::pair<::tyr::formalism::datalog::GroundConjunctiveConditionView, bool>
merge_p2d(GroundConjunctiveConditionView element,
          const ygg::UnorderedMap<PredicateView<FluentTag>, ::tyr::formalism::datalog::PredicateView<FluentTag>>& fluent_predicate_mapping,
          const ygg::UnorderedMap<PredicateView<DerivedTag>, ::tyr::formalism::datalog::PredicateView<FluentTag>>& derived_predicate_mapping,
          MergeDatalogContext& context);

std::pair<::tyr::formalism::datalog::GroundConjunctiveConditionView, bool>
merge_p2d(GroundConjunctiveConditionView element,
          ygg::UnorderedMap<GroundAtomView<FluentTag>, ::tyr::formalism::datalog::GroundAtomView<FluentTag>>& fluent_atom_mapping,
          const ygg::UnorderedMap<PredicateView<FluentTag>, ::tyr::formalism::datalog::PredicateView<FluentTag>>& fluent_predicate_mapping,
          ygg::UnorderedMap<GroundAtomView<DerivedTag>, ::tyr::formalism::datalog::GroundAtomView<FluentTag>>& derived_atom_mapping,
          const ygg::UnorderedMap<PredicateView<DerivedTag>, ::tyr::formalism::datalog::PredicateView<FluentTag>>& derived_predicate_mapping,
          MergeDatalogContext& context);

std::pair<::tyr::formalism::datalog::GroundConjunctiveConditionView, bool>
merge_p2d(GroundConjunctiveConditionView element,
          ygg::UnorderedMap<GroundAtomView<FluentTag>, ::tyr::formalism::datalog::GroundAtomView<FluentTag>>& fluent_atom_mapping,
          const ygg::UnorderedMap<PredicateView<FluentTag>, ::tyr::formalism::datalog::PredicateView<FluentTag>>& fluent_predicate_mapping,
          MergeDatalogContext& context);

// Numeric

extern template std::pair<::tyr::formalism::datalog::FunctionView<StaticTag>, bool> merge_p2d(FunctionView<StaticTag> element, MergeDatalogContext& context);
extern template std::pair<::tyr::formalism::datalog::FunctionView<FluentTag>, bool> merge_p2d(FunctionView<FluentTag> element, MergeDatalogContext& context);

extern template std::pair<::tyr::formalism::datalog::FunctionTermView<StaticTag>, bool> merge_p2d(FunctionTermView<StaticTag> element,
                                                                                                  MergeDatalogContext& context);
extern template std::pair<::tyr::formalism::datalog::FunctionTermView<FluentTag>, bool> merge_p2d(FunctionTermView<FluentTag> element,
                                                                                                  MergeDatalogContext& context);

extern template std::pair<::tyr::formalism::datalog::FunctionBindingView<StaticTag>, bool> merge_p2d(FunctionBindingView<StaticTag> element,
                                                                                                     MergeDatalogContext& context);
extern template std::pair<::tyr::formalism::datalog::FunctionBindingView<FluentTag>, bool> merge_p2d(FunctionBindingView<FluentTag> element,
                                                                                                     MergeDatalogContext& context);

extern template std::pair<::tyr::formalism::datalog::GroundFunctionTermView<StaticTag>, bool> merge_p2d(GroundFunctionTermView<StaticTag> element,
                                                                                                        MergeDatalogContext& context);
extern template std::pair<::tyr::formalism::datalog::GroundFunctionTermView<FluentTag>, bool> merge_p2d(GroundFunctionTermView<FluentTag> element,
                                                                                                        MergeDatalogContext& context);

extern template std::pair<::tyr::formalism::datalog::GroundFunctionTermValueView<StaticTag>, bool> merge_p2d(GroundFunctionTermValueView<StaticTag> element,
                                                                                                             MergeDatalogContext& context);
extern template std::pair<::tyr::formalism::datalog::GroundFunctionTermValueView<FluentTag>, bool> merge_p2d(GroundFunctionTermValueView<FluentTag> element,
                                                                                                             MergeDatalogContext& context);

extern template std::pair<::tyr::formalism::datalog::FunctionView<FluentTag>, bool> merge_p2d<AuxiliaryTag, FluentTag>(FunctionView<AuxiliaryTag> element,
                                                                                                                       MergeDatalogContext& context);
extern template std::pair<::tyr::formalism::datalog::FunctionTermView<FluentTag>, bool>
merge_p2d<AuxiliaryTag, FluentTag>(FunctionTermView<AuxiliaryTag> element, MergeDatalogContext& context);
extern template std::pair<::tyr::formalism::datalog::FunctionBindingView<FluentTag>, bool>
merge_p2d<AuxiliaryTag, FluentTag>(FunctionBindingView<AuxiliaryTag> element, MergeDatalogContext& context);
extern template std::pair<::tyr::formalism::datalog::GroundFunctionTermView<FluentTag>, bool>
merge_p2d<AuxiliaryTag, FluentTag>(GroundFunctionTermView<AuxiliaryTag> element, MergeDatalogContext& context);
extern template std::pair<::tyr::formalism::datalog::GroundFunctionTermValueView<FluentTag>, bool>
merge_p2d<AuxiliaryTag, FluentTag>(GroundFunctionTermValueView<AuxiliaryTag> element, MergeDatalogContext& context);

extern template std::pair<::tyr::formalism::datalog::NumericEffectView<FluentTag>, bool> merge_p2d<FluentTag, FluentTag>(NumericEffectView<FluentTag> element,
                                                                                                                         MergeDatalogContext& context);
extern template std::pair<::tyr::formalism::datalog::NumericEffectView<FluentTag>, bool>
merge_p2d<AuxiliaryTag, FluentTag>(NumericEffectView<AuxiliaryTag> element, MergeDatalogContext& context);
extern template ygg::Data<::tyr::formalism::datalog::NumericEffectOperator<FluentTag>>
merge_p2d<FluentTag, FluentTag>(NumericEffectOperatorView<FluentTag> element, MergeDatalogContext& context);
extern template ygg::Data<::tyr::formalism::datalog::NumericEffectOperator<FluentTag>>
merge_p2d<AuxiliaryTag, FluentTag>(NumericEffectOperatorView<AuxiliaryTag> element, MergeDatalogContext& context);

extern template std::pair<::tyr::formalism::datalog::GroundNumericEffectView<FluentTag>, bool>
merge_p2d<FluentTag, FluentTag>(GroundNumericEffectView<FluentTag> element, MergeDatalogContext& context);
extern template std::pair<::tyr::formalism::datalog::GroundNumericEffectView<FluentTag>, bool>
merge_p2d<AuxiliaryTag, FluentTag>(GroundNumericEffectView<AuxiliaryTag> element, MergeDatalogContext& context);
extern template ygg::Data<::tyr::formalism::datalog::GroundNumericEffectOperator<FluentTag>>
merge_p2d<FluentTag, FluentTag>(GroundNumericEffectOperatorView<FluentTag> element, MergeDatalogContext& context);
extern template ygg::Data<::tyr::formalism::datalog::GroundNumericEffectOperator<FluentTag>>
merge_p2d<AuxiliaryTag, FluentTag>(GroundNumericEffectOperatorView<AuxiliaryTag> element, MergeDatalogContext& context);

extern template std::pair<::tyr::formalism::datalog::UnaryOperatorView<ygg::Data<::tyr::formalism::datalog::FunctionExpression>>, bool>
merge_p2d(UnaryOperatorView<ygg::Data<FunctionExpression>> element, MergeDatalogContext& context);
extern template std::pair<::tyr::formalism::datalog::UnaryOperatorView<ygg::Data<::tyr::formalism::datalog::GroundFunctionExpression>>, bool>
merge_p2d(UnaryOperatorView<ygg::Data<GroundFunctionExpression>> element, MergeDatalogContext& context);

extern template std::pair<::tyr::formalism::datalog::BinaryOperatorView<BooleanOperatorKind, ygg::Data<::tyr::formalism::datalog::FunctionExpression>>, bool>
merge_p2d(BinaryOperatorView<BooleanOperatorKind, ygg::Data<FunctionExpression>> element, MergeDatalogContext& context);
extern template std::pair<::tyr::formalism::datalog::BinaryOperatorView<ArithmeticOperatorKind, ygg::Data<::tyr::formalism::datalog::FunctionExpression>>, bool>
merge_p2d(BinaryOperatorView<ArithmeticOperatorKind, ygg::Data<FunctionExpression>> element, MergeDatalogContext& context);
extern template std::pair<::tyr::formalism::datalog::BinaryOperatorView<BooleanOperatorKind, ygg::Data<::tyr::formalism::datalog::GroundFunctionExpression>>,
                          bool>
merge_p2d(BinaryOperatorView<BooleanOperatorKind, ygg::Data<GroundFunctionExpression>> element, MergeDatalogContext& context);
extern template std::pair<::tyr::formalism::datalog::BinaryOperatorView<ArithmeticOperatorKind, ygg::Data<::tyr::formalism::datalog::GroundFunctionExpression>>,
                          bool>
merge_p2d(BinaryOperatorView<ArithmeticOperatorKind, ygg::Data<GroundFunctionExpression>> element, MergeDatalogContext& context);

extern template std::pair<::tyr::formalism::datalog::MultiOperatorView<ygg::Data<::tyr::formalism::datalog::FunctionExpression>>, bool>
merge_p2d(MultiOperatorView<ygg::Data<FunctionExpression>> element, MergeDatalogContext& context);
extern template std::pair<::tyr::formalism::datalog::MultiOperatorView<ygg::Data<::tyr::formalism::datalog::GroundFunctionExpression>>, bool>
merge_p2d(MultiOperatorView<ygg::Data<GroundFunctionExpression>> element, MergeDatalogContext& context);

extern template ygg::Data<::tyr::formalism::datalog::ArithmeticOperator<ygg::Data<::tyr::formalism::datalog::FunctionExpression>>>
merge_p2d(ArithmeticOperatorView<ygg::Data<FunctionExpression>> element, MergeDatalogContext& context);
extern template ygg::Data<::tyr::formalism::datalog::ArithmeticOperator<ygg::Data<::tyr::formalism::datalog::GroundFunctionExpression>>>
merge_p2d(ArithmeticOperatorView<ygg::Data<GroundFunctionExpression>> element, MergeDatalogContext& context);

extern template ygg::Data<::tyr::formalism::datalog::BooleanOperator<ygg::Data<::tyr::formalism::datalog::FunctionExpression>>>
merge_p2d(BooleanOperatorView<ygg::Data<FunctionExpression>> element, MergeDatalogContext& context);
extern template ygg::Data<::tyr::formalism::datalog::BooleanOperator<ygg::Data<::tyr::formalism::datalog::GroundFunctionExpression>>>
merge_p2d(BooleanOperatorView<ygg::Data<GroundFunctionExpression>> element, MergeDatalogContext& context);
}

#endif

#endif
