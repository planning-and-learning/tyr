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

#ifndef TYR_FORMALISM_DECLARATIONS_HPP_
#define TYR_FORMALISM_DECLARATIONS_HPP_

#include "tyr/declarations.hpp"
#include "tyr/formalism/enums.hpp"
#include "tyr/formalism/parameter_index.hpp"

#include <boost/mp11.hpp>
#include <cista/containers/variant.h>
#include <cista/containers/vector.h>
#include <concepts>
#include <memory>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <urd/elements/concepts.hpp>
#include <urd/elements/flat.hpp>
#include <urd/elements/index.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <urd/tags.hpp>
#include <utility>
#include <variant>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/formalism/builder.hpp>
#include <yggdrasil/formalism/declarations.hpp>
#include <yggdrasil/formatting/cista_formatters.hpp>

namespace ygg::formalism
{
template<typename... Ts>
class RelationRepository;

template<typename... Ts>
class ConcurrentRelationRepository;

template<typename SymbolRepo, typename RelationRepo>
class Repository;
}

namespace tyr::formalism
{

/**
 * Tags to distinguish predicates and downstream types
 */

struct StaticTag
{
    static constexpr auto name = "Static";
};

struct FluentTag
{
    static constexpr auto name = "Fluent";
};

struct DerivedTag
{
    static constexpr auto name = "Derived";
};

struct AuxiliaryTag
{
    static constexpr auto name = "Auxiliary";
};

template<typename T>
concept FactKind = std::same_as<T, StaticTag> || std::same_as<T, FluentTag> || std::same_as<T, DerivedTag> || std::same_as<T, AuxiliaryTag>;

/// The fact kinds a numeric effect changes.
template<typename T>
concept NumericEffectFactKind = std::same_as<T, FluentTag> || std::same_as<T, AuxiliaryTag>;

using StaticFluentTags = boost::mp11::mp_list<StaticTag, FluentTag>;
using StaticFluentDerivedTags = boost::mp11::mp_list<StaticTag, FluentTag, DerivedTag>;
using StaticFluentAuxiliaryTags = boost::mp11::mp_list<StaticTag, FluentTag, AuxiliaryTag>;

/**
 * Tags to dispatch on the relation a rule derives: a predicate binding or a function binding. Keeping
 * them apart in the type system means a rule deriving an atom carries no numeric-effect head, so the
 * numeric expression machinery is absent rather than skipped at runtime.
 */

struct PredicateTag
{
    static constexpr auto name = "Predicate";
};

struct FunctionTag
{
    static constexpr auto name = "Function";
};

template<typename T>
concept RelationKind = std::same_as<T, PredicateTag> || std::same_as<T, FunctionTag>;

using PredicateFunctionTags = boost::mp11::mp_list<PredicateTag, FunctionTag>;

template<typename T>
concept OperatorKind = std::same_as<T, BooleanOperatorKind> || std::same_as<T, ArithmeticOperatorKind>;

/// The name of operator kind O in the names of the operators of that kind (UnaryOperatorGroundArithmetic): the kinds are
/// enums, which hold no name.
template<OperatorKind O>
inline constexpr std::string_view operator_kind_name = std::same_as<O, BooleanOperatorKind> ? std::string_view("Boolean") : std::string_view("Arithmetic");

/// The operator kind of the operators that only occur in arithmetic (unary and multi operators).
template<typename T>
concept ArithmeticOperatorKindOnly = std::same_as<T, ArithmeticOperatorKind>;

/**
 * Formalism tag
 */

struct Variable
{
    static constexpr std::string_view name = "Variable";
};

/// The variables of a scope, by parameter index (parameters_data.hpp).
struct Parameters
{
    static constexpr std::string_view name = "Parameters";
};

using Object = ::ygg::formalism::Object;

/// A term: an object, or a parameter of the enclosing rule or action. Stored as it is, not interned; read through a
/// repository (urd::resolve) as std::variant<ObjectView, ParameterIndex>; printed as the alternative it
/// holds (yggdrasil/formatting/cista_formatters.hpp).
using Term = urd::flat_variant<urd::index<Object>, ParameterIndex>;
using TermList = urd::flat_vector<Term>;

template<FactKind T>
struct Predicate
{
    static constexpr std::string_view name = urd::joined_name<"Predicate", T::name>;
};

template<FactKind T>
struct Function
{
    static constexpr std::string_view name = urd::joined_name<"Function", T::name>;
};

/**
 * Records shared by datalog and planning.
 */

template<TaskKind T, OperatorKind O>
struct UnaryOperator
{
    static constexpr std::string_view name = urd::joined_name<"UnaryOperator", T::name, ::tyr::formalism::operator_kind_name<O>>;
};

template<TaskKind T, OperatorKind O>
struct BinaryOperator
{
    static constexpr std::string_view name = urd::joined_name<"BinaryOperator", T::name, ::tyr::formalism::operator_kind_name<O>>;
};

template<TaskKind T, OperatorKind O>
struct MultiOperator
{
    static constexpr std::string_view name = urd::joined_name<"MultiOperator", T::name, ::tyr::formalism::operator_kind_name<O>>;
};

template<TaskKind T, FactKind F>
struct Atom
{
    static constexpr std::string_view name = urd::joined_name<"Atom", T::name, F::name>;
};

template<TaskKind T, FactKind F>
struct Literal
{
    static constexpr std::string_view name = urd::joined_name<"Literal", T::name, F::name>;
};

/// A conjunctive condition: its variables (lifted), literals and numeric constraints. Its lifted form is shared by
/// datalog (a lifted rule's body) and planning; its ground form is planning's (fluent facts as FDR facts), datalog's
/// ground rules have a RuleBody.
template<TaskKind T>
struct ConjunctiveCondition
{
    static constexpr std::string_view name = urd::joined_name<"ConjunctiveCondition", T::name>;
};

template<TaskKind T, FactKind F>
struct FunctionTerm
{
    static constexpr std::string_view name = urd::joined_name<"FunctionTerm", T::name, F::name>;
};

template<TaskKind T, FactKind F>
    requires std::same_as<T, GroundTag>
struct FunctionTermValue
{
    static constexpr std::string_view name = urd::joined_name<"FunctionTermValue", T::name, F::name>;
};

template<TaskKind T, FactKind F>
    requires std::same_as<F, FluentTag> || std::same_as<F, AuxiliaryTag>
struct NumericEffect
{
    static constexpr std::string_view name = urd::joined_name<"NumericEffect", T::name, F::name>;
};

/// A function expression: a number, an arithmetic operator or a function term (ground ones also the auxiliary term).
/// Stored as it is, not interned; read through a repository as its namespace's FunctionExpressionView.
template<TaskKind T>
using FunctionExpression = std::conditional_t<std::same_as<T, GroundTag>,
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

struct PositiveTag
{
};

struct NegativeTag
{
};

template<typename T>
concept PolarityKind = std::same_as<T, PositiveTag> || std::same_as<T, NegativeTag>;

using PositiveNegativeTags = boost::mp11::mp_list<PositiveTag, NegativeTag>;

/// The pointer to the member that Tag selects: Tag's position in Tags (a boost::mp11 list) picks one of Members, one per
/// tag.
template<typename Tags, typename Tag, auto... Members>
    requires(boost::mp11::mp_contains<Tags, Tag>::value && boost::mp11::mp_size<Tags>::value == sizeof...(Members))
inline constexpr auto tagged_member_pointer = std::get<boost::mp11::mp_find<Tags, Tag>::value>(std::tuple(Members...));

/// The member of self that Tag selects (see tagged_member_pointer).
template<typename Tags, typename Tag, auto... Members, typename Self>
constexpr auto& tagged_member(Self& self) noexcept
{ return self.*tagged_member_pointer<Tags, Tag, Members...>; }

}

