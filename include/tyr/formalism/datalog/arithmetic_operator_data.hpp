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

#ifndef TYR_FORMALISM_DATALOG_ARITHMETIC_OPERATOR_DATA_HPP_
#define TYR_FORMALISM_DATALOG_ARITHMETIC_OPERATOR_DATA_HPP_

#include <yggdrasil/containers/variant.hpp>
#include "tyr/formalism/datalog/declarations.hpp"

#include <stdexcept>
#include <variant>
#include <tuple>
#include <utility>
#include <yggdrasil/core/types.hpp>
#include <yggdrasil/core/types_utils.hpp>

namespace ygg
{

template<::tyr::TaskKind T>
struct Data<::tyr::formalism::datalog::ArithmeticOperator<T>>
{
    using Variant = ::ygg::IndexVariant<::tyr::formalism::datalog::ArithmeticExpressionTypes<T>>;

    Variant variant;

    template<typename C>
    using ViewVariant = ::ygg::ViewVariant<Variant, C>;

    Data() = default;
    explicit Data(Variant variant_) : variant(variant_)
    {
        if (!variant.valid())
            throw std::invalid_argument("ArithmeticOperator requires a valid variant");
    }
    template<typename C>
    Data(const ViewVariant<C>& variant_) :
        variant(std::visit([](const auto& view) -> Variant { return Variant(view.get_index()); }, variant_))
    {
    }

    static constexpr auto cista_members() noexcept { return ygg::Members<&Data::variant> {}; }
    static constexpr auto identifying_members() noexcept { return ygg::Members<&Data::variant> {}; }
    void clear() noexcept
    {
        ygg::clear_members(*this);
    }
};

static_assert(!ygg::uses_trivial_storage_v<::tyr::formalism::datalog::ArithmeticOperator<::tyr::LiftedTag>>);

}

#endif
