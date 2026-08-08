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

#ifndef TYR_ALGORITHMS_KCKP_KCKP_HPP_
#define TYR_ALGORITHMS_KCKP_KCKP_HPP_

#include <algorithm>
#include <boost/dynamic_bitset.hpp>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <vector>
#include <yggdrasil/containers/dynamic_bitset.hpp>
#include <yggdrasil/core/types.hpp>

namespace tyr::kckp
{

struct Vertex
{
    ygg::uint_t index;

    constexpr Vertex() noexcept : index(std::numeric_limits<ygg::uint_t>::max()) {}
    constexpr explicit Vertex(ygg::uint_t index) noexcept : index(index) {}

    friend constexpr bool operator==(Vertex, Vertex) noexcept = default;
};

struct GraphLayout
{
    struct BitsetInfo
    {
        ygg::uint_t bit_offset;
        ygg::uint_t num_bits;
        ygg::uint_t block_offset;
        ygg::uint_t num_blocks;
    };

    struct PartitionInfo
    {
        std::vector<BitsetInfo> infos;
        size_t num_blocks { 0 };
    };

    size_t num_vertices { 0 };
    size_t num_partitions { 0 };
    std::vector<std::vector<ygg::uint_t>> vertex_partitions;
    std::vector<ygg::uint_t> vertex_to_partition;
    std::vector<ygg::uint_t> vertex_to_bit;
    PartitionInfo info;

    GraphLayout() = default;
    GraphLayout(size_t num_vertices, const std::vector<std::vector<ygg::uint_t>>& vertex_partitions);
};

class VertexPartitions
{
public:
    explicit VertexPartitions(const GraphLayout& layout) : m_layout(layout), m_data(layout.info.num_blocks, 0) {}

    friend bool operator==(const VertexPartitions& lhs, const VertexPartitions& rhs) noexcept
    {
        return lhs.m_layout.num_vertices == rhs.m_layout.num_vertices && lhs.m_layout.num_partitions == rhs.m_layout.num_partitions && lhs.m_data == rhs.m_data;
    }

    void reset() noexcept { std::fill(m_data.begin(), m_data.end(), uint64_t { 0 }); }

    ygg::BitsetSpan<uint64_t> get_bitset(const GraphLayout::BitsetInfo& info) noexcept;
    ygg::BitsetSpan<const uint64_t> get_bitset(const GraphLayout::BitsetInfo& info) const noexcept;
    ygg::BitsetSpan<uint64_t> get_bitset(ygg::uint_t partition) noexcept;
    ygg::BitsetSpan<const uint64_t> get_bitset(ygg::uint_t partition) const noexcept;

    auto& data() noexcept { return m_data; }
    const auto& data() const noexcept { return m_data; }
    const auto& layout() const noexcept { return m_layout; }

private:
    const GraphLayout& m_layout;
    std::vector<uint64_t> m_data;
};

class AdjacencyMatrix
{
public:
    explicit AdjacencyMatrix(const GraphLayout& layout) : m_layout(layout), m_bitset_data(layout.num_vertices * layout.info.num_blocks) {}

    std::span<const uint64_t> get_row(ygg::uint_t vertex) const noexcept;
    ygg::BitsetSpan<uint64_t> get_bitset(ygg::uint_t vertex, ygg::uint_t partition) noexcept;
    ygg::BitsetSpan<const uint64_t> get_bitset(ygg::uint_t vertex, ygg::uint_t partition) const noexcept;

    const auto& layout() const noexcept { return m_layout; }

private:
    GraphLayout m_layout;
    std::vector<uint64_t> m_bitset_data;
};

class DeduplicatedAdjacencyMatrix
{
public:
    DeduplicatedAdjacencyMatrix() = default;
    explicit DeduplicatedAdjacencyMatrix(const GraphLayout& layout) : m_layout(layout) {}
    explicit DeduplicatedAdjacencyMatrix(const AdjacencyMatrix& matrix);

    ygg::BitsetSpan<const uint64_t> get_bitset(ygg::uint_t vertex, ygg::uint_t partition) const noexcept;

    const auto& layout() const noexcept { return m_layout; }
    const auto& row_offset() const noexcept { return m_row_offset; }
    const auto& row_data() const noexcept { return m_row_data; }
    const auto& bitset_data() const noexcept { return m_bitset_data; }

private:
    GraphLayout m_layout;
    std::vector<ygg::uint_t> m_row_offset;
    std::vector<ygg::uint_t> m_row_data;
    std::vector<uint64_t> m_bitset_data;
};

class KCKP;

GraphLayout create_graph_layout(std::span<const size_t> partition_sizes);

class Graph
{
public:
    Graph();

