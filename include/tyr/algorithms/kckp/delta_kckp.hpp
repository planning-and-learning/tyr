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

#ifndef TYR_ALGORITHMS_KCKP_DELTA_KCKP_HPP_
#define TYR_ALGORITHMS_KCKP_DELTA_KCKP_HPP_

#include "tyr/algorithms/kckp/kckp.hpp"

#include <bit>
#include <boost/dynamic_bitset.hpp>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <type_traits>
#include <utility>
#include <vector>

namespace tyr::kckp
{

struct Edge
{
    Vertex src;
    Vertex dst;

    constexpr Edge() noexcept = default;
    constexpr Edge(Vertex lhs, Vertex rhs) noexcept : src(lhs.index < rhs.index ? lhs : rhs), dst(lhs.index < rhs.index ? rhs : lhs) {}

    friend constexpr bool operator==(Edge, Edge) noexcept = default;
};

enum class AdjacencyKind
{
    IMPLICIT,
    STATIC_ONLY,
    RUNTIME,
};

class PartitionedAdjacencyLayout
{
public:
    static constexpr ygg::uint_t IMPLICIT = std::numeric_limits<ygg::uint_t>::max();
    static constexpr ygg::uint_t STATIC_ONLY = IMPLICIT - 1;

    PartitionedAdjacencyLayout() = default;
    PartitionedAdjacencyLayout(const GraphLayout& layout, std::span<const AdjacencyKind> adjacency_kinds);

    friend bool operator==(const PartitionedAdjacencyLayout&, const PartitionedAdjacencyLayout&) noexcept = default;

    ygg::uint_t get_offset(ygg::uint_t vertex, ygg::uint_t source_partition, ygg::uint_t target_partition) const noexcept;

    bool is_implicit(ygg::uint_t source_partition, ygg::uint_t target_partition) const noexcept
    {
        return get_pair_offset(source_partition, target_partition) == IMPLICIT;
    }

    bool is_static_only(ygg::uint_t source_partition, ygg::uint_t target_partition) const noexcept
    {
        return get_pair_offset(source_partition, target_partition) == STATIC_ONLY;
    }

    bool is_runtime(ygg::uint_t source_partition, ygg::uint_t target_partition) const noexcept
    {
        return get_pair_offset(source_partition, target_partition) < STATIC_ONLY;
    }

    size_t num_blocks() const noexcept { return m_num_blocks; }

private:
    ygg::uint_t get_pair_offset(ygg::uint_t source_partition, ygg::uint_t target_partition) const noexcept;

    size_t m_num_partitions { 0 };
    std::vector<ygg::uint_t> m_row_offsets;
    std::vector<ygg::uint_t> m_pair_offsets;
    size_t m_num_blocks { 0 };
};

class PartitionedAdjacencyMatrix
{
public:
    PartitionedAdjacencyMatrix(const GraphLayout& layout,
                               const PartitionedAdjacencyLayout& adjacency_layout,
                               const DeduplicatedAdjacencyMatrix& static_matrix,
                               const VertexPartitions& affected_partitions,
                               const VertexPartitions& delta_partitions) :
        m_layout(layout),
        m_adjacency_layout(adjacency_layout),
        m_static_matrix(static_matrix),
        m_affected_partitions(affected_partitions),
        m_delta_partitions(delta_partitions),
        m_touched_partitions(m_layout.num_vertices * m_layout.num_partitions, false),
        m_bitset_data(adjacency_layout.num_blocks(), 0)
    {
    }

    ygg::BitsetSpan<uint64_t> get_runtime_bitset(ygg::uint_t vertex, ygg::uint_t partition) noexcept;
    ygg::BitsetSpan<const uint64_t> get_runtime_bitset(ygg::uint_t vertex, ygg::uint_t partition) const noexcept;

    void copy_adjacency_to(ygg::uint_t vertex, ygg::uint_t partition, ygg::BitsetSpan<uint64_t> destination) const
    {
        assert(destination.size() == m_layout.info.infos[partition].num_bits);
        for_each_adjacency_block(vertex, partition, [&](size_t block, uint64_t adjacency) { destination.blocks()[block] = adjacency; });
    }

