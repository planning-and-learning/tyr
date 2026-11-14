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

#include <boost/mp11.hpp>
#include <cista/containers/variant.h>
#include <cista/containers/vector.h>
#include <concepts>
#include <memory>
#include <span>
#include <urd/elements/flat.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <utility>
#include <variant>
#include <vector>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/formalism/builder.hpp>
#include <yggdrasil/formalism/row_view.hpp>

namespace tyr::formalism::planning
{

/**
 * Formalism tag
 */

using EffectFamilyList = urd::Store<urd::sequence<EffectFamily>>;

inline bool is_compatible_effect_family(EffectFamily lhs, EffectFamily rhs)
{
    if (lhs == EffectFamily::None || rhs == EffectFamily::None)
        return true;  ///< first effect

    if (lhs == rhs)
        return lhs != EffectFamily::Assign;  ///< disallow double assignment.

    return false;  ///< disallow mixing assign, additive, or multiplicative
}

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
    using object_tag = ::tyr::formalism::ObjectTag;  ///< its rows (ygg::Row) are objects
};

template<TaskKind T>
struct Axiom
{
    using object_tag = ::tyr::formalism::ObjectTag;  ///< its rows (ygg::Row) are objects
};

struct Metric
{
};

template<TaskKind T>
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

struct Invariant
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

using CoreTypes = boost::mp11::mp_list<Variable, Object>;
using PredicateTypes = boost::mp11::mp_transform<Predicate, StaticFluentDerivedTags>;
template<TaskKind T>
using AtomTypes = boost::mp11::mp_list<Atom<T, StaticTag>, Atom<T, FluentTag>, Atom<T, DerivedTag>>;
template<TaskKind T>
using LiteralTypes = boost::mp11::mp_list<Literal<T, StaticTag>, Literal<T, FluentTag>, Literal<T, DerivedTag>>;
using FunctionTypes = boost::mp11::mp_transform<Function, StaticFluentAuxiliaryTags>;
template<TaskKind T>
using FunctionTermTypes = boost::mp11::mp_list<FunctionTerm<T, StaticTag>, FunctionTerm<T, FluentTag>, FunctionTerm<T, AuxiliaryTag>>;
template<TaskKind T>
using FunctionTermValueTypes = boost::mp11::mp_list<FunctionTermValue<T, StaticTag>, FunctionTermValue<T, FluentTag>, FunctionTermValue<T, AuxiliaryTag>>;
using FDRVariableTypes = boost::mp11::mp_transform<FDRVariable, boost::mp11::mp_list<FluentTag>>;
using InvariantTypes = boost::mp11::mp_list<Invariant>;
template<TaskKind T>
using ArithmeticExpressionTypes =
    boost::mp11::mp_list<UnaryOperator<T, ArithmeticOperatorKind>, BinaryOperator<T, ArithmeticOperatorKind>, MultiOperator<T, ArithmeticOperatorKind>>;
template<TaskKind T>
using BooleanExpressionTypes = boost::mp11::mp_list<BinaryOperator<T, BooleanOperatorKind>>;

using ExpressionTypes = boost::mp11::
    mp_append<ArithmeticExpressionTypes<LiftedTag>, BooleanExpressionTypes<LiftedTag>, ArithmeticExpressionTypes<GroundTag>, BooleanExpressionTypes<GroundTag>>;

template<TaskKind T>
using NumericEffectTypes = boost::mp11::mp_list<NumericEffect<T, FluentTag>, NumericEffect<T, AuxiliaryTag>>;

using EffectTypes = boost::mp11::mp_append<NumericEffectTypes<LiftedTag>, NumericEffectTypes<GroundTag>>;
using OperatorEffectTypes = boost::mp11::mp_append<NumericEffectTypes<LiftedTag>, NumericEffectTypes<GroundTag>>;
using ControlTypes = boost::mp11::mp_list<ConditionalEffect<LiftedTag>,
                                          ConditionalEffect<GroundTag>,
                                          ConjunctiveEffect<LiftedTag>,
                                          ConjunctiveEffect<GroundTag>,
                                          Action<LiftedTag>,
                                          Action<GroundTag>,
                                          Axiom<LiftedTag>,
                                          Axiom<GroundTag>>;
using StructureTypes = boost::mp11::mp_list<Action<LiftedTag>, Axiom<LiftedTag>>;
using ProblemTypes = boost::mp11::mp_list<Metric, Domain, Task<LiftedTag>, Task<GroundTag>>;
using ConditionTypes = boost::mp11::mp_list<ConjunctiveCondition<LiftedTag>, ConjunctiveCondition<GroundTag>>;

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
                                                     OperatorEffectTypes,
                                                     ControlTypes,
                                                     ProblemTypes,
                                                     FDRVariableTypes,
                                                     InvariantTypes,
                                                     ConditionTypes>;

