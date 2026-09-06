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

std::pair<VariableView, bool> merge_d2p(datalog::VariableView element, MergePlanningContext& context);

std::pair<ObjectView, bool> merge_d2p(datalog::ObjectView element, MergePlanningContext& context);

ygg::Data<Term> merge_d2p(datalog::TermView element, MergePlanningContext& context);

// Propositional

template<FactKind T_SRC, FactKind T_DST>
std::pair<PredicateView<T_DST>, bool> merge_d2p(datalog::PredicateView<T_SRC> element, MergePlanningContext& context);

template<FactKind T_SRC, FactKind T_DST>
std::pair<AtomView<LiftedTag, T_DST>, bool> merge_d2p(datalog::AtomView<LiftedTag, T_SRC> element,
                                                      const ygg::UnorderedMap<datalog::PredicateView<T_SRC>, PredicateView<T_DST>>& predicate_mapping,
                                                      MergePlanningContext& context);

template<FactKind T_SRC, FactKind T_DST>
std::pair<PredicateBindingView<T_DST>, bool> merge_d2p(datalog::PredicateBindingView<T_SRC> element,
                                                       const ygg::UnorderedMap<datalog::PredicateView<T_SRC>, PredicateView<T_DST>>& predicate_mapping,
                                                       MergePlanningContext& context);

template<FactKind T_SRC, FactKind T_DST>
std::pair<AtomView<GroundTag, T_DST>, bool> merge_atom_d2p(datalog::PredicateBindingView<T_SRC> element,
                                                           const ygg::UnorderedMap<datalog::PredicateView<T_SRC>, PredicateView<T_DST>>& predicate_mapping,
                                                           MergePlanningContext& context);

template<FactKind T_SRC, FactKind T_DST>
std::pair<AtomView<GroundTag, T_DST>, bool> merge_d2p(datalog::AtomView<GroundTag, T_SRC> element,
                                                      const ygg::UnorderedMap<datalog::PredicateView<T_SRC>, PredicateView<T_DST>>& predicate_mapping,
                                                      MergePlanningContext& context);

template<FactKind T_SRC, FactKind T_DST>
std::pair<LiteralView<LiftedTag, T_DST>, bool> merge_d2p(datalog::LiteralView<LiftedTag, T_SRC> element,
                                                         const ygg::UnorderedMap<datalog::PredicateView<T_SRC>, PredicateView<T_DST>>& predicate_mapping,
                                                         MergePlanningContext& context);

template<FactKind T_SRC, FactKind T_DST>
std::pair<LiteralView<GroundTag, T_DST>, bool> merge_d2p(datalog::LiteralView<GroundTag, T_SRC> element,
                                                         const ygg::UnorderedMap<datalog::PredicateView<T_SRC>, PredicateView<T_DST>>& predicate_mapping,
                                                         MergePlanningContext& context);

// Numeric

template<FactKind T>
std::pair<FunctionView<T>, bool> merge_d2p(datalog::FunctionView<T> element, MergePlanningContext& context);

template<FactKind T>
std::pair<FunctionTermView<LiftedTag, T>, bool> merge_d2p(datalog::FunctionTermView<LiftedTag, T> element, MergePlanningContext& context);

template<FactKind T>
std::pair<FunctionBindingView<T>, bool> merge_d2p(datalog::FunctionBindingView<T> element, MergePlanningContext& context);

template<FactKind T>
std::pair<FunctionTermView<GroundTag, T>, bool> merge_d2p(datalog::FunctionTermView<GroundTag, T> element, MergePlanningContext& context);

template<FactKind T>
std::pair<FunctionTermValueView<GroundTag, T>, bool> merge_d2p(datalog::FunctionTermValueView<GroundTag, T> element, MergePlanningContext& context);

ygg::Data<FunctionExpression<LiftedTag>> merge_d2p(datalog::FunctionExpressionView<LiftedTag> element, MergePlanningContext& context);

