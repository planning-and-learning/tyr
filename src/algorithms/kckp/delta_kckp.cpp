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

#include "tyr/algorithms/kckp/delta_kckp.hpp"

#include <stdexcept>

namespace tyr::kckp
{

PartitionedAdjacencyLayout::PartitionedAdjacencyLayout(const GraphLayout& layout, std::span<const AdjacencyKind> adjacency_kinds) :
    m_num_partitions(layout.num_partitions),
    m_row_offsets(layout.num_vertices),
    m_pair_offsets(),
    m_num_blocks(0)
{
    if (m_num_partitions != 0 && m_num_partitions > m_pair_offsets.max_size() / m_num_partitions)
        throw std::overflow_error("PartitionedAdjacencyLayout: partition-pair table size overflow");

    const auto num_partition_pairs = m_num_partitions * m_num_partitions;
    if (adjacency_kinds.size() != num_partition_pairs)
        throw std::invalid_argument("PartitionedAdjacencyLayout: expected one adjacency kind per partition pair");

    m_pair_offsets.assign(num_partition_pairs, IMPLICIT);
    const auto checked_add = [](size_t lhs, size_t rhs)
    {
        constexpr auto max_blocks = static_cast<size_t>(STATIC_ONLY);
        if (lhs >= max_blocks || rhs >= max_blocks - lhs)
            throw std::overflow_error("PartitionedAdjacencyLayout: adjacency block offset overflow");
        return lhs + rhs;
    };

    for (ygg::uint_t source_partition = 0; source_partition < m_num_partitions; ++source_partition)
    {
        auto row_blocks = size_t { 0 };
        for (ygg::uint_t target_partition = 0; target_partition < m_num_partitions; ++target_partition)
        {
            const auto pair = source_partition * m_num_partitions + target_partition;
            switch (adjacency_kinds[pair])
            {
                case AdjacencyKind::IMPLICIT:
                    break;
                case AdjacencyKind::STATIC_ONLY:
                    m_pair_offsets[pair] = STATIC_ONLY;
                    break;
                case AdjacencyKind::RUNTIME:
                    m_pair_offsets[pair] = static_cast<ygg::uint_t>(row_blocks);
                    row_blocks = checked_add(row_blocks, layout.info.infos[target_partition].num_blocks);
                    break;
                default:
                    throw std::invalid_argument("PartitionedAdjacencyLayout: invalid adjacency kind");
            }
        }

        for (const auto vertex : layout.vertex_partitions[source_partition])
        {
            m_row_offsets[vertex] = static_cast<ygg::uint_t>(m_num_blocks);
            m_num_blocks = checked_add(m_num_blocks, row_blocks);
        }
    }
}

DeltaKCKP::DeltaKCKP(const Graph& graph, const PartitionedAdjacencyLayout& adjacency_layout) :
    m_layout(graph.get_layout()),
    m_delta_graph(m_layout, adjacency_layout, graph.get_adjacency_matrix()),
    m_full_graph(m_layout, adjacency_layout, graph.get_adjacency_matrix())
{
}

void DeltaKCKP::reset()
{
    m_delta_graph.reset();
    m_full_graph.reset();
    m_delta_edges.clear();
    m_iteration = 0;
}

const std::vector<Edge>& DeltaKCKP::materialize_delta_edges()
{
    m_delta_edges.clear();
    m_delta_graph.matrix.for_each_edge([&](Edge edge) { m_delta_edges.push_back(edge); });
    return m_delta_edges;
}

void DeltaKCKP::seed_without_anchor(Workspace& workspace) const
{
    workspace.prepare(m_layout);
    workspace.partial_solution_size = 0;
    workspace.partition_bits.reset();
    workspace.anchor_pi = std::numeric_limits<ygg::uint_t>::max();
    workspace.anchor_pj = std::numeric_limits<ygg::uint_t>::max();

    auto compatible_row = workspace.compatible_vertices(0);
    for (ygg::uint_t partition = 0; partition < m_layout.num_partitions; ++partition)
    {
        const auto& info = m_layout.info.infos[partition];
        auto compatible = ygg::BitsetSpan<uint64_t>(compatible_row.data() + info.block_offset, info.num_bits);
        compatible.copy_from(m_full_graph.matrix.affected_partitions().get_bitset(info));
    }
}

bool DeltaKCKP::seed_from_anchor(const Edge& edge, Workspace& workspace) const
{
    workspace.prepare(m_layout);
    const auto source_partition = m_layout.vertex_to_partition[edge.src.index];
    const auto target_partition = m_layout.vertex_to_partition[edge.dst.index];
    assert(source_partition < target_partition);

    workspace.partial_solution[source_partition] = edge.src;
    workspace.partial_solution[target_partition] = edge.dst;
    workspace.partial_solution_size = 2;
    workspace.anchor_pi = source_partition;
    workspace.anchor_pj = target_partition;
    workspace.partition_bits.reset();
    workspace.partition_bits.set(source_partition);
    workspace.partition_bits.set(target_partition);

    auto compatible_row = workspace.compatible_vertices(0);
    for (ygg::uint_t partition = 0; partition < m_layout.num_partitions; ++partition)
    {
        if (partition == source_partition || partition == target_partition)
            continue;

        const auto& info = m_layout.info.infos[partition];
        auto compatible = ygg::BitsetSpan<uint64_t>(compatible_row.data() + info.block_offset, info.num_bits);
        m_full_graph.matrix.copy_adjacency_to(edge.src.index, partition, compatible);
        m_full_graph.matrix.intersect_adjacency_with(edge.dst.index, partition, compatible);
        if (!compatible.any())
            return false;

        if (partition < target_partition)
            m_delta_graph.matrix.subtract_adjacency_from(edge.src.index, partition, compatible);
        if (!compatible.any())
            return false;

        if (partition < source_partition)
            m_delta_graph.matrix.subtract_adjacency_from(edge.dst.index, partition, compatible);
        if (!compatible.any())
            return false;
    }
    return true;
}

ygg::uint_t DeltaKCKP::choose_best_partition(size_t depth, const Workspace& workspace) const
{
    auto best_partition = std::numeric_limits<ygg::uint_t>::max();
    auto best_set_bits = std::numeric_limits<ygg::uint_t>::max();
    const auto current_row = workspace.compatible_vertices(depth);

    for (ygg::uint_t partition = 0; partition < m_layout.num_partitions; ++partition)
    {
        if (workspace.partition_bits.test(partition))
            continue;

        const auto& info = m_layout.info.infos[partition];
        const auto current = ygg::BitsetSpan<const uint64_t>(current_row.data() + info.block_offset, info.num_bits);
        const auto num_set_bits = current.count();
        if (num_set_bits < best_set_bits)
        {
            best_set_bits = num_set_bits;
            best_partition = partition;
        }
    }
    return best_partition;
}

}
