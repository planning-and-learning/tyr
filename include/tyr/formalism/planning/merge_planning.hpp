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

#ifndef TYR_FORMALISM_PLANNING_MERGE_PLANNING_HPP_
#define TYR_FORMALISM_PLANNING_MERGE_PLANNING_HPP_

#include "tyr/formalism/datalog/builder.hpp"
#include "tyr/formalism/datalog/canonicalization.hpp"
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/planning/builder.hpp"
#include "tyr/formalism/planning/canonicalization.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/merge_decl.hpp"
#include "tyr/formalism/planning/merge_planning_decl.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <yggdrasil/containers/associative_containers.hpp>
#include <yggdrasil/containers/tuple.hpp>
#include <yggdrasil/core/concepts.hpp>
#include <yggdrasil/semantics/equal_to.hpp>
#include <yggdrasil/semantics/hash.hpp>

namespace tyr::formalism::planning
{

// Common

std::pair<VariableView, bool> merge_d2p(::tyr::formalism::datalog::VariableView element, MergePlanningContext& context);

std::pair<ObjectView, bool> merge_d2p(::tyr::formalism::datalog::ObjectView element, MergePlanningContext& context);

ygg::Data<::tyr::formalism::Term> merge_d2p(::tyr::formalism::datalog::TermView element, MergePlanningContext& context);

// Propositional

template<FactKind T_SRC, FactKind T_DST>
std::pair<PredicateView<T_DST>, bool> merge_d2p(::tyr::formalism::datalog::PredicateView<T_SRC> element, MergePlanningContext& context);

template<FactKind T_SRC, FactKind T_DST>
std::pair<AtomView<T_DST>, bool> merge_d2p(::tyr::formalism::datalog::AtomView<T_SRC> element,
                                           const ygg::UnorderedMap<::tyr::formalism::datalog::PredicateView<T_SRC>, PredicateView<T_DST>>& predicate_mapping,
                                           MergePlanningContext& context);

template<FactKind T_SRC, FactKind T_DST>
std::pair<PredicateBindingView<T_DST>, bool>
merge_d2p(::tyr::formalism::datalog::PredicateBindingView<T_SRC> element,
          const ygg::UnorderedMap<::tyr::formalism::datalog::PredicateView<T_SRC>, PredicateView<T_DST>>& predicate_mapping,
          MergePlanningContext& context);

template<FactKind T_SRC, FactKind T_DST>
std::pair<GroundAtomView<T_DST>, bool>
merge_atom_d2p(::tyr::formalism::datalog::PredicateBindingView<T_SRC> element,
               const ygg::UnorderedMap<::tyr::formalism::datalog::PredicateView<T_SRC>, PredicateView<T_DST>>& predicate_mapping,
               MergePlanningContext& context);

template<FactKind T_SRC, FactKind T_DST>
std::pair<GroundAtomView<T_DST>, bool>
merge_d2p(::tyr::formalism::datalog::GroundAtomView<T_SRC> element,
          const ygg::UnorderedMap<::tyr::formalism::datalog::PredicateView<T_SRC>, PredicateView<T_DST>>& predicate_mapping,
          MergePlanningContext& context);

template<FactKind T_SRC, FactKind T_DST>
std::pair<LiteralView<T_DST>, bool> merge_d2p(::tyr::formalism::datalog::LiteralView<T_SRC> element,
                                              const ygg::UnorderedMap<::tyr::formalism::datalog::PredicateView<T_SRC>, PredicateView<T_DST>>& predicate_mapping,
                                              MergePlanningContext& context);

template<FactKind T_SRC, FactKind T_DST>
std::pair<GroundLiteralView<T_DST>, bool>
merge_d2p(::tyr::formalism::datalog::GroundLiteralView<T_SRC> element,
          const ygg::UnorderedMap<::tyr::formalism::datalog::PredicateView<T_SRC>, PredicateView<T_DST>>& predicate_mapping,
          MergePlanningContext& context);

// Numeric

template<FactKind T>
std::pair<FunctionView<T>, bool> merge_d2p(::tyr::formalism::datalog::FunctionView<T> element, MergePlanningContext& context);

template<FactKind T>
std::pair<FunctionTermView<T>, bool> merge_d2p(::tyr::formalism::datalog::FunctionTermView<T> element, MergePlanningContext& context);

template<FactKind T>
std::pair<FunctionBindingView<T>, bool> merge_d2p(::tyr::formalism::datalog::FunctionBindingView<T> element, MergePlanningContext& context);

template<FactKind T>
std::pair<GroundFunctionTermView<T>, bool> merge_d2p(::tyr::formalism::datalog::GroundFunctionTermView<T> element, MergePlanningContext& context);

template<FactKind T>
std::pair<GroundFunctionTermValueView<T>, bool> merge_d2p(::tyr::formalism::datalog::GroundFunctionTermValueView<T> element, MergePlanningContext& context);

ygg::Data<FunctionExpression> merge_d2p(::tyr::formalism::datalog::FunctionExpressionView element, MergePlanningContext& context);

ygg::Data<GroundFunctionExpression> merge_d2p(::tyr::formalism::datalog::GroundFunctionExpressionView element, MergePlanningContext& context);

template<typename T>
std::pair<UnaryOperatorView<to_planning_payload_t<T>>, bool> merge_d2p(::tyr::formalism::datalog::UnaryOperatorView<T> element, MergePlanningContext& context);

template<BinaryOperatorKind O, typename T>
std::pair<BinaryOperatorView<O, to_planning_payload_t<T>>, bool> merge_d2p(::tyr::formalism::datalog::BinaryOperatorView<O, T> element,
                                                                           MergePlanningContext& context);

template<typename T>
std::pair<MultiOperatorView<to_planning_payload_t<T>>, bool> merge_d2p(::tyr::formalism::datalog::MultiOperatorView<T> element, MergePlanningContext& context);

template<typename T>
ygg::Data<ArithmeticOperator<to_planning_payload_t<T>>> merge_d2p(::tyr::formalism::datalog::ArithmeticOperatorView<T> element, MergePlanningContext& context);

template<typename T>
ygg::Data<BooleanOperator<to_planning_payload_t<T>>> merge_d2p(::tyr::formalism::datalog::BooleanOperatorView<T> element, MergePlanningContext& context);

// Common

inline std::pair<VariableView, bool> merge_d2p(::tyr::formalism::datalog::VariableView element, MergePlanningContext& context)
{
    auto variable_ptr = context.builder.template get_builder<::tyr::formalism::Variable>();
    auto& variable = *variable_ptr;
    variable.clear();

    variable.name = element.get_name();

    canonicalize(variable);
    return context.destination.get_or_create(variable);
}

inline std::pair<ObjectView, bool> merge_d2p(::tyr::formalism::datalog::ObjectView element, MergePlanningContext& context)
{
    auto object_ptr = context.builder.template get_builder<::tyr::formalism::Object>();
    auto& object = *object_ptr;
    object.clear();

    object.name = element.get_name();

    canonicalize(object);
    return context.destination.get_or_create(object);
}

inline ygg::Data<::tyr::formalism::Term> merge_d2p(::tyr::formalism::datalog::TermView element, MergePlanningContext& context)
{
    return visit(
        [&](auto&& arg)
        {
            using Alternative = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<Alternative, ::tyr::formalism::ParameterIndex>)
                return ygg::Data<::tyr::formalism::Term>(arg);
            else if constexpr (std::is_same_v<Alternative, ::tyr::formalism::datalog::ObjectView>)
                return ygg::Data<::tyr::formalism::Term>(merge_d2p(arg, context).first.get_index());
            else
                static_assert(ygg::dependent_false<Alternative>::value, "Missing case");
        },
        element.get_variant());
}

