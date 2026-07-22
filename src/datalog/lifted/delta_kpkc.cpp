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

#include "tyr/datalog/lifted/delta_kpkc.hpp"

#include "tyr/datalog/lifted/consistency_graph.hpp"

namespace tyr::datalog::kpkc
{

DeltaKPKC::DeltaKPKC(const StaticConsistencyGraph& static_graph) :
    m_layout(GraphLayout(static_graph.get_num_vertices(), static_graph.get_vertex_partitions())),
    m_iteration(0),
    m_delta_graph(m_layout, static_graph.get_variable_dependeny_graph()),
    m_full_graph(m_layout, static_graph.get_variable_dependeny_graph()),
    m_delta_edges()
{
}

void DeltaKPKC::set_next_assignment_sets(const StaticConsistencyGraph& static_graph, const AssignmentSets& assignment_sets)
{
    static_graph.initialize_dynamic_consistency_graphs(assignment_sets, m_layout, m_delta_graph, m_full_graph);

    m_delta_edges.clear();
    ++m_iteration;
}

void DeltaKPKC::reset()
{
    m_delta_graph.reset();
    m_full_graph.reset();
    m_delta_edges.clear();
    m_iteration = 0;
}

const std::vector<Edge>& DeltaKPKC::materialize_delta_edges()
{
    m_delta_edges.clear();
    m_delta_graph.matrix.for_each_edge([&](auto&& edge) { m_delta_edges.push_back(edge); });
    return m_delta_edges;
}

void DeltaKPKC::seed_without_anchor(Workspace& workspace) const
{
    workspace.partial_solution_size = 0;
    workspace.partition_bits.reset();
    workspace.anchor_pi = std::numeric_limits<ygg::uint_t>::max();  // unused
    workspace.anchor_pj = std::numeric_limits<ygg::uint_t>::max();  // unused

    auto cv_0_row = workspace.compatible_vertices_span(0);

    for (ygg::uint_t p = 0; p < m_layout.k; ++p)
    {
        const auto& info = m_layout.info.infos[p];
        auto cv_0 = ygg::BitsetSpan<uint64_t>(cv_0_row.data() + info.block_offset, info.num_bits);

        auto partition = m_full_graph.matrix.affected_partitions().get_bitset(info);
        cv_0.copy_from(partition);
    }
}

bool DeltaKPKC::seed_from_anchor(const Edge& edge, Workspace& workspace) const
{
    const ygg::uint_t pi = m_layout.vertex_to_partition[edge.src.index];
    const ygg::uint_t pj = m_layout.vertex_to_partition[edge.dst.index];
    assert(pi < pj);

    workspace.partial_solution[pi] = edge.src;
    workspace.partial_solution[pj] = edge.dst;
    workspace.partial_solution_size = 2;

    workspace.anchor_pi = pi;
    workspace.anchor_pj = pj;
    workspace.partition_bits.reset();
    workspace.partition_bits.set(pi);
    workspace.partition_bits.set(pj);

    auto cv_0_row = workspace.compatible_vertices_span(0);

    for (ygg::uint_t p = 0; p < m_layout.k; ++p)
    {
        if (p == pi || p == pj)
            continue;

        const auto& info = m_layout.info.infos[p];
        auto cv_0 = ygg::BitsetSpan<uint64_t>(cv_0_row.data() + info.block_offset, info.num_bits);
        auto full_src_adj_list = m_full_graph.matrix.get_bitset(edge.src.index, p);
        auto full_dst_adj_list = m_full_graph.matrix.get_bitset(edge.dst.index, p);

        cv_0.copy_from(full_src_adj_list);
        cv_0 &= full_dst_adj_list;

        if (!cv_0.any())
            return false;  ///< triangle pruning

        auto delta_src_adj_list = m_delta_graph.matrix.get_bitset(edge.src.index, p);

        if (p < pj)
            cv_0 -= delta_src_adj_list;

        if (!cv_0.any())
            return false;  ///< triangle pruning

        auto delta_dst_adj_list = m_delta_graph.matrix.get_bitset(edge.dst.index, p);

        if (p < pi)
            cv_0 -= delta_dst_adj_list;

        if (!cv_0.any())
            return false;  ///< triangle pruning
    }

    return true;
}

ygg::uint_t DeltaKPKC::choose_best_partition(size_t depth, const Workspace& workspace) const
{
    const ygg::uint_t k = m_layout.k;
    const auto& partition_bits = workspace.partition_bits;

    ygg::uint_t best_partition = std::numeric_limits<ygg::uint_t>::max();
    ygg::uint_t best_set_bits = std::numeric_limits<ygg::uint_t>::max();
    const auto cv_curr_row = workspace.compatible_vertices_span(depth);
    for (ygg::uint_t p = 0; p < k; ++p)
    {
        if (partition_bits.test(p))
            continue;

        const auto& info = m_layout.info.infos[p];
        auto cv_curr = ygg::BitsetSpan<const uint64_t>(cv_curr_row.data() + info.block_offset, info.num_bits);

        const auto num_set_bits = cv_curr.count();
        if (num_set_bits < best_set_bits)
        {
            best_set_bits = num_set_bits;
            best_partition = p;
        }
    }

    return best_partition;
}
}
