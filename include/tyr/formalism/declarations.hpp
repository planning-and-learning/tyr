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

#include <yggdrasil/containers/optional.hpp>
#include <yggdrasil/containers/variant.hpp>
#include <yggdrasil/containers/vector.hpp>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/core/type_list.hpp>
#include <yggdrasil/core/types.hpp>
#include <yggdrasil/core/types_utils.hpp>
#include <yggdrasil/formalism/declarations.hpp>

namespace ygg::formalism
{
template<typename... Ts>
class SymbolRepository;

template<typename ObjectTag, typename... Ts>
class RelationRepository;

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

using StaticFluentTags = ygg::TypeList<StaticTag, FluentTag>;
using StaticFluentDerivedTags = ygg::TypeList<StaticTag, FluentTag, DerivedTag>;
using StaticFluentAuxiliaryTags = ygg::TypeList<StaticTag, FluentTag, AuxiliaryTag>;
using FluentDerivedTags = ygg::TypeList<FluentTag, DerivedTag>;

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

using PredicateFunctionTags = ygg::TypeList<PredicateTag, FunctionTag>;

template<typename T>
concept BinaryOperatorKind = std::same_as<T, BooleanOperatorKind> || std::same_as<T, ArithmeticOperatorKind>;

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

struct Term
{
};

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

#endif
