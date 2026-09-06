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

#include "tyr/formalism/datalog/canonicalization.hpp"
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/datalog/repository.hpp"
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
std::pair<AtomView<::tyr::LiftedTag, T_DST>, bool>
merge_d2p(::tyr::formalism::datalog::AtomView<::tyr::LiftedTag, T_SRC> element,
          const ygg::UnorderedMap<::tyr::formalism::datalog::PredicateView<T_SRC>, PredicateView<T_DST>>& predicate_mapping,
          MergePlanningContext& context);

template<FactKind T_SRC, FactKind T_DST>
std::pair<PredicateBindingView<T_DST>, bool>
merge_d2p(::tyr::formalism::datalog::PredicateBindingView<T_SRC> element,
          const ygg::UnorderedMap<::tyr::formalism::datalog::PredicateView<T_SRC>, PredicateView<T_DST>>& predicate_mapping,
          MergePlanningContext& context);

template<FactKind T_SRC, FactKind T_DST>
std::pair<AtomView<::tyr::GroundTag, T_DST>, bool>
merge_atom_d2p(::tyr::formalism::datalog::PredicateBindingView<T_SRC> element,
               const ygg::UnorderedMap<::tyr::formalism::datalog::PredicateView<T_SRC>, PredicateView<T_DST>>& predicate_mapping,
               MergePlanningContext& context);

template<FactKind T_SRC, FactKind T_DST>
std::pair<AtomView<::tyr::GroundTag, T_DST>, bool>
merge_d2p(::tyr::formalism::datalog::AtomView<::tyr::GroundTag, T_SRC> element,
          const ygg::UnorderedMap<::tyr::formalism::datalog::PredicateView<T_SRC>, PredicateView<T_DST>>& predicate_mapping,
          MergePlanningContext& context);

template<FactKind T_SRC, FactKind T_DST>
std::pair<LiteralView<::tyr::LiftedTag, T_DST>, bool>
merge_d2p(::tyr::formalism::datalog::LiteralView<::tyr::LiftedTag, T_SRC> element,
          const ygg::UnorderedMap<::tyr::formalism::datalog::PredicateView<T_SRC>, PredicateView<T_DST>>& predicate_mapping,
          MergePlanningContext& context);

template<FactKind T_SRC, FactKind T_DST>
std::pair<LiteralView<::tyr::GroundTag, T_DST>, bool>
merge_d2p(::tyr::formalism::datalog::LiteralView<::tyr::GroundTag, T_SRC> element,
          const ygg::UnorderedMap<::tyr::formalism::datalog::PredicateView<T_SRC>, PredicateView<T_DST>>& predicate_mapping,
          MergePlanningContext& context);

// Numeric

template<FactKind T>
std::pair<FunctionView<T>, bool> merge_d2p(::tyr::formalism::datalog::FunctionView<T> element, MergePlanningContext& context);

template<FactKind T>
std::pair<FunctionTermView<::tyr::LiftedTag, T>, bool> merge_d2p(::tyr::formalism::datalog::FunctionTermView<::tyr::LiftedTag, T> element,
                                                                 MergePlanningContext& context);

template<FactKind T>
std::pair<FunctionBindingView<T>, bool> merge_d2p(::tyr::formalism::datalog::FunctionBindingView<T> element, MergePlanningContext& context);

template<FactKind T>
std::pair<FunctionTermView<::tyr::GroundTag, T>, bool> merge_d2p(::tyr::formalism::datalog::FunctionTermView<::tyr::GroundTag, T> element,
                                                                 MergePlanningContext& context);

template<FactKind T>
std::pair<FunctionTermValueView<::tyr::GroundTag, T>, bool> merge_d2p(::tyr::formalism::datalog::FunctionTermValueView<::tyr::GroundTag, T> element,
                                                                      MergePlanningContext& context);

ygg::Data<FunctionExpression<::tyr::LiftedTag>> merge_d2p(::tyr::formalism::datalog::FunctionExpressionView<::tyr::LiftedTag> element,
                                                          MergePlanningContext& context);

ygg::Data<FunctionExpression<::tyr::GroundTag>> merge_d2p(::tyr::formalism::datalog::FunctionExpressionView<::tyr::GroundTag> element,
                                                          MergePlanningContext& context);

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
    auto variable = ::tyr::formalism::planning::checkout<::tyr::formalism::Variable>(context.builder);

    variable->name = element.get_name();

    return ::tyr::formalism::planning::get_or_create(context.destination, *variable);
}

