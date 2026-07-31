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
#include "tyr/formalism/planning/ground_function_expression_data.hpp"
#include "tyr/formalism/planning/metric_data.hpp"
#include "tyr/formalism/planning/metric_index.hpp"

#include <yggdrasil/core/types.hpp>

namespace ygg
{

template<::tyr::formalism::planning::Context C>
class View<ygg::Index<::tyr::formalism::planning::Metric>, C>
{
private:
    const C* m_context;
    ygg::Index<::tyr::formalism::planning::Metric> m_handle;

public:
    View(ygg::Index<::tyr::formalism::planning::Metric> handle, const C& context) noexcept : m_context(&context), m_handle(handle) {}

    const auto& get_data() const noexcept { return get_repository(*m_context)[m_handle]; }
    const auto& get_context() const noexcept { return *m_context; }
    const auto& get_handle() const noexcept { return m_handle; }

    auto get_index() const noexcept { return m_handle; }
    auto get_optimization_direction() const noexcept { return get_data().optimization_direction; }
    auto get_fexpr() const noexcept { return ygg::make_view(get_data().fexpr, *m_context); }

    auto identifying_members() const noexcept { return std::tie(m_handle, m_context->get_index()); }
};

}

#endif
