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

#ifndef TYR_PLANNING_HEURISTICS_BLIND_HPP_
#define TYR_PLANNING_HEURISTICS_BLIND_HPP_

#include "tyr/planning/declarations.hpp"
#include "tyr/planning/heuristic.hpp"

#include <memory>

namespace tyr::planning
{

template<TaskKind Kind>
class BlindHeuristic : public Heuristic<Kind>
{
public:
    BlindHeuristic() = default;

    static std::shared_ptr<BlindHeuristic> create() { return std::make_shared<BlindHeuristic>(); }

    using Heuristic<Kind>::evaluate;

    void set_goal([[maybe_unused]] ::tyr::formalism::planning::GroundConjunctiveConditionView goal) override {}

    ygg::float_t evaluate([[maybe_unused]] const ygg::Builder<State<Kind>>& state) override { return ygg::float_t { 0 }; }

    [[nodiscard]] HeuristicPtr<Kind> make_worker([[maybe_unused]] ygg::ExecutionContextPtr execution_context) const override { return create(); }
};

}

#endif
