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
#include <string_view>
#include <tuple>
#include <type_traits>
#include <urd/elements/flat.hpp>
#include <urd/elements/index.hpp>
#include <urd/tags.hpp>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/formalism/declarations.hpp>
#include <yggdrasil/formatting/cista_formatters.hpp>

namespace ygg::formalism
{
template<typename... Ts>
class SymbolRepository;

template<typename... Ts>
class ConcurrentSymbolRepository;

template<typename Width, typename... Ts>
class RelationRepository;

template<typename Width, typename... Ts>
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

using Object = ::ygg::formalism::Object;

/// A term: an object, or a parameter of the enclosing rule or action. Stored as it is, not interned; read through a
/// repository (urd::resolve) as std::variant<ObjectView, ParameterIndex>; printed as the alternative it
/// holds (yggdrasil/formatting/cista_formatters.hpp).
using Term = urd::flat_variant<urd::index<Object>, ParameterIndex>;
using TermList = urd::flat_vector<Term>;

template<FactKind T>
struct Predicate
{
    static constexpr std::string_view name = "Predicate";
    static constexpr bool objects = true;  ///< its rows (ygg::Row) are objects
};

template<FactKind T>
struct Function
{
    static constexpr std::string_view name = "Function";
    static constexpr bool objects = true;  ///< its rows (ygg::Row) are objects
};

/**
 * Records shared by datalog and planning: both repositories store them, so their indices have one type.
 */

template<TaskKind T, OperatorKind O>
struct UnaryOperator
{
    static constexpr std::string_view name = "UnaryOperator";
};

template<TaskKind T, OperatorKind O>
struct BinaryOperator
{
    static constexpr std::string_view name = "BinaryOperator";
};

template<TaskKind T, OperatorKind O>
struct MultiOperator
{
    static constexpr std::string_view name = "MultiOperator";
};

template<TaskKind T, FactKind F>
struct Atom
{
    static constexpr std::string_view name = "Atom";
};

template<TaskKind T, FactKind F>
struct Literal
{
    static constexpr std::string_view name = "Literal";
};

template<TaskKind T, FactKind F>
struct FunctionTerm
{
    static constexpr std::string_view name = "FunctionTerm";
};

template<TaskKind T, FactKind F>
    requires std::same_as<T, GroundTag>
struct FunctionTermValue
{
    static constexpr std::string_view name = "FunctionTermValue";
};

template<TaskKind T, FactKind F>
    requires std::same_as<F, FluentTag> || std::same_as<F, AuxiliaryTag>
struct NumericEffect
{
    static constexpr std::string_view name = "NumericEffect";
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

#if defined(TYR_RELATION_STORAGE_WORD) == defined(TYR_RELATION_STORAGE_BIT)
#error "Exactly one of TYR_RELATION_STORAGE_WORD or TYR_RELATION_STORAGE_BIT must be defined"
#endif

namespace tyr::formalism
{
/// How tyr's relation repositories store an object column (yggdrasil's Width): 32 bits (urd::word), or the bits the
/// largest object index needs (urd::bit), as the build chooses.
#if defined(TYR_RELATION_STORAGE_WORD)
using RelationStorage = urd::word;
#else
using RelationStorage = urd::bit;
#endif

}

#endif
