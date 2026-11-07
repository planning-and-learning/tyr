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

#include "tyr/formalism/copy.hpp"
#include "tyr/formalism/datalog/canonicalization.hpp"
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/datalog/interning.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/write_context.hpp"

#include <boost/mp11/algorithm.hpp>
#include <optional>
#include <urd/elements/children.hpp>
#include <urd/elements/equal_to.hpp>
#include <urd/elements/hash.hpp>
#include <urd/repr/maps/std_map.hpp>
#include <urd/repr/maps/unordered_flat_map.hpp>
#include <urd/repr/values/unordered_flat_set.hpp>
#include <urd/store.hpp>

/// Planning records merged into a datalog repository (p2d): read through the source view's planning repository, written
/// through the destination WriteContext. A record whose type and kind stay (datalog and planning share their records'
/// types) is copied as it is with formalism::copy (urd::map_children). What is left here changes a record on the way: a
/// derived predicate becomes a fluent one and an auxiliary function a fluent one (datalog has neither), predicates come
/// from a caller's mapping, an FDR fact becomes a literal.
namespace tyr::formalism::planning
{

// Common

Term merge_p2d(TermView element, WriteContext<datalog::Builder, datalog::Repository>& context);

// Propositional

template<FactKind T_SRC, FactKind T_DST>
std::pair<datalog::PredicateView<T_DST>, bool> merge_p2d(PredicateView<T_SRC> element, WriteContext<datalog::Builder, datalog::Repository>& context);

template<FactKind T_SRC, FactKind T_DST = T_SRC>
std::pair<datalog::AtomView<LiftedTag, T_DST>, bool> merge_p2d(AtomView<LiftedTag, T_SRC> element,  //
                                                               const urd::Store<urd::map<PredicateView<T_SRC>, datalog::PredicateView<T_DST>>>& predicate_mapping,
                                                               WriteContext<datalog::Builder, datalog::Repository>& context);

template<FactKind T_SRC, FactKind T_DST = T_SRC>
std::pair<datalog::PredicateBindingView<T_DST>, bool>
merge_p2d(PredicateBindingView<T_SRC> element, datalog::PredicateView<T_DST> predicate, WriteContext<datalog::Builder, datalog::Repository>& context);

template<FactKind T_SRC, FactKind T_DST = T_SRC>
std::pair<datalog::PredicateBindingView<T_DST>, bool> merge_p2d(PredicateBindingView<T_SRC> element,  //
                                                                const urd::Store<urd::map<PredicateView<T_SRC>, datalog::PredicateView<T_DST>>>& predicate_mapping,
                                                                WriteContext<datalog::Builder, datalog::Repository>& context);

template<FactKind T_SRC, FactKind T_DST = T_SRC>
std::pair<datalog::AtomView<GroundTag, T_DST>, bool> merge_p2d(AtomView<GroundTag, T_SRC> element,  //
                                                               const urd::Store<urd::map<PredicateView<T_SRC>, datalog::PredicateView<T_DST>>>& predicate_mapping,
                                                               WriteContext<datalog::Builder, datalog::Repository>& context);

template<FactKind T_SRC, FactKind T_DST = T_SRC>
std::pair<datalog::AtomView<GroundTag, T_DST>, bool> merge_p2d(AtomView<GroundTag, T_SRC> element,  //
                                                               urd::Store<urd::map<AtomView<GroundTag, T_SRC>, datalog::AtomView<GroundTag, T_DST>>>& atom_mapping,
                                                               const urd::Store<urd::map<PredicateView<T_SRC>, datalog::PredicateView<T_DST>>>& predicate_mapping,
                                                               WriteContext<datalog::Builder, datalog::Repository>& context);

template<FactKind T_SRC, FactKind T_DST = T_SRC>
std::pair<datalog::LiteralView<LiftedTag, T_DST>, bool>
merge_p2d(LiteralView<LiftedTag, T_SRC> element,  //
          const urd::Store<urd::map<PredicateView<T_SRC>, datalog::PredicateView<T_DST>>>& predicate_mapping,
          WriteContext<datalog::Builder, datalog::Repository>& context);

std::optional<datalog::LiteralView<GroundTag, FluentTag>>
merge_p2d(FDRFactView<FluentTag> element,
          bool polarity,
          const urd::Store<urd::map<PredicateView<FluentTag>, datalog::PredicateView<FluentTag>>>& predicate_mapping,
          WriteContext<datalog::Builder, datalog::Repository>& context);

std::optional<datalog::LiteralView<GroundTag, FluentTag>>
merge_p2d(FDRFactView<FluentTag> element,
          bool polarity,
          urd::Store<urd::map<AtomView<GroundTag, FluentTag>, datalog::AtomView<GroundTag, FluentTag>>>& atom_mapping,
          const urd::Store<urd::map<PredicateView<FluentTag>, datalog::PredicateView<FluentTag>>>& predicate_mapping,
          WriteContext<datalog::Builder, datalog::Repository>& context);

// Numeric

template<FactKind T_SRC, FactKind T_DST = T_SRC>
std::pair<datalog::FunctionView<T_DST>, bool> merge_p2d(FunctionView<T_SRC> element, WriteContext<datalog::Builder, datalog::Repository>& context);

template<FactKind T_SRC, FactKind T_DST = T_SRC>
std::pair<datalog::FunctionTermView<LiftedTag, T_DST>, bool> merge_p2d(FunctionTermView<LiftedTag, T_SRC> element, WriteContext<datalog::Builder, datalog::Repository>& context);

template<FactKind T_SRC, FactKind T_DST = T_SRC>
std::pair<datalog::FunctionBindingView<T_DST>, bool>
merge_p2d(FunctionBindingView<T_SRC> element, datalog::FunctionView<T_DST> function, WriteContext<datalog::Builder, datalog::Repository>& context);

template<FactKind T_SRC, FactKind T_DST = T_SRC>
std::pair<datalog::FunctionBindingView<T_DST>, bool> merge_p2d(FunctionBindingView<T_SRC> element, WriteContext<datalog::Builder, datalog::Repository>& context);

template<FactKind T_SRC, FactKind T_DST = T_SRC>
std::pair<datalog::FunctionTermView<GroundTag, T_DST>, bool> merge_p2d(FunctionTermView<GroundTag, T_SRC> element, WriteContext<datalog::Builder, datalog::Repository>& context);

template<FactKind T_SRC, FactKind T_DST = T_SRC, TaskKind Kind>
    requires(std::same_as<T_SRC, FluentTag> || std::same_as<T_SRC, AuxiliaryTag>) && std::same_as<T_DST, FluentTag>
std::pair<datalog::NumericEffectView<Kind, T_DST>, bool> merge_p2d(NumericEffectView<Kind, T_SRC> element, WriteContext<datalog::Builder, datalog::Repository>& context);

FunctionExpression<LiftedTag> merge_p2d(FunctionExpressionView<LiftedTag> element, WriteContext<datalog::Builder, datalog::Repository>& context);

FunctionExpression<GroundTag> merge_p2d(FunctionExpressionView<GroundTag> element, WriteContext<datalog::Builder, datalog::Repository>& context);

template<typename T>
    requires boost::mp11::mp_contains<ExpressionTypes, T>::value
std::pair<urd::index_view<T, datalog::Repository>, bool> merge_p2d(urd::index_view<T, Repository> element, WriteContext<datalog::Builder, datalog::Repository>& context);

// Common

inline Term merge_p2d(TermView element, WriteContext<datalog::Builder, datalog::Repository>& context)
{
    return visit(
        [&]<typename Alternative>(const Alternative& arg)
        {
            if constexpr (std::is_same_v<Alternative, ParameterIndex>)
                return Term(arg);
            else
                return Term(copy(arg, context).first.index());
        },
        element);
}

// Propositional

template<FactKind T_SRC, FactKind T_DST>
std::pair<datalog::PredicateView<T_DST>, bool> merge_p2d(PredicateView<T_SRC> element, WriteContext<datalog::Builder, datalog::Repository>& context)
{
    auto predicate = datalog::checkout<Predicate<T_DST>>(context.builder());

    predicate->name = element.get_name();
    predicate->arity = element.get_arity();

    return datalog::insert(context.destination(), *predicate);
}

template<FactKind T_SRC, FactKind T_DST>
std::pair<datalog::AtomView<LiftedTag, T_DST>, bool> merge_p2d(AtomView<LiftedTag, T_SRC> element,  //
                                                               const urd::Store<urd::map<PredicateView<T_SRC>, datalog::PredicateView<T_DST>>>& predicate_mapping,
                                                               WriteContext<datalog::Builder, datalog::Repository>& context)
{
    auto atom = datalog::checkout<Atom<LiftedTag, T_DST>>(context.builder());

    atom->predicate = predicate_mapping.at(element.get_predicate()).index();
    for (const auto term : element.get_terms())
        atom->terms.push_back(merge_p2d(term, context));

    return datalog::insert(context.destination(), *atom);
}

template<FactKind T_SRC, FactKind T_DST>
std::pair<datalog::PredicateBindingView<T_DST>, bool> merge_p2d(PredicateBindingView<T_SRC> element,  //
                                                                const urd::Store<urd::map<PredicateView<T_SRC>, datalog::PredicateView<T_DST>>>& predicate_mapping,
                                                                WriteContext<datalog::Builder, datalog::Repository>& context)
{ return merge_p2d(element, predicate_mapping.at(element.get_relation()), context); }

template<FactKind T_SRC, FactKind T_DST>
std::pair<datalog::PredicateBindingView<T_DST>, bool>
merge_p2d(PredicateBindingView<T_SRC> element, datalog::PredicateView<T_DST> predicate, WriteContext<datalog::Builder, datalog::Repository>& context)
{
    auto binding = datalog::checkout<RelationBinding<Predicate<T_DST>>>(context.builder());

    binding->relation = predicate.index();
    for (const auto object : element.get_objects())
        binding->objects.push_back(object.index());

    return datalog::insert(context.destination(), *binding);
}

template<FactKind T_SRC, FactKind T_DST>
std::pair<datalog::AtomView<GroundTag, T_DST>, bool> merge_p2d(AtomView<GroundTag, T_SRC> element,  //
                                                               const urd::Store<urd::map<PredicateView<T_SRC>, datalog::PredicateView<T_DST>>>& predicate_mapping,
                                                               WriteContext<datalog::Builder, datalog::Repository>& context)
{
    auto atom = datalog::checkout<Atom<GroundTag, T_DST>>(context.builder());

    atom->binding = merge_p2d<T_SRC, T_DST>(element.get_row(), predicate_mapping, context).first.index();

    return datalog::insert(context.destination(), *atom);
}

template<FactKind T_SRC, FactKind T_DST>
std::pair<datalog::AtomView<GroundTag, T_DST>, bool> merge_p2d(AtomView<GroundTag, T_SRC> element,  //
                                                               urd::Store<urd::map<AtomView<GroundTag, T_SRC>, datalog::AtomView<GroundTag, T_DST>>>& atom_mapping,
                                                               const urd::Store<urd::map<PredicateView<T_SRC>, datalog::PredicateView<T_DST>>>& predicate_mapping,
                                                               WriteContext<datalog::Builder, datalog::Repository>& context)
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
          const urd::Store<urd::map<PredicateView<T_SRC>, datalog::PredicateView<T_DST>>>& predicate_mapping,
          WriteContext<datalog::Builder, datalog::Repository>& context)
{
    auto literal = datalog::checkout<Literal<LiftedTag, T_DST>>(context.builder());

    literal->polarity = element.get_polarity();
    literal->atom = merge_p2d<T_SRC, T_DST>(element.get_atom(), predicate_mapping, context).first.index();

    return datalog::insert(context.destination(), *literal);
}

