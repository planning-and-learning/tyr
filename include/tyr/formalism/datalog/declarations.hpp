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

template<FactKind T>
struct Atom
{
};

template<FactKind T>
struct Literal
{
};

template<FactKind T>
struct GroundAtom
{
};

template<FactKind T>
struct GroundLiteral
{
};

template<FactKind T>
struct FunctionTerm
{
};

struct FunctionExpression
{
};

template<FactKind T>
struct GroundFunctionTerm
{
};

struct GroundFunctionExpression
{
};

template<FactKind T>
struct GroundFunctionTermValue
{
};

template<FactKind T>
struct NumericEffect
{
};
template<FactKind T>
struct GroundNumericEffect
{
};

template<FactKind T>
struct NumericEffectOperator
{
};
template<FactKind T>
struct GroundNumericEffectOperator
{
};

struct ConjunctiveCondition
{
};

struct GroundConjunctiveCondition
{
};

struct ConjunctiveEffect
{
};

struct GroundConjunctiveEffect
{
};

struct ConditionalEffect
{
};

struct GroundConditionalEffect
{
};

/// A rule derives either a predicate binding (its head is a fluent atom) or a function binding (its
/// head is a numeric effect). Keeping the two apart in the type system means a predicate rule carries
/// no numeric-effect head, so the numeric expression machinery is absent rather than skipped.
template<RelationKind R>
struct Rule
{
};

template<RelationKind R>
struct GroundRule
{
};

/// The head type a rule of each relation kind derives: a predicate rule derives one fluent atom, a
/// function rule applies one numeric effect. No variant, so a predicate rule never names the numeric
/// effect types at all.
template<RelationKind R>
struct RuleHead;

template<>
struct RuleHead<::tyr::formalism::PredicateTag>
{
    using type = ygg::Index<Atom<::tyr::formalism::FluentTag>>;
    using ground_type = ygg::Index<GroundAtom<::tyr::formalism::FluentTag>>;
};

template<>
struct RuleHead<::tyr::formalism::FunctionTag>
{
    using type = ygg::Data<NumericEffectOperator<::tyr::formalism::FluentTag>>;
    using ground_type = ygg::Data<GroundNumericEffectOperator<::tyr::formalism::FluentTag>>;
};

template<RelationKind R>
using RuleHeadT = typename RuleHead<R>::type;

template<RelationKind R>
using GroundRuleHeadT = typename RuleHead<R>::ground_type;

struct Metric
{
};

template<::tyr::TaskKind Kind>
struct ProgramTag
{
};

using Program = ProgramTag<::tyr::LiftedTag>;
using GroundProgram = ProgramTag<::tyr::GroundTag>;

using CoreTypes = ygg::TypeList<Variable, Object>;
using PredicateTypes = ygg::MapTypeListT<Predicate, StaticFluentTags>;
using AtomTypes = ygg::MapTypeListT<Atom, StaticFluentTags>;
using GroundAtomTypes = ygg::MapTypeListT<GroundAtom, StaticFluentTags>;
using LiteralTypes = ygg::MapTypeListT<Literal, StaticFluentTags>;
using GroundLiteralTypes = ygg::MapTypeListT<GroundLiteral, StaticFluentTags>;
using FunctionTypes = ygg::MapTypeListT<Function, StaticFluentAuxiliaryTags>;
using FunctionTermTypes = ygg::MapTypeListT<FunctionTerm, StaticFluentAuxiliaryTags>;
using GroundFunctionTermTypes = ygg::MapTypeListT<GroundFunctionTerm, StaticFluentAuxiliaryTags>;
using GroundFunctionTermValueTypes = ygg::MapTypeListT<GroundFunctionTermValue, StaticFluentAuxiliaryTags>;
using NumericEffectTypes = ygg::TypeList<NumericEffect<FluentTag>>;
using GroundNumericEffectTypes = ygg::TypeList<GroundNumericEffect<FluentTag>>;
using NumericEffectOperatorTypes = ygg::TypeList<NumericEffectOperator<FluentTag>>;
using GroundNumericEffectOperatorTypes = ygg::TypeList<GroundNumericEffectOperator<FluentTag>>;

using LiftedUnaryOperatorType = UnaryOperator<ygg::Data<FunctionExpression>>;
template<BinaryOperatorKind Operator>
using LiftedBinaryOperatorType = BinaryOperator<Operator, ygg::Data<FunctionExpression>>;
using LiftedMultiOperatorType = MultiOperator<ygg::Data<FunctionExpression>>;