inline std::pair<ObjectView, bool> merge_d2p(::tyr::formalism::datalog::ObjectView element, MergePlanningContext& context)
{
    auto object = ::tyr::formalism::planning::checkout<::tyr::formalism::Object>(context.builder);

    object->name = element.get_name();

    return ::tyr::formalism::planning::get_or_create(context.destination, *object);
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
    auto predicate = ::tyr::formalism::planning::checkout<::tyr::formalism::Predicate<T_DST>>(context.builder);

    predicate->name = element.get_name();
    predicate->arity = element.get_arity();

    return ::tyr::formalism::planning::get_or_create(context.destination, *predicate);
}

template<FactKind T_SRC, FactKind T_DST>
std::pair<AtomView<::tyr::LiftedTag, T_DST>, bool>
merge_d2p(::tyr::formalism::datalog::AtomView<::tyr::LiftedTag, T_SRC> element,  //
          const ygg::UnorderedMap<::tyr::formalism::datalog::PredicateView<T_SRC>, PredicateView<T_DST>>& predicate_mapping,
          MergePlanningContext& context)
{
    auto atom = ::tyr::formalism::planning::checkout<Atom<::tyr::LiftedTag, T_DST>>(context.builder);

    atom->predicate = predicate_mapping.at(element.get_predicate()).get_index();
    for (const auto term : element.get_terms())
        atom->terms.push_back(merge_d2p(term, context));

    return ::tyr::formalism::planning::get_or_create(context.destination, *atom);
}

template<FactKind T_SRC, FactKind T_DST>
std::pair<PredicateBindingView<T_DST>, bool>
merge_d2p(::tyr::formalism::datalog::PredicateBindingView<T_SRC> element,  //
          const ygg::UnorderedMap<::tyr::formalism::datalog::PredicateView<T_SRC>, PredicateView<T_DST>>& predicate_mapping,
          MergePlanningContext& context)
{
    auto binding = ::tyr::formalism::planning::checkout<RelationBinding<Predicate<T_DST>>>(context.builder);

    binding->relation = predicate_mapping.at(element.get_relation()).get_index();
    for (const auto object : element.get_objects())
        binding->objects.push_back(object.get_index());

    return ::tyr::formalism::planning::get_or_create(context.destination, *binding);
}

template<FactKind T_SRC, FactKind T_DST>
std::pair<AtomView<::tyr::GroundTag, T_DST>, bool>
merge_atom_d2p(::tyr::formalism::datalog::PredicateBindingView<T_SRC> element,  //
               const ygg::UnorderedMap<::tyr::formalism::datalog::PredicateView<T_SRC>, PredicateView<T_DST>>& predicate_mapping,
               MergePlanningContext& context)
{
    auto atom = ::tyr::formalism::planning::checkout<Atom<::tyr::GroundTag, T_DST>>(context.builder);

    atom->binding = merge_d2p<T_SRC, T_DST>(element, predicate_mapping, context).first.get_index();

    return ::tyr::formalism::planning::get_or_create(context.destination, *atom);
}

template<FactKind T_SRC, FactKind T_DST>
std::pair<AtomView<::tyr::GroundTag, T_DST>, bool>
merge_d2p(::tyr::formalism::datalog::AtomView<::tyr::GroundTag, T_SRC> element,  //
          const ygg::UnorderedMap<::tyr::formalism::datalog::PredicateView<T_SRC>, PredicateView<T_DST>>& predicate_mapping,
          MergePlanningContext& context)
{
    auto atom = ::tyr::formalism::planning::checkout<Atom<::tyr::GroundTag, T_DST>>(context.builder);

    atom->binding = merge_d2p<T_SRC, T_DST>(element.get_row(), predicate_mapping, context).first.get_index();

    return ::tyr::formalism::planning::get_or_create(context.destination, *atom);
}

template<FactKind T_SRC, FactKind T_DST>
std::pair<LiteralView<::tyr::LiftedTag, T_DST>, bool>
merge_d2p(::tyr::formalism::datalog::LiteralView<::tyr::LiftedTag, T_SRC> element,  //
          const ygg::UnorderedMap<::tyr::formalism::datalog::PredicateView<T_SRC>, PredicateView<T_DST>>& predicate_mapping,
          MergePlanningContext& context)
{
    auto literal = ::tyr::formalism::planning::checkout<Literal<::tyr::LiftedTag, T_DST>>(context.builder);

    literal->polarity = element.get_polarity();
    literal->atom = merge_d2p<T_SRC, T_DST>(element.get_atom(), predicate_mapping, context).first.get_index();

    return ::tyr::formalism::planning::get_or_create(context.destination, *literal);
}

