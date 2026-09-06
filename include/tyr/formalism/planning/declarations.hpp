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

#include "tyr/declarations.hpp"
#include "tyr/formalism/declarations.hpp"

#include <memory>
#include <utility>
#include <vector>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/core/types.hpp>

namespace tyr::formalism::planning
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

template<TaskKind T>
struct FunctionExpression
{
};

template<TaskKind T, FactKind F>
struct FunctionTermValue
{
};

using EffectFamilyList = std::vector<EffectFamily>;

inline bool is_compatible_effect_family(EffectFamily lhs, EffectFamily rhs)
{
    if (lhs == EffectFamily::None || rhs == EffectFamily::None)
        return true;  ///< first effect

    if (lhs == rhs)
        return lhs != EffectFamily::Assign;  ///< disallow double assignment.

    return false;  ///< disallow mixing assign, additive, or multiplicative
}

template<TaskKind T, FactKind F>
struct NumericEffect
{
};

template<TaskKind T, FactKind F>
struct NumericEffectOperator
{
};

template<TaskKind T>
struct ConditionalEffect
{
};

template<TaskKind T>
struct ConjunctiveEffect
{
};

template<TaskKind T>
struct Action
{
};

template<TaskKind T>
struct Axiom
{
};

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

template<TaskKind T>
struct ConjunctiveCondition
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
template<TaskKind T>
using AtomTypes = ygg::TypeList<Atom<T, StaticTag>, Atom<T, FluentTag>, Atom<T, DerivedTag>>;
template<TaskKind T>
using LiteralTypes = ygg::TypeList<Literal<T, StaticTag>, Literal<T, FluentTag>, Literal<T, DerivedTag>>;
using FunctionTypes = ygg::MapTypeListT<Function, StaticFluentAuxiliaryTags>;
template<TaskKind T>
using FunctionTermTypes = ygg::TypeList<FunctionTerm<T, StaticTag>, FunctionTerm<T, FluentTag>, FunctionTerm<T, AuxiliaryTag>>;
template<TaskKind T>
using FunctionTermValueTypes = ygg::TypeList<FunctionTermValue<T, StaticTag>, FunctionTermValue<T, FluentTag>, FunctionTermValue<T, AuxiliaryTag>>;
using FDRVariableTypes = ygg::MapTypeListT<FDRVariable, ygg::TypeList<FluentTag>>;
using FDRFactTypes = ygg::MapTypeListT<FDRFact, ygg::TypeList<FluentTag>>;

template<TaskKind T>
using ArithmeticExpressionTypes = ygg::TypeList<UnaryOperator<T>, BinaryOperator<T, ArithmeticOperatorKind>, MultiOperator<T>>;
template<TaskKind T>
using BooleanExpressionTypes = ygg::TypeList<BinaryOperator<T, BooleanOperatorKind>>;

using ExpressionTypes = ygg::ConcatTypeListsT<ArithmeticExpressionTypes<LiftedTag>,
                                              BooleanExpressionTypes<LiftedTag>,
                                              ArithmeticExpressionTypes<GroundTag>,
                                              BooleanExpressionTypes<GroundTag>>;

template<TaskKind T>
using NumericEffectTypes = ygg::TypeList<NumericEffect<T, FluentTag>, NumericEffect<T, AuxiliaryTag>>;

template<TaskKind T>
using NumericEffectOperatorTypes = ygg::TypeList<NumericEffectOperator<T, FluentTag>, NumericEffectOperator<T, AuxiliaryTag>>;
using EffectTypes = ygg::ConcatTypeListsT<NumericEffectTypes<LiftedTag>,
                                          NumericEffectTypes<GroundTag>,
                                          NumericEffectOperatorTypes<LiftedTag>,
                                          NumericEffectOperatorTypes<GroundTag>>;
using OperatorEffectTypes = ygg::ConcatTypeListsT<NumericEffectTypes<LiftedTag>, NumericEffectTypes<GroundTag>>;
using ControlTypes = ygg::TypeList<ConditionalEffect<LiftedTag>,
                                   ConditionalEffect<GroundTag>,
                                   ConjunctiveEffect<LiftedTag>,
                                   ConjunctiveEffect<GroundTag>,
                                   Action<LiftedTag>,
                                   Action<GroundTag>,
                                   Axiom<LiftedTag>,
                                   Axiom<GroundTag>>;
