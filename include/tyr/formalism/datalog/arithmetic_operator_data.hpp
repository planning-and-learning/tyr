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

#include "tyr/formalism/datalog/binary_operator_index.hpp"
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/datalog/multi_operator_index.hpp"
#include "tyr/formalism/datalog/unary_operator_index.hpp"

#include <stdexcept>
#include <variant>
#include <yggdrasil/core/types.hpp>
#include <yggdrasil/core/types_utils.hpp>

namespace ygg
{

template<typename T>
struct Data<::tyr::formalism::datalog::ArithmeticOperator<T>>
{
    using OperatorType = ::tyr::formalism::ArithmeticOperatorKind;
    using Variant = ::cista::offset::variant<ygg::Index<::tyr::formalism::datalog::UnaryOperator<T>>,
                                             ygg::Index<::tyr::formalism::datalog::BinaryOperator<::tyr::formalism::ArithmeticOperatorKind, T>>,
                                             ygg::Index<::tyr::formalism::datalog::MultiOperator<T>>>;

    OperatorType operator_kind = OperatorType::Sub;
    Variant value;

    template<typename C>
    using ViewVariant = std::variant<::ygg::View<ygg::Index<::tyr::formalism::datalog::UnaryOperator<T>>, C>,
                                     ::ygg::View<ygg::Index<::tyr::formalism::datalog::BinaryOperator<::tyr::formalism::ArithmeticOperatorKind, T>>, C>,
                                     ::ygg::View<ygg::Index<::tyr::formalism::datalog::MultiOperator<T>>, C>>;

    Data() = default;
    Data(OperatorType operator_kind_, Variant value_) : operator_kind(operator_kind_), value(value_)
    {
        if (!value.valid() || (value.index() == 0 && !::tyr::formalism::is_unary(operator_kind))
            || (value.index() == 2 && !::tyr::formalism::is_multi(operator_kind)))
            throw std::invalid_argument("ArithmeticOperator kind does not match operator form");
    }
    template<typename C>
    Data(ViewVariant<C> value_) :
        operator_kind(std::visit([](const auto& view) { return view.get_operator(); }, value_)),
        value(std::visit([](const auto& view) -> Variant { return Variant(view.get_index()); }, value_))
    {
    }

    void clear() noexcept
    {
        operator_kind = OperatorType::Sub;
        ygg::clear(value);
    }

    auto cista_members() const noexcept { return std::tie(operator_kind, value); }
    auto identifying_members() const noexcept
    {
        return std::tuple<std::size_t, const OperatorType&, const Variant&>(value.index(), operator_kind, value);
    }
};

static_assert(!ygg::uses_trivial_storage_v<::tyr::formalism::datalog::ArithmeticOperator<ygg::Data<::tyr::formalism::datalog::FunctionExpression>>>);

}

#endif