template<FactKind T_SRC, FactKind T_DST>
std::pair<LiteralView<::tyr::GroundTag, T_DST>, bool>
merge_d2p(::tyr::formalism::datalog::LiteralView<::tyr::GroundTag, T_SRC> element,  //
          const ygg::UnorderedMap<::tyr::formalism::datalog::PredicateView<T_SRC>, PredicateView<T_DST>>& predicate_mapping,
          MergePlanningContext& context)
{
    auto literal = ::tyr::formalism::planning::checkout<Literal<::tyr::GroundTag, T_DST>>(context.builder);

    literal->polarity = element.get_polarity();
    literal->atom = merge_d2p<T_SRC, T_DST>(element.get_atom(), predicate_mapping, context).first.get_index();

    return ::tyr::formalism::planning::get_or_create(context.destination, *literal);
}

// Numeric

template<FactKind T>
std::pair<FunctionView<T>, bool> merge_d2p(::tyr::formalism::datalog::FunctionView<T> element, MergePlanningContext& context)
{
    auto function = ::tyr::formalism::planning::checkout<::tyr::formalism::Function<T>>(context.builder);

    function->name = element.get_name();
    function->arity = element.get_arity();

    return ::tyr::formalism::planning::get_or_create(context.destination, *function);
}

template<FactKind T>
std::pair<FunctionTermView<::tyr::LiftedTag, T>, bool> merge_d2p(::tyr::formalism::datalog::FunctionTermView<::tyr::LiftedTag, T> element,
                                                                 MergePlanningContext& context)
{
    auto fterm = ::tyr::formalism::planning::checkout<FunctionTerm<::tyr::LiftedTag, T>>(context.builder);

    fterm->function = element.get_function().get_index();
    for (const auto term : element.get_terms())
        fterm->terms.push_back(merge_d2p(term, context));

    return ::tyr::formalism::planning::get_or_create(context.destination, *fterm);
}

template<FactKind T>
std::pair<FunctionBindingView<T>, bool> merge_d2p(::tyr::formalism::datalog::FunctionBindingView<T> element, MergePlanningContext& context)
{
    auto binding = ::tyr::formalism::planning::checkout<RelationBinding<Function<T>>>(context.builder);

    binding->relation = merge_d2p(element.get_relation(), context).first.get_index();
    for (const auto object : element.get_objects())
        binding->objects.push_back(object.get_index());

    return ::tyr::formalism::planning::get_or_create(context.destination, *binding);
}

template<FactKind T>
std::pair<FunctionTermView<::tyr::GroundTag, T>, bool> merge_d2p(::tyr::formalism::datalog::FunctionTermView<::tyr::GroundTag, T> element,
                                                                 MergePlanningContext& context)
{
    auto fterm = ::tyr::formalism::planning::checkout<FunctionTerm<::tyr::GroundTag, T>>(context.builder);

    fterm->binding = merge_d2p(element.get_row(), context).first.get_index();

    return ::tyr::formalism::planning::get_or_create(context.destination, *fterm);
}

template<FactKind T>
std::pair<FunctionTermValueView<::tyr::GroundTag, T>, bool> merge_d2p(::tyr::formalism::datalog::FunctionTermValueView<::tyr::GroundTag, T> element,
                                                                      MergePlanningContext& context)
{
    auto fterm_value = ::tyr::formalism::planning::checkout<FunctionTermValue<::tyr::GroundTag, T>>(context.builder);

    fterm_value->fterm = merge_d2p(element.get_fterm(), context).first.get_index();
    fterm_value->value = element.get_value();

    return ::tyr::formalism::planning::get_or_create(context.destination, *fterm_value);
}

