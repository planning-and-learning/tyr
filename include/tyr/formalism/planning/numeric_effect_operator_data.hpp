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

#ifndef TYR_FORMALISM_PLANNING_NUMERIC_EFFECT_OPERATOR_DATA_HPP_
#define TYR_FORMALISM_PLANNING_NUMERIC_EFFECT_OPERATOR_DATA_HPP_

#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/numeric_effect_index.hpp"

#include <stdexcept>
#include <yggdrasil/containers/variant.hpp>
#include <yggdrasil/core/types.hpp>
#include <yggdrasil/core/types_utils.hpp>
#include <yggdrasil/semantics/comparison.hpp>
#include <yggdrasil/serialization/cista_comparators.hpp>
#include <yggdrasil/serialization/cista_equal_to.hpp>

namespace ygg
{

template<::tyr::TaskKind T>
struct Data<::tyr::formalism::planning::NumericEffectOperator<T, ::tyr::formalism::FluentTag>>
{
    using OperatorType = ::tyr::formalism::NumericEffectOperatorKind;
    using Variant = ::cista::offset::variant<ygg::Index<::tyr::formalism::planning::NumericEffect<T, ::tyr::formalism::FluentTag>>>;

    OperatorType operator_kind = OperatorType::Assign;
    Variant value;

    template<typename C>
    using ViewVariant = std::variant<::ygg::View<ygg::Index<::tyr::formalism::planning::NumericEffect<T, ::tyr::formalism::FluentTag>>, C>>;

    Data() = default;
    Data(OperatorType operator_kind_, Variant value_) : operator_kind(operator_kind_), value(value_)
    {
        if (!value.valid())
            throw std::invalid_argument("NumericEffectOperator requires a valid value");
    }
    // Python constructor
    template<typename C>
    Data(ViewVariant<C> value_) :
        operator_kind(std::visit([](const auto& view) { return view.get_operator(); }, value_)),
        value(std::visit([](const auto& view) -> Variant { return Variant(view.get_index()); }, value_))
    {
    }

    void clear() noexcept
    {
        operator_kind = OperatorType::Assign;
        ygg::clear(value);
    }

    auto cista_members() const noexcept { return std::tie(operator_kind, value); }
    auto identifying_members() const noexcept { return std::tie(operator_kind, value); }
};

template<::tyr::TaskKind T>
struct Data<::tyr::formalism::planning::NumericEffectOperator<T, ::tyr::formalism::AuxiliaryTag>>
{
    using OperatorType = ::tyr::formalism::NumericEffectOperatorKind;
    using Variant = ::cista::offset::variant<ygg::Index<::tyr::formalism::planning::NumericEffect<T, ::tyr::formalism::AuxiliaryTag>>>;

    OperatorType operator_kind = OperatorType::Increase;
    Variant value;

    template<typename C>
    using ViewVariant = std::variant<::ygg::View<ygg::Index<::tyr::formalism::planning::NumericEffect<T, ::tyr::formalism::AuxiliaryTag>>, C>>;

    Data() = default;
    Data(OperatorType operator_kind_, Variant value_) : operator_kind(operator_kind_), value(value_)
    {
        if (!value.valid() || operator_kind != OperatorType::Increase)
            throw std::invalid_argument("Auxiliary NumericEffectOperator requires Increase");
    }
    // Python constructor
    template<typename C>
    Data(ViewVariant<C> value_) :
        operator_kind(std::visit([](const auto& view) { return view.get_operator(); }, value_)),
        value(std::visit([](const auto& view) -> Variant { return Variant(view.get_index()); }, value_))
    {
    }

    void clear() noexcept
    {
        operator_kind = OperatorType::Increase;
        value.destruct();
        new (&value) Variant {};
    }

    auto cista_members() const noexcept { return std::tie(operator_kind, value); }
    auto identifying_members() const noexcept { return std::tie(operator_kind, value); }
};

static_assert(!ygg::uses_trivial_storage_v<::tyr::formalism::planning::NumericEffectOperator<::tyr::LiftedTag, ::tyr::formalism::FluentTag>>);
static_assert(!ygg::uses_trivial_storage_v<::tyr::formalism::planning::NumericEffectOperator<::tyr::GroundTag, ::tyr::formalism::FluentTag>>);
static_assert(!ygg::uses_trivial_storage_v<::tyr::formalism::planning::NumericEffectOperator<::tyr::LiftedTag, ::tyr::formalism::AuxiliaryTag>>);
static_assert(!ygg::uses_trivial_storage_v<::tyr::formalism::planning::NumericEffectOperator<::tyr::GroundTag, ::tyr::formalism::AuxiliaryTag>>);

}

#endif
