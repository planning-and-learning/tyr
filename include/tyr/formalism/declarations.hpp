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

#include <yggdrasil/core/config.hpp>
#include <yggdrasil/core/type_list.hpp>
#include <yggdrasil/containers/optional.hpp>
#include <yggdrasil/core/types.hpp>
#include <yggdrasil/core/types_utils.hpp>
#include <yggdrasil/containers/variant.hpp>
#include <yggdrasil/containers/vector.hpp>
#include <yggdrasil/formalism/declarations.hpp>
#include <yggdrasil/semantics/comparison.hpp>

#include <tuple>

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
 * Tags to dispatch operators
 */

struct Eq : ygg::comparison::Mixin<Eq>
{
    static constexpr int kind = 0;
    constexpr auto identifying_members() const noexcept { return std::tie(kind); }
    constexpr auto cista_members() const noexcept { return std::tie(); }
};
struct Ne : ygg::comparison::Mixin<Ne>
{
    static constexpr int kind = 1;
    constexpr auto identifying_members() const noexcept { return std::tie(kind); }
    constexpr auto cista_members() const noexcept { return std::tie(); }
};
struct Le : ygg::comparison::Mixin<Le>
{
    static constexpr int kind = 2;
    constexpr auto identifying_members() const noexcept { return std::tie(kind); }
    constexpr auto cista_members() const noexcept { return std::tie(); }
};
struct Lt : ygg::comparison::Mixin<Lt>
{
    static constexpr int kind = 3;
    constexpr auto identifying_members() const noexcept { return std::tie(kind); }
    constexpr auto cista_members() const noexcept { return std::tie(); }
};
struct Ge : ygg::comparison::Mixin<Ge>
{
    static constexpr int kind = 4;
    constexpr auto identifying_members() const noexcept { return std::tie(kind); }
    constexpr auto cista_members() const noexcept { return std::tie(); }
};
struct Gt : ygg::comparison::Mixin<Gt>
{
    static constexpr int kind = 5;
    constexpr auto identifying_members() const noexcept { return std::tie(kind); }
    constexpr auto cista_members() const noexcept { return std::tie(); }
};
struct Add : ygg::comparison::Mixin<Add>
{
    static constexpr int kind = 0;
    constexpr auto identifying_members() const noexcept { return std::tie(kind); }
    constexpr auto cista_members() const noexcept { return std::tie(); }
};
struct Sub : ygg::comparison::Mixin<Sub>
{
    static constexpr int kind = 1;
    constexpr auto identifying_members() const noexcept { return std::tie(kind); }
    constexpr auto cista_members() const noexcept { return std::tie(); }
};
struct Mul : ygg::comparison::Mixin<Mul>
{
    static constexpr int kind = 2;
    constexpr auto identifying_members() const noexcept { return std::tie(kind); }
    constexpr auto cista_members() const noexcept { return std::tie(); }
};
struct Div : ygg::comparison::Mixin<Div>
{
    static constexpr int kind = 3;
    constexpr auto identifying_members() const noexcept { return std::tie(kind); }
    constexpr auto cista_members() const noexcept { return std::tie(); }
};

template<typename T>
concept BooleanOpKind = std::same_as<T, Eq> || std::same_as<T, Ne> || std::same_as<T, Le> || std::same_as<T, Lt> || std::same_as<T, Ge> || std::same_as<T, Gt>;

template<typename T>
concept ArithmeticOpKind = std::same_as<T, Add> || std::same_as<T, Mul> || std::same_as<T, Div> || std::same_as<T, Sub>;

template<typename T>
concept OpKind = BooleanOpKind<T> || ArithmeticOpKind<T>;

using BooleanOpKinds = ygg::TypeList<Eq, Ne, Le, Lt, Ge, Gt>;
using ArithmeticOpKinds = ygg::TypeList<Add, Sub, Mul, Div>;
using UnaryArithmeticOpKinds = ygg::TypeList<Sub>;
using BinaryArithmeticOpKinds = ygg::TypeList<Add, Sub, Mul, Div>;
using MultiArithmeticOpKinds = ygg::TypeList<Add, Mul>;

enum class EffectFamily
{
    NONE = 0,
    ASSIGN = 1,
    INCREASE_DECREASE = 2,
    SCALE_UP_SCALE_DOWN = 3,
};

struct Assign : ygg::comparison::Mixin<Assign>
{
    static constexpr EffectFamily family = EffectFamily::ASSIGN;
    static constexpr int kind = 0;
    constexpr auto identifying_members() const noexcept { return std::tie(kind); }
    constexpr auto cista_members() const noexcept { return std::tie(); }
};
struct Increase : ygg::comparison::Mixin<Increase>
{
    static constexpr EffectFamily family = EffectFamily::INCREASE_DECREASE;
    static constexpr int kind = 1;
    constexpr auto identifying_members() const noexcept { return std::tie(kind); }
    constexpr auto cista_members() const noexcept { return std::tie(); }
};
struct Decrease : ygg::comparison::Mixin<Decrease>
{
    static constexpr EffectFamily family = EffectFamily::INCREASE_DECREASE;
    static constexpr int kind = 2;
    constexpr auto identifying_members() const noexcept { return std::tie(kind); }
    constexpr auto cista_members() const noexcept { return std::tie(); }
};
struct ScaleUp : ygg::comparison::Mixin<ScaleUp>
{
    static constexpr EffectFamily family = EffectFamily::SCALE_UP_SCALE_DOWN;
    static constexpr int kind = 3;
    constexpr auto identifying_members() const noexcept { return std::tie(kind); }
    constexpr auto cista_members() const noexcept { return std::tie(); }
};
struct ScaleDown : ygg::comparison::Mixin<ScaleDown>
{
    static constexpr EffectFamily family = EffectFamily::SCALE_UP_SCALE_DOWN;
    static constexpr int kind = 4;
    constexpr auto identifying_members() const noexcept { return std::tie(kind); }
    constexpr auto cista_members() const noexcept { return std::tie(); }
};

template<typename T>
concept NumericEffectOpKind =
    std::same_as<T, Assign> || std::same_as<T, Increase> || std::same_as<T, Decrease> || std::same_as<T, ScaleUp> || std::same_as<T, ScaleDown>;

using NumericEffectOpKinds = ygg::TypeList<Assign, Increase, Decrease, ScaleUp, ScaleDown>;

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
