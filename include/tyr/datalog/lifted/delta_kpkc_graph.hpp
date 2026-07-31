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

#ifndef TYR_DATALOG_DELTA_KPKC_GRAPH_HPP_
#define TYR_DATALOG_DELTA_KPKC_GRAPH_HPP_

#include "tyr/datalog/declarations.hpp"
#include "tyr/formalism/datalog/variable_dependency_graph.hpp"

#include <boost/dynamic_bitset.hpp>
#include <cstddef>
#include <iostream>
#include <vector>
#include <yggdrasil/containers/associative_containers.hpp>
#include <yggdrasil/containers/dynamic_bitset.hpp>
#include <yggdrasil/containers/vector.hpp>
#include <yggdrasil/formatting/cista_formatters.hpp>
#include <yggdrasil/semantics/containers/dynamic_bitset_equal_to.hpp>
#include <yggdrasil/semantics/containers/dynamic_bitset_hash.hpp>
#include <yggdrasil/semantics/equal_to.hpp>
#include <yggdrasil/semantics/hash.hpp>

namespace tyr::datalog::kpkc
{
struct Vertex
{
    ygg::uint_t index;

    constexpr Vertex() noexcept : index(std::numeric_limits<ygg::uint_t>::max()) {}
    constexpr explicit Vertex(ygg::uint_t i) noexcept : index(i) {}

    friend constexpr bool operator==(Vertex lhs, Vertex rhs) noexcept { return lhs.index == rhs.index; }
};

struct Edge
{
    Vertex src;
    Vertex dst;

    constexpr Edge() noexcept : src(), dst() {}
    constexpr Edge(Vertex u, Vertex v) noexcept : src(u.index < v.index ? u : v), dst(u.index < v.index ? v : u) {}

    friend constexpr bool operator==(Edge lhs, Edge rhs) noexcept { return lhs.src == rhs.src && lhs.dst == rhs.dst; }

    /// @brief Get the rank relative to a given number of vertices.
    /// @param nv is the total number of vertices.
    /// @return is the rank of the edge.
    ygg::uint_t rank(ygg::uint_t nv) const noexcept { return src.index * nv + dst.index; }
};

struct GraphLayout
{
    /// Meta
    size_t nv;
    size_t k;

    /// Vertex partitioning
    std::vector<std::vector<ygg::uint_t>> vertex_partitions;
    std::vector<ygg::uint_t> vertex_to_partition;
    std::vector<ygg::uint_t> vertex_to_bit;

    struct BitsetInfo
    {
        ygg::uint_t bit_offset;  // bit offset ignoring unused bits
        ygg::uint_t num_bits;

        ygg::uint_t block_offset;
        ygg::uint_t num_blocks;
    };

    struct PartitionInfo
    {
        std::vector<BitsetInfo> infos;
        size_t num_blocks;
    };

    PartitionInfo info;

    GraphLayout() = default;
    GraphLayout(size_t nv, const std::vector<std::vector<ygg::uint_t>>& vertex_partitions);
};

class VertexPartitions
{
public:
    explicit VertexPartitions(const GraphLayout& layout) : m_layout(layout), m_data(layout.info.num_blocks, 0) {}

    friend bool operator==(const VertexPartitions& lhs, const VertexPartitions& rhs) noexcept
    {
        return lhs.m_layout.nv == rhs.m_layout.nv && lhs.m_layout.k == rhs.m_layout.k && lhs.m_data == rhs.m_data;
    }

    void reset() noexcept { std::memset(m_data.data(), 0, m_data.size() * sizeof(uint64_t)); }

    auto get_bitset(const GraphLayout::BitsetInfo& info) noexcept { return ygg::BitsetSpan<uint64_t>(m_data.data() + info.block_offset, info.num_bits); }
    auto get_bitset(const GraphLayout::BitsetInfo& info) const noexcept
    {
        return ygg::BitsetSpan<const uint64_t>(m_data.data() + info.block_offset, info.num_bits);
    }
    auto get_bitset(ygg::uint_t p) noexcept { return get_bitset(m_layout.info.infos[p]); }
    auto get_bitset(ygg::uint_t p) const noexcept { return get_bitset(m_layout.info.infos[p]); }

