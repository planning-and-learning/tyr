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

#ifndef TYR_PLANNING_LIFTED_HEURISTICS_LMCUT_HPP_
#define TYR_PLANNING_LIFTED_HEURISTICS_LMCUT_HPP_

#include "tyr/planning/heuristic.hpp"
#include "tyr/planning/heuristics/lmcut.hpp"

#include <memory>

namespace tyr::planning
{
template<>
class LMCutHeuristic<LiftedTag> : public Heuristic<LiftedTag>
{
public:
    LMCutHeuristic(TaskPtr<LiftedTag> task, ygg::ExecutionContextPtr execution_context, CostMode cost_mode = CostMode::GENERAL);
    ~LMCutHeuristic() override;

    LMCutHeuristic(LMCutHeuristic&&) noexcept;
    LMCutHeuristic& operator=(LMCutHeuristic&&) noexcept;
    LMCutHeuristic(const LMCutHeuristic&) = delete;
    LMCutHeuristic& operator=(const LMCutHeuristic&) = delete;

    static LMCutHeuristicPtr<LiftedTag> create(TaskPtr<LiftedTag> task, ygg::ExecutionContextPtr execution_context, CostMode cost_mode = CostMode::GENERAL);

    void set_goal(::tyr::formalism::planning::GroundConjunctiveConditionView goal) override;
    ygg::float_t evaluate(const StateView<LiftedTag>& state) override;
    void print_summary(size_t verbosity) const override;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

}

#endif
