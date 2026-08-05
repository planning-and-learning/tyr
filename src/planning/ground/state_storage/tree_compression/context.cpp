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

#include "tyr/planning/ground/state_storage/tree_compression/context.hpp"

#include "tyr/planning/ground/task.hpp"

namespace tyr::planning
{
namespace
{
template<bool ThreadSafe, std::unsigned_integral Block>
auto compute_layout_data(const Task<GroundTag>& task) -> typename StateStorageContext<GroundTag, TreeCompression, ThreadSafe>::LayoutData
{
    using Context = StateStorageContext<GroundTag, TreeCompression, ThreadSafe>;
    using VariableInfo = Context::VariableInfo;
    using LayoutData = Context::LayoutData;

    constexpr ygg::uint_t bits_per_block = static_cast<ygg::uint_t>(ygg::bit::bits_per_block_v<Block>);

    auto layout = LayoutData {};

    ygg::uint_t current_bit = 0;

    for (const auto variable : task.get_formalism_task().get_task().get_fluent_variables())
    {
        const auto domain_size = static_cast<ygg::uint_t>(variable.get_atoms().size() + 1);
        const auto length = static_cast<ygg::uint_t>(ygg::bit::bits_needed(domain_size));

        layout.fluent_infos.push_back(VariableInfo {
            .begin = current_bit / bits_per_block,
            .offset = static_cast<uint8_t>(current_bit % bits_per_block),
            .length = static_cast<uint8_t>(length),
        });

        current_bit += length;
    }

    layout.fluent_array_size = ygg::bit::ceil_div(current_bit, bits_per_block);
    layout.derived_num_bits = static_cast<ygg::uint_t>(task.get_formalism_task().get_task().get_atoms<::tyr::formalism::DerivedTag>().size());
    layout.derived_array_size = ygg::bit::ceil_div(layout.derived_num_bits, bits_per_block);

    return layout;
}

}

template<bool ThreadSafe>
StateStorageContext<GroundTag, TreeCompression, ThreadSafe>::StateStorageContext(const Task<GroundTag>& task) :
    StateStorageContext(compute_layout_data<ThreadSafe, ygg::uint_t>(task))
{
}

template<bool ThreadSafe>
StateStorageContext<GroundTag, TreeCompression, ThreadSafe>::StateStorageContext(LayoutData&& layout_data) :
    fluent_infos(layout_data.fluent_infos),
    fluent_array_set(layout_data.fluent_array_size),
    derived_num_bits(layout_data.derived_num_bits),
    derived_array_set(layout_data.derived_array_size)
{
}

template struct StateStorageContext<GroundTag, TreeCompression, false>;
template struct StateStorageContext<GroundTag, TreeCompression, true>;

}
