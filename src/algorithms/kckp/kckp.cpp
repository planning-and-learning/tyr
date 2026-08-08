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

#include "tyr/algorithms/kckp/kckp.hpp"

namespace tyr::kckp
{

bool KCKP::seed_from_prefix(std::span<const Vertex> prefix, Workspace& workspace) const
{
    const auto& layout = m_graph.get_layout();
    workspace.prepare(layout);
    workspace.partition_bits.reset();
    workspace.partial_solution_size = static_cast<ygg::uint_t>(prefix.size());

    for (ygg::uint_t partition = 0; partition < prefix.size(); ++partition)
    {
        const auto vertex = prefix[partition];
        if (vertex.index >= layout.num_vertices || layout.vertex_to_partition[vertex.index] != partition)
            return false;

        workspace.partial_solution[partition] = vertex;
        workspace.partition_bits.set(partition);
        for (ygg::uint_t previous = 0; previous < partition; ++previous)
        {
            const auto previous_vertex = workspace.partial_solution[previous];
            const auto bit = layout.vertex_to_bit[vertex.index];
            if (!m_graph.get_adjacency_matrix().get_bitset(previous_vertex.index, partition).test(bit))
                return false;
        }
    }

    if (prefix.size() == layout.num_partitions)
        return true;

    auto initial = workspace.compatible_vertices(0);
    for (ygg::uint_t partition = static_cast<ygg::uint_t>(prefix.size()); partition < layout.num_partitions; ++partition)
    {
        const auto& info = layout.info.infos[partition];
        auto candidates = ygg::BitsetSpan<uint64_t>(initial.data() + info.block_offset, info.num_bits);
        candidates.set();
        for (ygg::uint_t fixed_partition = 0; fixed_partition < prefix.size(); ++fixed_partition)
        {
            const auto fixed_vertex = workspace.partial_solution[fixed_partition];
            candidates &= m_graph.get_adjacency_matrix().get_bitset(fixed_vertex.index, partition);
        }
        if (!candidates.any())
            return false;
    }
    return true;
}

ygg::uint_t KCKP::choose_next_partition(ygg::uint_t first_partition, const Workspace& workspace) const noexcept
{
    const auto& layout = m_graph.get_layout();
    for (ygg::uint_t partition = first_partition; partition < layout.num_partitions; ++partition)
        if (!workspace.partition_bits.test(partition))
            return partition;
    return std::numeric_limits<ygg::uint_t>::max();
}

bool KCKP::update_candidates(ygg::uint_t first_partition, Vertex source, size_t depth, Workspace& workspace) const
{
    const auto& layout = m_graph.get_layout();
    const auto current_row = workspace.compatible_vertices(depth);
    auto next_row = workspace.compatible_vertices(depth + 1);
    for (ygg::uint_t partition = first_partition; partition < layout.num_partitions; ++partition)
    {
        if (workspace.partition_bits.test(partition))
            continue;

        const auto& info = layout.info.infos[partition];
        const auto source_candidates = ygg::BitsetSpan<const uint64_t>(current_row.data() + info.block_offset, info.num_bits);
        auto destination = ygg::BitsetSpan<uint64_t>(next_row.data() + info.block_offset, info.num_bits);
        destination.copy_from(source_candidates);
        destination &= m_graph.get_adjacency_matrix().get_bitset(source.index, partition);
        if (!destination.any())
            return false;
    }
    return true;
}

}