inline std::optional<datalog::LiteralView<GroundTag, FluentTag>>
merge_p2d(FDRFactView<FluentTag> element,
          bool polarity,
          const urd::Store<urd::map<PredicateView<FluentTag>, datalog::PredicateView<FluentTag>>>& predicate_mapping,
          WriteContext<datalog::Builder, datalog::Repository>& context)
{
    const auto atom = element.get_atom();
    if (!atom.has_value())
        return std::nullopt;

    auto literal = datalog::checkout<Literal<GroundTag, FluentTag>>(context.builder());
    literal->polarity = polarity;
    literal->atom = merge_p2d(*atom, predicate_mapping, context).first.index();

    return datalog::insert(context.destination(), *literal).first;
}

inline std::optional<datalog::LiteralView<GroundTag, FluentTag>>
merge_p2d(FDRFactView<FluentTag> element,
          bool polarity,
          urd::Store<urd::map<AtomView<GroundTag, FluentTag>, datalog::AtomView<GroundTag, FluentTag>>>& atom_mapping,
          const urd::Store<urd::map<PredicateView<FluentTag>, datalog::PredicateView<FluentTag>>>& predicate_mapping,
          WriteContext<datalog::Builder, datalog::Repository>& context)
{
    const auto atom = element.get_atom();
    if (!atom.has_value())
        return std::nullopt;

    auto literal = datalog::checkout<Literal<GroundTag, FluentTag>>(context.builder());
    literal->polarity = polarity;
    literal->atom = merge_p2d(*atom, atom_mapping, predicate_mapping, context).first.index();

    return datalog::insert(context.destination(), *literal).first;
}

