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

template<typename T>
struct UnaryOperator
{
};

template<BinaryOperatorKind Operator, typename T>
struct BinaryOperator
{
};

template<typename T>
struct MultiOperator
{
};

template<typename T>
class BooleanOperator
{
};
template<typename T>
class ArithmeticOperator
{
};

template<::tyr::TaskKind T, FactKind F>
struct Atom
{
};

template<::tyr::TaskKind T, FactKind F>
struct Literal
{
};

template<::tyr::TaskKind T, FactKind F>
struct FunctionTerm
{
};

template<::tyr::TaskKind T, FactKind F>
struct NumericEffect
{
};

template<::tyr::TaskKind T, FactKind F>
struct NumericEffectOperator
{
};

template<::tyr::TaskKind T, FactKind F>
struct FunctionTermValue;

template<FactKind F>
struct FunctionTermValue<::tyr::GroundTag, F>
{
};

template<::tyr::TaskKind T>
struct FunctionExpression
{
};

template<::tyr::TaskKind T>
struct ConjunctiveCondition
{
};

template<::tyr::TaskKind T>
struct ConjunctiveEffect
{
};

template<::tyr::TaskKind T>
struct ConditionalEffect
{
};

template<::tyr::TaskKind T, RelationKind R>
struct Rule
{
};

template<::tyr::TaskKind T, RelationKind R>
struct RuleHead;

template<::tyr::TaskKind T>
struct RuleHead<T, PredicateTag>
{
    using type = ygg::Index<Atom<T, FluentTag>>;
};

template<::tyr::TaskKind T>
struct RuleHead<T, FunctionTag>
{
    using type = ygg::Data<NumericEffectOperator<T, FluentTag>>;
};

template<::tyr::TaskKind T, RelationKind R>
using RuleHeadT = typename RuleHead<T, R>::type;

struct Metric
{
};

template<::tyr::TaskKind T>
struct Program
{
};

using CoreTypes = ygg::TypeList<Variable, Object>;
using PredicateTypes = ygg::MapTypeListT<Predicate, StaticFluentTags>;
template<::tyr::TaskKind T>
using AtomTypes = ygg::MapTypeListSecondT<Atom, T, StaticFluentTags>;
template<::tyr::TaskKind T>
using LiteralTypes = ygg::MapTypeListSecondT<Literal, T, StaticFluentTags>;
using FunctionTypes = ygg::MapTypeListT<Function, StaticFluentAuxiliaryTags>;
template<::tyr::TaskKind T>
using FunctionTermTypes = ygg::MapTypeListSecondT<FunctionTerm, T, StaticFluentAuxiliaryTags>;
template<::tyr::TaskKind T>
using FunctionTermValueTypes = ygg::MapTypeListSecondT<FunctionTermValue, T, StaticFluentAuxiliaryTags>;
template<::tyr::TaskKind T>
using NumericEffectTypes = ygg::TypeList<NumericEffect<T, FluentTag>>;
template<::tyr::TaskKind T>
using NumericEffectOperatorTypes = ygg::TypeList<NumericEffectOperator<T, FluentTag>>;

using LiftedUnaryOperatorType = UnaryOperator<ygg::Data<FunctionExpression<::tyr::LiftedTag>>>;
template<BinaryOperatorKind Operator>
using LiftedBinaryOperatorType = BinaryOperator<Operator, ygg::Data<FunctionExpression<::tyr::LiftedTag>>>;
using LiftedMultiOperatorType = MultiOperator<ygg::Data<FunctionExpression<::tyr::LiftedTag>>>;

using GroundUnaryOperatorType = UnaryOperator<ygg::Data<FunctionExpression<::tyr::GroundTag>>>;
template<BinaryOperatorKind Operator>
using GroundBinaryOperatorType = BinaryOperator<Operator, ygg::Data<FunctionExpression<::tyr::GroundTag>>>;
using GroundMultiOperatorType = MultiOperator<ygg::Data<FunctionExpression<::tyr::GroundTag>>>;

using LiftedArithmeticExpressionTypes = ygg::TypeList<LiftedUnaryOperatorType, LiftedBinaryOperatorType<ArithmeticOperatorKind>, LiftedMultiOperatorType>;
using LiftedBooleanExpressionTypes = ygg::TypeList<LiftedBinaryOperatorType<BooleanOperatorKind>>;
using GroundArithmeticExpressionTypes = ygg::TypeList<GroundUnaryOperatorType, GroundBinaryOperatorType<ArithmeticOperatorKind>, GroundMultiOperatorType>;
using GroundBooleanExpressionTypes = ygg::TypeList<GroundBinaryOperatorType<BooleanOperatorKind>>;
using ExpressionTypes =
    ygg::ConcatTypeListsT<LiftedArithmeticExpressionTypes, LiftedBooleanExpressionTypes, GroundArithmeticExpressionTypes, GroundBooleanExpressionTypes>;