// Propositional

template<FactKind T_SRC, FactKind T_DST>
std::pair<PredicateView<T_DST>, bool> merge_d2p(::tyr::formalism::datalog::PredicateView<T_SRC> element, MergePlanningContext& context)
{
    auto predicate_ptr = context.builder.template get_builder<::tyr::formalism::Predicate<T_DST>>();
    auto& predicate = *predicate_ptr;
    predicate.clear();

    predicate.name = element.get_name();
    predicate.arity = element.get_arity();

    canonicalize(predicate);
    return context.destination.get_or_create(predicate);
}

template<FactKind T_SRC, FactKind T_DST>
std::pair<AtomView<T_DST>, bool> merge_d2p(::tyr::formalism::datalog::AtomView<T_SRC> element,  //
                                           const ygg::UnorderedMap<::tyr::formalism::datalog::PredicateView<T_SRC>, PredicateView<T_DST>>& predicate_mapping,
                                           MergePlanningContext& context)
{
    auto atom_ptr = context.builder.template get_builder<Atom<T_DST>>();
    auto& atom = *atom_ptr;
    atom.clear();

    atom.predicate = predicate_mapping.at(element.get_predicate()).get_index();
    for (const auto term : element.get_terms())
        atom.terms.push_back(merge_d2p(term, context));

    canonicalize(atom);
    return context.destination.get_or_create(atom);
}

