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

#include "common.hpp"

#include "tyr/formalism/planning/repository.hpp"

namespace fp = tyr::formalism::planning;

namespace tyr::planning
{

::cista::offset::string create_applicability_name(fp::ActionView<LiftedTag> action)
{
    return ::cista::offset::string { std::string { "@" } + action.get_name().str() + std::string("_") + std::to_string(action.get_index().get_value()) };
}

::cista::offset::string create_triggered_name(fp::ActionView<LiftedTag> action, fp::ConditionalEffectView<LiftedTag> cond_eff)
{
    return ::cista::offset::string { create_applicability_name(action).str() + std::string("_") + std::to_string(cond_eff.get_index().get_value()) };
}

::cista::offset::string create_applicability_name(fp::AxiomView<LiftedTag> axiom)
{
    return ::cista::offset::string { std::string { "@" } + axiom.get_head().get_predicate().get_name().str() + std::string("_")
                                     + std::to_string(axiom.get_index().get_value()) };
}

}
