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

#ifndef TYR_PLANNING_LIFTED_STATE_VIEW_HPP_
#define TYR_PLANNING_LIFTED_STATE_VIEW_HPP_

#include "tyr/planning/declarations.hpp"
#include "tyr/planning/lifted/state_storage/iterators.hpp"
#include "tyr/planning/state_view.hpp"

namespace ygg
{
namespace planning = ::tyr::planning;

using LiftedStateView = ygg::View<ygg::Index<planning::State<::tyr::LiftedTag>>, std::shared_ptr<planning::StateRepository<::tyr::LiftedTag>>>;
}

namespace tyr
{
using LiftedStateView = ygg::LiftedStateView;
}

#endif
