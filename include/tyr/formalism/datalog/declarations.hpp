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
#include "tyr/formalism/binding_index.hpp"
#include "tyr/formalism/declarations.hpp"

#include <memory>
#include <vector>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/core/types.hpp>

namespace tyr::formalism::datalog
{

/**
 * Formalism tag
 */

template<TaskKind T>
struct UnaryOperator
{
};

template<TaskKind T, BinaryOperatorKind O>
struct BinaryOperator
{
};

template<TaskKind T>
struct MultiOperator
{
};

template<TaskKind T>
class BooleanOperator
{
};
template<TaskKind T>
class ArithmeticOperator
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
struct NumericEffect
{
};

template<TaskKind T, FactKind F>
struct NumericEffectOperator
{
};

template<TaskKind T, FactKind F>
struct FunctionTermValue;

template<FactKind F>
struct FunctionTermValue<GroundTag, F>
{
};

template<TaskKind T>
struct FunctionExpression
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
    using type = ygg::Index<Atom<T, FluentTag>>;
};

template<TaskKind T>
struct RuleHead<T, FunctionTag>
{
    using type = ygg::Data<NumericEffectOperator<T, FluentTag>>;
};

template<TaskKind T, RelationKind R>
using RuleHeadT = typename RuleHead<T, R>::type;

struct Metric
{
};

template<TaskKind T>
struct Program
{
};

using CoreTypes = ygg::TypeList<Variable, Object>;
using PredicateTypes = ygg::MapTypeListT<Predicate, StaticFluentTags>;
template<TaskKind T>
using AtomTypes = ygg::MapTypeListSecondT<Atom, T, StaticFluentTags>;
template<TaskKind T>
using LiteralTypes = ygg::MapTypeListSecondT<Literal, T, StaticFluentTags>;
using FunctionTypes = ygg::MapTypeListT<Function, StaticFluentAuxiliaryTags>;
template<TaskKind T>
using FunctionTermTypes = ygg::MapTypeListSecondT<FunctionTerm, T, StaticFluentAuxiliaryTags>;
template<TaskKind T>
using FunctionTermValueTypes = ygg::MapTypeListSecondT<FunctionTermValue, T, StaticFluentAuxiliaryTags>;
template<TaskKind T>
using NumericEffectTypes = ygg::TypeList<NumericEffect<T, FluentTag>>;
template<TaskKind T>
using NumericEffectOperatorTypes = ygg::TypeList<NumericEffectOperator<T, FluentTag>>;

template<TaskKind T>
using ArithmeticExpressionTypes = ygg::TypeList<UnaryOperator<T>, BinaryOperator<T, ArithmeticOperatorKind>, MultiOperator<T>>;
template<TaskKind T>
using BooleanExpressionTypes = ygg::TypeList<BinaryOperator<T, BooleanOperatorKind>>;
using ExpressionTypes = ygg::ConcatTypeListsT<ArithmeticExpressionTypes<LiftedTag>,
                                              BooleanExpressionTypes<LiftedTag>,
                                              ArithmeticExpressionTypes<GroundTag>,
                                              BooleanExpressionTypes<GroundTag>>;
using EffectTypes = ygg::ConcatTypeListsT<NumericEffectTypes<LiftedTag>, NumericEffectTypes<GroundTag>>;
template<TaskKind T>
using RuleTypes = ygg::MapTypeListSecondT<Rule, T, PredicateFunctionTags>;

using CompoundTypes = ygg::ConcatTypeListsT<ygg::TypeList<ConjunctiveCondition<LiftedTag>, ConjunctiveEffect<LiftedTag>, ConditionalEffect<LiftedTag>>,
                                            RuleTypes<LiftedTag>,
                                            ygg::TypeList<ConjunctiveCondition<GroundTag>, ConjunctiveEffect<GroundTag>, ConditionalEffect<GroundTag>>,
                                            RuleTypes<GroundTag>,
                                            ygg::TypeList<Metric, Program<LiftedTag>, Program<GroundTag>>>;

using SymbolRepositoryTypes = ygg::ConcatTypeListsT<CoreTypes,
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
using RelationRepositoryTypes = ygg::ConcatTypeListsT<PredicateTypes, FunctionTypes, RuleTypes<LiftedTag>>;
using BuilderTypes = ygg::ConcatTypeListsT<SymbolRepositoryTypes, ygg::MapTypeListT<RelationBinding, RelationRepositoryTypes>>;

using SymbolRepository = ygg::ApplyTypeListT<::ygg::formalism::ConcurrentSymbolRepository, SymbolRepositoryTypes>;

template<typename... Ts>
using TaggedRelationRepository = ::ygg::formalism::ConcurrentRelationRepository<ObjectTag, Ts...>;