using GroundUnaryOperatorType = UnaryOperator<ygg::Data<GroundFunctionExpression>>;
template<BinaryOperatorKind Operator>
using GroundBinaryOperatorType = BinaryOperator<Operator, ygg::Data<GroundFunctionExpression>>;
using GroundMultiOperatorType = MultiOperator<ygg::Data<GroundFunctionExpression>>;

using LiftedArithmeticExpressionTypes = ygg::TypeList<LiftedUnaryOperatorType, LiftedBinaryOperatorType<ArithmeticOperatorKind>, LiftedMultiOperatorType>;
using LiftedBooleanExpressionTypes = ygg::TypeList<LiftedBinaryOperatorType<BooleanOperatorKind>>;

using GroundArithmeticExpressionTypes = ygg::TypeList<GroundUnaryOperatorType, GroundBinaryOperatorType<ArithmeticOperatorKind>, GroundMultiOperatorType>;
using GroundBooleanExpressionTypes = ygg::TypeList<GroundBinaryOperatorType<BooleanOperatorKind>>;

using ExpressionTypes =
    ygg::ConcatTypeListsT<LiftedArithmeticExpressionTypes, LiftedBooleanExpressionTypes, GroundArithmeticExpressionTypes, GroundBooleanExpressionTypes>;
using EffectTypes = ygg::ConcatTypeListsT<NumericEffectTypes, GroundNumericEffectTypes>;
using RuleTypes = ygg::MapTypeListT<Rule, PredicateFunctionTags>;
using GroundRuleTypes = ygg::MapTypeListT<GroundRule, PredicateFunctionTags>;

using CompoundTypes = ygg::ConcatTypeListsT<ygg::TypeList<ConjunctiveCondition, ConjunctiveEffect, ConditionalEffect>,
                                            RuleTypes,
                                            ygg::TypeList<GroundConjunctiveCondition, GroundConjunctiveEffect, GroundConditionalEffect>,
                                            GroundRuleTypes,
                                            ygg::TypeList<Metric, Program, GroundProgram>>;

using SymbolRepositoryTypes = ygg::ConcatTypeListsT<CoreTypes,
                                                    PredicateTypes,
                                                    AtomTypes,
                                                    GroundAtomTypes,
                                                    LiteralTypes,
                                                    GroundLiteralTypes,
                                                    FunctionTypes,
                                                    FunctionTermTypes,
                                                    GroundFunctionTermTypes,
                                                    GroundFunctionTermValueTypes,
                                                    ExpressionTypes,
                                                    EffectTypes,
                                                    CompoundTypes>;

using RelationRepositoryTypes = ygg::ConcatTypeListsT<PredicateTypes, FunctionTypes, RuleTypes>;
using BuilderTypes = ygg::ConcatTypeListsT<SymbolRepositoryTypes, ygg::MapTypeListT<RelationBinding, RelationRepositoryTypes>>;

using SymbolRepository = ygg::ApplyTypeListT<::ygg::formalism::SymbolRepository, SymbolRepositoryTypes>;

template<typename... Ts>
using TaggedRelationRepository = ::ygg::formalism::RelationRepository<ObjectTag, Ts...>;

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
using LiftedArithmeticOperatorView = ygg::View<ygg::Data<ArithmeticOperator<ygg::Data<FunctionExpression>>>, Repository>;
using GroundArithmeticOperatorView = ygg::View<ygg::Data<ArithmeticOperator<ygg::Data<GroundFunctionExpression>>>, Repository>;

template<typename T>
using ArithmeticOperatorListView = ygg::View<ygg::DataList<ArithmeticOperator<T>>, Repository>;
using LiftedArithmeticOperatorListView = ygg::View<ygg::DataList<ArithmeticOperator<ygg::Data<FunctionExpression>>>, Repository>;
using GroundArithmeticOperatorListView = ygg::View<ygg::DataList<ArithmeticOperator<ygg::Data<GroundFunctionExpression>>>, Repository>;

template<::tyr::formalism::FactKind T>
using AtomView = ygg::View<ygg::Index<Atom<T>>, Repository>;

template<::tyr::formalism::FactKind T>
using AtomListView = ygg::View<ygg::IndexList<Atom<T>>, Repository>;
template<::tyr::formalism::FactKind T>
using AtomViewList = std::vector<AtomView<T>>;

