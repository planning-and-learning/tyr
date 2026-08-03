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

#ifndef TYR_PLANNING_GROUND_HEURISTICS_RPG_MAX_HPP_
#define TYR_PLANNING_GROUND_HEURISTICS_RPG_MAX_HPP_

#include "tyr/planning/heuristic.hpp"
#include "tyr/planning/heuristics/rpg_max.hpp"

#include <memory>

namespace tyr::planning
{

template<>
class MaxRPGHeuristic<GroundTag> : public Heuristic<GroundTag>
{
public:
    MaxRPGHeuristic(TaskPtr<GroundTag> task, ygg::ExecutionContextPtr execution_context, CostMode cost_mode = CostMode::GENERAL);
    ~MaxRPGHeuristic() override;

    MaxRPGHeuristic(const MaxRPGHeuristic&) = delete;
    MaxRPGHeuristic& operator=(const MaxRPGHeuristic&) = delete;
    MaxRPGHeuristic(MaxRPGHeuristic&&) noexcept;
    MaxRPGHeuristic& operator=(MaxRPGHeuristic&&) noexcept;

    static MaxRPGHeuristicPtr<GroundTag> create(TaskPtr<GroundTag> task, ygg::ExecutionContextPtr execution_context, CostMode cost_mode = CostMode::GENERAL);

    void set_goal(::tyr::formalism::planning::GroundConjunctiveConditionView goal) override;
    ygg::float_t evaluate(const StateView<GroundTag>& state) override;
    [[nodiscard]] HeuristicPtr<GroundTag> make_worker(ygg::ExecutionContextPtr execution_context) const override;

private:
    struct Impl;
    explicit MaxRPGHeuristic(std::unique_ptr<Impl> impl);
    std::unique_ptr<Impl> m_impl;
};

}

#endif