ygg::Data<FunctionExpression<GroundTag>> merge_d2p(datalog::FunctionExpressionView<GroundTag> element, MergePlanningContext& context);

template<TaskKind T>
std::pair<UnaryOperatorView<T>, bool> merge_d2p(datalog::UnaryOperatorView<T> element, MergePlanningContext& context);

template<TaskKind T, BinaryOperatorKind O>
std::pair<BinaryOperatorView<T, O>, bool> merge_d2p(datalog::BinaryOperatorView<T, O> element, MergePlanningContext& context);

template<TaskKind T>
std::pair<MultiOperatorView<T>, bool> merge_d2p(datalog::MultiOperatorView<T> element, MergePlanningContext& context);

template<TaskKind T>
ygg::Data<ArithmeticOperator<T>> merge_d2p(datalog::ArithmeticOperatorView<T> element, MergePlanningContext& context);

template<TaskKind T>
ygg::Data<BooleanOperator<T>> merge_d2p(datalog::BooleanOperatorView<T> element, MergePlanningContext& context);

// Common

inline std::pair<VariableView, bool> merge_d2p(datalog::VariableView element, MergePlanningContext& context)
{
    auto variable = planning::checkout<Variable>(context.builder);

    variable->name = element.get_name();

    return planning::get_or_create(context.destination, *variable);
}

inline std::pair<ObjectView, bool> merge_d2p(datalog::ObjectView element, MergePlanningContext& context)
{
    auto object = planning::checkout<Object>(context.builder);

    object->name = element.get_name();

    return planning::get_or_create(context.destination, *object);
}

inline ygg::Data<Term> merge_d2p(datalog::TermView element, MergePlanningContext& context)
{
    return visit(
        [&](auto&& arg)
        {
            using Alternative = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<Alternative, ParameterIndex>)
                return ygg::Data<Term>(arg);
            else if constexpr (std::is_same_v<Alternative, datalog::ObjectView>)
                return ygg::Data<Term>(merge_d2p(arg, context).first.get_index());
            else
                static_assert(ygg::dependent_false<Alternative>::value, "Missing case");
        },
        element.get_variant());
}

// Propositional

template<FactKind T_SRC, FactKind T_DST>
std::pair<PredicateView<T_DST>, bool> merge_d2p(datalog::PredicateView<T_SRC> element, MergePlanningContext& context)
{
    auto predicate = planning::checkout<Predicate<T_DST>>(context.builder);

    predicate->name = element.get_name();
    predicate->arity = element.get_arity();

    return planning::get_or_create(context.destination, *predicate);
}

template<FactKind T_SRC, FactKind T_DST>
std::pair<AtomView<LiftedTag, T_DST>, bool> merge_d2p(datalog::AtomView<LiftedTag, T_SRC> element,  //
                                                      const ygg::UnorderedMap<datalog::PredicateView<T_SRC>, PredicateView<T_DST>>& predicate_mapping,
                                                      MergePlanningContext& context)
{
    auto atom = planning::checkout<Atom<LiftedTag, T_DST>>(context.builder);

    atom->predicate = predicate_mapping.at(element.get_predicate()).get_index();
    for (const auto term : element.get_terms())
        atom->terms.push_back(merge_d2p(term, context));

    return planning::get_or_create(context.destination, *atom);
}

template<FactKind T_SRC, FactKind T_DST>
std::pair<PredicateBindingView<T_DST>, bool> merge_d2p(datalog::PredicateBindingView<T_SRC> element,  //
                                                       const ygg::UnorderedMap<datalog::PredicateView<T_SRC>, PredicateView<T_DST>>& predicate_mapping,
                                                       MergePlanningContext& context)
{
    auto binding = planning::checkout<RelationBinding<Predicate<T_DST>>>(context.builder);

    binding->relation = predicate_mapping.at(element.get_relation()).get_index();
    for (const auto object : element.get_objects())
        binding->objects.push_back(object.get_index());

    return planning::get_or_create(context.destination, *binding);
}