// Numeric

template<FactKind T_SRC, FactKind T_DST>
std::pair<datalog::FunctionView<T_DST>, bool> merge_p2d(FunctionView<T_SRC> element, WriteContext<datalog::Builder, datalog::Repository>& context)
{
    auto function = datalog::checkout<Function<T_DST>>(context.builder());

    function->name = element.get_name();
    function->arity = element.get_arity();

    return datalog::insert(context.destination(), *function);
}

template<FactKind T_SRC, FactKind T_DST>
std::pair<datalog::FunctionTermView<LiftedTag, T_DST>, bool> merge_p2d(FunctionTermView<LiftedTag, T_SRC> element, WriteContext<datalog::Builder, datalog::Repository>& context)
{
    auto fterm = datalog::checkout<FunctionTerm<LiftedTag, T_DST>>(context.builder());

    fterm->function = merge_p2d<T_SRC, T_DST>(element.get_function(), context).first.index();
    for (const auto term : element.get_terms())
        fterm->terms.push_back(merge_p2d(term, context));

    return datalog::insert(context.destination(), *fterm);
}

template<FactKind T_SRC, FactKind T_DST>
std::pair<datalog::FunctionBindingView<T_DST>, bool> merge_p2d(FunctionBindingView<T_SRC> element, WriteContext<datalog::Builder, datalog::Repository>& context)
{ return merge_p2d(element, merge_p2d<T_SRC, T_DST>(element.get_relation(), context).first, context); }

