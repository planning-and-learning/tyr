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

#ifndef TYR_FORMALISM_PARAMETER_INDEX_HPP_
#define TYR_FORMALISM_PARAMETER_INDEX_HPP_

#include <urd/elements/flat.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <yggdrasil/core/unsigned_integer.hpp>

namespace tyr::formalism
{
/// A parameter's position: a nonnegative integer of its own type (no index: nothing resolves it).
struct ParameterIndex : ygg::UnsignedInteger<ParameterIndex>
{
    using UnsignedInteger::UnsignedInteger;
};

using ParameterList = urd::Store<urd::sequence<ParameterIndex>>;

}

#endif
