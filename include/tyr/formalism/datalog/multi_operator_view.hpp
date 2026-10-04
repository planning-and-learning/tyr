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

#ifndef TYR_FORMALISM_DATALOG_MULTI_OPERATOR_VIEW_HPP_
#define TYR_FORMALISM_DATALOG_MULTI_OPERATOR_VIEW_HPP_

#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/datalog/function_expression_view.hpp"
#include "tyr/formalism/datalog/multi_operator_index.hpp"

#include <yggdrasil/containers/vector.hpp>
#include <yggdrasil/core/types.hpp>
#include <yggdrasil/formalism/detail/view.hpp>

namespace ygg
{
template<::tyr::TaskKind T, ygg::formalism::SymbolContextFor<::tyr::formalism::datalog::MultiOperator<T>> C>
class View<ygg::Index<::tyr::formalism::datalog::MultiOperator<T>>, C> :
    public ygg::formalism::detail::View<ygg::Index<::tyr::formalism::datalog::MultiOperator<T>>, C>
{
public:
    using OperatorType = ::tyr::formalism::ArithmeticOperatorKind;

    View(ygg::Index<::tyr::formalism::datalog::MultiOperator<T>> handle, const C& context) noexcept :
        ygg::formalism::detail::View<ygg::Index<::tyr::formalism::datalog::MultiOperator<T>>, C>(handle, context)
    {
    }

    auto get_operator() const noexcept { return this->get_data().operator_kind; }
    auto get_args() const noexcept { return ygg::make_view(this->get_data().args, *this->m_context); }
};

}

#endif
