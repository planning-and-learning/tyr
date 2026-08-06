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

#ifndef TYR_PLANNING_HEURISTICS_RPG_ADD_HPP_
#define TYR_PLANNING_HEURISTICS_RPG_ADD_HPP_

#include "tyr/planning/heuristic.hpp"

#include <cstddef>
#include <memory>

namespace tyr::planning
{

template<TaskKind Kind>
class AddRPGHeuristic : public Heuristic<Kind>
{
public:
    AddRPGHeuristic(TaskPtr<Kind> task, ygg::ExecutionContextPtr execution_context, CostMode cost_mode = CostMode::GENERAL);
    ~AddRPGHeuristic() override;

    AddRPGHeuristic(const AddRPGHeuristic&) = delete;
    AddRPGHeuristic& operator=(const AddRPGHeuristic&) = delete;
    AddRPGHeuristic(AddRPGHeuristic&&) noexcept;
    AddRPGHeuristic& operator=(AddRPGHeuristic&&) noexcept;

    static AddRPGHeuristicPtr<Kind> create(TaskPtr<Kind> task, ygg::ExecutionContextPtr execution_context, CostMode cost_mode = CostMode::GENERAL);

    using Heuristic<Kind>::evaluate;

    void set_goal(::tyr::formalism::planning::GroundConjunctiveConditionView goal) override;
    ygg::float_t evaluate(const ygg::Builder<State<Kind>>& state) override;
    [[nodiscard]] HeuristicPtr<Kind> make_worker(ygg::ExecutionContextPtr execution_context) const override;
    void print_summary(size_t verbosity) const override;

private:
    struct Impl;
    explicit AddRPGHeuristic(std::unique_ptr<Impl> impl);
    std::unique_ptr<Impl> m_impl;
};

}

#endif