    void intersect_adjacency_with(ygg::uint_t vertex, ygg::uint_t partition, ygg::BitsetSpan<uint64_t> destination) const
    {
        assert(destination.size() == m_layout.info.infos[partition].num_bits);
        for_each_adjacency_block(vertex, partition, [&](size_t block, uint64_t adjacency) { destination.blocks()[block] &= adjacency; });
    }

    void subtract_adjacency_from(ygg::uint_t vertex, ygg::uint_t partition, ygg::BitsetSpan<uint64_t> destination) const
    {
        assert(destination.size() == m_layout.info.infos[partition].num_bits);
        for_each_adjacency_block(vertex, partition, [&](size_t block, uint64_t adjacency) { destination.blocks()[block] &= ~adjacency; });
    }

    template<typename Callback>
    void for_each_vertex(Callback&& callback) const noexcept
    {
        for (ygg::uint_t partition = 0; partition < m_layout.num_partitions; ++partition)
        {
            const auto& info = m_layout.info.infos[partition];
            const auto vertices = m_affected_partitions.get_bitset(info);
            for (auto bit = vertices.find_first(); bit != ygg::BitsetSpan<const uint64_t>::npos; bit = vertices.find_next(bit))
                callback(Vertex(info.bit_offset + static_cast<ygg::uint_t>(bit)));
        }
    }

    template<typename Callback>
    void for_each_edge(Callback&& callback) const noexcept
    {
        for (ygg::uint_t source_partition = 0; source_partition < m_layout.num_partitions; ++source_partition)
        {
            const auto& source_info = m_layout.info.infos[source_partition];
            const auto source_bits = m_affected_partitions.get_bitset(source_info);

            for (auto source_bit = source_bits.find_first(); source_bit != ygg::BitsetSpan<const uint64_t>::npos;
                 source_bit = source_bits.find_next(source_bit))
            {
                const auto source_vertex = source_info.bit_offset + static_cast<ygg::uint_t>(source_bit);
                for (ygg::uint_t target_partition = source_partition + 1; target_partition < m_layout.num_partitions; ++target_partition)
                {
                    const auto& target_info = m_layout.info.infos[target_partition];
                    for_each_adjacent_vertex(source_vertex,
                                             target_partition,
                                             [&](size_t target_bit)
                                             { callback(Edge(Vertex(source_vertex), Vertex(target_info.bit_offset + static_cast<ygg::uint_t>(target_bit)))); });
                }
            }
        }
    }

    void reset() noexcept
    {
        for (auto touched = m_touched_partitions.find_first(); touched != boost::dynamic_bitset<>::npos; touched = m_touched_partitions.find_next(touched))
        {
            const auto vertex = touched / m_layout.num_partitions;
            const auto partition = touched % m_layout.num_partitions;
            get_runtime_bitset(vertex, partition).reset();
        }
        m_touched_partitions.reset();
    }

    const auto& layout() const noexcept { return m_layout; }
    const auto& affected_partitions() const noexcept { return m_affected_partitions; }
    const auto& delta_partitions() const noexcept { return m_delta_partitions; }
    auto& touched_partitions() noexcept { return m_touched_partitions; }
    const auto& touched_partitions() const noexcept { return m_touched_partitions; }
    auto touched_partitions(ygg::uint_t vertex, ygg::uint_t partition) noexcept { return m_touched_partitions[vertex * m_layout.num_partitions + partition]; }
    const auto& bitset_data() const noexcept { return m_bitset_data; }

private:
    ygg::BitsetSpan<const uint64_t> get_contextual_partition(ygg::uint_t vertex, ygg::uint_t partition) const noexcept;

