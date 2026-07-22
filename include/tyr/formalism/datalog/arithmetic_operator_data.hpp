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

#include <variant>
#include <yggdrasil/core/types.hpp>
#include <yggdrasil/core/types_utils.hpp>

namespace ygg
{
using namespace ::tyr;

template<typename T>
struct Data<::tyr::formalism::datalog::ArithmeticOperator<T>>
{
    using Variant = ::cista::offset::variant<ygg::Index<::tyr::formalism::datalog::UnaryOperator<::tyr::formalism::Sub, T>>,
                                             ygg::Index<::tyr::formalism::datalog::BinaryOperator<::tyr::formalism::Add, T>>,
                                             ygg::Index<::tyr::formalism::datalog::BinaryOperator<::tyr::formalism::Sub, T>>,
                                             ygg::Index<::tyr::formalism::datalog::BinaryOperator<::tyr::formalism::Mul, T>>,
                                             ygg::Index<::tyr::formalism::datalog::BinaryOperator<::tyr::formalism::Div, T>>,
                                             ygg::Index<::tyr::formalism::datalog::MultiOperator<::tyr::formalism::Add, T>>,
                                             ygg::Index<::tyr::formalism::datalog::MultiOperator<::tyr::formalism::Mul, T>>>;

    Variant value;

    template<typename C>
    using ViewVariant = std::variant<::ygg::View<ygg::Index<::tyr::formalism::datalog::UnaryOperator<::tyr::formalism::Sub, T>>, C>,
                                     ::ygg::View<ygg::Index<::tyr::formalism::datalog::BinaryOperator<::tyr::formalism::Add, T>>, C>,
                                     ::ygg::View<ygg::Index<::tyr::formalism::datalog::BinaryOperator<::tyr::formalism::Sub, T>>, C>,
                                     ::ygg::View<ygg::Index<::tyr::formalism::datalog::BinaryOperator<::tyr::formalism::Mul, T>>, C>,
                                     ::ygg::View<ygg::Index<::tyr::formalism::datalog::BinaryOperator<::tyr::formalism::Div, T>>, C>,
                                     ::ygg::View<ygg::Index<::tyr::formalism::datalog::MultiOperator<::tyr::formalism::Add, T>>, C>,
                                     ::ygg::View<ygg::Index<::tyr::formalism::datalog::MultiOperator<::tyr::formalism::Mul, T>>, C>>;

    Data() = default;
    Data(Variant value_) : value(value_) {}
    template<typename C>
    Data(ViewVariant<C> value_) : value(std::visit([](const auto& view) -> Variant { return Variant(view.get_index()); }, value_))
    {
    }

    void clear() noexcept { ygg::clear(value); }

    auto cista_members() const noexcept { return std::tie(value); }
    auto identifying_members() const noexcept { return std::tie(value); }
};

static_assert(!ygg::uses_trivial_storage_v<::tyr::formalism::datalog::ArithmeticOperator<ygg::Data<::tyr::formalism::datalog::FunctionExpression>>>);

}

#endif
