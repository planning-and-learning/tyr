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

#include "tyr/formalism/enums.hpp"
#include "tyr/formalism/parameter_index.hpp"

#include <cista/containers/variant.h>
#include <cista/containers/vector.h>
#include <urd/elements/flat.hpp>
#include <urd/elements/index.hpp>
#include <urd/tags.hpp>
#include <yggdrasil/formatting/cista_formatters.hpp>

#include <yggdrasil/core/config.hpp>
#include <boost/mp11.hpp>
#include <yggdrasil/formalism/declarations.hpp>

namespace ygg::formalism
{
template<typename... Ts>
class SymbolRepository;

template<typename... Ts>
class ConcurrentSymbolRepository;

template<typename ObjectTag, typename... Ts>
class RelationRepository;

template<typename ObjectTag, typename... Ts>
class ConcurrentRelationRepository;

template<typename ObjectTag>
struct RelationRepositoryTraits;

template<typename SymbolRepo, typename RelationRepo>
class Repository;

template<typename SymbolRepo, typename RelationRepo>
class RepositoryFactory;
}

namespace tyr
{
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

using StaticFluentTags = boost::mp11::mp_list<StaticTag, FluentTag>;
using StaticFluentDerivedTags = boost::mp11::mp_list<StaticTag, FluentTag, DerivedTag>;
using StaticFluentAuxiliaryTags = boost::mp11::mp_list<StaticTag, FluentTag, AuxiliaryTag>;
using FluentDerivedTags = boost::mp11::mp_list<FluentTag, DerivedTag>;

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

/**
 * Formalism tag
 */

struct Variable
{
};

struct ObjectTag
{
};

using Object = ::ygg::formalism::Object<ObjectTag>;
using Row = ::ygg::formalism::Row;

/// A term: an object, or a parameter of the enclosing rule or action. Stored as it is, not interned; read through a
/// repository (urd::resolve) as std::variant<ObjectView, ParameterIndex>; printed as the alternative it
/// holds (yggdrasil/formatting/cista_formatters.hpp).
using Term = ::cista::offset::variant<urd::index<Object>, ParameterIndex>;
using TermList = ::cista::offset::vector<Term>;

template<typename T>
using RelationBinding = ::ygg::formalism::RelationBinding<T, ObjectTag>;

template<typename T>
using is_relation_binding = ::ygg::formalism::is_relation_binding<T>;

template<typename T>
inline constexpr bool is_relation_binding_v = ::ygg::formalism::is_relation_binding_v<T>;

template<typename T>
concept RelationBindingConcept = ::ygg::formalism::RelationBindingConcept<T>;

template<typename T>
concept NonRelationBindingConcept = ::ygg::formalism::NonRelationBindingConcept<T>;

template<FactKind T>
struct Predicate
{
};

template<FactKind T>
struct Function
{
};

/**
 * The relation a rule of kind R derives, for a given fact kind.
 */

template<RelationKind R, FactKind T>
struct Relation;

template<FactKind T>
struct Relation<PredicateTag, T>
{
    using type = Predicate<T>;
};

template<FactKind T>
struct Relation<FunctionTag, T>
{
    using type = Function<T>;
};

template<RelationKind R, FactKind T>
using RelationT = typename Relation<R, T>::type;

struct PositiveTag
{
};

struct NegativeTag
{
};

template<typename T>
concept PolarityKind = std::same_as<T, PositiveTag> || std::same_as<T, NegativeTag>;

}

#if defined(TYR_RELATION_STORAGE_WORD) == defined(TYR_RELATION_STORAGE_BIT)
#error "Exactly one of TYR_RELATION_STORAGE_WORD or TYR_RELATION_STORAGE_BIT must be defined"
#endif

namespace ygg::formalism
{

template<>
struct RelationRepositoryTraits<::tyr::formalism::ObjectTag>
{
#if defined(TYR_RELATION_STORAGE_WORD)
    using width = urd::word;
#else
    using width = urd::bit;
#endif
};

}

#endif