template<FactKind T_SRC, FactKind T_DST>
std::pair<datalog::FunctionBindingView<T_DST>, bool>
merge_p2d(FunctionBindingView<T_SRC> element, datalog::FunctionView<T_DST> function, WriteContext<datalog::Builder, datalog::Repository>& context)
{
    auto binding = datalog::checkout<RelationBinding<Function<T_DST>>>(context.builder());

    binding->relation = function.index();
    for (const auto object : element.get_objects())
        binding->objects.push_back(object.index());

    return datalog::insert(context.destination(), *binding);
}

template<FactKind T_SRC, FactKind T_DST>
std::pair<datalog::FunctionTermView<GroundTag, T_DST>, bool> merge_p2d(FunctionTermView<GroundTag, T_SRC> element, WriteContext<datalog::Builder, datalog::Repository>& context)
{
    auto fterm = datalog::checkout<FunctionTerm<GroundTag, T_DST>>(context.builder());

    fterm->binding = merge_p2d<T_SRC, T_DST>(element.get_row(), context).first.index();

    return datalog::insert(context.destination(), *fterm);
}

template<FactKind T_SRC, FactKind T_DST, TaskKind Kind>
    requires(std::same_as<T_SRC, FluentTag> || std::same_as<T_SRC, AuxiliaryTag>) && std::same_as<T_DST, FluentTag>