using RelationRepository = ygg::ApplyTypeListT<TaggedRelationRepository, RelationRepositoryTypes>;

using Repository = ::ygg::formalism::Repository<SymbolRepository, RelationRepository>;
using RepositoryPtr = std::shared_ptr<Repository>;

using RepositoryFactory = ::ygg::formalism::RepositoryFactory<SymbolRepository, RelationRepository>;
using RepositoryFactoryPtr = std::shared_ptr<RepositoryFactory>;

/**
 * Declaration-only view aliases.
 *
 * Include repository.hpp before constructing, storing, or accessing views.
 */
template<TaskKind T>
using ArithmeticOperatorView = ygg::View<ygg::Data<ArithmeticOperator<T>>, Repository>;

template<TaskKind T>
using ArithmeticOperatorListView = ygg::View<ygg::DataList<ArithmeticOperator<T>>, Repository>;

template<TaskKind T, BinaryOperatorKind O>
using BinaryOperatorView = ygg::View<ygg::Index<BinaryOperator<T, O>>, Repository>;

template<TaskKind T, BinaryOperatorKind O>
using BinaryOperatorListView = ygg::View<ygg::IndexList<BinaryOperator<T, O>>, Repository>;

template<FactKind F>
using PredicateBindingView = ygg::View<ygg::Index<RelationBinding<Predicate<F>>>, Repository>;
template<FactKind F>
using FunctionBindingView = ygg::View<ygg::Index<RelationBinding<Function<F>>>, Repository>;
template<RelationKind R>
using RuleBindingView = ygg::View<ygg::Index<RelationBinding<Rule<LiftedTag, R>>>, Repository>;

template<FactKind F>
using PredicateBindingForwardRangeView = ygg::View<RelationBindingsForwardRange<Predicate<F>, std::vector<ygg::Index<Row>>>, Repository>;
template<FactKind F>
using FunctionBindingRandomAccessRangeView = ygg::View<RelationBindingsRandomAccessRange<Function<F>, std::vector<ygg::Index<Row>>>, Repository>;

template<TaskKind T>
using BooleanOperatorView = ygg::View<ygg::Data<BooleanOperator<T>>, Repository>;

template<TaskKind T>
using BooleanOperatorListView = ygg::View<ygg::DataList<BooleanOperator<T>>, Repository>;
template<TaskKind T>
using BooleanOperatorViewList = std::vector<BooleanOperatorView<T>>;

template<FactKind F>
using FunctionView = ygg::View<ygg::Index<Function<F>>, Repository>;

template<FactKind F>
using FunctionListView = ygg::View<ygg::IndexList<Function<F>>, Repository>;
template<FactKind F>
using FunctionViewList = std::vector<FunctionView<F>>;

using MetricView = ygg::View<ygg::Index<Metric>, Repository>;

using MetricListView = ygg::View<ygg::IndexList<Metric>, Repository>;

template<TaskKind T>
using MultiOperatorView = ygg::View<ygg::Index<MultiOperator<T>>, Repository>;

template<TaskKind T>
using MultiOperatorListView = ygg::View<ygg::IndexList<MultiOperator<T>>, Repository>;

using ObjectView = ygg::View<ygg::Index<Object>, Repository>;

using ObjectListView = ygg::View<ygg::IndexList<Object>, Repository>;
using ObjectViewList = std::vector<ObjectView>;

template<FactKind F>
using PredicateView = ygg::View<ygg::Index<Predicate<F>>, Repository>;

template<FactKind F>
using PredicateListView = ygg::View<ygg::IndexList<Predicate<F>>, Repository>;
template<FactKind F>
using PredicateViewList = std::vector<PredicateView<F>>;

using TermView = ygg::View<ygg::Data<Term>, Repository>;

using TermListView = ygg::View<ygg::DataList<Term>, Repository>;
using TermViewList = std::vector<TermView>;

template<TaskKind T>
using UnaryOperatorView = ygg::View<ygg::Index<UnaryOperator<T>>, Repository>;

template<TaskKind T>
using UnaryOperatorListView = ygg::View<ygg::IndexList<UnaryOperator<T>>, Repository>;

using VariableView = ygg::View<ygg::Index<Variable>, Repository>;

using VariableListView = ygg::View<ygg::IndexList<Variable>, Repository>;
using VariableViewList = std::vector<VariableView>;