template<FactKind T_SRC, FactKind T_DST>
std::pair<AtomView<GroundTag, T_DST>, bool> merge_atom_d2p(datalog::PredicateBindingView<T_SRC> element,  //
                                                           const ygg::UnorderedMap<datalog::PredicateView<T_SRC>, PredicateView<T_DST>>& predicate_mapping,
                                                           MergePlanningContext& context)
{
    auto atom = planning::checkout<Atom<GroundTag, T_DST>>(context.builder);

    atom->binding = merge_d2p<T_SRC, T_DST>(element, predicate_mapping, context).first.get_index();

    return planning::get_or_create(context.destination, *atom);
}

template<FactKind T_SRC, FactKind T_DST>
std::pair<AtomView<GroundTag, T_DST>, bool> merge_d2p(datalog::AtomView<GroundTag, T_SRC> element,  //
                                                      const ygg::UnorderedMap<datalog::PredicateView<T_SRC>, PredicateView<T_DST>>& predicate_mapping,
                                                      MergePlanningContext& context)
{
    auto atom = planning::checkout<Atom<GroundTag, T_DST>>(context.builder);

    atom->binding = merge_d2p<T_SRC, T_DST>(element.get_row(), predicate_mapping, context).first.get_index();

    return planning::get_or_create(context.destination, *atom);
}

template<FactKind T_SRC, FactKind T_DST>
std::pair<LiteralView<LiftedTag, T_DST>, bool> merge_d2p(datalog::LiteralView<LiftedTag, T_SRC> element,  //
                                                         const ygg::UnorderedMap<datalog::PredicateView<T_SRC>, PredicateView<T_DST>>& predicate_mapping,
                                                         MergePlanningContext& context)
{
    auto literal = planning::checkout<Literal<LiftedTag, T_DST>>(context.builder);

    literal->polarity = element.get_polarity();
    literal->atom = merge_d2p<T_SRC, T_DST>(element.get_atom(), predicate_mapping, context).first.get_index();

    return planning::get_or_create(context.destination, *literal);
}

template<FactKind T_SRC, FactKind T_DST>
std::pair<LiteralView<GroundTag, T_DST>, bool> merge_d2p(datalog::LiteralView<GroundTag, T_SRC> element,  //
                                                         const ygg::UnorderedMap<datalog::PredicateView<T_SRC>, PredicateView<T_DST>>& predicate_mapping,
                                                         MergePlanningContext& context)
{
    auto literal = planning::checkout<Literal<GroundTag, T_DST>>(context.builder);

    literal->polarity = element.get_polarity();
    literal->atom = merge_d2p<T_SRC, T_DST>(element.get_atom(), predicate_mapping, context).first.get_index();

    return planning::get_or_create(context.destination, *literal);
}

// Numeric

template<FactKind T>
std::pair<FunctionView<T>, bool> merge_d2p(datalog::FunctionView<T> element, MergePlanningContext& context)
{
    auto function = planning::checkout<Function<T>>(context.builder);

    function->name = element.get_name();
    function->arity = element.get_arity();

    return planning::get_or_create(context.destination, *function);
}

template<FactKind T>
std::pair<FunctionTermView<LiftedTag, T>, bool> merge_d2p(datalog::FunctionTermView<LiftedTag, T> element, MergePlanningContext& context)
{
    auto fterm = planning::checkout<FunctionTerm<LiftedTag, T>>(context.builder);

    fterm->function = element.get_function().get_index();
    for (const auto term : element.get_terms())
        fterm->terms.push_back(merge_d2p(term, context));

    return planning::get_or_create(context.destination, *fterm);
}

template<FactKind T>
std::pair<FunctionBindingView<T>, bool> merge_d2p(datalog::FunctionBindingView<T> element, MergePlanningContext& context)
{
    auto binding = planning::checkout<RelationBinding<Function<T>>>(context.builder);

    binding->relation = merge_d2p(element.get_relation(), context).first.get_index();
    for (const auto object : element.get_objects())
        binding->objects.push_back(object.get_index());

    return planning::get_or_create(context.destination, *binding);
}