    auto& data() noexcept { return m_data; }
    const auto& data() const noexcept { return m_data; }
    const auto& layout() const noexcept { return m_layout; }

private:
    const GraphLayout& m_layout;

    /// Implicit storage: the active vertices in the partition
    std::vector<uint64_t> m_data;
};

class AdjacencyMatrix
{
public:
    AdjacencyMatrix(const GraphLayout& layout) : m_layout(layout), m_bitset_data(layout.nv * layout.k * layout.info.num_blocks) {}

    auto get_row(ygg::uint_t v) const noexcept
    {
        const auto& info = m_layout.info;
        const auto row_offset = info.num_blocks * v;
        return std::span<const uint64_t>(m_bitset_data.data() + row_offset, info.num_blocks);
    }

    auto get_bitset(ygg::uint_t v, ygg::uint_t p) noexcept
    {
        const auto row_offset = m_layout.info.num_blocks * v;
        const auto& info = m_layout.info.infos[p];
        return ygg::BitsetSpan<uint64_t>(m_bitset_data.data() + row_offset + info.block_offset, info.num_bits);
    }
    auto get_bitset(ygg::uint_t v, ygg::uint_t p) const noexcept
    {
        const auto row_offset = m_layout.info.num_blocks * v;
        const auto& info = m_layout.info.infos[p];
        return ygg::BitsetSpan<const uint64_t>(m_bitset_data.data() + row_offset + info.block_offset, info.num_bits);
    }

    const auto& layout() const noexcept { return m_layout; }

    const auto& bitset_data() const noexcept { return m_bitset_data; }

private:
    GraphLayout m_layout;

    std::vector<uint64_t> m_bitset_data;
};

class DeduplicatedAdjacencyMatrix
{
public:
    DeduplicatedAdjacencyMatrix(const GraphLayout& layout) : m_layout(layout), m_row_offset(), m_row_data(), m_bitset_data() {}

    DeduplicatedAdjacencyMatrix(const AdjacencyMatrix& m) : m_layout(m.layout())
    {
        auto row_to_offset = ygg::UnorderedMap<std::span<const uint64_t>, ygg::uint_t> {};
        auto partition_to_offset = ygg::UnorderedMap<ygg::BitsetSpan<const uint64_t>, ygg::uint_t> {};

        for (ygg::uint_t v = 0; v < m.layout().nv; ++v)
        {
            const auto [it1, success1] = row_to_offset.emplace(m.get_row(v), m_row_data.size());

            if (!success1)
            {
                m_row_offset.push_back(it1->second);
                continue;  ///< succeeded row deduplication
            }
            m_row_offset.push_back(it1->second);  /// Build new row

            for (ygg::uint_t k = 0; k < m.layout().k; ++k)
            {
                const auto b = m.get_bitset(v, k);
                const auto [it2, success2] = partition_to_offset.emplace(b, m_bitset_data.size());

                if (!success2)
                {
                    m_row_data.push_back(it2->second);
                    continue;  ///< succeeded partition deduplication
                }

                m_row_data.push_back(it2->second);  /// Build new bitset

                /// New bitset encountered
                m_bitset_data.insert(m_bitset_data.end(), b.blocks().begin(), b.blocks().end());
            }
        }
    }

    auto get_bitset(ygg::uint_t v, ygg::uint_t p) const noexcept
    {
        const auto& info = m_layout.info.infos[p];

        assert(v < m_row_offset.size());
        assert(m_row_offset[v] + p < m_row_data.size());
        const auto start = m_row_data[m_row_offset[v] + p];

        return ygg::BitsetSpan<const uint64_t>(m_bitset_data.data() + start, info.num_bits);
    }

    const auto& layout() const noexcept { return m_layout; }
    const auto& row_offset() const noexcept { return m_row_offset; }
    const auto& row_data() const noexcept { return m_row_data; }
    const auto& bitset_data() const noexcept { return m_bitset_data; }

private:
    GraphLayout m_layout;