template<TaskKind T, FactKind F>
using AtomListView = ygg::View<ygg::IndexList<Atom<T, F>>, Repository>;
template<TaskKind T, FactKind F>
using AtomView = ygg::View<ygg::Index<Atom<T, F>>, Repository>;
template<TaskKind T, FactKind F>
using AtomViewList = std::vector<AtomView<T, F>>;
template<TaskKind T>
using ConditionalEffectListView = ygg::View<ygg::IndexList<ConditionalEffect<T>>, Repository>;
template<TaskKind T>
using ConditionalEffectView = ygg::View<ygg::Index<ConditionalEffect<T>>, Repository>;
template<TaskKind T>
using ConditionalEffectViewList = std::vector<ConditionalEffectView<T>>;
template<TaskKind T>
using ConjunctiveConditionListView = ygg::View<ygg::IndexList<ConjunctiveCondition<T>>, Repository>;
template<TaskKind T>
using ConjunctiveConditionView = ygg::View<ygg::Index<ConjunctiveCondition<T>>, Repository>;
template<TaskKind T>
using ConjunctiveEffectListView = ygg::View<ygg::IndexList<ConjunctiveEffect<T>>, Repository>;
template<TaskKind T>
using ConjunctiveEffectView = ygg::View<ygg::Index<ConjunctiveEffect<T>>, Repository>;
template<TaskKind T>
using FunctionExpressionListView = ygg::View<ygg::DataList<FunctionExpression<T>>, Repository>;
template<TaskKind T>
using FunctionExpressionView = ygg::View<ygg::Data<FunctionExpression<T>>, Repository>;
template<TaskKind T, FactKind F>
using FunctionTermListView = ygg::View<ygg::IndexList<FunctionTerm<T, F>>, Repository>;
template<TaskKind T, FactKind F>
using FunctionTermValueListView = ygg::View<ygg::IndexList<FunctionTermValue<T, F>>, Repository>;
template<TaskKind T, FactKind F>
using FunctionTermValueView = ygg::View<ygg::Index<FunctionTermValue<T, F>>, Repository>;
template<TaskKind T, FactKind F>
using FunctionTermValueViewList = std::vector<FunctionTermValueView<T, F>>;
template<TaskKind T, FactKind F>
using FunctionTermView = ygg::View<ygg::Index<FunctionTerm<T, F>>, Repository>;
template<TaskKind T, FactKind F>
using FunctionTermViewList = std::vector<FunctionTermView<T, F>>;
template<TaskKind T, FactKind F>
using LiteralListView = ygg::View<ygg::IndexList<Literal<T, F>>, Repository>;
template<TaskKind T, FactKind F>
using LiteralView = ygg::View<ygg::Index<Literal<T, F>>, Repository>;
template<TaskKind T, FactKind F>
using LiteralViewList = std::vector<LiteralView<T, F>>;
template<TaskKind T, FactKind F>
using NumericEffectOperatorListView = ygg::View<ygg::DataList<NumericEffectOperator<T, F>>, Repository>;
template<TaskKind T, FactKind F>
using NumericEffectOperatorView = ygg::View<ygg::Data<NumericEffectOperator<T, F>>, Repository>;
template<TaskKind T, FactKind F>
using NumericEffectOperatorViewList = std::vector<NumericEffectOperatorView<T, F>>;
template<TaskKind T, FactKind F>
using NumericEffectView = ygg::View<ygg::Index<NumericEffect<T, F>>, Repository>;
template<TaskKind T>
using ProgramListView = ygg::View<ygg::IndexList<Program<T>>, Repository>;
template<TaskKind T>
using ProgramView = ygg::View<ygg::Index<Program<T>>, Repository>;
template<TaskKind T, RelationKind R>
using RuleListView = ygg::View<ygg::IndexList<Rule<T, R>>, Repository>;
template<TaskKind T, RelationKind R>
using RuleView = ygg::View<ygg::Index<Rule<T, R>>, Repository>;
template<TaskKind T, RelationKind R>
using RuleViewList = std::vector<RuleView<T, R>>;

/**
 * Context
 */

template<typename Repo, typename Tag>
concept RepositoryAccess = requires(const Repo& r, ygg::Index<Tag> idx) {
    requires ygg::CanonicalizableContext<ygg::Index<Tag>, Repo>;
    { r[idx] } -> std::same_as<const ygg::Data<Tag>&>;
};

template<typename Repo, typename... Tags>
constexpr bool repository_access_for_types(ygg::TypeList<Tags...>) noexcept
{
    return (RepositoryAccess<Repo, Tags> && ...);
}

template<typename T>
concept RepositoryConcept = repository_access_for_types<T>(SymbolRepositoryTypes {}) && repository_access_for_types<T>(RelationRepositoryTypes {});

template<typename T>
    requires RepositoryConcept<T>
inline const T& get_repository(const T& context) noexcept
{
    return context;
}

template<typename T>
concept Context = requires(const T& a) {
    { get_repository(a) } -> RepositoryConcept;
};

struct GrounderContext;
struct MergeContext;
class VariableDependencyGraph;

}

#endif