inline ygg::Data<FunctionExpression<::tyr::LiftedTag>> merge_d2p(::tyr::formalism::datalog::FunctionExpressionView<::tyr::LiftedTag> element,
                                                                 MergePlanningContext& context)
{
    return visit(
        [&](auto&& arg)
        {
            using Alternative = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<Alternative, ygg::float_t>)
                return ygg::Data<FunctionExpression<::tyr::LiftedTag>>(arg);
            else if constexpr (std::is_same_v<Alternative, ::tyr::formalism::datalog::LiftedArithmeticOperatorView>)
                return ygg::Data<FunctionExpression<::tyr::LiftedTag>>(merge_d2p(arg, context));
            else if constexpr (std::is_same_v<Alternative, ::tyr::formalism::datalog::FunctionTermView<::tyr::LiftedTag, AuxiliaryTag>>)
                throw std::logic_error("AuxiliaryTag FunctionTerm must not be merged.");
            else
                return ygg::Data<FunctionExpression<::tyr::LiftedTag>>(merge_d2p(arg, context).first.get_index());
        },
        element.get_variant());
}

inline ygg::Data<FunctionExpression<::tyr::GroundTag>> merge_d2p(::tyr::formalism::datalog::FunctionExpressionView<::tyr::GroundTag> element,
                                                                 MergePlanningContext& context)
{
    return visit(
        [&](auto&& arg)
        {
            using Alternative = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<Alternative, ygg::float_t>)
                return ygg::Data<FunctionExpression<::tyr::GroundTag>>(arg);
            else if constexpr (std::is_same_v<Alternative, ::tyr::formalism::datalog::GroundArithmeticOperatorView>)
                return ygg::Data<FunctionExpression<::tyr::GroundTag>>(merge_d2p(arg, context));
            else
                return ygg::Data<FunctionExpression<::tyr::GroundTag>>(merge_d2p(arg, context).first.get_index());
        },
        element.get_variant());
}

template<typename T>
std::pair<UnaryOperatorView<to_planning_payload_t<T>>, bool> merge_d2p(::tyr::formalism::datalog::UnaryOperatorView<T> element, MergePlanningContext& context)
{
    using T_DST = to_planning_payload_t<T>;

    auto unary = ::tyr::formalism::planning::checkout<UnaryOperator<T_DST>>(context.builder);

    unary->operator_kind = element.get_operator();
    unary->arg = merge_d2p(element.get_arg(), context);

    return ::tyr::formalism::planning::get_or_create(context.destination, *unary);
}

template<BinaryOperatorKind O, typename T>
std::pair<BinaryOperatorView<O, to_planning_payload_t<T>>, bool> merge_d2p(::tyr::formalism::datalog::BinaryOperatorView<O, T> element,
                                                                           MergePlanningContext& context)
{
    using T_DST = to_planning_payload_t<T>;

    auto binary = ::tyr::formalism::planning::checkout<BinaryOperator<O, T_DST>>(context.builder);

    binary->operator_kind = element.get_operator();
    binary->lhs = merge_d2p(element.get_lhs(), context);
    binary->rhs = merge_d2p(element.get_rhs(), context);

    return ::tyr::formalism::planning::get_or_create(context.destination, *binary);
}

template<typename T>
std::pair<MultiOperatorView<to_planning_payload_t<T>>, bool> merge_d2p(::tyr::formalism::datalog::MultiOperatorView<T> element, MergePlanningContext& context)
{
    using T_DST = to_planning_payload_t<T>;

    auto multi = ::tyr::formalism::planning::checkout<MultiOperator<T_DST>>(context.builder);

    multi->operator_kind = element.get_operator();
    for (const auto arg : element.get_args())
        multi->args.push_back(merge_d2p(arg, context));

    return ::tyr::formalism::planning::get_or_create(context.destination, *multi);
}

template<typename T>
ygg::Data<ArithmeticOperator<to_planning_payload_t<T>>> merge_d2p(::tyr::formalism::datalog::ArithmeticOperatorView<T> element, MergePlanningContext& context)
{
    using T_DST = to_planning_payload_t<T>;

    return visit([&](auto&& arg) { return ygg::Data<ArithmeticOperator<T_DST>>(arg.get_operator(), merge_d2p(arg, context).first.get_index()); },
                 element.get_variant());
}

template<typename T>
ygg::Data<BooleanOperator<to_planning_payload_t<T>>> merge_d2p(::tyr::formalism::datalog::BooleanOperatorView<T> element, MergePlanningContext& context)
{
    using T_DST = to_planning_payload_t<T>;

    return visit([&](auto&& arg) { return ygg::Data<BooleanOperator<T_DST>>(arg.get_operator(), merge_d2p(arg, context).first.get_index()); },
                 element.get_variant());
}

}

#ifndef TYR_HEADER_INSTANTIATION