    std::vector<ygg::uint_t> m_row_offset;  ///< m_row_offset[v] is the offset into m_row_data
    std::vector<ygg::uint_t> m_row_data;    ///< m_row_data[m_row_offset[v] + p] is the offset into m_bitset_data.
    std::vector<uint64_t> m_bitset_data;    ///< m_bitset_data.data() + m_row_data[m_row_offset[v] + p] is beginning of the bitset data for the set of vertices
                                            ///< from v into partition p.
};

class PartitionedAdjacencyMatrix
{
private:
    static constexpr ygg::uint_t UNUSED = std::numeric_limits<ygg::uint_t>::max();

public:
    PartitionedAdjacencyMatrix(const GraphLayout& layout,
                               const VertexPartitions& affected_partitions,
                               const VertexPartitions& delta_partitions,
                               const ::tyr::formalism::datalog::VariableDependencyGraph& dependency_graph) :
        m_layout(layout),
        m_affected_partitions(affected_partitions),
        m_delta_partitions(delta_partitions),
        m_dependency_graph(dependency_graph),
        m_adj_data(m_layout.nv * m_layout.k, Cell { UNUSED }),
        m_adj_span(m_adj_data.data(), std::array<size_t, 2> { m_layout.nv, m_layout.k }),
        m_touched_partitions(m_layout.nv * m_layout.k, false),
        m_bitset_data()
    {
        for (ygg::uint_t pi = 0; pi < m_layout.k; ++pi)
        {
            for (ygg::uint_t v : layout.vertex_partitions[pi])
            {
                for (ygg::uint_t pj = 0; pj < m_layout.k; ++pj)
                {
                    auto& cell = m_adj_span(v, pj);

                    if (dependency_graph.binary().has_dependency(pi, pj))
                    {
                        cell.offset = m_bitset_data.size();

                        const auto& info = m_layout.info.infos[pj];
                        m_bitset_data.resize(m_bitset_data.size() + info.num_blocks);
                    }
                    else
                    {
                        cell.offset = UNUSED;
                    }
                }
            }
        }
    }

    friend bool operator==(const PartitionedAdjacencyMatrix& lhs, const PartitionedAdjacencyMatrix& rhs) noexcept
    {
        return lhs.m_layout.nv == rhs.m_layout.nv && lhs.m_layout.k == rhs.m_layout.k && lhs.m_adj_data == rhs.m_adj_data
               && lhs.m_bitset_data == rhs.m_bitset_data;
    }

    auto get_bitset(ygg::uint_t v, ygg::uint_t p) noexcept
    {
        assert(m_dependency_graph.binary().has_dependency(m_layout.vertex_to_partition[v], p)
               && "Should only modify adjacency when there is a binary dependency");

        const auto& cell = m_adj_span(v, p);
        const auto& info = m_layout.info.infos[p];

        return ygg::BitsetSpan<uint64_t>(m_bitset_data.data() + cell.offset, info.num_bits);
    }

    auto get_bitset(ygg::uint_t v, ygg::uint_t p) const noexcept
    {
        const auto& cell = m_adj_span(v, p);
        const auto& info = m_layout.info.infos[p];
        const auto pv = m_layout.vertex_to_partition[v];

        if (m_dependency_graph.binary().has_dependency(pv, p))
        {
            return ygg::BitsetSpan<const uint64_t>(m_bitset_data.data() + cell.offset, info.num_bits);
        }
        else
        {
            assert(cell.offset == UNUSED);
            const auto bit = m_layout.vertex_to_bit[v];
            const auto& info_v = m_layout.info.infos[pv];
            const auto consistent_v = m_delta_partitions.get_bitset(info_v);

            return consistent_v.test(bit) ? m_affected_partitions.get_bitset(info) : m_delta_partitions.get_bitset(info);
        }
    }

    struct Cell
    {
        ygg::uint_t offset;

        friend bool operator==(const Cell& lhs, const Cell& rhs) noexcept { return lhs.offset == rhs.offset; }
    };

    template<typename Callback>
    void for_each_vertex(Callback&& callback) const noexcept
    {
        auto offset = ygg::uint_t(0);

        for (ygg::uint_t p = 0; p < m_layout.k; ++p)
        {
            const auto& info = m_layout.info.infos[p];
            auto partition = m_affected_partitions.get_bitset(info);

            for (auto bit = partition.find_first(); bit != ygg::BitsetSpan<const uint64_t>::npos; bit = partition.find_next(bit))
            {
                const ygg::uint_t v = offset + static_cast<ygg::uint_t>(bit);

                callback(Vertex(v));
            }

            offset += info.num_bits;
        }
    }