std::pair<datalog::NumericEffectView<Kind, T_DST>, bool> merge_p2d(NumericEffectView<Kind, T_SRC> element, WriteContext<datalog::Builder, datalog::Repository>& context)
{
    auto numeric_effect = datalog::checkout<NumericEffect<Kind, T_DST>>(context.builder());

    numeric_effect->operator_kind = element.get_operator();
    numeric_effect->fterm = merge_p2d<T_SRC, T_DST>(element.get_fterm(), context).first.index();
    numeric_effect->fexpr = merge_p2d(element.get_fexpr(), context);

    return datalog::insert(context.destination(), *numeric_effect);
}

inline FunctionExpression<LiftedTag> merge_p2d(FunctionExpressionView<LiftedTag> element, WriteContext<datalog::Builder, datalog::Repository>& context)
{
    return visit(
        [&]<typename Alternative>(const Alternative& arg) -> FunctionExpression<LiftedTag>
        {
            if constexpr (std::is_same_v<Alternative, ygg::float_t>)
                return FunctionExpression<LiftedTag>(arg);
            else
                return FunctionExpression<LiftedTag>(merge_p2d(arg, context).first.index());
        },
        element);
}

inline FunctionExpression<GroundTag> merge_p2d(FunctionExpressionView<GroundTag> element, WriteContext<datalog::Builder, datalog::Repository>& context)
{
    return visit(
        [&]<typename Alternative>(const Alternative& arg) -> FunctionExpression<GroundTag>
        {
            if constexpr (std::is_same_v<Alternative, ygg::float_t>)
                return FunctionExpression<GroundTag>(arg);
            else if constexpr (std::is_same_v<Alternative, FunctionTermView<GroundTag, AuxiliaryTag>>)
                return FunctionExpression<GroundTag>(merge_p2d<AuxiliaryTag, FluentTag>(arg, context).first.index());
            else
                return FunctionExpression<GroundTag>(merge_p2d(arg, context).first.index());
        },
        element);
}

/// An arithmetic or boolean operator: its operator kind kept, each of its function expressions merged (an auxiliary
/// function term in one made fluent), by urd::map_variant_children over the source record.
template<typename T>
    requires boost::mp11::mp_contains<ExpressionTypes, T>::value
std::pair<urd::index_view<T, datalog::Repository>, bool> merge_p2d(urd::index_view<T, Repository> element, WriteContext<datalog::Builder, datalog::Repository>& context)
{
    auto record = datalog::checkout<T>(context.builder());

    [&]<auto... Ms>(const urd::data<T>& source, urd::Members<Ms...>) { (((*record).*Ms = source.*Ms), ...); }(element.data(),
                                                                                                             urd::data<T>::cista_members());
    *record = urd::map_variant_children<boost::mp11::mp_list<>, T>(std::move(*record),
                                                                   element.context(),
                                                                   [&](const auto& expression) { return merge_p2d(expression, context); });

    return datalog::insert(context.destination(), *record);
}

}

#ifndef TYR_HEADER_INSTANTIATION

