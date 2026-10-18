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

#ifndef TYR_TESTS_UNIT_PLANNING_ALGORITHMS_STATISTICS_HEURISTICS_HPP_
#define TYR_TESTS_UNIT_PLANNING_ALGORITHMS_STATISTICS_HEURISTICS_HPP_

#include "statistics.hpp"

#include <stdexcept>
#include <string>
#include <tyr/declarations.hpp>
#include <tyr/planning/heuristics/blind.hpp>
#include <tyr/planning/heuristics/lmcut.hpp>
#include <tyr/planning/heuristics/rpg_add.hpp>
#include <tyr/planning/heuristics/rpg_ff.hpp>
#include <tyr/planning/heuristics/rpg_max.hpp>
#include <utility>

namespace tyr::tests
{

inline std::pair<std::string, CostMode> parse_costed_heuristic_key(const std::string& key)
{
    const auto pos = key.rfind('_');
    if (pos == std::string::npos)
        throw std::invalid_argument("parse_costed_heuristic_key(...): missing cost suffix in key: " + key);

    const auto suffix = key.substr(pos + 1);
    if (suffix == "unit")
        return { key.substr(0, pos), CostMode::UNIT };
    if (suffix == "general")
        return { key.substr(0, pos), CostMode::GENERAL };

    throw std::invalid_argument("parse_costed_heuristic_key(...): unknown cost suffix in key: " + key);
}

template<TaskKind Kind>
planning::HeuristicPtr<Kind> create_search_heuristic(const std::string& name, const SearchContext<Kind>& context, CostMode cost_mode)
{
    if (name == "blind")
        return planning::BlindHeuristic<Kind>::create();
    if (name == "hmax")
        return planning::MaxRPGHeuristic<Kind>::create(context.task, context.execution_context, cost_mode);
    if (name == "hadd")
        return planning::AddRPGHeuristic<Kind>::create(context.task, context.execution_context, cost_mode);
    if (name == "hff")
        return planning::FFRPGHeuristic<Kind>::create(context.task, context.execution_context, cost_mode);
    if (name == "hlmcut")
        return planning::LMCutHeuristic<Kind>::create(context.task, context.execution_context, cost_mode);
    throw std::invalid_argument("create_search_heuristic(...): unknown heuristic: " + name);
}

}

#endif