namespace tyr::formalism
{

/**
 * The records of planning and of Datalog: declared with the shared ones, since the one repository holds them all (its
 * type list below). Their data, builders and views live in formalism/planning/ and formalism/datalog/.
 */

template<TaskKind T>
struct ConditionalEffect
{
    static constexpr std::string_view name = urd::joined_name<"ConditionalEffect", T::name>;
};

template<TaskKind T>
struct ConjunctiveEffect
{
    static constexpr std::string_view name = urd::joined_name<"ConjunctiveEffect", T::name>;
};

template<TaskKind T>
struct Action
{
    static constexpr std::string_view name = urd::joined_name<"Action", T::name>;
};

template<TaskKind T>
struct Axiom
{
    static constexpr std::string_view name = urd::joined_name<"Axiom", T::name>;
};

struct Metric
{
    static constexpr std::string_view name = "Metric";
};

template<TaskKind T>
struct Task
{
    static constexpr std::string_view name = urd::joined_name<"Task", T::name>;
};

struct Domain
{
    static constexpr std::string_view name = "Domain";
};

template<FactKind T>
struct FDRVariable
{
    static constexpr std::string_view name = urd::joined_name<"FDRVariable", T::name>;
};

struct Invariant
{
    static constexpr std::string_view name = "Invariant";
};

template<FactKind T>
struct FDRFact
{
    static constexpr std::string_view name = urd::joined_name<"FDRFact", T::name>;
};

/// The body of a ground rule (and a program's goal): its static, fluent and derived literals and numeric constraints. A
/// lifted rule's body is the shared formalism::ConjunctiveCondition<LiftedTag>.
template<TaskKind T>
    requires std::same_as<T, GroundTag>
struct RuleBody
{
    static constexpr std::string_view name = urd::joined_name<"RuleBody", T::name>;
};

/// A rule: its heads, fluent atoms, are derived from one instantiation of its body.
template<TaskKind T>
struct Rule
{
    static constexpr std::string_view name = urd::joined_name<"Rule", T::name>;
};

template<TaskKind T>
struct Program
{
    static constexpr std::string_view name = urd::joined_name<"Program", T::name>;
};

/**
 * The repository
 */

using CoreTypes = boost::mp11::mp_list<Variable, Parameters, Object>;
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
/// The Datalog programs made from a task: rules over its records (a lifted rule's body is a shared condition), the
/// ground rules' bodies and the programs.
using DatalogTypes = boost::mp11::mp_list<Rule<LiftedTag>, RuleBody<GroundTag>, Rule<GroundTag>, Program<LiftedTag>, Program<GroundTag>>;

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
                                                     ConditionTypes,
                                                     DatalogTypes>;

using RelationRepositoryTypes = boost::mp11::mp_append<PredicateTypes, FunctionTypes, StructureTypes, boost::mp11::mp_list<Rule<LiftedTag>>>;
using BuilderTypes = boost::mp11::mp_append<SymbolRepositoryTypes, boost::mp11::mp_transform<ygg::Row, RelationRepositoryTypes>>;

template<typename... Ts>
using StoredRelationRepository = ::ygg::formalism::ConcurrentRelationRepository<Ts...>;

using RelationRepository = boost::mp11::mp_apply<StoredRelationRepository, RelationRepositoryTypes>;

template<typename... Ts>
using StoredRelations = ::ygg::formalism::ConcurrentRelations<Ts...>;

/// Named by what it holds (yggdrasil's Symbols and Relations), not by its stores: a call with a repository argument
/// then never instantiates them (yggdrasil/formalism/declarations.hpp).
using Repository = ::ygg::formalism::Repository<boost::mp11::mp_apply<::ygg::formalism::ConcurrentSymbols, SymbolRepositoryTypes>,
                                                boost::mp11::mp_apply<StoredRelations, RelationRepositoryTypes>>;
/// The pools of a writer's builders and scratch records, interned into a Repository (Repository::insert).
using Builder = boost::mp11::mp_apply<ygg::formalism::BuilderStorage, boost::mp11::mp_push_front<BuilderTypes, Repository>>;
using RepositoryPtr = std::shared_ptr<Repository>;

/**
 * Declaration-only view aliases of the shared records (planning's in planning/declarations.hpp, Datalog's in
 * datalog/declarations.hpp).
 *
 * Include repository.hpp before constructing, storing, or accessing views.
 */

template<TaskKind T, FactKind F>
using AtomView = urd::index_view<Atom<T, F>, Repository>;
template<TaskKind T, FactKind F>
using AtomListView = urd::resolved_range<urd::flat_vector<urd::index<Atom<T, F>>>, Repository>;
template<TaskKind T, FactKind F>
using AtomViewList = urd::Store<urd::sequence<AtomView<T, F>>>;

template<TaskKind T, OperatorKind O>
using BinaryOperatorView = urd::index_view<BinaryOperator<T, O>, Repository>;

template<FactKind T>
using PredicateRowView = ygg::RowView<Predicate<T>, Repository>;
template<FactKind T>
using FunctionRowView = ygg::RowView<Function<T>, Repository>;
template<FactKind F>
using PredicateRowKey = urd::key<Predicate<F>, ygg::Row<Predicate<F>>>;
template<FactKind F>
using FunctionRowKey = urd::key<Function<F>, ygg::Row<Function<F>>>;
/// Predicate rows read from a sequence of their indices.
template<FactKind F>
using PredicateRowsView = ygg::RowsView<Predicate<F>, urd::Store<urd::sequence<urd::index<ygg::Row<Predicate<F>>>>>, Repository>;

template<TaskKind T>
using BooleanOperatorView = urd::index_view<BinaryOperator<T, ::tyr::formalism::BooleanOperatorKind>, Repository>;

template<TaskKind T>
using BooleanOperatorListView = urd::resolved_range<urd::flat_vector<urd::index<BinaryOperator<T, ::tyr::formalism::BooleanOperatorKind>>>, Repository>;
template<TaskKind T>
using BooleanOperatorViewList = urd::Store<urd::sequence<BooleanOperatorView<T>>>;

template<TaskKind T>
using ConjunctiveConditionView = urd::index_view<ConjunctiveCondition<T>, Repository>;

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

/// A term read through the repository: its object's view, or the parameter (formalism/declarations.hpp).
using TermView = std::variant<ObjectView, ParameterIndex>;
using TermListView = urd::resolved_range<TermList, Repository>;
using TermViewList = urd::Store<urd::sequence<TermView>>;

template<TaskKind T, OperatorKind O>
using UnaryOperatorView = urd::index_view<UnaryOperator<T, O>, Repository>;

using VariableView = urd::index_view<Variable, Repository>;
using ParametersView = urd::index_view<Parameters, Repository>;
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

class VariableDependencyGraph;

}

#endif
