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

#ifndef TYR_PLANNING_GROUND_HEURISTICS_LMCUT_HPP_
#define TYR_PLANNING_GROUND_HEURISTICS_LMCUT_HPP_

#include "tyr/planning/heuristic.hpp"
#include "tyr/planning/heuristics/lmcut.hpp"

#include <memory>

namespace tyr::planning
{

template<>
class LMCutHeuristic<GroundTag> : public Heuristic<GroundTag>
{
public:
    LMCutHeuristic(TaskPtr<GroundTag> task, ygg::ExecutionContextPtr execution_context, CostMode cost_mode = CostMode::GENERAL);
    ~LMCutHeuristic() override;

    LMCutHeuristic(const LMCutHeuristic&) = delete;
    LMCutHeuristic& operator=(const LMCutHeuristic&) = delete;
    LMCutHeuristic(LMCutHeuristic&&) noexcept;
    LMCutHeuristic& operator=(LMCutHeuristic&&) noexcept;

    static LMCutHeuristicPtr<GroundTag> create(TaskPtr<GroundTag> task, ygg::ExecutionContextPtr execution_context, CostMode cost_mode = CostMode::GENERAL);

    using Heuristic<GroundTag>::evaluate;

    void set_goal(::tyr::formalism::planning::GroundConjunctiveConditionView goal) override;
    ygg::float_t evaluate(const ygg::Builder<State<GroundTag>>& state) override;
    ::tyr::formalism::planning::GroundAtomViewList<::tyr::formalism::FluentTag> compute_cut_frontier_atoms(const ygg::Builder<State<GroundTag>>& state);
    [[nodiscard]] HeuristicPtr<GroundTag> make_worker(ygg::ExecutionContextPtr execution_context) const override;

private:
    struct Impl;
    explicit LMCutHeuristic(std::unique_ptr<Impl> impl);
    std::unique_ptr<Impl> m_impl;
};

}

#endif
