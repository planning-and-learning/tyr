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

#ifndef TYR_FORMALISM_DATALOG_GROUND_NUMERIC_EFFECT_OPERATOR_DATA_HPP_
#define TYR_FORMALISM_DATALOG_GROUND_NUMERIC_EFFECT_OPERATOR_DATA_HPP_

#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/datalog/ground_numeric_effect_index.hpp"

#include <stdexcept>
#include <variant>
#include <yggdrasil/containers/variant.hpp>
#include <yggdrasil/core/types.hpp>
#include <yggdrasil/core/types_utils.hpp>

namespace ygg
{

template<>
struct Data<::tyr::formalism::datalog::GroundNumericEffectOperator<::tyr::formalism::FluentTag>>
{
    using OperatorType = ::tyr::formalism::NumericEffectOperatorKind;
    using Variant = ::cista::offset::variant<ygg::Index<::tyr::formalism::datalog::GroundNumericEffect<::tyr::formalism::FluentTag>>>;

    OperatorType operator_kind = OperatorType::Assign;
    Variant value;

    template<typename C>
    using ViewVariant = std::variant<::ygg::View<ygg::Index<::tyr::formalism::datalog::GroundNumericEffect<::tyr::formalism::FluentTag>>, C>>;

    Data() = default;
    Data(OperatorType operator_kind_, Variant value_) : operator_kind(operator_kind_), value(value_)
    {
        if (!value.valid())
            throw std::invalid_argument("GroundNumericEffectOperator requires a valid value");
    }
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

static_assert(!ygg::uses_trivial_storage_v<::tyr::formalism::datalog::GroundNumericEffectOperator<::tyr::formalism::FluentTag>>);

}

#endif