template<::tyr::formalism::BinaryOperatorKind Operator, typename T>
using BinaryOperatorView = ygg::View<ygg::Index<BinaryOperator<Operator, T>>, Repository>;
template<::tyr::formalism::BinaryOperatorKind Operator>
using LiftedBinaryOperatorView = ygg::View<ygg::Index<BinaryOperator<Operator, ygg::Data<FunctionExpression>>>, Repository>;
template<::tyr::formalism::BinaryOperatorKind Operator>
using GroundBinaryOperatorView = ygg::View<ygg::Index<BinaryOperator<Operator, ygg::Data<GroundFunctionExpression>>>, Repository>;

template<::tyr::formalism::BinaryOperatorKind Operator, typename T>
using BinaryOperatorListView = ygg::View<ygg::IndexList<BinaryOperator<Operator, T>>, Repository>;
template<::tyr::formalism::BinaryOperatorKind Operator>
using LiftedBinaryOperatorListView = ygg::View<ygg::IndexList<BinaryOperator<Operator, ygg::Data<FunctionExpression>>>, Repository>;
template<::tyr::formalism::BinaryOperatorKind Operator>
using GroundBinaryOperatorListView = ygg::View<ygg::IndexList<BinaryOperator<Operator, ygg::Data<GroundFunctionExpression>>>, Repository>;

template<FactKind T>
using PredicateBindingView = ygg::View<ygg::Index<RelationBinding<Predicate<T>>>, Repository>;
template<FactKind T>
using FunctionBindingView = ygg::View<ygg::Index<RelationBinding<Function<T>>>, Repository>;
template<RelationKind R>
using RuleBindingView = ygg::View<ygg::Index<RelationBinding<Rule<R>>>, Repository>;

template<FactKind T>
using PredicateBindingForwardRangeView = ygg::View<RelationBindingsForwardRange<Predicate<T>, std::vector<ygg::Index<Row>>>, Repository>;
template<FactKind T>
using FunctionBindingRandomAccessRangeView = ygg::View<RelationBindingsRandomAccessRange<Function<T>, std::vector<ygg::Index<Row>>>, Repository>;

template<typename T>
using BooleanOperatorView = ygg::View<ygg::Data<BooleanOperator<T>>, Repository>;
using LiftedBooleanOperatorView = ygg::View<ygg::Data<BooleanOperator<ygg::Data<FunctionExpression>>>, Repository>;
using GroundBooleanOperatorView = ygg::View<ygg::Data<BooleanOperator<ygg::Data<GroundFunctionExpression>>>, Repository>;

template<typename T>
using BooleanOperatorListView = ygg::View<ygg::DataList<BooleanOperator<T>>, Repository>;
using LiftedBooleanOperatorListView = ygg::View<ygg::DataList<BooleanOperator<ygg::Data<FunctionExpression>>>, Repository>;
using GroundBooleanOperatorListView = ygg::View<ygg::DataList<BooleanOperator<ygg::Data<GroundFunctionExpression>>>, Repository>;
using LiftedBooleanOperatorViewList = std::vector<LiftedBooleanOperatorView>;
using GroundBooleanOperatorViewList = std::vector<GroundBooleanOperatorView>;

using ConjunctiveConditionView = ygg::View<ygg::Index<ConjunctiveCondition>, Repository>;

using ConjunctiveConditionListView = ygg::View<ygg::IndexList<ConjunctiveCondition>, Repository>;

using ConjunctiveEffectView = ygg::View<ygg::Index<ConjunctiveEffect>, Repository>;

using ConjunctiveEffectListView = ygg::View<ygg::IndexList<ConjunctiveEffect>, Repository>;

using ConditionalEffectView = ygg::View<ygg::Index<ConditionalEffect>, Repository>;

using ConditionalEffectListView = ygg::View<ygg::IndexList<ConditionalEffect>, Repository>;
using ConditionalEffectViewList = std::vector<ConditionalEffectView>;

using FunctionExpressionView = ygg::View<ygg::Data<FunctionExpression>, Repository>;

using FunctionExpressionListView = ygg::View<ygg::DataList<FunctionExpression>, Repository>;

template<::tyr::formalism::FactKind T>
using FunctionTermView = ygg::View<ygg::Index<FunctionTerm<T>>, Repository>;

template<::tyr::formalism::FactKind T>
using FunctionTermListView = ygg::View<ygg::IndexList<FunctionTerm<T>>, Repository>;