namespace tyr::formalism::planning
{
extern template std::pair<datalog::PredicateBindingView<StaticTag>, bool>
merge_p2d(PredicateBindingView<StaticTag> element, datalog::PredicateView<StaticTag> relation, WriteContext<datalog::Builder, datalog::Repository>& context);
extern template std::pair<datalog::PredicateBindingView<FluentTag>, bool>
merge_p2d(PredicateBindingView<FluentTag> element, datalog::PredicateView<FluentTag> relation, WriteContext<datalog::Builder, datalog::Repository>& context);
extern template std::pair<datalog::PredicateBindingView<FluentTag>, bool>
merge_p2d(PredicateBindingView<DerivedTag> element, datalog::PredicateView<FluentTag> relation, WriteContext<datalog::Builder, datalog::Repository>& context);
extern template std::pair<datalog::FunctionBindingView<StaticTag>, bool>
merge_p2d(FunctionBindingView<StaticTag> element, datalog::FunctionView<StaticTag> relation, WriteContext<datalog::Builder, datalog::Repository>& context);
extern template std::pair<datalog::FunctionBindingView<FluentTag>, bool>
merge_p2d(FunctionBindingView<FluentTag> element, datalog::FunctionView<FluentTag> relation, WriteContext<datalog::Builder, datalog::Repository>& context);
extern template std::pair<datalog::FunctionBindingView<FluentTag>, bool>
merge_p2d(FunctionBindingView<AuxiliaryTag> element, datalog::FunctionView<FluentTag> relation, WriteContext<datalog::Builder, datalog::Repository>& context);

extern template std::pair<datalog::PredicateView<FluentTag>, bool> merge_p2d<DerivedTag, FluentTag>(PredicateView<DerivedTag> element,
                                                                                                   WriteContext<datalog::Builder, datalog::Repository>& context);

extern template std::pair<datalog::AtomView<LiftedTag, StaticTag>, bool>
merge_p2d(AtomView<LiftedTag, StaticTag> element,
          const urd::Store<urd::map<PredicateView<StaticTag>, datalog::PredicateView<StaticTag>>>& predicate_mapping,
          WriteContext<datalog::Builder, datalog::Repository>& context);
extern template std::pair<datalog::AtomView<LiftedTag, FluentTag>, bool>
merge_p2d(AtomView<LiftedTag, FluentTag> element,
          const urd::Store<urd::map<PredicateView<FluentTag>, datalog::PredicateView<FluentTag>>>& predicate_mapping,
          WriteContext<datalog::Builder, datalog::Repository>& context);
extern template std::pair<datalog::AtomView<LiftedTag, FluentTag>, bool>
merge_p2d(AtomView<LiftedTag, DerivedTag> element,
          const urd::Store<urd::map<PredicateView<DerivedTag>, datalog::PredicateView<FluentTag>>>& predicate_mapping,
          WriteContext<datalog::Builder, datalog::Repository>& context);
extern template std::pair<datalog::PredicateBindingView<StaticTag>, bool>
merge_p2d(PredicateBindingView<StaticTag> element,
          const urd::Store<urd::map<PredicateView<StaticTag>, datalog::PredicateView<StaticTag>>>& predicate_mapping,
          WriteContext<datalog::Builder, datalog::Repository>& context);
extern template std::pair<datalog::PredicateBindingView<FluentTag>, bool>
merge_p2d(PredicateBindingView<FluentTag> element,
          const urd::Store<urd::map<PredicateView<FluentTag>, datalog::PredicateView<FluentTag>>>& predicate_mapping,
          WriteContext<datalog::Builder, datalog::Repository>& context);
extern template std::pair<datalog::PredicateBindingView<FluentTag>, bool>
merge_p2d(PredicateBindingView<DerivedTag> element,
          const urd::Store<urd::map<PredicateView<DerivedTag>, datalog::PredicateView<FluentTag>>>& predicate_mapping,
          WriteContext<datalog::Builder, datalog::Repository>& context);

extern template std::pair<datalog::AtomView<GroundTag, StaticTag>, bool>
merge_p2d(AtomView<GroundTag, StaticTag> element,
          const urd::Store<urd::map<PredicateView<StaticTag>, datalog::PredicateView<StaticTag>>>& predicate_mapping,
          WriteContext<datalog::Builder, datalog::Repository>& context);
extern template std::pair<datalog::AtomView<GroundTag, FluentTag>, bool>
merge_p2d(AtomView<GroundTag, FluentTag> element,
          const urd::Store<urd::map<PredicateView<FluentTag>, datalog::PredicateView<FluentTag>>>& predicate_mapping,
          WriteContext<datalog::Builder, datalog::Repository>& context);
extern template std::pair<datalog::AtomView<GroundTag, FluentTag>, bool>
merge_p2d(AtomView<GroundTag, DerivedTag> element,
          const urd::Store<urd::map<PredicateView<DerivedTag>, datalog::PredicateView<FluentTag>>>& predicate_mapping,
          WriteContext<datalog::Builder, datalog::Repository>& context);

extern template std::pair<datalog::LiteralView<LiftedTag, StaticTag>, bool>
merge_p2d(LiteralView<LiftedTag, StaticTag> element,
          const urd::Store<urd::map<PredicateView<StaticTag>, datalog::PredicateView<StaticTag>>>& predicate_mapping,
          WriteContext<datalog::Builder, datalog::Repository>& context);
extern template std::pair<datalog::LiteralView<LiftedTag, FluentTag>, bool>
merge_p2d(LiteralView<LiftedTag, FluentTag> element,
          const urd::Store<urd::map<PredicateView<FluentTag>, datalog::PredicateView<FluentTag>>>& predicate_mapping,
          WriteContext<datalog::Builder, datalog::Repository>& context);
extern template std::pair<datalog::LiteralView<LiftedTag, FluentTag>, bool>
merge_p2d(LiteralView<LiftedTag, DerivedTag> element,
          const urd::Store<urd::map<PredicateView<DerivedTag>, datalog::PredicateView<FluentTag>>>& predicate_mapping,
          WriteContext<datalog::Builder, datalog::Repository>& context);

// Numeric

extern template std::pair<datalog::FunctionView<StaticTag>, bool> merge_p2d(FunctionView<StaticTag> element, WriteContext<datalog::Builder, datalog::Repository>& context);
extern template std::pair<datalog::FunctionView<FluentTag>, bool> merge_p2d(FunctionView<FluentTag> element, WriteContext<datalog::Builder, datalog::Repository>& context);
extern template std::pair<datalog::FunctionView<FluentTag>, bool> merge_p2d<AuxiliaryTag, FluentTag>(FunctionView<AuxiliaryTag> element,
                                                                                                     WriteContext<datalog::Builder, datalog::Repository>& context);

extern template std::pair<datalog::FunctionTermView<LiftedTag, StaticTag>, bool> merge_p2d(FunctionTermView<LiftedTag, StaticTag> element,
                                                                                           WriteContext<datalog::Builder, datalog::Repository>& context);
extern template std::pair<datalog::FunctionTermView<LiftedTag, FluentTag>, bool> merge_p2d(FunctionTermView<LiftedTag, FluentTag> element,
                                                                                           WriteContext<datalog::Builder, datalog::Repository>& context);
extern template std::pair<datalog::FunctionTermView<LiftedTag, FluentTag>, bool>
merge_p2d<AuxiliaryTag, FluentTag>(FunctionTermView<LiftedTag, AuxiliaryTag> element, WriteContext<datalog::Builder, datalog::Repository>& context);

extern template std::pair<datalog::FunctionBindingView<StaticTag>, bool> merge_p2d(FunctionBindingView<StaticTag> element, WriteContext<datalog::Builder, datalog::Repository>& context);
extern template std::pair<datalog::FunctionBindingView<FluentTag>, bool> merge_p2d(FunctionBindingView<FluentTag> element, WriteContext<datalog::Builder, datalog::Repository>& context);
extern template std::pair<datalog::FunctionBindingView<FluentTag>, bool> merge_p2d<AuxiliaryTag, FluentTag>(FunctionBindingView<AuxiliaryTag> element,
                                                                                                            WriteContext<datalog::Builder, datalog::Repository>& context);

extern template std::pair<datalog::FunctionTermView<GroundTag, StaticTag>, bool> merge_p2d(FunctionTermView<GroundTag, StaticTag> element,
                                                                                           WriteContext<datalog::Builder, datalog::Repository>& context);
extern template std::pair<datalog::FunctionTermView<GroundTag, FluentTag>, bool> merge_p2d(FunctionTermView<GroundTag, FluentTag> element,
                                                                                           WriteContext<datalog::Builder, datalog::Repository>& context);
extern template std::pair<datalog::FunctionTermView<GroundTag, FluentTag>, bool>
merge_p2d<AuxiliaryTag, FluentTag>(FunctionTermView<GroundTag, AuxiliaryTag> element, WriteContext<datalog::Builder, datalog::Repository>& context);

extern template std::pair<datalog::NumericEffectView<LiftedTag, FluentTag>, bool>
merge_p2d<FluentTag, FluentTag>(NumericEffectView<LiftedTag, FluentTag> element, WriteContext<datalog::Builder, datalog::Repository>& context);
extern template std::pair<datalog::NumericEffectView<LiftedTag, FluentTag>, bool>
merge_p2d<AuxiliaryTag, FluentTag>(NumericEffectView<LiftedTag, AuxiliaryTag> element, WriteContext<datalog::Builder, datalog::Repository>& context);
extern template std::pair<datalog::NumericEffectView<GroundTag, FluentTag>, bool>
merge_p2d<FluentTag, FluentTag>(NumericEffectView<GroundTag, FluentTag> element, WriteContext<datalog::Builder, datalog::Repository>& context);
extern template std::pair<datalog::NumericEffectView<GroundTag, FluentTag>, bool>
merge_p2d<AuxiliaryTag, FluentTag>(NumericEffectView<GroundTag, AuxiliaryTag> element, WriteContext<datalog::Builder, datalog::Repository>& context);

extern template std::pair<datalog::UnaryOperatorView<LiftedTag, ArithmeticOperatorKind>, bool>
merge_p2d(UnaryOperatorView<LiftedTag, ArithmeticOperatorKind> element, WriteContext<datalog::Builder, datalog::Repository>& context);
extern template std::pair<datalog::UnaryOperatorView<GroundTag, ArithmeticOperatorKind>, bool>
merge_p2d(UnaryOperatorView<GroundTag, ArithmeticOperatorKind> element, WriteContext<datalog::Builder, datalog::Repository>& context);
extern template std::pair<datalog::BinaryOperatorView<LiftedTag, BooleanOperatorKind>, bool>
merge_p2d(BinaryOperatorView<LiftedTag, BooleanOperatorKind> element, WriteContext<datalog::Builder, datalog::Repository>& context);
extern template std::pair<datalog::BinaryOperatorView<LiftedTag, ArithmeticOperatorKind>, bool>
merge_p2d(BinaryOperatorView<LiftedTag, ArithmeticOperatorKind> element, WriteContext<datalog::Builder, datalog::Repository>& context);
extern template std::pair<datalog::BinaryOperatorView<GroundTag, BooleanOperatorKind>, bool>
merge_p2d(BinaryOperatorView<GroundTag, BooleanOperatorKind> element, WriteContext<datalog::Builder, datalog::Repository>& context);
extern template std::pair<datalog::BinaryOperatorView<GroundTag, ArithmeticOperatorKind>, bool>
merge_p2d(BinaryOperatorView<GroundTag, ArithmeticOperatorKind> element, WriteContext<datalog::Builder, datalog::Repository>& context);
extern template std::pair<datalog::MultiOperatorView<LiftedTag, ArithmeticOperatorKind>, bool>
merge_p2d(MultiOperatorView<LiftedTag, ArithmeticOperatorKind> element, WriteContext<datalog::Builder, datalog::Repository>& context);
extern template std::pair<datalog::MultiOperatorView<GroundTag, ArithmeticOperatorKind>, bool>
merge_p2d(MultiOperatorView<GroundTag, ArithmeticOperatorKind> element, WriteContext<datalog::Builder, datalog::Repository>& context);

}

#endif

#endif