    template<typename Callback>
    void for_each_edge(Callback&& callback) const noexcept
    {
        ygg::uint_t src_offset = 0;

        for (ygg::uint_t pi = 0; pi < m_layout.k; ++pi)
        {
            const auto& info_i = m_layout.info.infos[pi];
            auto src_bits = m_affected_partitions.get_bitset(info_i);

            for (auto bi = src_bits.find_first(); bi != ygg::BitsetSpan<const uint64_t>::npos; bi = src_bits.find_next(bi))
            {
                const ygg::uint_t vi = src_offset + static_cast<ygg::uint_t>(bi);

                ygg::uint_t dst_offset = src_offset + info_i.num_bits;

                for (ygg::uint_t pj = pi + 1; pj < m_layout.k; ++pj)
                {
                    const auto& info_j = m_layout.info.infos[pj];

                    auto dst_active = m_affected_partitions.get_bitset(info_j);

                    auto adj = get_bitset(vi, pj);

                    for_each_bit(
                        [&](auto&& bj)
                        {
                            const ygg::uint_t vj = dst_offset + static_cast<ygg::uint_t>(bj);

                            callback(Edge(Vertex(vi), Vertex(vj)));
                        },
                        [](auto&& a, auto&& b) noexcept { return a & b; },
                        adj,
                        dst_active);

                    dst_offset += info_j.num_bits;
                }
            }

            src_offset += info_i.num_bits;
        }
    }

    void reset() noexcept
    {
        for (auto t = m_touched_partitions.find_first(); t != boost::dynamic_bitset<>::npos; t = m_touched_partitions.find_next(t))
        {
            const auto v = t / m_layout.k;
            const auto p = t % m_layout.k;

            get_bitset(v, p).reset();
        }

        m_touched_partitions.reset();
    }

    const auto& get_cell(ygg::uint_t v, ygg::uint_t p) const noexcept { return m_adj_span(v, p); }

    const auto& layout() const noexcept { return m_layout; }
    const auto& affected_partitions() const noexcept { return m_affected_partitions; }
    const auto& delta_partitions() const noexcept { return m_delta_partitions; }
    auto& touched_partitions() noexcept { return m_touched_partitions; }
    const auto& touched_partitions() const noexcept { return m_touched_partitions; }
    auto touched_partitions(ygg::uint_t v, ygg::uint_t p) noexcept { return m_touched_partitions[v * m_layout.k + p]; }
    const auto& adj_data() const noexcept { return m_adj_data; }
    auto adj_span() const noexcept { return m_adj_span; }
    const auto& bitset_data() const noexcept { return m_bitset_data; }

private:
    const GraphLayout& m_layout;
    const VertexPartitions& m_affected_partitions;
    const VertexPartitions& m_delta_partitions;
    const ::tyr::formalism::datalog::VariableDependencyGraph& m_dependency_graph;

    /// v x k matrix where each cell refers to a bitset either stored explicitly or referring implicitly to a vertex partition.
    std::vector<Cell> m_adj_data;
    ygg::MDSpan<Cell, 2> m_adj_span;

    /// v x k bitset to track touched cell bitsets
    boost::dynamic_bitset<> m_touched_partitions;

    /// Explicit storage
    std::vector<uint64_t> m_bitset_data;
};

struct Graph
{
    Graph(const GraphLayout& layout, const ::tyr::formalism::datalog::VariableDependencyGraph& dependency_graph) :
        affected_partitions(layout),
        delta_partitions(layout),
        matrix(layout, affected_partitions, delta_partitions, dependency_graph)
    {
    }

    friend bool same_affected_partitions(const Graph& lhs, const Graph& rhs) noexcept { return lhs.affected_partitions == rhs.affected_partitions; }

    friend bool same_delta_partitions(const Graph& lhs, const Graph& rhs) noexcept { return lhs.delta_partitions == rhs.delta_partitions; }

    friend bool same_matrix(const Graph& lhs, const Graph& rhs) noexcept { return lhs.matrix == rhs.matrix; }

    friend bool operator==(const Graph& lhs, const Graph& rhs) noexcept
    {
        return same_affected_partitions(lhs, rhs) && same_delta_partitions(lhs, rhs) && same_matrix(lhs, rhs);
    }

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

}

#endif