    template<typename Callback>
    void for_each_adjacency_block(ygg::uint_t vertex, ygg::uint_t partition, Callback&& callback) const
    {
        const auto source_partition = m_layout.vertex_to_partition[vertex];
        const auto offset = m_adjacency_layout.get_offset(vertex, source_partition, partition);

        if (offset < PartitionedAdjacencyLayout::STATIC_ONLY)
        {
            const auto& info = m_layout.info.infos[partition];
            const auto blocks = std::span<const uint64_t>(m_bitset_data.data() + offset, info.num_blocks);
            for (size_t block = 0; block < blocks.size(); ++block)
                callback(block, blocks[block]);
            return;
        }

        const auto partition_blocks = get_contextual_partition(vertex, partition).blocks();
        if (offset == PartitionedAdjacencyLayout::IMPLICIT)
        {
            for (size_t block = 0; block < partition_blocks.size(); ++block)
                callback(block, partition_blocks[block]);
            return;
        }

        assert(offset == PartitionedAdjacencyLayout::STATIC_ONLY);
        const auto static_blocks = m_static_matrix.get_bitset(vertex, partition).blocks();
        assert(static_blocks.size() == partition_blocks.size());
        for (size_t block = 0; block < partition_blocks.size(); ++block)
            callback(block, static_blocks[block] & partition_blocks[block]);
    }

    template<typename Callback>
    void for_each_adjacent_vertex(ygg::uint_t vertex, ygg::uint_t partition, Callback&& callback) const
    {
        const auto active_blocks = m_affected_partitions.get_bitset(partition).blocks();
        const auto num_bits = m_layout.info.infos[partition].num_bits;
        for_each_adjacency_block(vertex,
                                 partition,
                                 [&](size_t block, uint64_t adjacency)
                                 {
                                     auto word = adjacency & active_blocks[block];
                                     while (word)
                                     {
                                         const auto bit = static_cast<size_t>(std::countr_zero(word));
                                         const auto index = block * ygg::BitsetSpan<const uint64_t>::Digits + bit;
                                         if (index < num_bits)
                                             callback(index);
                                         word &= word - 1;
                                     }
                                 });
    }

    const GraphLayout& m_layout;
    const PartitionedAdjacencyLayout& m_adjacency_layout;
    const DeduplicatedAdjacencyMatrix& m_static_matrix;
    const VertexPartitions& m_affected_partitions;
    const VertexPartitions& m_delta_partitions;
    boost::dynamic_bitset<> m_touched_partitions;
    std::vector<uint64_t> m_bitset_data;
};

struct FullGraph
{
    FullGraph(const GraphLayout& layout, const PartitionedAdjacencyLayout& adjacency_layout, const DeduplicatedAdjacencyMatrix& static_matrix) :
        affected_partitions(layout),
        matrix(layout, adjacency_layout, static_matrix, affected_partitions, affected_partitions)
    {
    }

    FullGraph(const FullGraph&) = delete;
    FullGraph& operator=(const FullGraph&) = delete;
    FullGraph(FullGraph&&) = delete;
    FullGraph& operator=(FullGraph&&) = delete;

    void reset() noexcept
    {
        affected_partitions.reset();
        matrix.reset();
    }

    VertexPartitions affected_partitions;
    PartitionedAdjacencyMatrix matrix;
};

struct DeltaGraph
{
    DeltaGraph(const GraphLayout& layout, const PartitionedAdjacencyLayout& adjacency_layout, const DeduplicatedAdjacencyMatrix& static_matrix) :
        affected_partitions(layout),
        delta_partitions(layout),
        matrix(layout, adjacency_layout, static_matrix, affected_partitions, delta_partitions)
    {
    }

    DeltaGraph(const DeltaGraph&) = delete;
    DeltaGraph& operator=(const DeltaGraph&) = delete;
    DeltaGraph(DeltaGraph&&) = delete;
    DeltaGraph& operator=(DeltaGraph&&) = delete;

    void reset() noexcept
    {
        affected_partitions.reset();
        delta_partitions.reset();
        matrix.reset();
    }

    VertexPartitions affected_partitions;
    VertexPartitions delta_partitions;
    PartitionedAdjacencyMatrix matrix;
};

class DeltaKCKP
{
public:
    DeltaKCKP(const Graph& graph, const PartitionedAdjacencyLayout& adjacency_layout);
    DeltaKCKP(Graph&&, const PartitionedAdjacencyLayout&) = delete;
    DeltaKCKP(const Graph&, PartitionedAdjacencyLayout&&) = delete;
    DeltaKCKP(Graph&&, PartitionedAdjacencyLayout&&) = delete;
    DeltaKCKP(const DeltaKCKP&) = delete;
    DeltaKCKP& operator=(const DeltaKCKP&) = delete;
    DeltaKCKP(DeltaKCKP&&) = delete;
    DeltaKCKP& operator=(DeltaKCKP&&) = delete;

