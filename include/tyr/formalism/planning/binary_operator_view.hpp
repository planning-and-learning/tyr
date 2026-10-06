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

#ifndef TYR_FORMALISM_PLANNING_BINARY_OPERATOR_VIEW_HPP_
#define TYR_FORMALISM_PLANNING_BINARY_OPERATOR_VIEW_HPP_

#include "tyr/formalism/planning/binary_operator_index.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/function_expression_view.hpp"

#include <yggdrasil/containers/variant.hpp>
#include <yggdrasil/core/types.hpp>
#include <yggdrasil/formalism/detail/view.hpp>

namespace ygg
{
template<::tyr::TaskKind T, ::tyr::formalism::BinaryOperatorKind O, ygg::formalism::SymbolContextFor<::tyr::formalism::planning::BinaryOperator<T, O>> C>
class View<ygg::Index<::tyr::formalism::planning::BinaryOperator<T, O>>, C> :
    public ygg::formalism::detail::View<ygg::Index<::tyr::formalism::planning::BinaryOperator<T, O>>, C>
{
public:
    View(ygg::Index<::tyr::formalism::planning::BinaryOperator<T, O>> handle, const C& context) noexcept :
        ygg::formalism::detail::View<ygg::Index<::tyr::formalism::planning::BinaryOperator<T, O>>, C>(handle, context)
    {
    }

    auto get_operator() const noexcept { return this->get_data().operator_kind; }
    auto get_lhs() const noexcept { return ygg::make_view(this->get_data().lhs, *this->m_context); }
    auto get_rhs() const noexcept { return ygg::make_view(this->get_data().rhs, *this->m_context); }
};

}

#endif
