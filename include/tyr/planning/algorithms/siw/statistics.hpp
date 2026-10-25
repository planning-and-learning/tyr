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

#ifndef TYR_PLANNING_ALGORITHMS_SIW_STATISTICS_HPP_
#define TYR_PLANNING_ALGORITHMS_SIW_STATISTICS_HPP_

#include "tyr/planning/declarations.hpp"

#include <algorithm>
#include <optional>
#include <yggdrasil/core/types.hpp>

namespace tyr::planning::siw
{

template<TaskKind Kind>
class Statistics
{
private:
    std::optional<ygg::uint_t> m_maximum_effective_width;
    double m_total_effective_width = 0.;
    ygg::uint_t m_num_solved_subsearches = 0;

public:
    void clear() noexcept
    {
        m_maximum_effective_width = std::nullopt;
        m_total_effective_width = 0.;
        m_num_solved_subsearches = 0;
    }

    void add_effective_width(ygg::uint_t width)
    {
        m_maximum_effective_width = m_maximum_effective_width ? std::max(*m_maximum_effective_width, width) : width;
        m_total_effective_width += static_cast<double>(width);
        ++m_num_solved_subsearches;
    }

    [[nodiscard]] std::optional<ygg::uint_t> get_maximum_effective_width() const { return m_maximum_effective_width; }
    [[nodiscard]] std::optional<double> get_average_effective_width() const
    {
        if (m_num_solved_subsearches == 0)
            return std::nullopt;
        return m_total_effective_width / static_cast<double>(m_num_solved_subsearches);
    }
    [[nodiscard]] ygg::uint_t get_num_solved_subsearches() const { return m_num_solved_subsearches; }
};

}  // namespace tyr::planning::siw

#endif
