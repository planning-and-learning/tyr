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

#ifndef TYR_PLANNING_GROUND_STATE_LAYOUT_HPP_
#define TYR_PLANNING_GROUND_STATE_LAYOUT_HPP_

#include "tyr/planning/declarations.hpp"
#include "tyr/planning/state_storage.hpp"

#include <cstdint>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <yggdrasil/core/config.hpp>

namespace tyr::planning
{

/// Where the fluent FDR values and derived atoms of a ground task's states lie, packed bitwise into ygg::uint_t blocks.
template<>
struct StateLayout<GroundTag>
{
    /// Bit range of a fluent FDR variable's value.
    struct VariableInfo
    {
        ygg::uint_t begin;
        uint8_t offset;
        uint8_t length;
    };

    explicit StateLayout(const Task<GroundTag>& task);

    urd::Store<urd::sequence<VariableInfo>> fluent_infos;
    ygg::uint_t fluent_num_blocks;
    ygg::uint_t derived_num_bits;
};

}

#endif