template<FactKind T>
std::pair<FunctionTermView<GroundTag, T>, bool> merge_d2p(datalog::FunctionTermView<GroundTag, T> element, MergePlanningContext& context)
{
    auto fterm = planning::checkout<FunctionTerm<GroundTag, T>>(context.builder);

    fterm->binding = merge_d2p(element.get_row(), context).first.get_index();

    return planning::get_or_create(context.destination, *fterm);
}

template<FactKind T>
std::pair<FunctionTermValueView<GroundTag, T>, bool> merge_d2p(datalog::FunctionTermValueView<GroundTag, T> element, MergePlanningContext& context)
{
    auto fterm_value = planning::checkout<FunctionTermValue<GroundTag, T>>(context.builder);

    fterm_value->fterm = merge_d2p(element.get_fterm(), context).first.get_index();
    fterm_value->value = element.get_value();

    return planning::get_or_create(context.destination, *fterm_value);
}

inline ygg::Data<FunctionExpression<LiftedTag>> merge_d2p(datalog::FunctionExpressionView<LiftedTag> element, MergePlanningContext& context)
{
    return visit(
        [&](auto&& arg)
        {
            using Alternative = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<Alternative, ygg::float_t>)
                return ygg::Data<FunctionExpression<LiftedTag>>(arg);
            else if constexpr (std::is_same_v<Alternative, datalog::ArithmeticOperatorView<LiftedTag>>)
                return ygg::Data<FunctionExpression<LiftedTag>>(merge_d2p(arg, context));
            else if constexpr (std::is_same_v<Alternative, datalog::FunctionTermView<LiftedTag, AuxiliaryTag>>)
                throw std::logic_error("AuxiliaryTag FunctionTerm must not be merged.");
            else
                return ygg::Data<FunctionExpression<LiftedTag>>(merge_d2p(arg, context).first.get_index());
        },
        element.get_variant());
}

inline ygg::Data<FunctionExpression<GroundTag>> merge_d2p(datalog::FunctionExpressionView<GroundTag> element, MergePlanningContext& context)
{
    return visit(
        [&](auto&& arg)
        {
            using Alternative = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<Alternative, ygg::float_t>)
                return ygg::Data<FunctionExpression<GroundTag>>(arg);
            else if constexpr (std::is_same_v<Alternative, datalog::ArithmeticOperatorView<GroundTag>>)
                return ygg::Data<FunctionExpression<GroundTag>>(merge_d2p(arg, context));
            else
                return ygg::Data<FunctionExpression<GroundTag>>(merge_d2p(arg, context).first.get_index());
        },
        element.get_variant());
}

template<TaskKind T>
std::pair<UnaryOperatorView<T>, bool> merge_d2p(datalog::UnaryOperatorView<T> element, MergePlanningContext& context)
{
    auto unary = planning::checkout<UnaryOperator<T>>(context.builder);

    unary->operator_kind = element.get_operator();
    unary->arg = merge_d2p(element.get_arg(), context);

    return planning::get_or_create(context.destination, *unary);
}

template<TaskKind T, BinaryOperatorKind O>
std::pair<BinaryOperatorView<T, O>, bool> merge_d2p(datalog::BinaryOperatorView<T, O> element, MergePlanningContext& context)
{
    auto binary = planning::checkout<BinaryOperator<T, O>>(context.builder);

    binary->operator_kind = element.get_operator();
    binary->lhs = merge_d2p(element.get_lhs(), context);
    binary->rhs = merge_d2p(element.get_rhs(), context);

    return planning::get_or_create(context.destination, *binary);
}

template<TaskKind T>
std::pair<MultiOperatorView<T>, bool> merge_d2p(datalog::MultiOperatorView<T> element, MergePlanningContext& context)
{
    auto multi = planning::checkout<MultiOperator<T>>(context.builder);

    multi->operator_kind = element.get_operator();
    for (const auto arg : element.get_args())
        multi->args.push_back(merge_d2p(arg, context));

    return planning::get_or_create(context.destination, *multi);
}