using EffectTypes = ygg::ConcatTypeListsT<NumericEffectTypes<::tyr::LiftedTag>, NumericEffectTypes<::tyr::GroundTag>>;
template<::tyr::TaskKind T>
using RuleTypes = ygg::MapTypeListSecondT<Rule, T, PredicateFunctionTags>;

using CompoundTypes =
    ygg::ConcatTypeListsT<ygg::TypeList<ConjunctiveCondition<::tyr::LiftedTag>, ConjunctiveEffect<::tyr::LiftedTag>, ConditionalEffect<::tyr::LiftedTag>>,
                          RuleTypes<::tyr::LiftedTag>,
                          ygg::TypeList<ConjunctiveCondition<::tyr::GroundTag>, ConjunctiveEffect<::tyr::GroundTag>, ConditionalEffect<::tyr::GroundTag>>,
                          RuleTypes<::tyr::GroundTag>,
                          ygg::TypeList<Metric, Program<::tyr::LiftedTag>, Program<::tyr::GroundTag>>>;

using SymbolRepositoryTypes = ygg::ConcatTypeListsT<CoreTypes,
                                                    PredicateTypes,
                                                    AtomTypes<::tyr::LiftedTag>,
                                                    AtomTypes<::tyr::GroundTag>,
                                                    LiteralTypes<::tyr::LiftedTag>,
                                                    LiteralTypes<::tyr::GroundTag>,
                                                    FunctionTypes,
                                                    FunctionTermTypes<::tyr::LiftedTag>,
                                                    FunctionTermTypes<::tyr::GroundTag>,
                                                    FunctionTermValueTypes<::tyr::GroundTag>,
                                                    ExpressionTypes,
                                                    EffectTypes,
                                                    CompoundTypes>;
using RelationRepositoryTypes = ygg::ConcatTypeListsT<PredicateTypes, FunctionTypes, RuleTypes<::tyr::LiftedTag>>;
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
template<typename T>
using ArithmeticOperatorView = ygg::View<ygg::Data<ArithmeticOperator<T>>, Repository>;
using LiftedArithmeticOperatorView = ygg::View<ygg::Data<ArithmeticOperator<ygg::Data<FunctionExpression<::tyr::LiftedTag>>>>, Repository>;
using GroundArithmeticOperatorView = ygg::View<ygg::Data<ArithmeticOperator<ygg::Data<FunctionExpression<::tyr::GroundTag>>>>, Repository>;

template<typename T>
using ArithmeticOperatorListView = ygg::View<ygg::DataList<ArithmeticOperator<T>>, Repository>;
using LiftedArithmeticOperatorListView = ygg::View<ygg::DataList<ArithmeticOperator<ygg::Data<FunctionExpression<::tyr::LiftedTag>>>>, Repository>;
using GroundArithmeticOperatorListView = ygg::View<ygg::DataList<ArithmeticOperator<ygg::Data<FunctionExpression<::tyr::GroundTag>>>>, Repository>;

template<::tyr::formalism::BinaryOperatorKind Operator, typename T>
using BinaryOperatorView = ygg::View<ygg::Index<BinaryOperator<Operator, T>>, Repository>;
template<::tyr::formalism::BinaryOperatorKind Operator>
using LiftedBinaryOperatorView = ygg::View<ygg::Index<BinaryOperator<Operator, ygg::Data<FunctionExpression<::tyr::LiftedTag>>>>, Repository>;
template<::tyr::formalism::BinaryOperatorKind Operator>
using GroundBinaryOperatorView = ygg::View<ygg::Index<BinaryOperator<Operator, ygg::Data<FunctionExpression<::tyr::GroundTag>>>>, Repository>;

template<::tyr::formalism::BinaryOperatorKind Operator, typename T>
using BinaryOperatorListView = ygg::View<ygg::IndexList<BinaryOperator<Operator, T>>, Repository>;
template<::tyr::formalism::BinaryOperatorKind Operator>
using LiftedBinaryOperatorListView = ygg::View<ygg::IndexList<BinaryOperator<Operator, ygg::Data<FunctionExpression<::tyr::LiftedTag>>>>, Repository>;
template<::tyr::formalism::BinaryOperatorKind Operator>
using GroundBinaryOperatorListView = ygg::View<ygg::IndexList<BinaryOperator<Operator, ygg::Data<FunctionExpression<::tyr::GroundTag>>>>, Repository>;

