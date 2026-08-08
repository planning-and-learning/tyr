/*
 * Copyright (C) 2026 Dominik Drexler
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

#include <algorithm>
#include <deque>
#include <stdexcept>
#include <utility>
#include <yggdrasil/containers/associative_containers.hpp>
#include <yggdrasil/semantics/containers/dynamic_bitset_equal_to.hpp>
#include <yggdrasil/semantics/containers/dynamic_bitset_hash.hpp>

namespace tyr::kckp
{
namespace
{
[[maybe_unused]] bool verify_vertex_partitions(const std::vector<std::vector<ygg::uint_t>>& partitions)
{
    auto expected = ygg::uint_t { 0 };
    for (const auto& partition : partitions)
        for (const auto vertex : partition)
            if (vertex != expected++)
                return false;
    return true;
}

size_t pair_rank(size_t num_partitions, ygg::uint_t first, ygg::uint_t second) noexcept { return size_t(first) * num_partitions + second; }
}

GraphLayout create_graph_layout(std::span<const size_t> partition_sizes, std::span<const ygg::uint_t> vertex_labels)
{
    auto partitions = std::vector<std::vector<ygg::uint_t>> {};
    partitions.reserve(partition_sizes.size());
    auto vertex = ygg::uint_t { 0 };
    for (const auto partition_size : partition_sizes)
    {
        auto& partition = partitions.emplace_back();
        partition.reserve(partition_size);
        for (size_t i = 0; i < partition_size; ++i)
            partition.push_back(vertex++);
    }
    return GraphLayout(vertex, partitions, vertex_labels);
}

GraphLayout::GraphLayout(size_t num_vertices_, const std::vector<std::vector<ygg::uint_t>>& partitions, std::span<const ygg::uint_t> labels) :
    num_vertices(num_vertices_),
    num_partitions(partitions.size()),
    vertex_partitions(partitions),
    vertex_to_partition(num_vertices),
    vertex_to_bit(num_vertices),
    vertex_labels(labels.begin(), labels.end())
{
    assert(verify_vertex_partitions(partitions));
    assert(labels.empty() || labels.size() == num_vertices);

    if (labels.empty())
    {
        vertex_labels.reserve(num_vertices);
        for (ygg::uint_t vertex = 0; vertex < num_vertices; ++vertex)
            vertex_labels.push_back(vertex);
    }

    auto block_offset = ygg::uint_t { 0 };
    auto bit_offset = ygg::uint_t { 0 };
    info.infos.reserve(num_partitions);
    for (ygg::uint_t partition = 0; partition < num_partitions; ++partition)
    {
        const auto partition_size = static_cast<ygg::uint_t>(partitions[partition].size());
        const auto partition_blocks = static_cast<ygg::uint_t>(ygg::BitsetSpan<uint64_t>::num_blocks(partition_size));
        info.infos.push_back(BitsetInfo { bit_offset, partition_size, block_offset, partition_blocks });
        block_offset += partition_blocks;

        auto bit = ygg::uint_t { 0 };
        for (const auto vertex : partitions[partition])
        {
            vertex_to_partition[vertex] = partition;
            vertex_to_bit[vertex] = bit++;
            ++bit_offset;
        }
    }
    info.num_blocks = block_offset;
}

DeduplicatedAdjacencyMatrix::DeduplicatedAdjacencyMatrix(const AdjacencyMatrix& matrix) : m_layout(matrix.layout())
{
    auto row_to_offset = ygg::UnorderedMap<std::span<const uint64_t>, ygg::uint_t> {};
    auto partition_to_offset = ygg::UnorderedMap<ygg::BitsetSpan<const uint64_t>, ygg::uint_t> {};
    for (ygg::uint_t vertex = 0; vertex < m_layout.num_vertices; ++vertex)
    {
        const auto [row_it, new_row] = row_to_offset.emplace(matrix.get_row(vertex), m_row_data.size());
        m_row_offset.push_back(row_it->second);
        if (!new_row)
            continue;

        for (ygg::uint_t partition = 0; partition < m_layout.num_partitions; ++partition)
        {
            const auto bitset = matrix.get_bitset(vertex, partition);
            const auto [partition_it, new_partition] = partition_to_offset.emplace(bitset, m_bitset_data.size());
            m_row_data.push_back(partition_it->second);
            if (new_partition)
                m_bitset_data.insert(m_bitset_data.end(), bitset.blocks().begin(), bitset.blocks().end());
        }
    }
}

Graph::Graph() : m_satisfiable(true), m_layout(create_graph_layout(std::span<const size_t> {})), m_adjacency(m_layout) {}

Graph Graph::create(bool satisfiable, AdjacencyMatrix adjacency, boost::dynamic_bitset<> universal_partition_pairs)
{
    return Graph(satisfiable, std::move(adjacency), std::move(universal_partition_pairs));
}

Graph::Graph(bool satisfiable, AdjacencyMatrix adjacency, boost::dynamic_bitset<> universal_partition_pairs)
{
    const auto old_layout = adjacency.layout();
    assert(universal_partition_pairs.size() == old_layout.num_partitions * old_layout.num_partitions);

    auto live = VertexPartitions(old_layout);
    if (satisfiable)
        for (ygg::uint_t partition = 0; partition < old_layout.num_partitions; ++partition)
        {
            auto bits = live.get_bitset(partition);
            bits.set();
            if (bits.empty())
                satisfiable = false;
        }
    if (!satisfiable)
        live.reset();

    auto removed = std::deque<ygg::uint_t> {};
    auto wiped_out = !satisfiable && old_layout.num_partitions != 0;
    const auto remove_vertex = [&](ygg::uint_t vertex)
    {
        const auto partition = old_layout.vertex_to_partition[vertex];
        auto bits = live.get_bitset(partition);
        const auto bit = old_layout.vertex_to_bit[vertex];
        if (!bits.test(bit))
            return;

        bits.reset(bit);
        removed.push_back(vertex);
        if (bits.none())
        {
            live.reset();
            removed.clear();
            wiped_out = true;
        }
    };

    auto support_counts = std::vector<ygg::uint_t>(old_layout.num_vertices * old_layout.num_partitions);
    if (!wiped_out)
        for (ygg::uint_t source = 0; source < old_layout.num_vertices && !wiped_out; ++source)
        {
            const auto source_partition = old_layout.vertex_to_partition[source];
            for (ygg::uint_t target_partition = 0; target_partition < old_layout.num_partitions; ++target_partition)
            {
                if (source_partition == target_partition
                    || universal_partition_pairs.test(pair_rank(old_layout.num_partitions, source_partition, target_partition)))
                    continue;

                auto& count = support_counts[size_t(source) * old_layout.num_partitions + target_partition];
                count = static_cast<ygg::uint_t>(adjacency.get_bitset(source, target_partition).count());
                if (count == 0)
                {
                    remove_vertex(source);
                    break;
                }
            }
        }

    while (!removed.empty() && !wiped_out)
    {
        const auto target = removed.front();
        removed.pop_front();
        const auto target_partition = old_layout.vertex_to_partition[target];

        for (ygg::uint_t source_partition = 0; source_partition < old_layout.num_partitions && !wiped_out; ++source_partition)
        {
            if (source_partition == target_partition
                || universal_partition_pairs.test(pair_rank(old_layout.num_partitions, source_partition, target_partition)))
                continue;

            const auto& source_info = old_layout.info.infos[source_partition];
            const auto reverse_supports = adjacency.get_bitset(target, source_partition);
            auto source_live = live.get_bitset(source_partition);
            for (auto source_bit = reverse_supports.find_first(); source_bit != ygg::BitsetSpan<const uint64_t>::npos && !wiped_out;
                 source_bit = reverse_supports.find_next(source_bit))
            {
                if (!source_live.test(source_bit))
                    continue;

                const auto source = source_info.bit_offset + source_bit;
                auto& count = support_counts[size_t(source) * old_layout.num_partitions + target_partition];
                assert(count != 0);
                if (--count == 0)
                    remove_vertex(source);
            }
        }
    }

    auto old_to_new = std::vector<ygg::uint_t>(old_layout.num_vertices, std::numeric_limits<ygg::uint_t>::max());
    auto old_to_new_bit = std::vector<std::vector<ygg::uint_t>>(old_layout.num_partitions);
    auto partition_sizes = std::vector<size_t>(old_layout.num_partitions);
    auto vertex_labels = std::vector<ygg::uint_t> {};
    vertex_labels.reserve(old_layout.num_vertices);
    auto new_vertex = ygg::uint_t { 0 };
    for (ygg::uint_t partition = 0; partition < old_layout.num_partitions; ++partition)
    {
        const auto& info = old_layout.info.infos[partition];
        auto& bit_mapping = old_to_new_bit[partition];
        bit_mapping.resize(info.num_bits, std::numeric_limits<ygg::uint_t>::max());
        const auto live_bits = live.get_bitset(partition);
        partition_sizes[partition] = live_bits.count();
        auto compact_bit = ygg::uint_t { 0 };
        for (auto bit = live_bits.find_first(); bit != ygg::BitsetSpan<const uint64_t>::npos; bit = live_bits.find_next(bit))
        {
            vertex_labels.push_back(old_layout.vertex_labels[info.bit_offset + bit]);
            bit_mapping[bit] = compact_bit++;
            old_to_new[info.bit_offset + bit] = new_vertex++;
        }
    }

    m_satisfiable = satisfiable && std::ranges::all_of(partition_sizes, [](const auto size) { return size != 0; });
    if (partition_sizes.empty())
        m_satisfiable = satisfiable;
    m_layout = create_graph_layout(partition_sizes, vertex_labels);

    auto compact = AdjacencyMatrix(m_layout);
    for (ygg::uint_t old_source = 0; old_source < old_layout.num_vertices; ++old_source)
    {
        const auto compact_source = old_to_new[old_source];
        if (compact_source == std::numeric_limits<ygg::uint_t>::max())
            continue;
        for (ygg::uint_t target_partition = 0; target_partition < old_layout.num_partitions; ++target_partition)
        {
            auto new_bits = compact.get_bitset(compact_source, target_partition);
            const auto source_partition = old_layout.vertex_to_partition[old_source];
            if (source_partition != target_partition
                && universal_partition_pairs.test(pair_rank(old_layout.num_partitions, source_partition, target_partition)))
            {
                new_bits.set();
                continue;
            }

            const auto old_bits = adjacency.get_bitset(old_source, target_partition);
            for (auto old_bit = old_bits.find_first(); old_bit != ygg::BitsetSpan<const uint64_t>::npos; old_bit = old_bits.find_next(old_bit))
                if (const auto compact_bit = old_to_new_bit[target_partition][old_bit]; compact_bit != std::numeric_limits<ygg::uint_t>::max())
                    new_bits.set(compact_bit);
        }
    }
    m_adjacency = DeduplicatedAdjacencyMatrix(compact);
}

}
