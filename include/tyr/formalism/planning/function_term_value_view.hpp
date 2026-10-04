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

#ifndef TYR_FORMALISM_PLANNING_FUNCTION_TERM_VALUE_VIEW_HPP_
#define TYR_FORMALISM_PLANNING_FUNCTION_TERM_VALUE_VIEW_HPP_

#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/function_term_value_index.hpp"
#include "tyr/formalism/planning/function_term_view.hpp"

#include <utility>
#include <vector>
#include <yggdrasil/core/types.hpp>
#include <yggdrasil/formalism/detail/view.hpp>

namespace ygg
{
template<::tyr::formalism::FactKind F, ygg::formalism::SymbolContextFor<::tyr::formalism::planning::FunctionTermValue<::tyr::GroundTag, F>> C>
class View<ygg::Index<::tyr::formalism::planning::FunctionTermValue<::tyr::GroundTag, F>>, C> :
    public ygg::formalism::detail::View<ygg::Index<::tyr::formalism::planning::FunctionTermValue<::tyr::GroundTag, F>>, C>
{
public:
    View(ygg::Index<::tyr::formalism::planning::FunctionTermValue<::tyr::GroundTag, F>> handle, const C& context) noexcept :
        ygg::formalism::detail::View<ygg::Index<::tyr::formalism::planning::FunctionTermValue<::tyr::GroundTag, F>>, C>(handle, context)
    {
    }

    auto get_fterm() const noexcept { return ygg::make_view(this->get_data().fterm, *this->m_context); }
    auto get_value() const noexcept { return this->get_data().value; }
};
}

#endif