template<FactKind T_SRC, FactKind T_DST>
std::pair<PredicateBindingView<T_DST>, bool>
merge_d2p(::tyr::formalism::datalog::PredicateBindingView<T_SRC> element,  //
          const ygg::UnorderedMap<::tyr::formalism::datalog::PredicateView<T_SRC>, PredicateView<T_DST>>& predicate_mapping,
          MergePlanningContext& context)
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
std::pair<GroundAtomView<T_DST>, bool>
merge_atom_d2p(::tyr::formalism::datalog::PredicateBindingView<T_SRC> element,  //
               const ygg::UnorderedMap<::tyr::formalism::datalog::PredicateView<T_SRC>, PredicateView<T_DST>>& predicate_mapping,
               MergePlanningContext& context)
{
    auto atom_ptr = context.builder.template get_builder<GroundAtom<T_DST>>();
    auto& atom = *atom_ptr;
    atom.clear();

    atom.binding = merge_d2p<T_SRC, T_DST>(element, predicate_mapping, context).first.get_index();

    canonicalize(atom);
    return context.destination.get_or_create(atom);
}

template<FactKind T_SRC, FactKind T_DST>
std::pair<GroundAtomView<T_DST>, bool>
merge_d2p(::tyr::formalism::datalog::GroundAtomView<T_SRC> element,  //
          const ygg::UnorderedMap<::tyr::formalism::datalog::PredicateView<T_SRC>, PredicateView<T_DST>>& predicate_mapping,
          MergePlanningContext& context)
{
    auto atom_ptr = context.builder.template get_builder<GroundAtom<T_DST>>();
    auto& atom = *atom_ptr;
    atom.clear();

    atom.binding = merge_d2p<T_SRC, T_DST>(element.get_row(), predicate_mapping, context).first.get_index();

    canonicalize(atom);
    return context.destination.get_or_create(atom);
}

template<FactKind T_SRC, FactKind T_DST>
std::pair<LiteralView<T_DST>, bool> merge_d2p(::tyr::formalism::datalog::LiteralView<T_SRC> element,  //
                                              const ygg::UnorderedMap<::tyr::formalism::datalog::PredicateView<T_SRC>, PredicateView<T_DST>>& predicate_mapping,
                                              MergePlanningContext& context)
{
    auto literal_ptr = context.builder.template get_builder<Literal<T_DST>>();
    auto& literal = *literal_ptr;
    literal.clear();

    literal.polarity = element.get_polarity();
    literal.atom = merge_d2p<T_SRC, T_DST>(element.get_atom(), predicate_mapping, context).first.get_index();

    canonicalize(literal);
    return context.destination.get_or_create(literal);
}

template<FactKind T_SRC, FactKind T_DST>
std::pair<GroundLiteralView<T_DST>, bool>
merge_d2p(::tyr::formalism::datalog::GroundLiteralView<T_SRC> element,  //
          const ygg::UnorderedMap<::tyr::formalism::datalog::PredicateView<T_SRC>, PredicateView<T_DST>>& predicate_mapping,
          MergePlanningContext& context)
{
    auto literal_ptr = context.builder.template get_builder<GroundLiteral<T_DST>>();
    auto& literal = *literal_ptr;
    literal.clear();

    literal.polarity = element.get_polarity();
    literal.atom = merge_d2p<T_SRC, T_DST>(element.get_atom(), predicate_mapping, context).first.get_index();

    canonicalize(literal);
    return context.destination.get_or_create(literal);
}