using StructureTypes = ygg::TypeList<Action<LiftedTag>, Axiom<LiftedTag>>;
using ProblemTypes = ygg::TypeList<Metric, Domain, Task, FDRTask>;
using ConditionTypes = ygg::TypeList<ConjunctiveCondition<LiftedTag>, ConjunctiveCondition<GroundTag>>;

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
                                                    OperatorEffectTypes,
                                                    ControlTypes,
                                                    ProblemTypes,
                                                    FDRVariableTypes,
                                                    ConditionTypes>;

using RelationRepositoryTypes = ygg::ConcatTypeListsT<PredicateTypes, FunctionTypes, StructureTypes>;
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
using ActionView = ygg::View<ygg::Index<Action<T>>, Repository>;
template<TaskKind T>
using ActionListView = ygg::View<ygg::IndexList<Action<T>>, Repository>;
template<TaskKind T>
using ActionViewList = std::vector<ActionView<T>>;

template<TaskKind T>
using ArithmeticOperatorView = ygg::View<ygg::Data<ArithmeticOperator<T>>, Repository>;

template<TaskKind T>
using ArithmeticOperatorListView = ygg::View<ygg::DataList<ArithmeticOperator<T>>, Repository>;

template<TaskKind T, FactKind F>
using AtomView = ygg::View<ygg::Index<Atom<T, F>>, Repository>;
template<TaskKind T, FactKind F>
using AtomListView = ygg::View<ygg::IndexList<Atom<T, F>>, Repository>;
template<TaskKind T, FactKind F>
using AtomViewList = std::vector<AtomView<T, F>>;

template<TaskKind T>
using AxiomView = ygg::View<ygg::Index<Axiom<T>>, Repository>;
template<TaskKind T>
using AxiomListView = ygg::View<ygg::IndexList<Axiom<T>>, Repository>;
template<TaskKind T>
using AxiomViewList = std::vector<AxiomView<T>>;

template<TaskKind T, BinaryOperatorKind O>
using BinaryOperatorView = ygg::View<ygg::Index<BinaryOperator<T, O>>, Repository>;

template<TaskKind T, BinaryOperatorKind O>
using BinaryOperatorListView = ygg::View<ygg::IndexList<BinaryOperator<T, O>>, Repository>;

template<FactKind T>
using PredicateBindingView = ygg::View<ygg::Index<RelationBinding<Predicate<T>>>, Repository>;
template<FactKind T>
using FunctionBindingView = ygg::View<ygg::Index<RelationBinding<Function<T>>>, Repository>;
using ActionBindingView = ygg::View<ygg::Index<RelationBinding<Action<LiftedTag>>>, Repository>;
using AxiomBindingView = ygg::View<ygg::Index<RelationBinding<Axiom<LiftedTag>>>, Repository>;

template<TaskKind T>
using BooleanOperatorView = ygg::View<ygg::Data<BooleanOperator<T>>, Repository>;

template<TaskKind T>
using BooleanOperatorListView = ygg::View<ygg::DataList<BooleanOperator<T>>, Repository>;
template<TaskKind T>
using BooleanOperatorViewList = std::vector<BooleanOperatorView<T>>;

template<TaskKind T>
using ConditionalEffectView = ygg::View<ygg::Index<ConditionalEffect<T>>, Repository>;
template<TaskKind T>
using ConditionalEffectListView = ygg::View<ygg::IndexList<ConditionalEffect<T>>, Repository>;
template<TaskKind T>
using ConditionalEffectViewList = std::vector<ygg::View<ygg::Index<ConditionalEffect<T>>, Repository>>;

template<TaskKind T>
using ConjunctiveConditionView = ygg::View<ygg::Index<ConjunctiveCondition<T>>, Repository>;
template<TaskKind T>
using ConjunctiveConditionListView = ygg::View<ygg::IndexList<ConjunctiveCondition<T>>, Repository>;

template<TaskKind T>
using ConjunctiveEffectView = ygg::View<ygg::Index<ConjunctiveEffect<T>>, Repository>;
template<TaskKind T>
using ConjunctiveEffectListView = ygg::View<ygg::IndexList<ConjunctiveEffect<T>>, Repository>;

using DomainView = ygg::View<ygg::Index<Domain>, Repository>;
using DomainListView = ygg::View<ygg::IndexList<Domain>, Repository>;

template<FactKind T>
using FDRFactView = ygg::View<ygg::Data<FDRFact<T>>, Repository>;
template<FactKind T>
using FDRFactListView = ygg::View<ygg::DataList<FDRFact<T>>, Repository>;
template<FactKind T>
using FDRFactViewList = std::vector<FDRFactView<T>>;

using FDRTaskView = ygg::View<ygg::Index<FDRTask>, Repository>;
using FDRTaskListView = ygg::View<ygg::IndexList<FDRTask>, Repository>;

