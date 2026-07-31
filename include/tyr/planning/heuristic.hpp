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

#ifndef TYR_PLANNING_HEURISTIC_HPP_
#define TYR_PLANNING_HEURISTIC_HPP_

#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/ground/state_view.hpp"
#include "tyr/planning/lifted/state_view.hpp"
#include "tyr/planning/node.hpp"

#include <yggdrasil/containers/associative_containers.hpp>
#include <yggdrasil/core/config.hpp>

namespace tyr::planning
{

template<TaskKind Kind>
class Heuristic
{
public:
    virtual ~Heuristic() = default;

    virtual void set_goal(::tyr::formalism::planning::GroundConjunctiveConditionView goal) = 0;

    virtual ygg::float_t evaluate(const StateView<Kind>& state) = 0;

    virtual ygg::float_t evaluate(const Node<Kind>& node) { return evaluate(node.get_state()); }

    virtual const ygg::UnorderedSet<::tyr::formalism::planning::ActionBindingView>& get_preferred_actions()
    {
        static const auto actions = ygg::UnorderedSet<::tyr::formalism::planning::ActionBindingView> {};
        return actions;
    }

    virtual void print_summary([[maybe_unused]] size_t verbosity) const {}
};

}

#endif