// Numeric

template<FactKind T>
std::pair<FunctionView<T>, bool> merge_d2p(::tyr::formalism::datalog::FunctionView<T> element, MergePlanningContext& context)
{
    auto function_ptr = context.builder.template get_builder<::tyr::formalism::Function<T>>();
    auto& function = *function_ptr;
    function.clear();

    function.name = element.get_name();
    function.arity = element.get_arity();

    canonicalize(function);
    return context.destination.get_or_create(function);
}

template<FactKind T>
std::pair<FunctionTermView<T>, bool> merge_d2p(::tyr::formalism::datalog::FunctionTermView<T> element, MergePlanningContext& context)
{
    auto fterm_ptr = context.builder.template get_builder<FunctionTerm<T>>();
    auto& fterm = *fterm_ptr;
    fterm.clear();

    fterm.function = element.get_function().get_index();
    for (const auto term : element.get_terms())
        fterm.terms.push_back(merge_d2p(term, context));

    canonicalize(fterm);
    return context.destination.get_or_create(fterm);
}

template<FactKind T>
std::pair<FunctionBindingView<T>, bool> merge_d2p(::tyr::formalism::datalog::FunctionBindingView<T> element, MergePlanningContext& context)
{
    auto binding_ptr = context.builder.template get_builder<RelationBinding<Function<T>>>();
    auto& binding = *binding_ptr;
    binding.clear();

    binding.relation = merge_d2p(element.get_relation(), context).first.get_index();
    for (const auto object : element.get_objects())
        binding.objects.push_back(object.get_index());

    canonicalize(binding);
    return context.destination.get_or_create(binding);
}

template<FactKind T>
std::pair<GroundFunctionTermView<T>, bool> merge_d2p(::tyr::formalism::datalog::GroundFunctionTermView<T> element, MergePlanningContext& context)
{
    auto fterm_ptr = context.builder.template get_builder<GroundFunctionTerm<T>>();
    auto& fterm = *fterm_ptr;
    fterm.clear();

    fterm.binding = merge_d2p(element.get_row(), context).first.get_index();

    canonicalize(fterm);
    return context.destination.get_or_create(fterm);
}

template<FactKind T>
std::pair<GroundFunctionTermValueView<T>, bool> merge_d2p(::tyr::formalism::datalog::GroundFunctionTermValueView<T> element, MergePlanningContext& context)
{
    auto fterm_value_ptr = context.builder.template get_builder<GroundFunctionTermValue<T>>();
    auto& fterm_value = *fterm_value_ptr;
    fterm_value.clear();

    fterm_value.fterm = merge_d2p(element.get_fterm(), context).first.get_index();
    fterm_value.value = element.get_value();

    canonicalize(fterm_value);
    return context.destination.get_or_create(fterm_value);
}

inline ygg::Data<FunctionExpression> merge_d2p(::tyr::formalism::datalog::FunctionExpressionView element, MergePlanningContext& context)
{
    return visit(
        [&](auto&& arg)
        {
            using Alternative = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<Alternative, ygg::float_t>)
                return ygg::Data<FunctionExpression>(arg);
            else if constexpr (std::is_same_v<Alternative, ::tyr::formalism::datalog::LiftedArithmeticOperatorView>)
                return ygg::Data<FunctionExpression>(merge_d2p(arg, context));
            else if constexpr (std::is_same_v<Alternative, ::tyr::formalism::datalog::FunctionTermView<AuxiliaryTag>>)
                throw std::logic_error("AuxiliaryTag FunctionTerm must not be merged.");
            else
                return ygg::Data<FunctionExpression>(merge_d2p(arg, context).first.get_index());
        },
        element.get_variant());
}

