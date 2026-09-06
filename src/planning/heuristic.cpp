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

#include "tyr/planning/heuristic.hpp"

#include "tyr/formalism/planning/repository.hpp"
#include "tyr/planning/ground/state_view.hpp"
#include "tyr/planning/lifted/state_view.hpp"

namespace tyr::planning
{

template<TaskKind Kind>
ygg::float_t Heuristic<Kind>::evaluate(const StateView<Kind>& state)
{
    return evaluate(state.get_state_builder());
}

template<TaskKind Kind>
const ygg::UnorderedSet<formalism::planning::ActionBindingView>& Heuristic<Kind>::get_preferred_actions()
{
    static const auto actions = ygg::UnorderedSet<formalism::planning::ActionBindingView> {};
    return actions;
}

template class Heuristic<GroundTag>;
template class Heuristic<LiftedTag>;

}
