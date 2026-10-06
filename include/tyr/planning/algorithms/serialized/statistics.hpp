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

#ifndef TYR_PLANNING_ALGORITHMS_SERIALIZED_STATISTICS_HPP_
#define TYR_PLANNING_ALGORITHMS_SERIALIZED_STATISTICS_HPP_

#include "tyr/planning/algorithms/utils.hpp"

#include <concepts>
#include <vector>
#include <yggdrasil/core/types.hpp>

namespace tyr::planning::serialized
{

template<std::copy_constructible Stats = tyr::planning::Statistics>
class Statistics
{
public:
    using SearchStatistics = tyr::planning::Statistics;
    using SolverStatistics = Stats;

private:
    std::vector<SearchStatistics> m_search_statistics;
    std::vector<SolverStatistics> m_solver_statistics;

public:
    void clear()
    {
        m_search_statistics.clear();
        m_solver_statistics.clear();
    }

    void add_search_statistics(const SearchStatistics& statistics) { m_search_statistics.push_back(statistics); }
    void add_solver_statistics(const SolverStatistics& statistics) { m_solver_statistics.push_back(statistics); }

    ygg::uint_t get_num_subsearches() const { return static_cast<ygg::uint_t>(m_search_statistics.size()); }
    const std::vector<SearchStatistics>& get_search_statistics() const { return m_search_statistics; }
    const std::vector<SolverStatistics>& get_solver_statistics() const { return m_solver_statistics; }
};

}  // namespace tyr::planning::serialized

#endif