inline ygg::Data<GroundFunctionExpression> merge_d2p(::tyr::formalism::datalog::GroundFunctionExpressionView element, MergePlanningContext& context)
{
    return visit(
        [&](auto&& arg)
        {
            using Alternative = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<Alternative, ygg::float_t>)
                return ygg::Data<GroundFunctionExpression>(arg);
            else if constexpr (std::is_same_v<Alternative, ::tyr::formalism::datalog::GroundArithmeticOperatorView>)
                return ygg::Data<GroundFunctionExpression>(merge_d2p(arg, context));
            else
                return ygg::Data<GroundFunctionExpression>(merge_d2p(arg, context).first.get_index());
        },
        element.get_variant());
}

template<typename T>
std::pair<UnaryOperatorView<to_planning_payload_t<T>>, bool> merge_d2p(::tyr::formalism::datalog::UnaryOperatorView<T> element, MergePlanningContext& context)
{
    using T_DST = to_planning_payload_t<T>;

    auto unary_ptr = context.builder.template get_builder<UnaryOperator<T_DST>>();
    auto& unary = *unary_ptr;
    unary.clear();

    unary.operator_kind = element.get_operator();
    unary.arg = merge_d2p(element.get_arg(), context);

    canonicalize(unary);
    return context.destination.get_or_create(unary);
}

template<BinaryOperatorKind O, typename T>
std::pair<BinaryOperatorView<O, to_planning_payload_t<T>>, bool> merge_d2p(::tyr::formalism::datalog::BinaryOperatorView<O, T> element,
                                                                           MergePlanningContext& context)
{
    using T_DST = to_planning_payload_t<T>;

    auto binary_ptr = context.builder.template get_builder<BinaryOperator<O, T_DST>>();
    auto& binary = *binary_ptr;
    binary.clear();

    binary.operator_kind = element.get_operator();
    binary.lhs = merge_d2p(element.get_lhs(), context);
    binary.rhs = merge_d2p(element.get_rhs(), context);

    canonicalize(binary);
    return context.destination.get_or_create(binary);
}

template<typename T>
std::pair<MultiOperatorView<to_planning_payload_t<T>>, bool> merge_d2p(::tyr::formalism::datalog::MultiOperatorView<T> element, MergePlanningContext& context)
{
    using T_DST = to_planning_payload_t<T>;

    auto multi_ptr = context.builder.template get_builder<MultiOperator<T_DST>>();
    auto& multi = *multi_ptr;
    multi.clear();

    multi.operator_kind = element.get_operator();
    for (const auto arg : element.get_args())
        multi.args.push_back(merge_d2p(arg, context));

    canonicalize(multi);
    return context.destination.get_or_create(multi);
}

template<typename T>
ygg::Data<ArithmeticOperator<to_planning_payload_t<T>>> merge_d2p(::tyr::formalism::datalog::ArithmeticOperatorView<T> element, MergePlanningContext& context)
{
    using T_DST = to_planning_payload_t<T>;

    return visit([&](auto&& arg) { return ygg::Data<ArithmeticOperator<T_DST>>(merge_d2p(arg, context).first.get_index()); }, element.get_variant());
}

template<typename T>
ygg::Data<BooleanOperator<to_planning_payload_t<T>>> merge_d2p(::tyr::formalism::datalog::BooleanOperatorView<T> element, MergePlanningContext& context)
{
    using T_DST = to_planning_payload_t<T>;

    return visit([&](auto&& arg) { return ygg::Data<BooleanOperator<T_DST>>(merge_d2p(arg, context).first.get_index()); }, element.get_variant());
}

}

#ifndef TYR_HEADER_INSTANTIATION

