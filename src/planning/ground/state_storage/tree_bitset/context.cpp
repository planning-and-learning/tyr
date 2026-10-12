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

#include "tyr/planning/ground/state_storage/tree_bitset/context.hpp"

#include "tyr/planning/ground/task.hpp"

#include <yggdrasil/core/bit.hpp>

namespace tyr::planning
{

template<bool ThreadSafe>
StateStorageContext<GroundTag, TreeBitset, ThreadSafe>::StateStorageContext(const Task<GroundTag>& task)
{
    constexpr auto bits_per_block = static_cast<ygg::uint_t>(ygg::bit::bits_per_block_v<tree_bitset::Block>);

    auto current_bit = ygg::uint_t { 0 };
    for (const auto variable : task.get_formalism_task().get_task().get_fluent_variables())
    {
        const auto domain_size = static_cast<ygg::uint_t>(variable.get_atoms().size() + 1);
        const auto length = static_cast<ygg::uint_t>(ygg::bit::bits_needed(domain_size));

        fluent_infos.push_back(VariableInfo {
            .begin = current_bit / bits_per_block,
            .offset = static_cast<uint8_t>(current_bit % bits_per_block),
            .length = static_cast<uint8_t>(length),
        });

        current_bit += length;
    }

    fluent_num_blocks = ygg::bit::ceil_div(current_bit, bits_per_block);
    derived_num_bits = static_cast<ygg::uint_t>(task.get_formalism_task().get_task().get_atoms<formalism::DerivedTag>().size());
}

template struct StateStorageContext<GroundTag, TreeBitset, false>;
template struct StateStorageContext<GroundTag, TreeBitset, true>;

}