template<FactKind T>
using FDRVariableView = ygg::View<ygg::Index<FDRVariable<T>>, Repository>;
template<FactKind T>
using FDRVariableListView = ygg::View<ygg::IndexList<FDRVariable<T>>, Repository>;
template<FactKind T>
using FDRVariableViewList = std::vector<FDRVariableView<T>>;

template<TaskKind T>
using FunctionExpressionView = ygg::View<ygg::Data<FunctionExpression<T>>, Repository>;
template<TaskKind T>
using FunctionExpressionListView = ygg::View<ygg::DataList<FunctionExpression<T>>, Repository>;

template<TaskKind T, FactKind F>
using FunctionTermView = ygg::View<ygg::Index<FunctionTerm<T, F>>, Repository>;
template<TaskKind T, FactKind F>
using FunctionTermListView = ygg::View<ygg::IndexList<FunctionTerm<T, F>>, Repository>;

template<TaskKind T, FactKind F>
using FunctionTermViewList = std::vector<FunctionTermView<T, F>>;

template<FactKind T>
using FunctionView = ygg::View<ygg::Index<Function<T>>, Repository>;
template<FactKind T>
using FunctionListView = ygg::View<ygg::IndexList<Function<T>>, Repository>;
template<FactKind T>
using FunctionViewList = std::vector<FunctionView<T>>;

template<TaskKind T, FactKind F>
using FunctionTermValueView = ygg::View<ygg::Index<FunctionTermValue<T, F>>, Repository>;
template<TaskKind T, FactKind F>
using FunctionTermValueListView = ygg::View<ygg::IndexList<FunctionTermValue<T, F>>, Repository>;
template<TaskKind T, FactKind F>
using FunctionTermValueViewList = std::vector<FunctionTermValueView<T, F>>;

template<TaskKind T, FactKind F>
using FunctionTermViewValuePair = std::pair<ygg::View<ygg::Index<FunctionTerm<T, F>>, Repository>, ygg::float_t>;
template<TaskKind T, FactKind F>
using FunctionTermViewValuePairList = std::vector<FunctionTermViewValuePair<T, F>>;

template<TaskKind T, FactKind F>
using LiteralView = ygg::View<ygg::Index<Literal<T, F>>, Repository>;
template<TaskKind T, FactKind F>
using LiteralListView = ygg::View<ygg::IndexList<Literal<T, F>>, Repository>;
template<TaskKind T, FactKind F>
using LiteralViewList = std::vector<LiteralView<T, F>>;

using MetricView = ygg::View<ygg::Index<Metric>, Repository>;
using MetricListView = ygg::View<ygg::IndexList<Metric>, Repository>;

template<TaskKind T>
using MultiOperatorView = ygg::View<ygg::Index<MultiOperator<T>>, Repository>;

template<TaskKind T>
using MultiOperatorListView = ygg::View<ygg::IndexList<MultiOperator<T>>, Repository>;

template<TaskKind T, FactKind F>
using NumericEffectOperatorView = ygg::View<ygg::Data<NumericEffectOperator<T, F>>, Repository>;
template<TaskKind T, FactKind F>
using NumericEffectOperatorListView = ygg::View<ygg::DataList<NumericEffectOperator<T, F>>, Repository>;
template<TaskKind T, FactKind F>
using NumericEffectOperatorViewList = std::vector<NumericEffectOperatorView<T, F>>;

template<TaskKind T, FactKind F>
using NumericEffectView = ygg::View<ygg::Index<NumericEffect<T, F>>, Repository>;
template<TaskKind T, FactKind F>
using NumericEffectListView = ygg::View<ygg::IndexList<NumericEffect<T, F>>, Repository>;

using ObjectView = ygg::View<ygg::Index<Object>, Repository>;
using ObjectListView = ygg::View<ygg::IndexList<Object>, Repository>;
using ObjectViewList = std::vector<ObjectView>;

template<FactKind T>
using PredicateView = ygg::View<ygg::Index<Predicate<T>>, Repository>;
template<FactKind T>
using PredicateListView = ygg::View<ygg::IndexList<Predicate<T>>, Repository>;
template<FactKind T>
using PredicateViewList = std::vector<PredicateView<T>>;

using TaskView = ygg::View<ygg::Index<Task>, Repository>;
using TaskListView = ygg::View<ygg::IndexList<Task>, Repository>;

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
struct GrounderContext;
class PlanningDomain;
class PlanningFDRTask;
class PlanningTask;

}

#endif
