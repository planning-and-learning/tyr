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

#include <bit>
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
    AdjacencyMatrix(const GraphLayout& layout) : m_layout(layout), m_bitset_data(layout.nv * layout.info.num_blocks) {}

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

class PartitionedAdjacencyLayout
{
public:
    static constexpr ygg::uint_t IMPLICIT = std::numeric_limits<ygg::uint_t>::max();
    static constexpr ygg::uint_t STATIC_ONLY = IMPLICIT - 1;

    PartitionedAdjacencyLayout() = default;
    PartitionedAdjacencyLayout(const GraphLayout& layout, const ::tyr::formalism::datalog::VariableDependencyGraph& dependency_graph);

    friend bool operator==(const PartitionedAdjacencyLayout&, const PartitionedAdjacencyLayout&) noexcept = default;

    ygg::uint_t get_offset(ygg::uint_t v, ygg::uint_t pi, ygg::uint_t pj) const noexcept
    {
        assert(v < m_row_offsets.size());
        assert(pi < m_k);
        assert(pj < m_k);

        const auto pair_offset = m_pair_offsets[pi * m_k + pj];
        return pair_offset >= STATIC_ONLY ? pair_offset : m_row_offsets[v] + pair_offset;
    }

    bool is_implicit(ygg::uint_t pi, ygg::uint_t pj) const noexcept { return get_pair_offset(pi, pj) == IMPLICIT; }
    bool is_static_only(ygg::uint_t pi, ygg::uint_t pj) const noexcept { return get_pair_offset(pi, pj) == STATIC_ONLY; }
    bool is_runtime(ygg::uint_t pi, ygg::uint_t pj) const noexcept { return get_pair_offset(pi, pj) < STATIC_ONLY; }

    size_t num_blocks() const noexcept { return m_num_blocks; }

private:
    ygg::uint_t get_pair_offset(ygg::uint_t pi, ygg::uint_t pj) const noexcept
    {
        assert(pi < m_k);
        assert(pj < m_k);
        return m_pair_offsets[pi * m_k + pj];
    }

    size_t m_k { 0 };
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
        m_touched_partitions(m_layout.nv * m_layout.k, false),
        m_bitset_data(adjacency_layout.num_blocks(), 0)
    {
    }

    auto get_runtime_bitset(ygg::uint_t v, ygg::uint_t p) noexcept
    {
        const auto offset = m_adjacency_layout.get_offset(v, m_layout.vertex_to_partition[v], p);
        assert(offset < PartitionedAdjacencyLayout::STATIC_ONLY && "Only runtime-dependent adjacency is mutable");
        const auto& info = m_layout.info.infos[p];

        return ygg::BitsetSpan<uint64_t>(m_bitset_data.data() + offset, info.num_bits);
    }

    auto get_runtime_bitset(ygg::uint_t v, ygg::uint_t p) const noexcept
    {
        const auto& info = m_layout.info.infos[p];
        const auto pv = m_layout.vertex_to_partition[v];
        const auto offset = m_adjacency_layout.get_offset(v, pv, p);
        assert(offset < PartitionedAdjacencyLayout::STATIC_ONLY && "Only runtime-dependent adjacency is stored");

        return ygg::BitsetSpan<const uint64_t>(m_bitset_data.data() + offset, info.num_bits);
    }

    void copy_adjacency_to(ygg::uint_t v, ygg::uint_t p, ygg::BitsetSpan<uint64_t> destination) const
    {
        assert(destination.size() == m_layout.info.infos[p].num_bits);
        for_each_adjacency_block(v, p, [&](size_t block, uint64_t adjacency) { destination.blocks()[block] = adjacency; });
    }

    void intersect_adjacency_with(ygg::uint_t v, ygg::uint_t p, ygg::BitsetSpan<uint64_t> destination) const
    {
        assert(destination.size() == m_layout.info.infos[p].num_bits);
        for_each_adjacency_block(v, p, [&](size_t block, uint64_t adjacency) { destination.blocks()[block] &= adjacency; });
    }

