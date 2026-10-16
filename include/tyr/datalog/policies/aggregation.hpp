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

#ifndef TYR_DATALOG_POLICIES_AGGREGATION_HPP_
#define TYR_DATALOG_POLICIES_AGGREGATION_HPP_

#include <algorithm>
#include <yggdrasil/core/config.hpp>

namespace tyr::datalog
{
using Cost = ygg::float_t;

struct SumAggregation
{
    static constexpr Cost identity() noexcept { return Cost(0); }
    constexpr Cost operator()(Cost acc, Cost x) const noexcept { return acc + x; }
};

struct MaxAggregation
{
    static constexpr Cost identity() noexcept { return Cost(0); }
    constexpr Cost operator()(Cost acc, Cost x) const noexcept { return std::max(acc, x); }
};

/// The part of `total` not yet covered by `used` (saturating at zero).
inline Cost reduce_cost(Cost total, Cost used) noexcept { return used >= total ? Cost(0) : total - used; }

}

#endif