template<TaskKind T>
ygg::Data<ArithmeticOperator<T>> merge_d2p(datalog::ArithmeticOperatorView<T> element, MergePlanningContext& context)
{
    return visit([&](auto&& arg) { return ygg::Data<ArithmeticOperator<T>>(arg.get_operator(), merge_d2p(arg, context).first.get_index()); },
                 element.get_variant());
}

template<TaskKind T>
ygg::Data<BooleanOperator<T>> merge_d2p(datalog::BooleanOperatorView<T> element, MergePlanningContext& context)
{
    return visit([&](auto&& arg) { return ygg::Data<BooleanOperator<T>>(arg.get_operator(), merge_d2p(arg, context).first.get_index()); },
                 element.get_variant());
}

}

#ifndef TYR_HEADER_INSTANTIATION

namespace tyr::formalism::planning
{
extern template std::pair<PredicateView<StaticTag>, bool> merge_d2p(datalog::PredicateView<StaticTag> element, MergePlanningContext& context);
extern template std::pair<PredicateView<FluentTag>, bool> merge_d2p(datalog::PredicateView<FluentTag> element, MergePlanningContext& context);
extern template std::pair<PredicateView<DerivedTag>, bool> merge_d2p(datalog::PredicateView<FluentTag> element, MergePlanningContext& context);

extern template std::pair<AtomView<LiftedTag, StaticTag>, bool>
merge_d2p(datalog::AtomView<LiftedTag, StaticTag> element,
          const ygg::UnorderedMap<datalog::PredicateView<StaticTag>, PredicateView<StaticTag>>& predicate_mapping,
          MergePlanningContext& context);
extern template std::pair<AtomView<LiftedTag, FluentTag>, bool>
merge_d2p(datalog::AtomView<LiftedTag, FluentTag> element,
          const ygg::UnorderedMap<datalog::PredicateView<FluentTag>, PredicateView<FluentTag>>& predicate_mapping,
          MergePlanningContext& context);
extern template std::pair<AtomView<LiftedTag, DerivedTag>, bool>
merge_d2p(datalog::AtomView<LiftedTag, FluentTag> element,
          const ygg::UnorderedMap<datalog::PredicateView<FluentTag>, PredicateView<DerivedTag>>& predicate_mapping,
          MergePlanningContext& context);

extern template std::pair<PredicateBindingView<StaticTag>, bool>
merge_d2p(datalog::PredicateBindingView<StaticTag> element,
          const ygg::UnorderedMap<datalog::PredicateView<StaticTag>, PredicateView<StaticTag>>& predicate_mapping,
          MergePlanningContext& context);
extern template std::pair<PredicateBindingView<FluentTag>, bool>
merge_d2p(datalog::PredicateBindingView<FluentTag> element,
          const ygg::UnorderedMap<datalog::PredicateView<FluentTag>, PredicateView<FluentTag>>& predicate_mapping,
          MergePlanningContext& context);
extern template std::pair<PredicateBindingView<DerivedTag>, bool>
merge_d2p(datalog::PredicateBindingView<FluentTag> element,
          const ygg::UnorderedMap<datalog::PredicateView<FluentTag>, PredicateView<DerivedTag>>& predicate_mapping,
          MergePlanningContext& context);

extern template std::pair<AtomView<GroundTag, StaticTag>, bool>
merge_atom_d2p<StaticTag, StaticTag>(datalog::PredicateBindingView<StaticTag> element,
                                     const ygg::UnorderedMap<datalog::PredicateView<StaticTag>, PredicateView<StaticTag>>& predicate_mapping,
                                     MergePlanningContext& context);
extern template std::pair<AtomView<GroundTag, FluentTag>, bool>
merge_atom_d2p<FluentTag, FluentTag>(datalog::PredicateBindingView<FluentTag> element,
                                     const ygg::UnorderedMap<datalog::PredicateView<FluentTag>, PredicateView<FluentTag>>& predicate_mapping,
                                     MergePlanningContext& context);
extern template std::pair<AtomView<GroundTag, DerivedTag>, bool>
merge_atom_d2p<FluentTag, DerivedTag>(datalog::PredicateBindingView<FluentTag> element,
                                      const ygg::UnorderedMap<datalog::PredicateView<FluentTag>, PredicateView<DerivedTag>>& predicate_mapping,
                                      MergePlanningContext& context);

extern template std::pair<AtomView<GroundTag, StaticTag>, bool>
merge_d2p(datalog::AtomView<GroundTag, StaticTag> element,
          const ygg::UnorderedMap<datalog::PredicateView<StaticTag>, PredicateView<StaticTag>>& predicate_mapping,
          MergePlanningContext& context);
extern template std::pair<AtomView<GroundTag, FluentTag>, bool>
merge_d2p(datalog::AtomView<GroundTag, FluentTag> element,
          const ygg::UnorderedMap<datalog::PredicateView<FluentTag>, PredicateView<FluentTag>>& predicate_mapping,
          MergePlanningContext& context);
extern template std::pair<AtomView<GroundTag, DerivedTag>, bool>
merge_d2p(datalog::AtomView<GroundTag, FluentTag> element,
          const ygg::UnorderedMap<datalog::PredicateView<FluentTag>, PredicateView<DerivedTag>>& predicate_mapping,
          MergePlanningContext& context);

extern template std::pair<LiteralView<LiftedTag, StaticTag>, bool>
merge_d2p(datalog::LiteralView<LiftedTag, StaticTag> element,
          const ygg::UnorderedMap<datalog::PredicateView<StaticTag>, PredicateView<StaticTag>>& predicate_mapping,
          MergePlanningContext& context);
extern template std::pair<LiteralView<LiftedTag, FluentTag>, bool>
merge_d2p(datalog::LiteralView<LiftedTag, FluentTag> element,
          const ygg::UnorderedMap<datalog::PredicateView<FluentTag>, PredicateView<FluentTag>>& predicate_mapping,
          MergePlanningContext& context);
extern template std::pair<LiteralView<LiftedTag, DerivedTag>, bool>
merge_d2p(datalog::LiteralView<LiftedTag, FluentTag> element,
          const ygg::UnorderedMap<datalog::PredicateView<FluentTag>, PredicateView<DerivedTag>>& predicate_mapping,
          MergePlanningContext& context);

extern template std::pair<LiteralView<GroundTag, StaticTag>, bool>
merge_d2p(datalog::LiteralView<GroundTag, StaticTag> element,
          const ygg::UnorderedMap<datalog::PredicateView<StaticTag>, PredicateView<StaticTag>>& predicate_mapping,
          MergePlanningContext& context);
extern template std::pair<LiteralView<GroundTag, FluentTag>, bool>
merge_d2p(datalog::LiteralView<GroundTag, FluentTag> element,
          const ygg::UnorderedMap<datalog::PredicateView<FluentTag>, PredicateView<FluentTag>>& predicate_mapping,
          MergePlanningContext& context);
extern template std::pair<LiteralView<GroundTag, DerivedTag>, bool>
merge_d2p(datalog::LiteralView<GroundTag, FluentTag> element,
          const ygg::UnorderedMap<datalog::PredicateView<FluentTag>, PredicateView<DerivedTag>>& predicate_mapping,
          MergePlanningContext& context);

// Numeric

extern template std::pair<FunctionView<StaticTag>, bool> merge_d2p(datalog::FunctionView<StaticTag> element, MergePlanningContext& context);
extern template std::pair<FunctionView<FluentTag>, bool> merge_d2p(datalog::FunctionView<FluentTag> element, MergePlanningContext& context);

extern template std::pair<FunctionTermView<LiftedTag, StaticTag>, bool> merge_d2p(datalog::FunctionTermView<LiftedTag, StaticTag> element,
                                                                                  MergePlanningContext& context);
extern template std::pair<FunctionTermView<LiftedTag, FluentTag>, bool> merge_d2p(datalog::FunctionTermView<LiftedTag, FluentTag> element,
                                                                                  MergePlanningContext& context);

extern template std::pair<FunctionBindingView<StaticTag>, bool> merge_d2p(datalog::FunctionBindingView<StaticTag> element, MergePlanningContext& context);
extern template std::pair<FunctionBindingView<FluentTag>, bool> merge_d2p(datalog::FunctionBindingView<FluentTag> element, MergePlanningContext& context);

extern template std::pair<FunctionTermView<GroundTag, StaticTag>, bool> merge_d2p(datalog::FunctionTermView<GroundTag, StaticTag> element,
                                                                                  MergePlanningContext& context);
extern template std::pair<FunctionTermView<GroundTag, FluentTag>, bool> merge_d2p(datalog::FunctionTermView<GroundTag, FluentTag> element,
                                                                                  MergePlanningContext& context);

extern template std::pair<FunctionTermValueView<GroundTag, StaticTag>, bool> merge_d2p(datalog::FunctionTermValueView<GroundTag, StaticTag> element,
                                                                                       MergePlanningContext& context);
extern template std::pair<FunctionTermValueView<GroundTag, FluentTag>, bool> merge_d2p(datalog::FunctionTermValueView<GroundTag, FluentTag> element,
                                                                                       MergePlanningContext& context);

extern template std::pair<UnaryOperatorView<LiftedTag>, bool> merge_d2p(datalog::UnaryOperatorView<LiftedTag> element, MergePlanningContext& context);
extern template std::pair<UnaryOperatorView<GroundTag>, bool> merge_d2p(datalog::UnaryOperatorView<GroundTag> element, MergePlanningContext& context);

extern template std::pair<BinaryOperatorView<LiftedTag, BooleanOperatorKind>, bool>
merge_d2p(datalog::BinaryOperatorView<LiftedTag, BooleanOperatorKind> element, MergePlanningContext& context);
extern template std::pair<BinaryOperatorView<LiftedTag, ArithmeticOperatorKind>, bool>
merge_d2p(datalog::BinaryOperatorView<LiftedTag, ArithmeticOperatorKind> element, MergePlanningContext& context);
extern template std::pair<BinaryOperatorView<GroundTag, BooleanOperatorKind>, bool>
merge_d2p(datalog::BinaryOperatorView<GroundTag, BooleanOperatorKind> element, MergePlanningContext& context);
extern template std::pair<BinaryOperatorView<GroundTag, ArithmeticOperatorKind>, bool>
merge_d2p(datalog::BinaryOperatorView<GroundTag, ArithmeticOperatorKind> element, MergePlanningContext& context);

extern template std::pair<MultiOperatorView<LiftedTag>, bool> merge_d2p(datalog::MultiOperatorView<LiftedTag> element, MergePlanningContext& context);
extern template std::pair<MultiOperatorView<GroundTag>, bool> merge_d2p(datalog::MultiOperatorView<GroundTag> element, MergePlanningContext& context);

extern template ygg::Data<ArithmeticOperator<LiftedTag>> merge_d2p(datalog::ArithmeticOperatorView<LiftedTag> element, MergePlanningContext& context);
extern template ygg::Data<ArithmeticOperator<GroundTag>> merge_d2p(datalog::ArithmeticOperatorView<GroundTag> element, MergePlanningContext& context);

extern template ygg::Data<BooleanOperator<LiftedTag>> merge_d2p(datalog::BooleanOperatorView<LiftedTag> element, MergePlanningContext& context);
extern template ygg::Data<BooleanOperator<GroundTag>> merge_d2p(datalog::BooleanOperatorView<GroundTag> element, MergePlanningContext& context);
}

#endif

#endif