namespace tyr::formalism::planning
{
extern template std::pair<PredicateView<StaticTag>, bool> merge_d2p(::tyr::formalism::datalog::PredicateView<StaticTag> element, MergePlanningContext& context);
extern template std::pair<PredicateView<FluentTag>, bool> merge_d2p(::tyr::formalism::datalog::PredicateView<FluentTag> element, MergePlanningContext& context);
extern template std::pair<PredicateView<DerivedTag>, bool> merge_d2p(::tyr::formalism::datalog::PredicateView<FluentTag> element,
                                                                     MergePlanningContext& context);

extern template std::pair<AtomView<StaticTag>, bool>
merge_d2p(::tyr::formalism::datalog::AtomView<StaticTag> element,
          const ygg::UnorderedMap<::tyr::formalism::datalog::PredicateView<StaticTag>, PredicateView<StaticTag>>& predicate_mapping,
          MergePlanningContext& context);
extern template std::pair<AtomView<FluentTag>, bool>
merge_d2p(::tyr::formalism::datalog::AtomView<FluentTag> element,
          const ygg::UnorderedMap<::tyr::formalism::datalog::PredicateView<FluentTag>, PredicateView<FluentTag>>& predicate_mapping,
          MergePlanningContext& context);
extern template std::pair<AtomView<DerivedTag>, bool>
merge_d2p(::tyr::formalism::datalog::AtomView<FluentTag> element,
          const ygg::UnorderedMap<::tyr::formalism::datalog::PredicateView<FluentTag>, PredicateView<DerivedTag>>& predicate_mapping,
          MergePlanningContext& context);

extern template std::pair<PredicateBindingView<StaticTag>, bool>
merge_d2p(::tyr::formalism::datalog::PredicateBindingView<StaticTag> element,
          const ygg::UnorderedMap<::tyr::formalism::datalog::PredicateView<StaticTag>, PredicateView<StaticTag>>& predicate_mapping,
          MergePlanningContext& context);
extern template std::pair<PredicateBindingView<FluentTag>, bool>
merge_d2p(::tyr::formalism::datalog::PredicateBindingView<FluentTag> element,
          const ygg::UnorderedMap<::tyr::formalism::datalog::PredicateView<FluentTag>, PredicateView<FluentTag>>& predicate_mapping,
          MergePlanningContext& context);
extern template std::pair<PredicateBindingView<DerivedTag>, bool>
merge_d2p(::tyr::formalism::datalog::PredicateBindingView<FluentTag> element,
          const ygg::UnorderedMap<::tyr::formalism::datalog::PredicateView<FluentTag>, PredicateView<DerivedTag>>& predicate_mapping,
          MergePlanningContext& context);

extern template std::pair<GroundAtomView<StaticTag>, bool>
merge_atom_d2p<StaticTag, StaticTag>(::tyr::formalism::datalog::PredicateBindingView<StaticTag> element,
                                     const ygg::UnorderedMap<::tyr::formalism::datalog::PredicateView<StaticTag>, PredicateView<StaticTag>>& predicate_mapping,
                                     MergePlanningContext& context);
extern template std::pair<GroundAtomView<FluentTag>, bool>
merge_atom_d2p<FluentTag, FluentTag>(::tyr::formalism::datalog::PredicateBindingView<FluentTag> element,
                                     const ygg::UnorderedMap<::tyr::formalism::datalog::PredicateView<FluentTag>, PredicateView<FluentTag>>& predicate_mapping,
                                     MergePlanningContext& context);
extern template std::pair<GroundAtomView<DerivedTag>, bool> merge_atom_d2p<FluentTag, DerivedTag>(
    ::tyr::formalism::datalog::PredicateBindingView<FluentTag> element,
    const ygg::UnorderedMap<::tyr::formalism::datalog::PredicateView<FluentTag>, PredicateView<DerivedTag>>& predicate_mapping,
    MergePlanningContext& context);

extern template std::pair<GroundAtomView<StaticTag>, bool>
merge_d2p(::tyr::formalism::datalog::GroundAtomView<StaticTag> element,
          const ygg::UnorderedMap<::tyr::formalism::datalog::PredicateView<StaticTag>, PredicateView<StaticTag>>& predicate_mapping,
          MergePlanningContext& context);
extern template std::pair<GroundAtomView<FluentTag>, bool>
merge_d2p(::tyr::formalism::datalog::GroundAtomView<FluentTag> element,
          const ygg::UnorderedMap<::tyr::formalism::datalog::PredicateView<FluentTag>, PredicateView<FluentTag>>& predicate_mapping,
          MergePlanningContext& context);
extern template std::pair<GroundAtomView<DerivedTag>, bool>
merge_d2p(::tyr::formalism::datalog::GroundAtomView<FluentTag> element,
          const ygg::UnorderedMap<::tyr::formalism::datalog::PredicateView<FluentTag>, PredicateView<DerivedTag>>& predicate_mapping,
          MergePlanningContext& context);

extern template std::pair<LiteralView<StaticTag>, bool>
merge_d2p(::tyr::formalism::datalog::LiteralView<StaticTag> element,
          const ygg::UnorderedMap<::tyr::formalism::datalog::PredicateView<StaticTag>, PredicateView<StaticTag>>& predicate_mapping,
          MergePlanningContext& context);
extern template std::pair<LiteralView<FluentTag>, bool>
merge_d2p(::tyr::formalism::datalog::LiteralView<FluentTag> element,
          const ygg::UnorderedMap<::tyr::formalism::datalog::PredicateView<FluentTag>, PredicateView<FluentTag>>& predicate_mapping,
          MergePlanningContext& context);
extern template std::pair<LiteralView<DerivedTag>, bool>
merge_d2p(::tyr::formalism::datalog::LiteralView<FluentTag> element,
          const ygg::UnorderedMap<::tyr::formalism::datalog::PredicateView<FluentTag>, PredicateView<DerivedTag>>& predicate_mapping,
          MergePlanningContext& context);

extern template std::pair<GroundLiteralView<StaticTag>, bool>
merge_d2p(::tyr::formalism::datalog::GroundLiteralView<StaticTag> element,
          const ygg::UnorderedMap<::tyr::formalism::datalog::PredicateView<StaticTag>, PredicateView<StaticTag>>& predicate_mapping,
          MergePlanningContext& context);
extern template std::pair<GroundLiteralView<FluentTag>, bool>
merge_d2p(::tyr::formalism::datalog::GroundLiteralView<FluentTag> element,
          const ygg::UnorderedMap<::tyr::formalism::datalog::PredicateView<FluentTag>, PredicateView<FluentTag>>& predicate_mapping,
          MergePlanningContext& context);
extern template std::pair<GroundLiteralView<DerivedTag>, bool>
merge_d2p(::tyr::formalism::datalog::GroundLiteralView<FluentTag> element,
          const ygg::UnorderedMap<::tyr::formalism::datalog::PredicateView<FluentTag>, PredicateView<DerivedTag>>& predicate_mapping,
          MergePlanningContext& context);

// Numeric

extern template std::pair<FunctionView<StaticTag>, bool> merge_d2p(::tyr::formalism::datalog::FunctionView<StaticTag> element, MergePlanningContext& context);
extern template std::pair<FunctionView<FluentTag>, bool> merge_d2p(::tyr::formalism::datalog::FunctionView<FluentTag> element, MergePlanningContext& context);

extern template std::pair<FunctionTermView<StaticTag>, bool> merge_d2p(::tyr::formalism::datalog::FunctionTermView<StaticTag> element,
                                                                       MergePlanningContext& context);
extern template std::pair<FunctionTermView<FluentTag>, bool> merge_d2p(::tyr::formalism::datalog::FunctionTermView<FluentTag> element,
                                                                       MergePlanningContext& context);

extern template std::pair<FunctionBindingView<StaticTag>, bool> merge_d2p(::tyr::formalism::datalog::FunctionBindingView<StaticTag> element,
                                                                          MergePlanningContext& context);
extern template std::pair<FunctionBindingView<FluentTag>, bool> merge_d2p(::tyr::formalism::datalog::FunctionBindingView<FluentTag> element,
                                                                          MergePlanningContext& context);

extern template std::pair<GroundFunctionTermView<StaticTag>, bool> merge_d2p(::tyr::formalism::datalog::GroundFunctionTermView<StaticTag> element,
                                                                             MergePlanningContext& context);
extern template std::pair<GroundFunctionTermView<FluentTag>, bool> merge_d2p(::tyr::formalism::datalog::GroundFunctionTermView<FluentTag> element,
                                                                             MergePlanningContext& context);

extern template std::pair<GroundFunctionTermValueView<StaticTag>, bool> merge_d2p(::tyr::formalism::datalog::GroundFunctionTermValueView<StaticTag> element,
                                                                                  MergePlanningContext& context);
extern template std::pair<GroundFunctionTermValueView<FluentTag>, bool> merge_d2p(::tyr::formalism::datalog::GroundFunctionTermValueView<FluentTag> element,
                                                                                  MergePlanningContext& context);

extern template std::pair<UnaryOperatorView<ygg::Data<FunctionExpression>>, bool>
merge_d2p(::tyr::formalism::datalog::UnaryOperatorView<ygg::Data<::tyr::formalism::datalog::FunctionExpression>> element, MergePlanningContext& context);
extern template std::pair<UnaryOperatorView<ygg::Data<GroundFunctionExpression>>, bool>
merge_d2p(::tyr::formalism::datalog::UnaryOperatorView<ygg::Data<::tyr::formalism::datalog::GroundFunctionExpression>> element, MergePlanningContext& context);

extern template std::pair<BinaryOperatorView<BooleanOperatorKind, ygg::Data<FunctionExpression>>, bool>
merge_d2p(::tyr::formalism::datalog::BinaryOperatorView<BooleanOperatorKind, ygg::Data<::tyr::formalism::datalog::FunctionExpression>> element,
          MergePlanningContext& context);
extern template std::pair<BinaryOperatorView<ArithmeticOperatorKind, ygg::Data<FunctionExpression>>, bool>
merge_d2p(::tyr::formalism::datalog::BinaryOperatorView<ArithmeticOperatorKind, ygg::Data<::tyr::formalism::datalog::FunctionExpression>> element,
          MergePlanningContext& context);
extern template std::pair<BinaryOperatorView<BooleanOperatorKind, ygg::Data<GroundFunctionExpression>>, bool>
merge_d2p(::tyr::formalism::datalog::BinaryOperatorView<BooleanOperatorKind, ygg::Data<::tyr::formalism::datalog::GroundFunctionExpression>> element,
          MergePlanningContext& context);
extern template std::pair<BinaryOperatorView<ArithmeticOperatorKind, ygg::Data<GroundFunctionExpression>>, bool>
merge_d2p(::tyr::formalism::datalog::BinaryOperatorView<ArithmeticOperatorKind, ygg::Data<::tyr::formalism::datalog::GroundFunctionExpression>> element,
          MergePlanningContext& context);

extern template std::pair<MultiOperatorView<ygg::Data<FunctionExpression>>, bool>
merge_d2p(::tyr::formalism::datalog::MultiOperatorView<ygg::Data<::tyr::formalism::datalog::FunctionExpression>> element, MergePlanningContext& context);
extern template std::pair<MultiOperatorView<ygg::Data<GroundFunctionExpression>>, bool>
merge_d2p(::tyr::formalism::datalog::MultiOperatorView<ygg::Data<::tyr::formalism::datalog::GroundFunctionExpression>> element, MergePlanningContext& context);

extern template ygg::Data<ArithmeticOperator<ygg::Data<FunctionExpression>>>
merge_d2p(::tyr::formalism::datalog::ArithmeticOperatorView<ygg::Data<::tyr::formalism::datalog::FunctionExpression>> element, MergePlanningContext& context);
extern template ygg::Data<ArithmeticOperator<ygg::Data<GroundFunctionExpression>>>
merge_d2p(::tyr::formalism::datalog::ArithmeticOperatorView<ygg::Data<::tyr::formalism::datalog::GroundFunctionExpression>> element,
          MergePlanningContext& context);

extern template ygg::Data<BooleanOperator<ygg::Data<FunctionExpression>>>
merge_d2p(::tyr::formalism::datalog::BooleanOperatorView<ygg::Data<::tyr::formalism::datalog::FunctionExpression>> element, MergePlanningContext& context);
extern template ygg::Data<BooleanOperator<ygg::Data<GroundFunctionExpression>>>
merge_d2p(::tyr::formalism::datalog::BooleanOperatorView<ygg::Data<::tyr::formalism::datalog::GroundFunctionExpression>> element,
          MergePlanningContext& context);
}

#endif

#endif
