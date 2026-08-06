/*
 * Copyright (C) 2026 Dominik Drexler
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

#ifndef TYR_SRC_PLANNING_ALGORITHMS_SEARCH_ENGINE_SUCCESSOR_GENERATOR_ACCESS_HPP_
#define TYR_SRC_PLANNING_ALGORITHMS_SEARCH_ENGINE_SUCCESSOR_GENERATOR_ACCESS_HPP_

#include "tyr/planning/successor_generator.hpp"

namespace tyr::planning::detail
{

class SuccessorGeneratorAccess
{
public:
    template<TaskKind Kind>
    static CompletedActionResult complete(SuccessorGenerator<Kind>& generator, ygg::Builder<State<Kind>>& state, PendingActionResult result)
    {
        return generator.complete_successor_state(state, result);
    }

    template<TaskKind Kind>
    static Node<Kind>
    register_state(SuccessorGenerator<Kind>& generator, ygg::SharedObjectPoolPtr<ygg::Builder<State<Kind>>, true> state, CompletedActionResult result)
    {
        return generator.register_completed_successor_state(std::move(state), result);
    }
};

}

#endif
