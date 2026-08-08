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

#include "tyr/datalog/lifted/consistency_graph.hpp"
#include "tyr/datalog/lifted/delta_kpkc.hpp"
#include "tyr/formalism/datalog/expression_arity.hpp"

#include <stdexcept>

namespace f = tyr::formalism;
namespace fd = tyr::formalism::datalog;

namespace tyr::datalog::kpkc
{

[[maybe_unused]] static bool verify_vertex_partitions(const std::vector<std::vector<ygg::uint_t>>& vertex_partitions)
{
    ygg::uint_t i = 0;
    for (const auto& partition : vertex_partitions)
        for (const auto& v : partition)
            if (v != i++)
                return false;
    return true;
}

[[maybe_unused]] static bool verify_vertex_to_partition(size_t nv, size_t k, const std::vector<ygg::uint_t>& vertex_to_partition)
{
    if (vertex_to_partition.size() != nv)
        return false;

    for (ygg::uint_t v = 0; v < nv; ++v)
        if (vertex_to_partition[v] >= k)
            return false;

    return true;
}

GraphLayout::GraphLayout(size_t nv, const std::vector<std::vector<ygg::uint_t>>& vertex_partitions_) :
    nv(nv),
    k(vertex_partitions_.size()),
    vertex_partitions(vertex_partitions_),
    vertex_to_partition(),
    vertex_to_bit(),
    info()
{
    assert(verify_vertex_partitions(vertex_partitions_));

    vertex_to_partition.resize(nv);
    vertex_to_bit.resize(nv);
    info.infos.reserve(k);

    ygg::uint_t block_offset = ygg::uint_t(0);
    ygg::uint_t bit_offset = ygg::uint_t(0);

    for (size_t p = 0; p < k; ++p)
    {
        const auto& partition = vertex_partitions[p];

        const auto partition_size = static_cast<ygg::uint_t>(partition.size());
        const auto partition_blocks = static_cast<ygg::uint_t>(ygg::BitsetSpan<uint64_t>::num_blocks(partition_size));
        info.infos.push_back(GraphLayout::BitsetInfo { bit_offset, partition_size, block_offset, partition_blocks });
        block_offset += partition_blocks;

        ygg::uint_t bit = 0;
        for (const auto& v : partition)
        {
            vertex_to_bit[v] = bit++;
            vertex_to_partition[v] = p;

            ++bit_offset;
        }
    }

    info.num_blocks = block_offset;

    assert(verify_vertex_to_partition(nv, k, vertex_to_partition));
}

PartitionedAdjacencyLayout::PartitionedAdjacencyLayout(const GraphLayout& layout, const ::tyr::formalism::datalog::VariableDependencyGraph& dependency_graph) :
    m_k(layout.k),
    m_row_offsets(layout.nv),
    m_pair_offsets(),
    m_num_blocks(0)
{
    if (m_k != 0 && m_k > m_pair_offsets.max_size() / m_k)
        throw std::overflow_error("PartitionedAdjacencyLayout: partition-pair table size overflow");
    m_pair_offsets.assign(m_k * m_k, IMPLICIT);

    const auto checked_add = [](size_t lhs, size_t rhs)
    {
        constexpr auto max_blocks = static_cast<size_t>(STATIC_ONLY);
        if (lhs >= max_blocks || rhs >= max_blocks - lhs)
            throw std::overflow_error("PartitionedAdjacencyLayout: adjacency block offset overflow");
        return lhs + rhs;
    };

    for (ygg::uint_t pi = 0; pi < m_k; ++pi)
    {
        auto row_blocks = size_t { 0 };
        for (ygg::uint_t pj = 0; pj < m_k; ++pj)
        {
            const auto& dependencies = dependency_graph.binary();
            if (!dependencies.has_dependency(pi, pj))
                continue;

            const auto has_runtime_dependency = dependencies.has_literal_dependency<::tyr::formalism::FluentTag, ::tyr::formalism::PositiveTag>(pi, pj)
                                                || dependencies.has_literal_dependency<::tyr::formalism::FluentTag, ::tyr::formalism::NegativeTag>(pi, pj)
                                                || dependencies.has_numeric_dependency(pi, pj);
            if (!has_runtime_dependency)
            {
                m_pair_offsets[pi * m_k + pj] = STATIC_ONLY;
                continue;
            }

            m_pair_offsets[pi * m_k + pj] = static_cast<ygg::uint_t>(row_blocks);
            row_blocks = checked_add(row_blocks, layout.info.infos[pj].num_blocks);
        }

        for (const auto v : layout.vertex_partitions[pi])
        {
            m_row_offsets[v] = static_cast<ygg::uint_t>(m_num_blocks);
            m_num_blocks = checked_add(m_num_blocks, row_blocks);
        }
    }
}

Workspace::Workspace(const GraphLayout& graph) :
    compatible_vertices_data(graph.k * graph.info.num_blocks, 0),
    compatible_vertices_span(compatible_vertices_data.data(), std::array<size_t, 2> { graph.k, graph.info.num_blocks }),
    partition_bits(graph.k, false),
    partial_solution(graph.k),
    partial_solution_size(0)
{
}

}