template<FactKind T>
using FunctionView = ygg::View<ygg::Index<Function<T>>, Repository>;

template<FactKind T>
using FunctionListView = ygg::View<ygg::IndexList<Function<T>>, Repository>;
template<FactKind T>
using FunctionViewList = std::vector<FunctionView<T>>;

template<::tyr::formalism::FactKind T>
using GroundAtomView = ygg::View<ygg::Index<GroundAtom<T>>, Repository>;

template<::tyr::formalism::FactKind T>
using GroundAtomListView = ygg::View<ygg::IndexList<GroundAtom<T>>, Repository>;
template<::tyr::formalism::FactKind T>
using GroundAtomViewList = std::vector<GroundAtomView<T>>;

using GroundConjunctiveConditionView = ygg::View<ygg::Index<GroundConjunctiveCondition>, Repository>;

using GroundConjunctiveConditionListView = ygg::View<ygg::IndexList<GroundConjunctiveCondition>, Repository>;

using GroundConjunctiveEffectView = ygg::View<ygg::Index<GroundConjunctiveEffect>, Repository>;

using GroundConjunctiveEffectListView = ygg::View<ygg::IndexList<GroundConjunctiveEffect>, Repository>;

using GroundConditionalEffectView = ygg::View<ygg::Index<GroundConditionalEffect>, Repository>;

using GroundConditionalEffectListView = ygg::View<ygg::IndexList<GroundConditionalEffect>, Repository>;
using GroundConditionalEffectViewList = std::vector<GroundConditionalEffectView>;

using GroundFunctionExpressionView = ygg::View<ygg::Data<GroundFunctionExpression>, Repository>;

using GroundFunctionExpressionListView = ygg::View<ygg::DataList<GroundFunctionExpression>, Repository>;

template<::tyr::formalism::FactKind T>
using GroundFunctionTermValueView = ygg::View<ygg::Index<GroundFunctionTermValue<T>>, Repository>;

template<::tyr::formalism::FactKind T>
using GroundFunctionTermValueListView = ygg::View<ygg::IndexList<GroundFunctionTermValue<T>>, Repository>;
template<::tyr::formalism::FactKind T>
using GroundFunctionTermValueViewList = std::vector<GroundFunctionTermValueView<T>>;

template<::tyr::formalism::FactKind T>
using GroundFunctionTermView = ygg::View<ygg::Index<GroundFunctionTerm<T>>, Repository>;

template<::tyr::formalism::FactKind T>
using GroundFunctionTermListView = ygg::View<ygg::IndexList<GroundFunctionTerm<T>>, Repository>;
template<::tyr::formalism::FactKind T>
using GroundFunctionTermViewList = std::vector<GroundFunctionTermView<T>>;

template<::tyr::formalism::FactKind T>
using GroundLiteralView = ygg::View<ygg::Index<GroundLiteral<T>>, Repository>;

template<::tyr::formalism::FactKind T>
using GroundLiteralListView = ygg::View<ygg::IndexList<GroundLiteral<T>>, Repository>;
template<::tyr::formalism::FactKind T>
using GroundLiteralViewList = std::vector<GroundLiteralView<T>>;

template<::tyr::formalism::FactKind T>
using GroundNumericEffectView = ygg::View<ygg::Index<GroundNumericEffect<T>>, Repository>;

template<::tyr::formalism::FactKind T>
using GroundNumericEffectOperatorView = ygg::View<ygg::Data<GroundNumericEffectOperator<T>>, Repository>;
template<::tyr::formalism::FactKind T>
using GroundNumericEffectOperatorListView = ygg::View<ygg::DataList<GroundNumericEffectOperator<T>>, Repository>;
template<::tyr::formalism::FactKind T>
using GroundNumericEffectOperatorViewList = std::vector<GroundNumericEffectOperatorView<T>>;

template<RelationKind R>
using GroundRuleView = ygg::View<ygg::Index<GroundRule<R>>, Repository>;

template<RelationKind R>
using GroundRuleListView = ygg::View<ygg::IndexList<GroundRule<R>>, Repository>;
template<RelationKind R>
using GroundRuleViewList = std::vector<GroundRuleView<R>>;

using GroundProgramView = ygg::View<ygg::Index<ProgramTag<::tyr::GroundTag>>, Repository>;

using GroundProgramListView = ygg::View<ygg::IndexList<ProgramTag<::tyr::GroundTag>>, Repository>;

