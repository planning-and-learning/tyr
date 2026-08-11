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

#ifndef TYR_SRC_PLANNING_PROGRAMS_COMMON_HPP_
#define TYR_SRC_PLANNING_PROGRAMS_COMMON_HPP_

#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/planning/declarations.hpp"

#include <cista/containers/string.h>
#include <yggdrasil/core/types.hpp>

namespace tyr::planning
{
extern ::cista::offset::string create_applicability_name(::tyr::formalism::planning::ActionView action);

extern ::cista::offset::string create_triggered_name(::tyr::formalism::planning::ActionView action, ::tyr::formalism::planning::ConditionalEffectView cond_eff);

extern ::cista::offset::string create_applicability_name(::tyr::formalism::planning::AxiomView axiom);

}

#endif