    /// Constrained adjacency and universal-pair flags must both be symmetric.
    static Graph create(bool satisfiable, AdjacencyMatrix adjacency, boost::dynamic_bitset<> universal_partition_pairs);

    bool is_satisfiable() const noexcept { return m_satisfiable; }
    const GraphLayout& get_layout() const noexcept { return m_layout; }
    const DeduplicatedAdjacencyMatrix& get_adjacency_matrix() const noexcept { return m_adjacency; }
    Vertex get_original_vertex(Vertex vertex) const noexcept;

private:
    Graph(bool satisfiable, AdjacencyMatrix adjacency, boost::dynamic_bitset<> universal_partition_pairs);

    bool m_satisfiable { true };
    GraphLayout m_layout;
    std::vector<Vertex> m_original_vertices;
    DeduplicatedAdjacencyMatrix m_adjacency;
};

struct Workspace
{
    std::vector<uint64_t> compatible_vertices_data;
    size_t compatible_row_blocks { 0 };
    boost::dynamic_bitset<> partition_bits;
    std::vector<Vertex> partial_solution;
    ygg::uint_t partial_solution_size { 0 };
    ygg::uint_t anchor_pi { std::numeric_limits<ygg::uint_t>::max() };
    ygg::uint_t anchor_pj { std::numeric_limits<ygg::uint_t>::max() };

    Workspace() = default;
    explicit Workspace(const GraphLayout& layout) { prepare(layout); }

    void prepare(const GraphLayout& layout)
    {
        if (compatible_row_blocks == layout.info.num_blocks && partition_bits.size() == layout.num_partitions
            && partial_solution.size() == layout.num_partitions)
            return;

        compatible_row_blocks = layout.info.num_blocks;
        compatible_vertices_data.resize(layout.num_partitions * compatible_row_blocks);
        partition_bits.resize(layout.num_partitions);
        partial_solution.resize(layout.num_partitions);
    }

    std::span<uint64_t> compatible_vertices(size_t depth) noexcept;
    std::span<const uint64_t> compatible_vertices(size_t depth) const noexcept;
};

namespace detail
{
template<typename SelectPartition, typename UpdateCandidates, typename Callback>
void complete_clique(const GraphLayout& layout,
                     size_t depth,
                     Workspace& workspace,
                     SelectPartition& select_partition,
                     UpdateCandidates& update_candidates,
                     Callback& callback)
{
    const auto partition = select_partition(depth, workspace);
    if (partition == std::numeric_limits<ygg::uint_t>::max())
        return;

    const auto& info = layout.info.infos[partition];
    const auto row = workspace.compatible_vertices(depth);
    const auto candidates = ygg::BitsetSpan<const uint64_t>(row.data() + info.block_offset, info.num_bits);

    for (auto bit = candidates.find_first(); bit != ygg::BitsetSpan<const uint64_t>::npos; bit = candidates.find_next(bit))
    {
        const auto vertex = Vertex(info.bit_offset + bit);
        workspace.partial_solution[partition] = vertex;
        ++workspace.partial_solution_size;

        if (workspace.partial_solution_size == layout.num_partitions)
            callback(workspace.partial_solution);
        else
        {
            workspace.partition_bits.set(partition);
            if (update_candidates(vertex, depth, workspace))
                complete_clique(layout, depth + 1, workspace, select_partition, update_candidates, callback);
            workspace.partition_bits.reset(partition);
        }

        --workspace.partial_solution_size;
    }
}
}

class KCKP
{
public:
    explicit KCKP(const Graph& graph) : m_graph(graph) {}
    KCKP(Graph&&) = delete;

    template<typename Callback>
    void for_each_clique(Callback&& callback, Workspace& workspace) const
    {
        if (!m_graph.is_satisfiable() || !seed_from_prefix(std::span<const Vertex> {}, workspace))
            return;

        if (m_graph.get_layout().num_partitions == 0)
        {
            callback(workspace.partial_solution);
            return;
        }

        complete_from_prefix(0, workspace, callback);
    }