    template<typename Initializer>
    void update(Initializer&& initializer)
    {
        m_delta_graph.reset();
        std::forward<Initializer>(initializer)(m_delta_graph, m_full_graph);
        m_delta_edges.clear();
        ++m_iteration;
    }

    void reset();

    template<typename Callback>
    void for_each_clique(Callback&& callback, Workspace& workspace) const
    {
        workspace.prepare(m_layout);
        const auto num_partitions = m_layout.num_partitions;
        if (num_partitions == 0)
        {
            workspace.partial_solution_size = 0;
            callback(workspace.partial_solution);
        }
        else if (num_partitions == 1)
        {
            m_full_graph.matrix.for_each_vertex(
                [&](Vertex vertex)
                {
                    workspace.partial_solution[0] = vertex;
                    workspace.partial_solution_size = 1;
                    callback(workspace.partial_solution);
                });
        }
        else if (num_partitions == 2)
        {
            m_full_graph.matrix.for_each_edge(
                [&](Edge edge)
                {
                    workspace.partial_solution[0] = edge.src;
                    workspace.partial_solution[1] = edge.dst;
                    workspace.partial_solution_size = 2;
                    callback(workspace.partial_solution);
                });
        }
        else
        {
            seed_without_anchor(workspace);
            complete_from_seed<void>(std::forward<Callback>(callback), 0, workspace);
        }
    }

    template<typename Callback>
    void for_each_delta_clique(Callback&& callback, Workspace& workspace) const
    {
        workspace.prepare(m_layout);
        if (m_iteration == 1)
        {
            for_each_clique(std::forward<Callback>(callback), workspace);
            return;
        }

        const auto num_partitions = m_layout.num_partitions;
        if (num_partitions == 0)
            return;
        if (num_partitions == 1)
        {
            m_delta_graph.matrix.for_each_vertex(
                [&](Vertex vertex)
                {
                    workspace.partial_solution[0] = vertex;
                    workspace.partial_solution_size = 1;
                    callback(workspace.partial_solution);
                });
            return;
        }
        if (num_partitions == 2)
        {
            m_delta_graph.matrix.for_each_edge(
                [&](Edge edge)
                {
                    workspace.partial_solution[0] = edge.src;
                    workspace.partial_solution[1] = edge.dst;
                    workspace.partial_solution_size = 2;
                    callback(workspace.partial_solution);
                });
            return;
        }

        m_delta_graph.matrix.for_each_edge(
            [&](Edge edge)
            {
                if (seed_from_anchor(edge, workspace))
                    complete_from_seed<Edge>(callback, 0, workspace);
            });
    }

    const std::vector<Edge>& materialize_delta_edges();
    void seed_without_anchor(Workspace& workspace) const;
    bool seed_from_anchor(const Edge& edge, Workspace& workspace) const;

    template<typename Anchor, typename Callback>
    void complete_from_seed(Callback&& callback, size_t depth, Workspace& workspace) const
    {
        assert(depth < m_layout.num_partitions);
        auto select_partition = [&](size_t current_depth, const Workspace& current) { return choose_best_partition(current_depth, current); };
        auto update_candidates = [&](Vertex source, size_t current_depth, Workspace& current)
        { return update_candidates_for<Anchor>(source, current_depth, current); };
        detail::complete_clique(m_layout, depth, workspace, select_partition, update_candidates, callback);
    }

    const GraphLayout& get_graph_layout() const noexcept { return m_layout; }
    const DeltaGraph& get_delta_graph() const noexcept { return m_delta_graph; }
    const FullGraph& get_full_graph() const noexcept { return m_full_graph; }
    size_t get_iteration() const noexcept { return m_iteration; }

private:
    ygg::uint_t choose_best_partition(size_t depth, const Workspace& workspace) const;