using MetricView = ygg::View<ygg::Index<Metric>, Repository>;

using MetricListView = ygg::View<ygg::IndexList<Metric>, Repository>;

template<::tyr::formalism::FactKind T>
using LiteralView = ygg::View<ygg::Index<Literal<T>>, Repository>;

template<::tyr::formalism::FactKind T>
using LiteralListView = ygg::View<ygg::IndexList<Literal<T>>, Repository>;
template<::tyr::formalism::FactKind T>
using LiteralViewList = std::vector<LiteralView<T>>;

template<typename T>
using MultiOperatorView = ygg::View<ygg::Index<MultiOperator<T>>, Repository>;
using LiftedMultiOperatorView = ygg::View<ygg::Index<MultiOperator<ygg::Data<FunctionExpression>>>, Repository>;
using GroundMultiOperatorView = ygg::View<ygg::Index<MultiOperator<ygg::Data<GroundFunctionExpression>>>, Repository>;

template<typename T>
using MultiOperatorListView = ygg::View<ygg::IndexList<MultiOperator<T>>, Repository>;
using LiftedMultiOperatorListView = ygg::View<ygg::IndexList<MultiOperator<ygg::Data<FunctionExpression>>>, Repository>;
using GroundMultiOperatorListView = ygg::View<ygg::IndexList<MultiOperator<ygg::Data<GroundFunctionExpression>>>, Repository>;

using ObjectView = ygg::View<ygg::Index<Object>, Repository>;

using ObjectListView = ygg::View<ygg::IndexList<Object>, Repository>;
using ObjectViewList = std::vector<ObjectView>;

template<::tyr::formalism::FactKind T>
using NumericEffectView = ygg::View<ygg::Index<NumericEffect<T>>, Repository>;

template<::tyr::formalism::FactKind T>
using NumericEffectOperatorView = ygg::View<ygg::Data<NumericEffectOperator<T>>, Repository>;
template<::tyr::formalism::FactKind T>
using NumericEffectOperatorListView = ygg::View<ygg::DataList<NumericEffectOperator<T>>, Repository>;
template<::tyr::formalism::FactKind T>
using NumericEffectOperatorViewList = std::vector<NumericEffectOperatorView<T>>;

template<FactKind T>
using PredicateView = ygg::View<ygg::Index<Predicate<T>>, Repository>;

template<FactKind T>
using PredicateListView = ygg::View<ygg::IndexList<Predicate<T>>, Repository>;
template<FactKind T>
using PredicateViewList = std::vector<PredicateView<T>>;

template<::tyr::TaskKind Kind>
using ProgramView = ygg::View<ygg::Index<ProgramTag<Kind>>, Repository>;

template<::tyr::TaskKind Kind>
using ProgramListView = ygg::View<ygg::IndexList<ProgramTag<Kind>>, Repository>;

template<RelationKind R>
using RuleView = ygg::View<ygg::Index<Rule<R>>, Repository>;

template<RelationKind R>
using RuleListView = ygg::View<ygg::IndexList<Rule<R>>, Repository>;
template<RelationKind R>
using RuleViewList = std::vector<RuleView<R>>;

using TermView = ygg::View<ygg::Data<Term>, Repository>;

using TermListView = ygg::View<ygg::DataList<Term>, Repository>;
using TermViewList = std::vector<TermView>;

template<typename T>
using UnaryOperatorView = ygg::View<ygg::Index<UnaryOperator<T>>, Repository>;
using LiftedUnaryOperatorView = ygg::View<ygg::Index<UnaryOperator<ygg::Data<FunctionExpression>>>, Repository>;
using GroundUnaryOperatorView = ygg::View<ygg::Index<UnaryOperator<ygg::Data<GroundFunctionExpression>>>, Repository>;

template<typename T>
using UnaryOperatorListView = ygg::View<ygg::IndexList<UnaryOperator<T>>, Repository>;
using LiftedUnaryOperatorListView = ygg::View<ygg::IndexList<UnaryOperator<ygg::Data<FunctionExpression>>>, Repository>;
using GroundUnaryOperatorListView = ygg::View<ygg::IndexList<UnaryOperator<ygg::Data<GroundFunctionExpression>>>, Repository>;

using VariableView = ygg::View<ygg::Index<Variable>, Repository>;

using VariableListView = ygg::View<ygg::IndexList<Variable>, Repository>;
using VariableViewList = std::vector<VariableView>;

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

}

#endif