template<FactKind F>
using PredicateBindingView = ygg::View<ygg::Index<RelationBinding<Predicate<F>>>, Repository>;
template<FactKind F>
using FunctionBindingView = ygg::View<ygg::Index<RelationBinding<Function<F>>>, Repository>;
template<RelationKind R>
using RuleBindingView = ygg::View<ygg::Index<RelationBinding<Rule<::tyr::LiftedTag, R>>>, Repository>;

template<FactKind F>
using PredicateBindingForwardRangeView = ygg::View<RelationBindingsForwardRange<Predicate<F>, std::vector<ygg::Index<Row>>>, Repository>;
template<FactKind F>
using FunctionBindingRandomAccessRangeView = ygg::View<RelationBindingsRandomAccessRange<Function<F>, std::vector<ygg::Index<Row>>>, Repository>;

template<typename T>
using BooleanOperatorView = ygg::View<ygg::Data<BooleanOperator<T>>, Repository>;
using LiftedBooleanOperatorView = ygg::View<ygg::Data<BooleanOperator<ygg::Data<FunctionExpression<::tyr::LiftedTag>>>>, Repository>;
using GroundBooleanOperatorView = ygg::View<ygg::Data<BooleanOperator<ygg::Data<FunctionExpression<::tyr::GroundTag>>>>, Repository>;

template<typename T>
using BooleanOperatorListView = ygg::View<ygg::DataList<BooleanOperator<T>>, Repository>;
using LiftedBooleanOperatorListView = ygg::View<ygg::DataList<BooleanOperator<ygg::Data<FunctionExpression<::tyr::LiftedTag>>>>, Repository>;
using GroundBooleanOperatorListView = ygg::View<ygg::DataList<BooleanOperator<ygg::Data<FunctionExpression<::tyr::GroundTag>>>>, Repository>;
using LiftedBooleanOperatorViewList = std::vector<LiftedBooleanOperatorView>;
using GroundBooleanOperatorViewList = std::vector<GroundBooleanOperatorView>;

template<FactKind F>
using FunctionView = ygg::View<ygg::Index<Function<F>>, Repository>;

template<FactKind F>
using FunctionListView = ygg::View<ygg::IndexList<Function<F>>, Repository>;
template<FactKind F>
using FunctionViewList = std::vector<FunctionView<F>>;

using MetricView = ygg::View<ygg::Index<Metric>, Repository>;

using MetricListView = ygg::View<ygg::IndexList<Metric>, Repository>;

template<typename T>
using MultiOperatorView = ygg::View<ygg::Index<MultiOperator<T>>, Repository>;
using LiftedMultiOperatorView = ygg::View<ygg::Index<MultiOperator<ygg::Data<FunctionExpression<::tyr::LiftedTag>>>>, Repository>;
using GroundMultiOperatorView = ygg::View<ygg::Index<MultiOperator<ygg::Data<FunctionExpression<::tyr::GroundTag>>>>, Repository>;

template<typename T>
using MultiOperatorListView = ygg::View<ygg::IndexList<MultiOperator<T>>, Repository>;
using LiftedMultiOperatorListView = ygg::View<ygg::IndexList<MultiOperator<ygg::Data<FunctionExpression<::tyr::LiftedTag>>>>, Repository>;
using GroundMultiOperatorListView = ygg::View<ygg::IndexList<MultiOperator<ygg::Data<FunctionExpression<::tyr::GroundTag>>>>, Repository>;

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

template<typename T>
using UnaryOperatorView = ygg::View<ygg::Index<UnaryOperator<T>>, Repository>;
using LiftedUnaryOperatorView = ygg::View<ygg::Index<UnaryOperator<ygg::Data<FunctionExpression<::tyr::LiftedTag>>>>, Repository>;
using GroundUnaryOperatorView = ygg::View<ygg::Index<UnaryOperator<ygg::Data<FunctionExpression<::tyr::GroundTag>>>>, Repository>;

template<typename T>
using UnaryOperatorListView = ygg::View<ygg::IndexList<UnaryOperator<T>>, Repository>;
using LiftedUnaryOperatorListView = ygg::View<ygg::IndexList<UnaryOperator<ygg::Data<FunctionExpression<::tyr::LiftedTag>>>>, Repository>;
using GroundUnaryOperatorListView = ygg::View<ygg::IndexList<UnaryOperator<ygg::Data<FunctionExpression<::tyr::GroundTag>>>>, Repository>;