    template<typename Anchor>
    bool update_candidates_for(Vertex source, size_t depth, Workspace& workspace) const
    {
        const auto source_partition = m_layout.vertex_to_partition[source.index];
        assert(source_partition != workspace.anchor_pi);
        assert(source_partition != workspace.anchor_pj);

        const auto current_row = workspace.compatible_vertices(depth);
        auto next_row = workspace.compatible_vertices(depth + 1);
        for (ygg::uint_t partition = 0; partition < m_layout.num_partitions; ++partition)
        {
            if (workspace.partition_bits.test(partition))
                continue;

            const auto& info = m_layout.info.infos[partition];
            const auto current = ygg::BitsetSpan<const uint64_t>(current_row.data() + info.block_offset, info.num_bits);
            auto next = ygg::BitsetSpan<uint64_t>(next_row.data() + info.block_offset, info.num_bits);
            next.copy_from(current);
            m_full_graph.matrix.intersect_adjacency_with(source.index, partition, next);
            if (!next.any())
                return false;

            if constexpr (std::is_same_v<Anchor, Edge>)
            {
                if (source_partition < workspace.anchor_pi || partition < workspace.anchor_pi)
                {
                    m_delta_graph.matrix.subtract_adjacency_from(source.index, partition, next);
                    if (!next.any())
                        return false;
                }
            }
        }
        return true;
    }

    const GraphLayout& m_layout;
    size_t m_iteration { 0 };
    DeltaGraph m_delta_graph;
    FullGraph m_full_graph;
    std::vector<Edge> m_delta_edges;
};

inline ygg::uint_t PartitionedAdjacencyLayout::get_offset(ygg::uint_t vertex, ygg::uint_t source_partition, ygg::uint_t target_partition) const noexcept
{
    assert(vertex < m_row_offsets.size());
    assert(source_partition < m_num_partitions);
    assert(target_partition < m_num_partitions);

    const auto pair_offset = m_pair_offsets[source_partition * m_num_partitions + target_partition];
    return pair_offset >= STATIC_ONLY ? pair_offset : m_row_offsets[vertex] + pair_offset;
}

inline ygg::uint_t PartitionedAdjacencyLayout::get_pair_offset(ygg::uint_t source_partition, ygg::uint_t target_partition) const noexcept
{
    assert(source_partition < m_num_partitions);
    assert(target_partition < m_num_partitions);
    return m_pair_offsets[source_partition * m_num_partitions + target_partition];
}

inline ygg::BitsetSpan<uint64_t> PartitionedAdjacencyMatrix::get_runtime_bitset(ygg::uint_t vertex, ygg::uint_t partition) noexcept
{
    const auto offset = m_adjacency_layout.get_offset(vertex, m_layout.vertex_to_partition[vertex], partition);
    assert(offset < PartitionedAdjacencyLayout::STATIC_ONLY && "Only runtime-dependent adjacency is mutable");
    const auto& info = m_layout.info.infos[partition];
    return ygg::BitsetSpan<uint64_t>(m_bitset_data.data() + offset, info.num_bits);
}

inline ygg::BitsetSpan<const uint64_t> PartitionedAdjacencyMatrix::get_runtime_bitset(ygg::uint_t vertex, ygg::uint_t partition) const noexcept
{
    const auto& info = m_layout.info.infos[partition];
    const auto source_partition = m_layout.vertex_to_partition[vertex];
    const auto offset = m_adjacency_layout.get_offset(vertex, source_partition, partition);
    assert(offset < PartitionedAdjacencyLayout::STATIC_ONLY && "Only runtime-dependent adjacency is stored");
    return ygg::BitsetSpan<const uint64_t>(m_bitset_data.data() + offset, info.num_bits);
}

inline ygg::BitsetSpan<const uint64_t> PartitionedAdjacencyMatrix::get_contextual_partition(ygg::uint_t vertex, ygg::uint_t partition) const noexcept
{
    const auto source_partition = m_layout.vertex_to_partition[vertex];
    const auto source_bit = m_layout.vertex_to_bit[vertex];
    const auto source_is_delta = m_delta_partitions.get_bitset(m_layout.info.infos[source_partition]).test(source_bit);
    return source_is_delta ? m_affected_partitions.get_bitset(partition) : m_delta_partitions.get_bitset(partition);
}

}

#endif