    void subtract_adjacency_from(ygg::uint_t v, ygg::uint_t p, ygg::BitsetSpan<uint64_t> destination) const
    {
        assert(destination.size() == m_layout.info.infos[p].num_bits);
        for_each_adjacency_block(v, p, [&](size_t block, uint64_t adjacency) { destination.blocks()[block] &= ~adjacency; });
    }

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

                    for_each_adjacent_vertex(vi,
                                             pj,
                                             [&](auto&& bj)
                                             {
                                                 const ygg::uint_t vj = dst_offset + static_cast<ygg::uint_t>(bj);
                                                 callback(Edge(Vertex(vi), Vertex(vj)));
                                             });

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

            get_runtime_bitset(v, p).reset();
        }

        m_touched_partitions.reset();
    }

    const auto& layout() const noexcept { return m_layout; }
    const auto& affected_partitions() const noexcept { return m_affected_partitions; }
    const auto& delta_partitions() const noexcept { return m_delta_partitions; }
    auto& touched_partitions() noexcept { return m_touched_partitions; }
    const auto& touched_partitions() const noexcept { return m_touched_partitions; }
    auto touched_partitions(ygg::uint_t v, ygg::uint_t p) noexcept { return m_touched_partitions[v * m_layout.k + p]; }
    const auto& bitset_data() const noexcept { return m_bitset_data; }

private:
    auto get_contextual_partition(ygg::uint_t v, ygg::uint_t p) const noexcept
    {
        const auto pv = m_layout.vertex_to_partition[v];
        const auto bit = m_layout.vertex_to_bit[v];
        const auto source_delta = m_delta_partitions.get_bitset(m_layout.info.infos[pv]).test(bit);
        return source_delta ? m_affected_partitions.get_bitset(p) : m_delta_partitions.get_bitset(p);
    }

    template<typename Callback>
    void for_each_adjacency_block(ygg::uint_t v, ygg::uint_t p, Callback&& callback) const
    {
        const auto pv = m_layout.vertex_to_partition[v];
        const auto offset = m_adjacency_layout.get_offset(v, pv, p);

        if (offset < PartitionedAdjacencyLayout::STATIC_ONLY)
        {
            const auto& info = m_layout.info.infos[p];
            const auto blocks = std::span<const uint64_t>(m_bitset_data.data() + offset, info.num_blocks);
            for (size_t block = 0; block < blocks.size(); ++block)
                callback(block, blocks[block]);
            return;
        }

        const auto partition_blocks = get_contextual_partition(v, p).blocks();
        if (offset == PartitionedAdjacencyLayout::IMPLICIT)
        {
            for (size_t block = 0; block < partition_blocks.size(); ++block)
                callback(block, partition_blocks[block]);
            return;
        }

        assert(offset == PartitionedAdjacencyLayout::STATIC_ONLY);
        const auto static_blocks = m_static_matrix.get_bitset(v, p).blocks();
        assert(static_blocks.size() == partition_blocks.size());
        for (size_t block = 0; block < partition_blocks.size(); ++block)
            callback(block, static_blocks[block] & partition_blocks[block]);
    }

    template<typename Callback>
    void for_each_adjacent_vertex(ygg::uint_t v, ygg::uint_t p, Callback&& callback) const
    {
        const auto active_blocks = m_affected_partitions.get_bitset(p).blocks();
        const auto num_bits = m_layout.info.infos[p].num_bits;
        for_each_adjacency_block(v,
                                 p,
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

    /// v x k bitset to track touched cell bitsets
    boost::dynamic_bitset<> m_touched_partitions;

    /// Runtime-filtered storage
    std::vector<uint64_t> m_bitset_data;
};

struct FullGraph
{
    FullGraph(const GraphLayout& layout, const PartitionedAdjacencyLayout& adjacency_layout, const DeduplicatedAdjacencyMatrix& static_matrix) :
        affected_partitions(layout),
        matrix(layout, adjacency_layout, static_matrix, affected_partitions, affected_partitions)
    {
    }

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
