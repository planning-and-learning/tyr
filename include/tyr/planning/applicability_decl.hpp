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

#ifndef TYR_PLANNING_APPLICABILITY_DECL_HPP_
#define TYR_PLANNING_APPLICABILITY_DECL_HPP_

#include "tyr/planning/declarations.hpp"

#include <yggdrasil/core/config.hpp>

namespace tyr::planning
{

template<TaskKind Kind>
struct StateContext
{
    const Task<Kind>& task;
    const ygg::Builder<State<Kind>>& state_builder;
    ygg::float_t auxiliary_value;
};

}

#endif