using RelationRepositoryTypes = boost::mp11::mp_append<PredicateTypes, FunctionTypes, StructureTypes>;
using BuilderTypes = boost::mp11::mp_append<SymbolRepositoryTypes, boost::mp11::mp_transform<ygg::Row, RelationRepositoryTypes>>;
using Builder = boost::mp11::mp_apply<ygg::formalism::BuilderStorage, BuilderTypes>;

using SymbolRepository = boost::mp11::mp_apply<::ygg::formalism::ConcurrentSymbolRepository, SymbolRepositoryTypes>;

template<typename... Ts>
using TaggedRelationRepository = ::ygg::formalism::ConcurrentRelationRepository<ObjectTag, Ts...>;

using RelationRepository = boost::mp11::mp_apply<TaggedRelationRepository, RelationRepositoryTypes>;

template<typename... Ts>
using TaggedRelations = ::ygg::formalism::ConcurrentRelations<ObjectTag, Ts...>;

/// Named by what it holds (yggdrasil's Symbols and Relations), not by its stores: a call with a repository argument
/// then never instantiates them (yggdrasil/formalism/declarations.hpp).
using Repository = ::ygg::formalism::Repository<boost::mp11::mp_apply<::ygg::formalism::ConcurrentSymbols, SymbolRepositoryTypes>,
                                                boost::mp11::mp_apply<TaggedRelations, RelationRepositoryTypes>>;
using RepositoryPtr = std::shared_ptr<Repository>;

/**
 * Declaration-only view aliases.
 *
 * Include repository.hpp before constructing, storing, or accessing views.
 */
template<TaskKind T>
using ActionView = urd::index_view<Action<T>, Repository>;
template<TaskKind T>
using ActionViewList = urd::Store<urd::sequence<ActionView<T>>>;

template<TaskKind T, FactKind F>
using AtomView = urd::index_view<Atom<T, F>, Repository>;
template<TaskKind T, FactKind F>
using AtomListView = urd::resolved_range<urd::flat_vector<urd::index<Atom<T, F>>>, Repository>;
template<TaskKind T, FactKind F>
using AtomViewList = urd::Store<urd::sequence<AtomView<T, F>>>;

template<TaskKind T>
using AxiomView = urd::index_view<Axiom<T>, Repository>;
template<TaskKind T>
using AxiomViewList = urd::Store<urd::sequence<AxiomView<T>>>;

template<TaskKind T, OperatorKind O>
using BinaryOperatorView = urd::index_view<BinaryOperator<T, O>, Repository>;

template<FactKind T>
using PredicateRowView = ygg::RowView<Predicate<T>, Repository>;
template<FactKind T>
using FunctionRowView = ygg::RowView<Function<T>, Repository>;
using ActionRowView = ygg::RowView<Action<LiftedTag>, Repository>;
using ActionRowDataView = urd::borrowed_view<ygg::Row<Action<LiftedTag>>, Repository>;
using AxiomRowView = ygg::RowView<Axiom<LiftedTag>, Repository>;

template<TaskKind T>
using BooleanOperatorView = urd::index_view<BinaryOperator<T, ::tyr::formalism::BooleanOperatorKind>, Repository>;

template<TaskKind T>
using BooleanOperatorListView = urd::resolved_range<urd::flat_vector<urd::index<BinaryOperator<T, ::tyr::formalism::BooleanOperatorKind>>>, Repository>;
template<TaskKind T>
using BooleanOperatorViewList = urd::Store<urd::sequence<BooleanOperatorView<T>>>;

template<TaskKind T>
using ConditionalEffectView = urd::index_view<ConditionalEffect<T>, Repository>;
template<TaskKind T>
using ConditionalEffectListView = urd::resolved_range<urd::flat_vector<urd::index<ConditionalEffect<T>>>, Repository>;
template<TaskKind T>
using ConditionalEffectViewList = urd::Store<urd::sequence<urd::index_view<ConditionalEffect<T>, Repository>>>;

template<TaskKind T>
using ConjunctiveConditionView = urd::index_view<ConjunctiveCondition<T>, Repository>;
template<TaskKind T>
using ConjunctiveEffectView = urd::index_view<ConjunctiveEffect<T>, Repository>;
using DomainView = urd::index_view<Domain, Repository>;
template<FactKind T>
using FDRFactView = urd::borrowed_view<FDRFact<T>, Repository>;
template<FactKind T>
using FDRFactListView = urd::resolved_range<urd::flat_vector<urd::data<FDRFact<T>>>, Repository>;
template<FactKind T>
using FDRFactViewList = urd::Store<urd::sequence<FDRFactView<T>>>;

