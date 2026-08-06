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

#ifndef TYR_PLANNING_GROUND_HEURISTICS_RPG_FF_HPP_
#define TYR_PLANNING_GROUND_HEURISTICS_RPG_FF_HPP_

#include "tyr/planning/heuristic.hpp"
#include "tyr/planning/heuristics/rpg_ff.hpp"

#include <memory>

namespace tyr::planning
{

template<>
class FFRPGHeuristic<GroundTag> : public Heuristic<GroundTag>
{
public:
    FFRPGHeuristic(TaskPtr<GroundTag> task, ygg::ExecutionContextPtr execution_context, CostMode cost_mode = CostMode::GENERAL);
    ~FFRPGHeuristic() override;

    FFRPGHeuristic(const FFRPGHeuristic&) = delete;
    FFRPGHeuristic& operator=(const FFRPGHeuristic&) = delete;
    FFRPGHeuristic(FFRPGHeuristic&&) noexcept;
    FFRPGHeuristic& operator=(FFRPGHeuristic&&) noexcept;

    static FFRPGHeuristicPtr<GroundTag> create(TaskPtr<GroundTag> task, ygg::ExecutionContextPtr execution_context, CostMode cost_mode = CostMode::GENERAL);

    using Heuristic<GroundTag>::evaluate;

    void set_goal(::tyr::formalism::planning::GroundConjunctiveConditionView goal) override;
    ygg::float_t evaluate(const ygg::Builder<State<GroundTag>>& state) override;
    [[nodiscard]] HeuristicPtr<GroundTag> make_worker(ygg::ExecutionContextPtr execution_context) const override;

    const ygg::UnorderedSet<::tyr::formalism::planning::ActionBindingView>& get_preferred_actions() override;

private:
    struct Impl;
    explicit FFRPGHeuristic(std::unique_ptr<Impl> impl);
    std::unique_ptr<Impl> m_impl;
};

}

#endif
