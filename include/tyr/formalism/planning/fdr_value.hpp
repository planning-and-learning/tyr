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

#ifndef TYR_FORMALISM_PLANNING_FDR_VALUE_HPP_
#define TYR_FORMALISM_PLANNING_FDR_VALUE_HPP_

#include "tyr/formalism/planning/declarations.hpp"

#include <urd/elements/flat.hpp>
#include <yggdrasil/core/unsigned_integer.hpp>

namespace tyr::formalism::planning
{
/// A variable's value: a nonnegative integer of its own type. Value 0, none(), is the variable's none value.
struct FDRValue : ygg::UnsignedInteger<FDRValue>
{
    using UnsignedInteger::UnsignedInteger;

    static constexpr FDRValue none() noexcept { return FDRValue { 0 }; }
};

}

#endif