template<FactKind T>
using FDRVariableView = urd::index_view<FDRVariable<T>, Repository>;
template<FactKind T>
using FDRVariableViewList = urd::Store<urd::sequence<FDRVariableView<T>>>;

using InvariantView = urd::index_view<Invariant, Repository>;
using InvariantViewList = urd::Store<urd::sequence<InvariantView>>;

template<TaskKind T, FactKind F>
using FunctionTermView = urd::index_view<FunctionTerm<T, F>, Repository>;
template<TaskKind T, FactKind F>
using FunctionTermListView = urd::resolved_range<urd::flat_vector<urd::index<FunctionTerm<T, F>>>, Repository>;

template<FactKind T>
using FunctionView = urd::index_view<Function<T>, Repository>;
template<FactKind T>
using FunctionListView = urd::resolved_range<urd::flat_vector<urd::index<Function<T>>>, Repository>;
template<FactKind T>
using FunctionViewList = urd::Store<urd::sequence<FunctionView<T>>>;

template<TaskKind T, FactKind F>
using FunctionTermValueView = urd::index_view<FunctionTermValue<T, F>, Repository>;
template<TaskKind T, FactKind F>
using FunctionTermValueListView = urd::resolved_range<urd::flat_vector<urd::index<FunctionTermValue<T, F>>>, Repository>;
template<TaskKind T, FactKind F>
using FunctionTermValueViewList = urd::Store<urd::sequence<FunctionTermValueView<T, F>>>;

template<TaskKind T, FactKind F>
using FunctionTermViewValuePair = std::pair<urd::index_view<FunctionTerm<T, F>, Repository>, ygg::float_t>;
template<TaskKind T, FactKind F>
using LiteralView = urd::index_view<Literal<T, F>, Repository>;
template<TaskKind T, FactKind F>
using LiteralListView = urd::resolved_range<urd::flat_vector<urd::index<Literal<T, F>>>, Repository>;
template<TaskKind T, FactKind F>
using LiteralViewList = urd::Store<urd::sequence<LiteralView<T, F>>>;

using MetricView = urd::index_view<Metric, Repository>;
template<TaskKind T, OperatorKind O>
using MultiOperatorView = urd::index_view<MultiOperator<T, O>, Repository>;

template<TaskKind T, FactKind F>
using NumericEffectOperatorView = urd::index_view<NumericEffect<T, F>, Repository>;
template<TaskKind T, FactKind F>
using NumericEffectOperatorListView = urd::resolved_range<urd::flat_vector<urd::index<NumericEffect<T, F>>>, Repository>;
template<TaskKind T, FactKind F>
using NumericEffectOperatorViewList = urd::Store<urd::sequence<NumericEffectOperatorView<T, F>>>;

template<TaskKind T, FactKind F>
using NumericEffectView = urd::index_view<NumericEffect<T, F>, Repository>;
using ObjectView = urd::index_view<Object, Repository>;
using ObjectViewList = urd::Store<urd::sequence<ObjectView>>;

template<FactKind T>
using PredicateView = urd::index_view<Predicate<T>, Repository>;
template<FactKind T>
using PredicateListView = urd::resolved_range<urd::flat_vector<urd::index<Predicate<T>>>, Repository>;
template<FactKind T>
using PredicateViewList = urd::Store<urd::sequence<PredicateView<T>>>;

template<TaskKind T>
using TaskView = urd::index_view<Task<T>, Repository>;
template<TaskKind T>
using TaskListView = urd::resolved_range<urd::flat_vector<urd::index<Task<T>>>, Repository>;

/// A term read through the repository: its object's view, or the parameter (formalism/declarations.hpp).
using TermView = std::variant<ObjectView, ParameterIndex>;
using TermListView = urd::resolved_range<TermList, Repository>;
using TermViewList = urd::Store<urd::sequence<TermView>>;

template<TaskKind T, OperatorKind O>
using UnaryOperatorView = urd::index_view<UnaryOperator<T, O>, Repository>;

using VariableView = urd::index_view<Variable, Repository>;
using VariableViewList = urd::Store<urd::sequence<VariableView>>;

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
class FDRContext;
using FDRContextPtr = std::shared_ptr<FDRContext>;
class PlanningDomain;
template<TaskKind T>
class PlanningTask;

}

#endif