    template<typename Callback>
    void for_each_compatible_extension(std::span<const Vertex> prefix, Workspace& workspace, Callback&& callback) const
    {
        const auto& layout = m_graph.get_layout();
        assert(prefix.size() <= layout.num_partitions);
        if (!m_graph.is_satisfiable() || !seed_from_prefix(prefix, workspace))
            return;

        if (prefix.size() == layout.num_partitions)
        {
            callback(std::span<const Vertex> {});
            return;
        }

        const auto first_partition = static_cast<ygg::uint_t>(prefix.size());
        auto emit = [&](const std::vector<Vertex>& clique)
        { callback(std::span<const Vertex>(clique.data() + first_partition, layout.num_partitions - first_partition)); };
        complete_from_prefix(first_partition, workspace, emit);
    }

    const Graph& get_graph() const noexcept { return m_graph; }

private:
    bool seed_from_prefix(std::span<const Vertex> prefix, Workspace& workspace) const;

    ygg::uint_t choose_next_partition(ygg::uint_t first_partition, const Workspace& workspace) const noexcept;

    bool update_candidates(ygg::uint_t first_partition, Vertex source, size_t depth, Workspace& workspace) const;

    template<typename Callback>
    void complete_from_prefix(ygg::uint_t first_partition, Workspace& workspace, Callback&& callback) const
    {
        const auto& layout = m_graph.get_layout();
        auto select_partition = [&](size_t, const Workspace& current) { return choose_next_partition(first_partition, current); };
        auto update_compatible = [&](Vertex source, size_t depth, Workspace& current) { return update_candidates(first_partition, source, depth, current); };
        detail::complete_clique(layout, 0, workspace, select_partition, update_compatible, callback);
    }

    const Graph& m_graph;
};

inline ygg::BitsetSpan<uint64_t> VertexPartitions::get_bitset(const GraphLayout::BitsetInfo& info) noexcept
{
    return ygg::BitsetSpan<uint64_t>(m_data.data() + info.block_offset, info.num_bits);
}

inline ygg::BitsetSpan<const uint64_t> VertexPartitions::get_bitset(const GraphLayout::BitsetInfo& info) const noexcept
{
    return ygg::BitsetSpan<const uint64_t>(m_data.data() + info.block_offset, info.num_bits);
}

inline ygg::BitsetSpan<uint64_t> VertexPartitions::get_bitset(ygg::uint_t partition) noexcept { return get_bitset(m_layout.info.infos[partition]); }

inline ygg::BitsetSpan<const uint64_t> VertexPartitions::get_bitset(ygg::uint_t partition) const noexcept { return get_bitset(m_layout.info.infos[partition]); }

inline std::span<const uint64_t> AdjacencyMatrix::get_row(ygg::uint_t vertex) const noexcept
{
    const auto offset = m_layout.info.num_blocks * vertex;
    return std::span<const uint64_t>(m_bitset_data.data() + offset, m_layout.info.num_blocks);
}

inline ygg::BitsetSpan<uint64_t> AdjacencyMatrix::get_bitset(ygg::uint_t vertex, ygg::uint_t partition) noexcept
{
    const auto offset = m_layout.info.num_blocks * vertex;
    const auto& info = m_layout.info.infos[partition];
    return ygg::BitsetSpan<uint64_t>(m_bitset_data.data() + offset + info.block_offset, info.num_bits);
}

inline ygg::BitsetSpan<const uint64_t> AdjacencyMatrix::get_bitset(ygg::uint_t vertex, ygg::uint_t partition) const noexcept
{
    const auto offset = m_layout.info.num_blocks * vertex;
    const auto& info = m_layout.info.infos[partition];
    return ygg::BitsetSpan<const uint64_t>(m_bitset_data.data() + offset + info.block_offset, info.num_bits);
}

inline ygg::BitsetSpan<const uint64_t> DeduplicatedAdjacencyMatrix::get_bitset(ygg::uint_t vertex, ygg::uint_t partition) const noexcept
{
    const auto& info = m_layout.info.infos[partition];
    assert(vertex < m_row_offset.size());
    assert(m_row_offset[vertex] + partition < m_row_data.size());
    const auto start = m_row_data[m_row_offset[vertex] + partition];
    return ygg::BitsetSpan<const uint64_t>(m_bitset_data.data() + start, info.num_bits);
}

inline Vertex Graph::get_original_vertex(Vertex vertex) const noexcept { return m_original_vertices[vertex.index]; }

inline std::span<uint64_t> Workspace::compatible_vertices(size_t depth) noexcept
{
    return std::span<uint64_t>(compatible_vertices_data.data() + depth * compatible_row_blocks, compatible_row_blocks);
}

inline std::span<const uint64_t> Workspace::compatible_vertices(size_t depth) const noexcept
{
    return std::span<const uint64_t>(compatible_vertices_data.data() + depth * compatible_row_blocks, compatible_row_blocks);
}

}

#endif
