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

#ifndef TYR_FORMALISM_PLANNING_METRIC_VIEW_HPP_
#define TYR_FORMALISM_PLANNING_METRIC_VIEW_HPP_

#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/function_expression_data.hpp"
#include "tyr/formalism/planning/metric_data.hpp"
#include "tyr/formalism/planning/metric_index.hpp"

#include <yggdrasil/core/types.hpp>
#include <yggdrasil/formalism/detail/view.hpp>

namespace ygg
{

template<ygg::formalism::SymbolContextFor<::tyr::formalism::planning::Metric> C>
class View<ygg::Index<::tyr::formalism::planning::Metric>, C> : public ygg::formalism::detail::View<ygg::Index<::tyr::formalism::planning::Metric>, C>
{
public:
    View(ygg::Index<::tyr::formalism::planning::Metric> handle, const C& context) noexcept :
        ygg::formalism::detail::View<ygg::Index<::tyr::formalism::planning::Metric>, C>(handle, context)
    {
    }

    auto get_optimization_direction() const noexcept { return this->get_data().optimization_direction; }
    auto get_fexpr() const noexcept { return ygg::make_view(this->get_data().fexpr, *this->m_context); }
};

}

#endif