namespace tyr::formalism::planning
{
extern template std::pair<PredicateView<StaticTag>, bool> merge_d2p(::tyr::formalism::datalog::PredicateView<StaticTag> element, MergePlanningContext& context);
extern template std::pair<PredicateView<FluentTag>, bool> merge_d2p(::tyr::formalism::datalog::PredicateView<FluentTag> element, MergePlanningContext& context);
extern template std::pair<PredicateView<DerivedTag>, bool> merge_d2p(::tyr::formalism::datalog::PredicateView<FluentTag> element,
                                                                     MergePlanningContext& context);

extern template std::pair<AtomView<::tyr::LiftedTag, StaticTag>, bool>
merge_d2p(::tyr::formalism::datalog::AtomView<::tyr::LiftedTag, StaticTag> element,
          const ygg::UnorderedMap<::tyr::formalism::datalog::PredicateView<StaticTag>, PredicateView<StaticTag>>& predicate_mapping,
          MergePlanningContext& context);
extern template std::pair<AtomView<::tyr::LiftedTag, FluentTag>, bool>
merge_d2p(::tyr::formalism::datalog::AtomView<::tyr::LiftedTag, FluentTag> element,
          const ygg::UnorderedMap<::tyr::formalism::datalog::PredicateView<FluentTag>, PredicateView<FluentTag>>& predicate_mapping,
          MergePlanningContext& context);
extern template std::pair<AtomView<::tyr::LiftedTag, DerivedTag>, bool>
merge_d2p(::tyr::formalism::datalog::AtomView<::tyr::LiftedTag, FluentTag> element,
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

extern template std::pair<AtomView<::tyr::GroundTag, StaticTag>, bool>
merge_atom_d2p<StaticTag, StaticTag>(::tyr::formalism::datalog::PredicateBindingView<StaticTag> element,
                                     const ygg::UnorderedMap<::tyr::formalism::datalog::PredicateView<StaticTag>, PredicateView<StaticTag>>& predicate_mapping,
                                     MergePlanningContext& context);
extern template std::pair<AtomView<::tyr::GroundTag, FluentTag>, bool>
merge_atom_d2p<FluentTag, FluentTag>(::tyr::formalism::datalog::PredicateBindingView<FluentTag> element,
                                     const ygg::UnorderedMap<::tyr::formalism::datalog::PredicateView<FluentTag>, PredicateView<FluentTag>>& predicate_mapping,
                                     MergePlanningContext& context);
extern template std::pair<AtomView<::tyr::GroundTag, DerivedTag>, bool> merge_atom_d2p<FluentTag, DerivedTag>(
    ::tyr::formalism::datalog::PredicateBindingView<FluentTag> element,
    const ygg::UnorderedMap<::tyr::formalism::datalog::PredicateView<FluentTag>, PredicateView<DerivedTag>>& predicate_mapping,
    MergePlanningContext& context);

extern template std::pair<AtomView<::tyr::GroundTag, StaticTag>, bool>
merge_d2p(::tyr::formalism::datalog::AtomView<::tyr::GroundTag, StaticTag> element,
          const ygg::UnorderedMap<::tyr::formalism::datalog::PredicateView<StaticTag>, PredicateView<StaticTag>>& predicate_mapping,
          MergePlanningContext& context);
extern template std::pair<AtomView<::tyr::GroundTag, FluentTag>, bool>
merge_d2p(::tyr::formalism::datalog::AtomView<::tyr::GroundTag, FluentTag> element,
          const ygg::UnorderedMap<::tyr::formalism::datalog::PredicateView<FluentTag>, PredicateView<FluentTag>>& predicate_mapping,
          MergePlanningContext& context);
extern template std::pair<AtomView<::tyr::GroundTag, DerivedTag>, bool>
merge_d2p(::tyr::formalism::datalog::AtomView<::tyr::GroundTag, FluentTag> element,
          const ygg::UnorderedMap<::tyr::formalism::datalog::PredicateView<FluentTag>, PredicateView<DerivedTag>>& predicate_mapping,
          MergePlanningContext& context);

extern template std::pair<LiteralView<::tyr::LiftedTag, StaticTag>, bool>
merge_d2p(::tyr::formalism::datalog::LiteralView<::tyr::LiftedTag, StaticTag> element,
          const ygg::UnorderedMap<::tyr::formalism::datalog::PredicateView<StaticTag>, PredicateView<StaticTag>>& predicate_mapping,
          MergePlanningContext& context);
extern template std::pair<LiteralView<::tyr::LiftedTag, FluentTag>, bool>
merge_d2p(::tyr::formalism::datalog::LiteralView<::tyr::LiftedTag, FluentTag> element,
          const ygg::UnorderedMap<::tyr::formalism::datalog::PredicateView<FluentTag>, PredicateView<FluentTag>>& predicate_mapping,
          MergePlanningContext& context);
extern template std::pair<LiteralView<::tyr::LiftedTag, DerivedTag>, bool>
merge_d2p(::tyr::formalism::datalog::LiteralView<::tyr::LiftedTag, FluentTag> element,
          const ygg::UnorderedMap<::tyr::formalism::datalog::PredicateView<FluentTag>, PredicateView<DerivedTag>>& predicate_mapping,
          MergePlanningContext& context);

extern template std::pair<LiteralView<::tyr::GroundTag, StaticTag>, bool>
merge_d2p(::tyr::formalism::datalog::LiteralView<::tyr::GroundTag, StaticTag> element,
          const ygg::UnorderedMap<::tyr::formalism::datalog::PredicateView<StaticTag>, PredicateView<StaticTag>>& predicate_mapping,
          MergePlanningContext& context);
extern template std::pair<LiteralView<::tyr::GroundTag, FluentTag>, bool>
merge_d2p(::tyr::formalism::datalog::LiteralView<::tyr::GroundTag, FluentTag> element,
          const ygg::UnorderedMap<::tyr::formalism::datalog::PredicateView<FluentTag>, PredicateView<FluentTag>>& predicate_mapping,
          MergePlanningContext& context);
extern template std::pair<LiteralView<::tyr::GroundTag, DerivedTag>, bool>
merge_d2p(::tyr::formalism::datalog::LiteralView<::tyr::GroundTag, FluentTag> element,
          const ygg::UnorderedMap<::tyr::formalism::datalog::PredicateView<FluentTag>, PredicateView<DerivedTag>>& predicate_mapping,
          MergePlanningContext& context);

// Numeric

extern template std::pair<FunctionView<StaticTag>, bool> merge_d2p(::tyr::formalism::datalog::FunctionView<StaticTag> element, MergePlanningContext& context);
extern template std::pair<FunctionView<FluentTag>, bool> merge_d2p(::tyr::formalism::datalog::FunctionView<FluentTag> element, MergePlanningContext& context);

extern template std::pair<FunctionTermView<::tyr::LiftedTag, StaticTag>, bool>
merge_d2p(::tyr::formalism::datalog::FunctionTermView<::tyr::LiftedTag, StaticTag> element, MergePlanningContext& context);
extern template std::pair<FunctionTermView<::tyr::LiftedTag, FluentTag>, bool>
merge_d2p(::tyr::formalism::datalog::FunctionTermView<::tyr::LiftedTag, FluentTag> element, MergePlanningContext& context);

extern template std::pair<FunctionBindingView<StaticTag>, bool> merge_d2p(::tyr::formalism::datalog::FunctionBindingView<StaticTag> element,
                                                                          MergePlanningContext& context);
extern template std::pair<FunctionBindingView<FluentTag>, bool> merge_d2p(::tyr::formalism::datalog::FunctionBindingView<FluentTag> element,
                                                                          MergePlanningContext& context);

extern template std::pair<FunctionTermView<::tyr::GroundTag, StaticTag>, bool>
merge_d2p(::tyr::formalism::datalog::FunctionTermView<::tyr::GroundTag, StaticTag> element, MergePlanningContext& context);
extern template std::pair<FunctionTermView<::tyr::GroundTag, FluentTag>, bool>
merge_d2p(::tyr::formalism::datalog::FunctionTermView<::tyr::GroundTag, FluentTag> element, MergePlanningContext& context);

extern template std::pair<FunctionTermValueView<::tyr::GroundTag, StaticTag>, bool>
merge_d2p(::tyr::formalism::datalog::FunctionTermValueView<::tyr::GroundTag, StaticTag> element, MergePlanningContext& context);
extern template std::pair<FunctionTermValueView<::tyr::GroundTag, FluentTag>, bool>
merge_d2p(::tyr::formalism::datalog::FunctionTermValueView<::tyr::GroundTag, FluentTag> element, MergePlanningContext& context);

extern template std::pair<UnaryOperatorView<ygg::Data<FunctionExpression<::tyr::LiftedTag>>>, bool>
merge_d2p(::tyr::formalism::datalog::UnaryOperatorView<ygg::Data<::tyr::formalism::datalog::FunctionExpression<::tyr::LiftedTag>>> element,
          MergePlanningContext& context);
extern template std::pair<UnaryOperatorView<ygg::Data<FunctionExpression<::tyr::GroundTag>>>, bool>
merge_d2p(::tyr::formalism::datalog::UnaryOperatorView<ygg::Data<::tyr::formalism::datalog::FunctionExpression<::tyr::GroundTag>>> element,
          MergePlanningContext& context);

extern template std::pair<BinaryOperatorView<BooleanOperatorKind, ygg::Data<FunctionExpression<::tyr::LiftedTag>>>, bool> merge_d2p(
    ::tyr::formalism::datalog::BinaryOperatorView<BooleanOperatorKind, ygg::Data<::tyr::formalism::datalog::FunctionExpression<::tyr::LiftedTag>>> element,
    MergePlanningContext& context);
extern template std::pair<BinaryOperatorView<ArithmeticOperatorKind, ygg::Data<FunctionExpression<::tyr::LiftedTag>>>, bool> merge_d2p(
    ::tyr::formalism::datalog::BinaryOperatorView<ArithmeticOperatorKind, ygg::Data<::tyr::formalism::datalog::FunctionExpression<::tyr::LiftedTag>>> element,
    MergePlanningContext& context);
extern template std::pair<BinaryOperatorView<BooleanOperatorKind, ygg::Data<FunctionExpression<::tyr::GroundTag>>>, bool> merge_d2p(
    ::tyr::formalism::datalog::BinaryOperatorView<BooleanOperatorKind, ygg::Data<::tyr::formalism::datalog::FunctionExpression<::tyr::GroundTag>>> element,
    MergePlanningContext& context);
extern template std::pair<BinaryOperatorView<ArithmeticOperatorKind, ygg::Data<FunctionExpression<::tyr::GroundTag>>>, bool> merge_d2p(
    ::tyr::formalism::datalog::BinaryOperatorView<ArithmeticOperatorKind, ygg::Data<::tyr::formalism::datalog::FunctionExpression<::tyr::GroundTag>>> element,
    MergePlanningContext& context);

extern template std::pair<MultiOperatorView<ygg::Data<FunctionExpression<::tyr::LiftedTag>>>, bool>
merge_d2p(::tyr::formalism::datalog::MultiOperatorView<ygg::Data<::tyr::formalism::datalog::FunctionExpression<::tyr::LiftedTag>>> element,
          MergePlanningContext& context);
extern template std::pair<MultiOperatorView<ygg::Data<FunctionExpression<::tyr::GroundTag>>>, bool>
merge_d2p(::tyr::formalism::datalog::MultiOperatorView<ygg::Data<::tyr::formalism::datalog::FunctionExpression<::tyr::GroundTag>>> element,
          MergePlanningContext& context);

extern template ygg::Data<ArithmeticOperator<ygg::Data<FunctionExpression<::tyr::LiftedTag>>>>
merge_d2p(::tyr::formalism::datalog::ArithmeticOperatorView<ygg::Data<::tyr::formalism::datalog::FunctionExpression<::tyr::LiftedTag>>> element,
          MergePlanningContext& context);
extern template ygg::Data<ArithmeticOperator<ygg::Data<FunctionExpression<::tyr::GroundTag>>>>
merge_d2p(::tyr::formalism::datalog::ArithmeticOperatorView<ygg::Data<::tyr::formalism::datalog::FunctionExpression<::tyr::GroundTag>>> element,
          MergePlanningContext& context);

extern template ygg::Data<BooleanOperator<ygg::Data<FunctionExpression<::tyr::LiftedTag>>>>
merge_d2p(::tyr::formalism::datalog::BooleanOperatorView<ygg::Data<::tyr::formalism::datalog::FunctionExpression<::tyr::LiftedTag>>> element,
          MergePlanningContext& context);
extern template ygg::Data<BooleanOperator<ygg::Data<FunctionExpression<::tyr::GroundTag>>>>
merge_d2p(::tyr::formalism::datalog::BooleanOperatorView<ygg::Data<::tyr::formalism::datalog::FunctionExpression<::tyr::GroundTag>>> element,
          MergePlanningContext& context);
}

#endif

#endif
