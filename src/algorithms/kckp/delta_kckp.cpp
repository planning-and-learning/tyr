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
#include <urd/graphs.hpp>
#include <urd/sets.hpp>
#include <urd/store.hpp>

namespace tyr::kckp
{

PartitionedAdjacencyLayout::PartitionedAdjacencyLayout(const Graph& graph, std::span<const AdjacencyKind> adjacency_kinds) :
    m_num_partitions(graph.num_parts()),
    m_kinds(adjacency_kinds.begin(), adjacency_kinds.end()),
    m_pair_rows(adjacency_kinds.size(), 0),
    m_first_rows(graph.num_vertices(), 0)
{
    if (adjacency_kinds.size() != m_num_partitions * m_num_partitions)
        throw std::invalid_argument("PartitionedAdjacencyLayout: expected one adjacency kind per partition pair");

    auto next_row = size_t { 0 };
    for (size_t source_partition = 0; source_partition < m_num_partitions; ++source_partition)
    {
        auto rows_per_vertex = size_t { 0 };
        for (size_t target_partition = 0; target_partition < m_num_partitions; ++target_partition)
        {
            const auto pair = (source_partition * m_num_partitions) + target_partition;
            switch (m_kinds[pair])
            {
                case AdjacencyKind::IMPLICIT:
                case AdjacencyKind::STATIC_ONLY:
                    break;
                case AdjacencyKind::RUNTIME:
                    m_pair_rows[pair] = rows_per_vertex++;
                    break;
                default:
                    throw std::invalid_argument("PartitionedAdjacencyLayout: invalid adjacency kind");
            }
        }
        for (const auto vertex : graph.part(source_partition))
        {
            m_first_rows[vertex.index] = next_row;
            next_row += rows_per_vertex;
        }
    }
}

PartitionedAdjacencyMatrix::PartitionedAdjacencyMatrix(const Graph& graph,
                                                       const PartitionedAdjacencyLayout& adjacency_layout,
                                                       const VertexSets& affected,
                                                       const VertexSets& delta) :
    m_graph(&graph),
    m_adjacency_layout(&adjacency_layout),
    m_affected(&affected),
    m_delta(&delta)
{
    // The rows in the layout's order: vertex after vertex, each into its runtime parts in order.
    const auto parts = graph.empty_parts();
    for (size_t source_partition = 0; source_partition < graph.num_parts(); ++source_partition)
        for ([[maybe_unused]] const auto vertex : graph.part(source_partition))
            for (size_t target_partition = 0; target_partition < graph.num_parts(); ++target_partition)
                if (adjacency_layout.is_runtime(source_partition, target_partition))
                {
                    assert(adjacency_layout.row(vertex, source_partition, target_partition) == m_rows.size());
                    m_rows.push_back(parts[target_partition]);
                }
}

void PartitionedAdjacencyMatrix::add_edge(Vertex u, Vertex v)
{
    const auto u_partition = m_graph->part_of(u);
    const auto v_partition = m_graph->part_of(v);
    const auto u_row = m_adjacency_layout->row(u, u_partition, v_partition);
    const auto v_row = m_adjacency_layout->row(v, v_partition, u_partition);
    m_rows[u_row].insert(v);
    m_rows[v_row].insert(u);
    m_touched.insert(u_row);
    m_touched.insert(v_row);
}

void PartitionedAdjacencyMatrix::reset() noexcept
{
    for (const auto row : m_touched)
        m_rows[row].clear();
    m_touched.clear();
}

void PartitionedAdjacencyMatrix::copy_adjacency_to(Vertex vertex, size_t partition, VertexSet& destination) const
{
    switch (m_adjacency_layout->kind(m_graph->part_of(vertex), partition))
    {
        case AdjacencyKind::RUNTIME:
            destination = runtime_neighbours(vertex, partition);
            return;
        case AdjacencyKind::IMPLICIT:
            destination = context(vertex, partition);
            return;
        case AdjacencyKind::STATIC_ONLY:
            destination = context(vertex, partition);
            urd::intersect_neighbours(m_graph->rows(), vertex, partition, destination);
            return;
    }
}

void PartitionedAdjacencyMatrix::intersect_adjacency(Vertex vertex, size_t partition, const VertexSet& source, VertexSet& destination) const
{
    switch (m_adjacency_layout->kind(m_graph->part_of(vertex), partition))
    {
        case AdjacencyKind::RUNTIME:
            urd::set_intersection(source, runtime_neighbours(vertex, partition), destination);
            return;
        case AdjacencyKind::IMPLICIT:
            urd::set_intersection(source, context(vertex, partition), destination);
            return;
        case AdjacencyKind::STATIC_ONLY:
            urd::intersect_neighbours(m_graph->rows(), vertex, partition, source, destination);
            destination &= context(vertex, partition);
            return;
    }
}

void PartitionedAdjacencyMatrix::subtract_adjacency_from(Vertex vertex, size_t partition, VertexSet& destination, VertexSet& scratch) const
{
    switch (m_adjacency_layout->kind(m_graph->part_of(vertex), partition))
    {
        case AdjacencyKind::RUNTIME:
            destination -= runtime_neighbours(vertex, partition);
            return;
        case AdjacencyKind::IMPLICIT:
            destination -= context(vertex, partition);
            return;
        case AdjacencyKind::STATIC_ONLY:
            scratch = context(vertex, partition);
            urd::intersect_neighbours(m_graph->rows(), vertex, partition, scratch);
            destination -= scratch;
            return;
    }
}

bool PartitionedAdjacencyMatrix::is_adjacent(Vertex vertex, Vertex target) const
{
    const auto target_partition = m_graph->part_of(target);
    switch (m_adjacency_layout->kind(m_graph->part_of(vertex), target_partition))
    {
        case AdjacencyKind::RUNTIME:
            return runtime_neighbours(vertex, target_partition).contains(target);
        case AdjacencyKind::IMPLICIT:
            return context(vertex, target_partition).contains(target);
        case AdjacencyKind::STATIC_ONLY:
            return context(vertex, target_partition).contains(target) && m_graph->adjacent(vertex, target);
    }
    return false;
}

DeltaKCKP::DeltaKCKP(const Graph& graph, const PartitionedAdjacencyLayout& adjacency_layout) :
    m_graph(&graph),
    m_delta_graph(graph, adjacency_layout),
    m_full_graph(graph, adjacency_layout)
{
}

void DeltaKCKP::reset()
{
    m_delta_graph.reset();
    m_full_graph.reset();
    m_delta_edges.clear();
    m_iteration = 0;
}

const urd::Store<urd::sequence<Edge>>& DeltaKCKP::materialize_delta_edges()
{
    m_delta_edges.clear();
    m_delta_graph.matrix.for_each_edge([&](Edge edge) { m_delta_edges.push_back(edge); });
    return m_delta_edges;
}

void DeltaKCKP::seed_without_anchor(Workspace& workspace) const
{
    workspace.prepare(*m_graph);
    workspace.partial_solution_size = 0;
    workspace.partition_bits.clear();
    workspace.anchor_pi = std::numeric_limits<ygg::uint_t>::max();
    workspace.anchor_pj = std::numeric_limits<ygg::uint_t>::max();

    for (size_t partition = 0; partition < m_graph->num_parts(); ++partition)
        workspace.candidates_at(0, partition) = m_full_graph.affected_partitions[partition];
}

bool DeltaKCKP::seed_from_anchor(const Edge& edge, Workspace& workspace) const
{ return prepare_anchor_source(edge.src, m_graph->part_of(edge.dst), workspace) && seed_from_prepared_anchor(edge, workspace); }

bool DeltaKCKP::prepare_anchor_source(Vertex source, ygg::uint_t target_partition, Workspace& workspace) const
{
    workspace.prepare(*m_graph);
    const auto source_partition = m_graph->part_of(source);
    for (ygg::uint_t partition = 0; partition < m_graph->num_parts(); ++partition)
    {
        if (partition == source_partition || partition == target_partition)
            continue;

        auto& row = workspace.anchor_source[partition];
        m_full_graph.matrix.copy_adjacency_to(source, partition, row);
        if (partition < target_partition)
            m_delta_graph.matrix.subtract_adjacency_from(source, partition, row, workspace.scratch[partition]);
        if (row.empty())
            return false;
    }
    return true;
}

bool DeltaKCKP::seed_from_prepared_anchor(const Edge& edge, Workspace& workspace) const
{
    const auto source_partition = m_graph->part_of(edge.src);
    const auto target_partition = m_graph->part_of(edge.dst);
    assert(source_partition < target_partition);

    workspace.partial_solution[source_partition] = edge.src;
    workspace.partial_solution[target_partition] = edge.dst;
    workspace.partial_solution_size = 2;
    workspace.anchor_pi = source_partition;
    workspace.anchor_pj = target_partition;
    workspace.partition_bits.clear();
    workspace.partition_bits.insert(source_partition);
    workspace.partition_bits.insert(target_partition);

    for (ygg::uint_t partition = 0; partition < m_graph->num_parts(); ++partition)
    {
        if (partition == source_partition || partition == target_partition)
            continue;

        auto& compatible = workspace.candidates_at(0, partition);
        m_full_graph.matrix.intersect_adjacency(edge.dst, partition, workspace.anchor_source[partition], compatible);
        if (partition < source_partition)
            m_delta_graph.matrix.subtract_adjacency_from(edge.dst, partition, compatible, workspace.scratch[partition]);
        if (compatible.empty())
            return false;
    }
    return true;
}

bool DeltaKCKP::seed_from_vertices(std::span<const Vertex> vertices, Workspace& workspace) const
{
    assert(!vertices.empty());
    workspace.prepare(*m_graph);
    workspace.partial_solution_size = 0;
    workspace.partition_bits.clear();
    workspace.anchor_pi = std::numeric_limits<ygg::uint_t>::max();
    workspace.anchor_pj = std::numeric_limits<ygg::uint_t>::max();

    for (size_t i = 0; i < vertices.size(); ++i)
    {
        const auto vertex = vertices[i];
        const auto partition = m_graph->part_of(vertex);
        assert(!workspace.partition_bits.contains(partition));

        if (!m_full_graph.affected_partitions[partition].contains(vertex) || m_delta_graph.delta_partitions[partition].contains(vertex))
            return false;

        for (size_t j = 0; j < i; ++j)
            if (!m_full_graph.matrix.is_adjacent(vertices[j], vertex) || m_delta_graph.matrix.is_adjacent(vertices[j], vertex))
                return false;

        workspace.partial_solution[partition] = vertex;
        workspace.partition_bits.insert(partition);
        ++workspace.partial_solution_size;
    }

    for (ygg::uint_t partition = 0; partition < m_graph->num_parts(); ++partition)
    {
        if (workspace.partition_bits.contains(partition))
            continue;

        auto& compatible = workspace.candidates_at(0, partition);
        m_full_graph.matrix.copy_adjacency_to(vertices.front(), partition, compatible);
        for (const auto vertex : vertices.subspan(1))
            m_full_graph.matrix.intersect_adjacency(vertex, partition, compatible, compatible);
        for (const auto vertex : vertices)
            m_delta_graph.matrix.subtract_adjacency_from(vertex, partition, compatible, workspace.scratch[partition]);
        if (compatible.empty())
            return false;
    }
    return true;
}

ygg::uint_t DeltaKCKP::choose_best_partition(size_t depth, const Workspace& workspace) const
{
    auto best_partition = std::numeric_limits<ygg::uint_t>::max();
    auto best_size = std::numeric_limits<size_t>::max();
    for (ygg::uint_t partition = 0; partition < m_graph->num_parts(); ++partition)
    {
        if (workspace.partition_bits.contains(partition))
            continue;

        const auto size = workspace.candidates_at(depth, partition).size();
        if (size < best_size)
        {
            best_size = size;
            best_partition = partition;
        }
    }
    return best_partition;
}

}
