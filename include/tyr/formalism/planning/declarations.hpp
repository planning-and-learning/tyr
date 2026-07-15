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

#ifndef TYR_FORMALISM_PLANNING_DECLARATIONS_HPP_
#define TYR_FORMALISM_PLANNING_DECLARATIONS_HPP_

#include <yggdrasil/core/config.hpp>
#include <yggdrasil/core/types.hpp>
#include "tyr/formalism/declarations.hpp"

#include <memory>

namespace tyr::formalism::planning
{

/**
 * Formalism tag
 */

template<OpKind Op, typename T>
struct UnaryOperator
{
};

template<OpKind Op, typename T>
struct BinaryOperator
{
};

template<OpKind Op, typename T>
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

using EffectFamilyList = std::vector<EffectFamily>;

inline bool is_compatible_effect_family(EffectFamily lhs, EffectFamily rhs)
{
    if (lhs == EffectFamily::NONE || rhs == EffectFamily::NONE)
        return true;  ///< first effect

    if (lhs == rhs)
        return lhs != EffectFamily::ASSIGN;  ///< disallow double assignment.

    return false;  ///< disallow mixing assign, additive, or multiplicative
}

template<NumericEffectOpKind Op, FactKind T>
struct NumericEffect
{
};
template<NumericEffectOpKind Op, FactKind T>
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

struct ConditionalEffect
{
};
struct GroundConditionalEffect
{
};

struct ConjunctiveEffect
{
};
struct GroundConjunctiveEffect
{
};

struct Action
{
};
struct GroundAction
{
};

struct Axiom
{
};
struct GroundAxiom
{
};

struct Minimize
{
    static constexpr int kind = 0;
    auto identifying_members() const noexcept { return std::tie(kind); }
};
struct Maximize
{
    static constexpr int kind = 1;
    auto identifying_members() const noexcept { return std::tie(kind); }
};

template<typename T>
concept ObjectiveKind = std::same_as<T, Minimize> || std::same_as<T, Maximize>;

struct Metric
{
};

struct Task
{
};

struct Domain
{
};

template<FactKind T>
struct FDRVariable
{
};

template<FactKind T>
struct FDRFact
{
};

struct ConjunctiveCondition
{
};

struct GroundConjunctiveCondition
{
};

struct FDRAction
{
};

struct FDRAxiom
{
};

struct FDRTask
{
};

using CoreTypes = ygg::TypeList<Variable, Object>;
using PredicateTypes = ygg::MapTypeListT<Predicate, StaticFluentDerivedTags>;
using AtomTypes = ygg::MapTypeListT<Atom, StaticFluentDerivedTags>;
using GroundAtomTypes = ygg::MapTypeListT<GroundAtom, StaticFluentDerivedTags>;
using LiteralTypes = ygg::MapTypeListT<Literal, StaticFluentDerivedTags>;
using GroundLiteralTypes = ygg::MapTypeListT<GroundLiteral, StaticFluentDerivedTags>;
using FunctionTypes = ygg::MapTypeListT<Function, StaticFluentAuxiliaryTags>;
using FunctionTermTypes = ygg::MapTypeListT<FunctionTerm, StaticFluentAuxiliaryTags>;
using GroundFunctionTermTypes = ygg::MapTypeListT<GroundFunctionTerm, StaticFluentAuxiliaryTags>;
using GroundFunctionTermValueTypes = ygg::MapTypeListT<GroundFunctionTermValue, StaticFluentAuxiliaryTags>;
using FDRVariableTypes = ygg::MapTypeListT<FDRVariable, ygg::TypeList<FluentTag>>;
using FDRFactTypes = ygg::MapTypeListT<FDRFact, ygg::TypeList<FluentTag>>;

template<typename Op>
using LiftedUnaryOperatorType = UnaryOperator<Op, ygg::Data<FunctionExpression>>;

template<typename Op>
using LiftedBinaryOperatorType = BinaryOperator<Op, ygg::Data<FunctionExpression>>;

template<typename Op>
using LiftedMultiOperatorType = MultiOperator<Op, ygg::Data<FunctionExpression>>;

template<typename Op>
using GroundUnaryOperatorType = UnaryOperator<Op, ygg::Data<GroundFunctionExpression>>;

template<typename Op>
using GroundBinaryOperatorType = BinaryOperator<Op, ygg::Data<GroundFunctionExpression>>;

template<typename Op>
using GroundMultiOperatorType = MultiOperator<Op, ygg::Data<GroundFunctionExpression>>;

using LiftedArithmeticExpressionTypes = ygg::ConcatTypeListsT<ygg::MapTypeListT<LiftedUnaryOperatorType, UnaryArithmeticOpKinds>,
                                                         ygg::MapTypeListT<LiftedBinaryOperatorType, BinaryArithmeticOpKinds>,
                                                         ygg::MapTypeListT<LiftedMultiOperatorType, MultiArithmeticOpKinds>>;

using LiftedBooleanExpressionTypes = ygg::MapTypeListT<LiftedBinaryOperatorType, BooleanOpKinds>;

using GroundArithmeticExpressionTypes = ygg::ConcatTypeListsT<ygg::MapTypeListT<GroundUnaryOperatorType, UnaryArithmeticOpKinds>,
                                                         ygg::MapTypeListT<GroundBinaryOperatorType, BinaryArithmeticOpKinds>,
                                                         ygg::MapTypeListT<GroundMultiOperatorType, MultiArithmeticOpKinds>>;

using GroundBooleanExpressionTypes = ygg::MapTypeListT<GroundBinaryOperatorType, BooleanOpKinds>;

using ExpressionTypes =
    ygg::ConcatTypeListsT<LiftedArithmeticExpressionTypes, LiftedBooleanExpressionTypes, GroundArithmeticExpressionTypes, GroundBooleanExpressionTypes>;

using NumericEffectTypes = ygg::TypeList<NumericEffect<Assign, FluentTag>,
                                    NumericEffect<Increase, FluentTag>,
                                    NumericEffect<Decrease, FluentTag>,
                                    NumericEffect<ScaleUp, FluentTag>,
                                    NumericEffect<ScaleDown, FluentTag>,
                                    NumericEffect<Increase, AuxiliaryTag>>;

using GroundNumericEffectTypes = ygg::TypeList<GroundNumericEffect<Assign, FluentTag>,
                                          GroundNumericEffect<Increase, FluentTag>,
                                          GroundNumericEffect<Decrease, FluentTag>,
                                          GroundNumericEffect<ScaleUp, FluentTag>,
                                          GroundNumericEffect<ScaleDown, FluentTag>,
                                          GroundNumericEffect<Increase, AuxiliaryTag>>;

using NumericEffectOperatorTypes = ygg::TypeList<NumericEffectOperator<FluentTag>, NumericEffectOperator<AuxiliaryTag>>;
using GroundNumericEffectOperatorTypes = ygg::TypeList<GroundNumericEffectOperator<FluentTag>, GroundNumericEffectOperator<AuxiliaryTag>>;
using EffectTypes = ygg::ConcatTypeListsT<NumericEffectTypes, GroundNumericEffectTypes, NumericEffectOperatorTypes, GroundNumericEffectOperatorTypes>;
using OperatorEffectTypes = ygg::ConcatTypeListsT<NumericEffectTypes, GroundNumericEffectTypes>;
using ControlTypes = ygg::TypeList<ConditionalEffect, GroundConditionalEffect, ConjunctiveEffect, GroundConjunctiveEffect, Action, GroundAction, Axiom, GroundAxiom>;
using StructureTypes = ygg::TypeList<Action, Axiom>;
using ProblemTypes = ygg::TypeList<Metric, Domain, Task, FDRTask>;
using ConditionTypes = ygg::TypeList<ConjunctiveCondition, GroundConjunctiveCondition>;

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
                                               OperatorEffectTypes,
                                               ControlTypes,
                                               ProblemTypes,
                                               FDRVariableTypes,
                                               ConditionTypes>;

using RelationRepositoryTypes = ygg::ConcatTypeListsT<PredicateTypes, FunctionTypes, StructureTypes>;
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

class FDRContext;
using FDRContextPtr = std::shared_ptr<FDRContext>;

}

#endif
