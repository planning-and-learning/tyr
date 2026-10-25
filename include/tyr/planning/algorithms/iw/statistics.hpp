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

#ifndef TYR_PLANNING_ALGORITHMS_IW_STATISTICS_HPP_
#define TYR_PLANNING_ALGORITHMS_IW_STATISTICS_HPP_

#include "tyr/planning/declarations.hpp"

#include <optional>
#include <yggdrasil/core/types.hpp>

namespace tyr::planning::iw
{

template<TaskKind Kind>
class Statistics
{
private:
    std::optional<ygg::uint_t> m_solution_arity;

public:
    void clear() noexcept { m_solution_arity = std::nullopt; }
    void set_solution_arity(ygg::uint_t arity) { m_solution_arity = arity; }
    [[nodiscard]] std::optional<ygg::uint_t> get_solution_arity() const { return m_solution_arity; }
};

}

#endif
