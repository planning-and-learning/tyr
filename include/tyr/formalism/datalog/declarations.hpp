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

#ifndef TYR_FORMALISM_DATALOG_DECLARATIONS_HPP_
#define TYR_FORMALISM_DATALOG_DECLARATIONS_HPP_

#include "tyr/declarations.hpp"
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include "tyr/formalism/binding_index.hpp"
#include "tyr/formalism/declarations.hpp"

#include <cista/containers/variant.h>
#include <cista/containers/vector.h>
#include <concepts>
#include <memory>
#include <variant>
#include <vector>
#include <yggdrasil/core/config.hpp>
#include <urd/elements/flat.hpp>
#include <boost/mp11.hpp>
#include <yggdrasil/formalism/builder.hpp>

namespace tyr::formalism::datalog
{

/**
 * Formalism tag
 */

template<TaskKind T, OperatorKind O>
struct UnaryOperator
{
};

template<TaskKind T, OperatorKind O>
struct BinaryOperator
{
};

template<TaskKind T, OperatorKind O>
struct MultiOperator
{
};

template<TaskKind T, FactKind F>
struct Atom
{
};

template<TaskKind T, FactKind F>
struct Literal
{
};

template<TaskKind T, FactKind F>
struct FunctionTerm
{
};

template<TaskKind T, FactKind F>
    requires std::same_as<F, FluentTag>
struct NumericEffect
{
};

template<TaskKind T, FactKind F>
    requires std::same_as<T, GroundTag>
struct FunctionTermValue
{
};

template<TaskKind T>
struct ConjunctiveCondition
{
};

template<TaskKind T>
struct ConjunctiveEffect
{
};

template<TaskKind T>
struct ConditionalEffect
{
};

template<TaskKind T, RelationKind R>
struct Rule
{
};

template<TaskKind T, RelationKind R>
struct RuleHead;

template<TaskKind T>
struct RuleHead<T, PredicateTag>
{
    using type = urd::flat_vector<urd::index<Atom<T, FluentTag>>>;
};

template<TaskKind T>
struct RuleHead<T, FunctionTag>
{
    using type = urd::index<NumericEffect<T, FluentTag>>;
};

template<TaskKind T, RelationKind R>
using RuleHeadT = typename RuleHead<T, R>::type;

template<TaskKind T>
struct Program
{
};

using CoreTypes = boost::mp11::mp_list<Variable, Object>;
using PredicateTypes = boost::mp11::mp_transform<Predicate, StaticFluentTags>;
template<TaskKind T>
using AtomTypes = boost::mp11::mp_transform_q<boost::mp11::mp_bind_front<Atom, T>, StaticFluentTags>;
template<TaskKind T>
using LiteralTypes = boost::mp11::mp_transform_q<boost::mp11::mp_bind_front<Literal, T>, StaticFluentTags>;
using FunctionTypes = boost::mp11::mp_transform<Function, StaticFluentAuxiliaryTags>;
template<TaskKind T>
using FunctionTermTypes = boost::mp11::mp_transform_q<boost::mp11::mp_bind_front<FunctionTerm, T>, StaticFluentAuxiliaryTags>;
template<TaskKind T>
using FunctionTermValueTypes = boost::mp11::mp_transform_q<boost::mp11::mp_bind_front<FunctionTermValue, T>, StaticFluentAuxiliaryTags>;
template<TaskKind T>
using NumericEffectTypes = boost::mp11::mp_list<NumericEffect<T, FluentTag>>;
template<TaskKind T>
using ArithmeticExpressionTypes = boost::mp11::mp_list<UnaryOperator<T, ArithmeticOperatorKind>, BinaryOperator<T, ArithmeticOperatorKind>, MultiOperator<T, ArithmeticOperatorKind>>;
template<TaskKind T>
using BooleanExpressionTypes = boost::mp11::mp_list<BinaryOperator<T, BooleanOperatorKind>>;
using ExpressionTypes = boost::mp11::mp_append<ArithmeticExpressionTypes<LiftedTag>,
                                              BooleanExpressionTypes<LiftedTag>,
                                              ArithmeticExpressionTypes<GroundTag>,
                                              BooleanExpressionTypes<GroundTag>>;
using EffectTypes = boost::mp11::mp_append<NumericEffectTypes<LiftedTag>, NumericEffectTypes<GroundTag>>;
template<TaskKind T>
using RuleTypes = boost::mp11::mp_transform_q<boost::mp11::mp_bind_front<Rule, T>, PredicateFunctionTags>;

using CompoundTypes = boost::mp11::mp_append<boost::mp11::mp_list<ConjunctiveCondition<LiftedTag>, ConjunctiveEffect<LiftedTag>, ConditionalEffect<LiftedTag>>,
                                            RuleTypes<LiftedTag>,
                                            boost::mp11::mp_list<ConjunctiveCondition<GroundTag>, ConjunctiveEffect<GroundTag>, ConditionalEffect<GroundTag>>,
                                            RuleTypes<GroundTag>,
                                            boost::mp11::mp_list<Program<LiftedTag>, Program<GroundTag>>>;

using SymbolRepositoryTypes = boost::mp11::mp_append<CoreTypes,
                                                    PredicateTypes,
                                                    AtomTypes<LiftedTag>,
                                                    AtomTypes<GroundTag>,
                                                    LiteralTypes<LiftedTag>,
                                                    LiteralTypes<GroundTag>,
                                                    FunctionTypes,
                                                    FunctionTermTypes<LiftedTag>,
                                                    FunctionTermTypes<GroundTag>,
                                                    FunctionTermValueTypes<GroundTag>,
                                                    ExpressionTypes,
                                                    EffectTypes,
                                                    CompoundTypes>;
using RelationRepositoryTypes = boost::mp11::mp_append<PredicateTypes, FunctionTypes, RuleTypes<LiftedTag>>;
using BuilderTypes = boost::mp11::mp_append<SymbolRepositoryTypes, boost::mp11::mp_transform<RelationBinding, RelationRepositoryTypes>>;
using Builder = boost::mp11::mp_apply<ygg::formalism::BuilderStorage, BuilderTypes>;

using SymbolRepository = boost::mp11::mp_apply<::ygg::formalism::ConcurrentSymbolRepository, SymbolRepositoryTypes>;

template<typename... Ts>
using TaggedRelationRepository = ::ygg::formalism::ConcurrentRelationRepository<ObjectTag, Ts...>;

using RelationRepository = boost::mp11::mp_apply<TaggedRelationRepository, RelationRepositoryTypes>;

using Repository = ::ygg::formalism::Repository<SymbolRepository, RelationRepository>;
using RepositoryPtr = std::shared_ptr<Repository>;

using RepositoryFactory = ::ygg::formalism::RepositoryFactory<SymbolRepository, RelationRepository>;
using RepositoryFactoryPtr = std::shared_ptr<RepositoryFactory>;

/**
 * Declaration-only view aliases.
 *
 * Include repository.hpp before constructing, storing, or accessing views.
 */
template<TaskKind T, OperatorKind O>
using BinaryOperatorView = urd::index_view<BinaryOperator<T, O>, Repository>;

template<FactKind F>
using PredicateBindingView = ::ygg::formalism::RelationBindingView<ygg::formalism::RelationBindingIndex<Predicate<F>, ::tyr::formalism::ObjectTag>, Repository>;
template<FactKind F>
using FunctionBindingView = ::ygg::formalism::RelationBindingView<ygg::formalism::RelationBindingIndex<Function<F>, ::tyr::formalism::ObjectTag>, Repository>;
template<RelationKind R>
using RuleBindingView = ::ygg::formalism::RelationBindingView<ygg::formalism::RelationBindingIndex<Rule<LiftedTag, R>, ::tyr::formalism::ObjectTag>, Repository>;

template<FactKind F>
using PredicateBindingIndex = ygg::formalism::RelationBindingIndex<Predicate<F>, ::tyr::formalism::ObjectTag>;
template<FactKind F>
using FunctionBindingIndex = ygg::formalism::RelationBindingIndex<Function<F>, ::tyr::formalism::ObjectTag>;
template<RelationKind R>
using RuleBindingIndex = ygg::formalism::RelationBindingIndex<Rule<LiftedTag, R>, ::tyr::formalism::ObjectTag>;

template<FactKind F>
using PredicateBindingForwardRangeView = RelationBindingsView<Predicate<F>, urd::Store<urd::sequence<urd::index<Row>>>, Repository>;
template<FactKind F>
using FunctionBindingRandomAccessRangeView = RelationBindingsView<Function<F>, urd::Store<urd::sequence<urd::index<Row>>>, Repository>;

template<TaskKind T>
using BooleanOperatorView = urd::index_view<BinaryOperator<T, ::tyr::formalism::BooleanOperatorKind>, Repository>;

template<TaskKind T>
using BooleanOperatorListView = urd::resolved_range<urd::flat_vector<urd::index<BinaryOperator<T, ::tyr::formalism::BooleanOperatorKind>>>, Repository>;
template<TaskKind T>
using BooleanOperatorViewList = urd::Store<urd::sequence<BooleanOperatorView<T>>>;

template<FactKind F>
using FunctionView = urd::index_view<Function<F>, Repository>;

template<FactKind F>
using FunctionListView = urd::resolved_range<urd::flat_vector<urd::index<Function<F>>>, Repository>;
template<FactKind F>
using FunctionViewList = urd::Store<urd::sequence<FunctionView<F>>>;

template<TaskKind T, OperatorKind O>
using MultiOperatorView = urd::index_view<MultiOperator<T, O>, Repository>;

using ObjectView = urd::index_view<Object, Repository>;

using ObjectViewList = urd::Store<urd::sequence<ObjectView>>;

template<FactKind F>
using PredicateView = urd::index_view<Predicate<F>, Repository>;

template<FactKind F>
using PredicateListView = urd::resolved_range<urd::flat_vector<urd::index<Predicate<F>>>, Repository>;
template<FactKind F>
using PredicateViewList = urd::Store<urd::sequence<PredicateView<F>>>;

/// A term read through the repository: its object's view, or the parameter (formalism/declarations.hpp).
using TermView = std::variant<ObjectView, ParameterIndex>;

using TermListView = urd::resolved_range<TermList, Repository>;
using TermViewList = urd::Store<urd::sequence<TermView>>;

template<TaskKind T, OperatorKind O>
using UnaryOperatorView = urd::index_view<UnaryOperator<T, O>, Repository>;

using VariableView = urd::index_view<Variable, Repository>;

using VariableViewList = urd::Store<urd::sequence<VariableView>>;

template<TaskKind T, FactKind F>
using AtomListView = urd::resolved_range<urd::flat_vector<urd::index<Atom<T, F>>>, Repository>;
template<TaskKind T, FactKind F>
using AtomView = urd::index_view<Atom<T, F>, Repository>;
template<TaskKind T, FactKind F>
using AtomViewList = urd::Store<urd::sequence<AtomView<T, F>>>;
template<TaskKind T>
using ConditionalEffectListView = urd::resolved_range<urd::flat_vector<urd::index<ConditionalEffect<T>>>, Repository>;
template<TaskKind T>
using ConditionalEffectView = urd::index_view<ConditionalEffect<T>, Repository>;
template<TaskKind T>
using ConditionalEffectViewList = urd::Store<urd::sequence<ConditionalEffectView<T>>>;
template<TaskKind T>
using ConjunctiveConditionView = urd::index_view<ConjunctiveCondition<T>, Repository>;
template<TaskKind T>
using ConjunctiveEffectView = urd::index_view<ConjunctiveEffect<T>, Repository>;
template<TaskKind T, FactKind F>
using FunctionTermListView = urd::resolved_range<urd::flat_vector<urd::index<FunctionTerm<T, F>>>, Repository>;
template<TaskKind T, FactKind F>
using FunctionTermValueListView = urd::resolved_range<urd::flat_vector<urd::index<FunctionTermValue<T, F>>>, Repository>;
template<TaskKind T, FactKind F>
using FunctionTermValueView = urd::index_view<FunctionTermValue<T, F>, Repository>;
template<TaskKind T, FactKind F>
using FunctionTermValueViewList = urd::Store<urd::sequence<FunctionTermValueView<T, F>>>;
template<TaskKind T, FactKind F>
using FunctionTermView = urd::index_view<FunctionTerm<T, F>, Repository>;
template<TaskKind T, FactKind F>
using LiteralListView = urd::resolved_range<urd::flat_vector<urd::index<Literal<T, F>>>, Repository>;
template<TaskKind T, FactKind F>
using LiteralView = urd::index_view<Literal<T, F>, Repository>;
template<TaskKind T, FactKind F>
using LiteralViewList = urd::Store<urd::sequence<LiteralView<T, F>>>;
template<TaskKind T, FactKind F>
using NumericEffectOperatorListView = urd::resolved_range<urd::flat_vector<urd::index<NumericEffect<T, F>>>, Repository>;
template<TaskKind T, FactKind F>
using NumericEffectOperatorView = urd::index_view<NumericEffect<T, F>, Repository>;
template<TaskKind T, FactKind F>
using NumericEffectOperatorViewList = urd::Store<urd::sequence<NumericEffectOperatorView<T, F>>>;
template<TaskKind T, FactKind F>
using NumericEffectView = urd::index_view<NumericEffect<T, F>, Repository>;
template<TaskKind T>
using ProgramView = urd::index_view<Program<T>, Repository>;
template<TaskKind T, RelationKind R>
using RuleView = urd::index_view<Rule<T, R>, Repository>;
template<TaskKind T, RelationKind R>
using RuleViewList = urd::Store<urd::sequence<RuleView<T, R>>>;

/// A function expression: a number, an arithmetic operator or a function term (ground ones also the auxiliary term).
/// Stored as it is, not interned; read through a repository as FunctionExpressionView.
template<TaskKind T>
using FunctionExpression =
    std::conditional_t<std::same_as<T, GroundTag>,
                       urd::flat_variant<ygg::float_t,
                                                urd::index<UnaryOperator<T, ArithmeticOperatorKind>>,
                                                urd::index<BinaryOperator<T, ArithmeticOperatorKind>>,
                                                urd::index<MultiOperator<T, ArithmeticOperatorKind>>,
                                                urd::index<FunctionTerm<T, StaticTag>>,
                                                urd::index<FunctionTerm<T, FluentTag>>,
                                                urd::index<FunctionTerm<T, AuxiliaryTag>>>,
                       urd::flat_variant<ygg::float_t,
                                                urd::index<UnaryOperator<T, ArithmeticOperatorKind>>,
                                                urd::index<BinaryOperator<T, ArithmeticOperatorKind>>,
                                                urd::index<MultiOperator<T, ArithmeticOperatorKind>>,
                                                urd::index<FunctionTerm<T, StaticTag>>,
                                                urd::index<FunctionTerm<T, FluentTag>>>>;
template<TaskKind T>
using FunctionExpressionList = urd::flat_vector<FunctionExpression<T>>;

/// A function expression read through the repository: the number, or the view of the record, it holds.
template<TaskKind T>
using FunctionExpressionView = std::conditional_t<std::same_as<T, GroundTag>,
                                                  std::variant<ygg::float_t,
                                                               UnaryOperatorView<T, ArithmeticOperatorKind>,
                                                               BinaryOperatorView<T, ArithmeticOperatorKind>,
                                                               MultiOperatorView<T, ArithmeticOperatorKind>,
                                                               FunctionTermView<T, StaticTag>,
                                                               FunctionTermView<T, FluentTag>,
                                                               FunctionTermView<T, AuxiliaryTag>>,
                                                  std::variant<ygg::float_t,
                                                               UnaryOperatorView<T, ArithmeticOperatorKind>,
                                                               BinaryOperatorView<T, ArithmeticOperatorKind>,
                                                               MultiOperatorView<T, ArithmeticOperatorKind>,
                                                               FunctionTermView<T, StaticTag>,
                                                               FunctionTermView<T, FluentTag>>>;
class VariableDependencyGraph;

}

#endif