using VariableView = ygg::View<ygg::Index<Variable>, Repository>;

using VariableListView = ygg::View<ygg::IndexList<Variable>, Repository>;
using VariableViewList = std::vector<VariableView>;

template<::tyr::TaskKind T, FactKind F>
using AtomListView = ygg::View<ygg::IndexList<Atom<T, F>>, Repository>;
template<::tyr::TaskKind T, FactKind F>
using AtomView = ygg::View<ygg::Index<Atom<T, F>>, Repository>;
template<::tyr::TaskKind T, FactKind F>
using AtomViewList = std::vector<AtomView<T, F>>;
template<::tyr::TaskKind T>
using ConditionalEffectListView = ygg::View<ygg::IndexList<ConditionalEffect<T>>, Repository>;
template<::tyr::TaskKind T>
using ConditionalEffectView = ygg::View<ygg::Index<ConditionalEffect<T>>, Repository>;
template<::tyr::TaskKind T>
using ConditionalEffectViewList = std::vector<ConditionalEffectView<T>>;
template<::tyr::TaskKind T>
using ConjunctiveConditionListView = ygg::View<ygg::IndexList<ConjunctiveCondition<T>>, Repository>;
template<::tyr::TaskKind T>
using ConjunctiveConditionView = ygg::View<ygg::Index<ConjunctiveCondition<T>>, Repository>;
template<::tyr::TaskKind T>
using ConjunctiveEffectListView = ygg::View<ygg::IndexList<ConjunctiveEffect<T>>, Repository>;
template<::tyr::TaskKind T>
using ConjunctiveEffectView = ygg::View<ygg::Index<ConjunctiveEffect<T>>, Repository>;
template<::tyr::TaskKind T>
using FunctionExpressionListView = ygg::View<ygg::DataList<FunctionExpression<T>>, Repository>;
template<::tyr::TaskKind T>
using FunctionExpressionView = ygg::View<ygg::Data<FunctionExpression<T>>, Repository>;
template<::tyr::TaskKind T, FactKind F>
using FunctionTermListView = ygg::View<ygg::IndexList<FunctionTerm<T, F>>, Repository>;
template<::tyr::TaskKind T, FactKind F>
using FunctionTermValueListView = ygg::View<ygg::IndexList<FunctionTermValue<T, F>>, Repository>;
template<::tyr::TaskKind T, FactKind F>
using FunctionTermValueView = ygg::View<ygg::Index<FunctionTermValue<T, F>>, Repository>;
template<::tyr::TaskKind T, FactKind F>
using FunctionTermValueViewList = std::vector<FunctionTermValueView<T, F>>;
template<::tyr::TaskKind T, FactKind F>
using FunctionTermView = ygg::View<ygg::Index<FunctionTerm<T, F>>, Repository>;
template<::tyr::TaskKind T, FactKind F>
using FunctionTermViewList = std::vector<FunctionTermView<T, F>>;
template<::tyr::TaskKind T, FactKind F>
using LiteralListView = ygg::View<ygg::IndexList<Literal<T, F>>, Repository>;
template<::tyr::TaskKind T, FactKind F>
using LiteralView = ygg::View<ygg::Index<Literal<T, F>>, Repository>;
template<::tyr::TaskKind T, FactKind F>
using LiteralViewList = std::vector<LiteralView<T, F>>;
template<::tyr::TaskKind T, FactKind F>
using NumericEffectOperatorListView = ygg::View<ygg::DataList<NumericEffectOperator<T, F>>, Repository>;
template<::tyr::TaskKind T, FactKind F>
using NumericEffectOperatorView = ygg::View<ygg::Data<NumericEffectOperator<T, F>>, Repository>;
template<::tyr::TaskKind T, FactKind F>
using NumericEffectOperatorViewList = std::vector<NumericEffectOperatorView<T, F>>;
template<::tyr::TaskKind T, FactKind F>
using NumericEffectView = ygg::View<ygg::Index<NumericEffect<T, F>>, Repository>;
template<::tyr::TaskKind T>
using ProgramListView = ygg::View<ygg::IndexList<Program<T>>, Repository>;
template<::tyr::TaskKind T>
using ProgramView = ygg::View<ygg::Index<Program<T>>, Repository>;
template<::tyr::TaskKind T, RelationKind R>
using RuleListView = ygg::View<ygg::IndexList<Rule<T, R>>, Repository>;
template<::tyr::TaskKind T, RelationKind R>
using RuleView = ygg::View<ygg::Index<Rule<T, R>>, Repository>;
template<::tyr::TaskKind T, RelationKind R>
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
