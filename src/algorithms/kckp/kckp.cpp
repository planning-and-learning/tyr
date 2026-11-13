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

#include <algorithm>

namespace tyr::kckp
{

void Workspace::prepare(const Graph& graph)
{
    const auto wanted = graph.bounds();
    if (std::ranges::equal(bounds, wanted))
        return;

    bounds.assign(wanted.begin(), wanted.end());
    const auto parts = graph.empty_parts();
    candidates.clear();
    for (size_t depth = 0; depth < parts.size(); ++depth)
        for (const auto& part : parts)
            candidates.push_back(part);
    anchor_source = parts;
    scratch = parts;
    partition_bits.clear();
    partial_solution.resize(parts.size());
}

bool KCKP::seed_from_prefix(std::span<const Vertex> prefix, Workspace& workspace) const
{
    const auto& graph = *m_graph;
    workspace.prepare(graph);
    workspace.partition_bits.clear();
    workspace.partial_solution_size = static_cast<ygg::uint_t>(prefix.size());

    for (ygg::uint_t partition = 0; partition < prefix.size(); ++partition)
    {
        const auto vertex = prefix[partition];
        if (vertex.index >= graph.num_vertices() || graph.part_of(vertex) != partition)
            return false;

        workspace.partial_solution[partition] = vertex;
        workspace.partition_bits.insert(partition);
        for (ygg::uint_t previous = 0; previous < partition; ++previous)
            if (!graph.adjacent(workspace.partial_solution[previous], vertex))
                return false;
    }

    if (prefix.size() == graph.num_parts())
        return true;

    for (auto partition = prefix.size(); partition < graph.num_parts(); ++partition)
    {
        auto& candidates = workspace.candidates_at(0, partition);
        candidates = graph.all(partition);
        for (ygg::uint_t fixed_partition = 0; fixed_partition < prefix.size(); ++fixed_partition)
            urd::intersect_neighbours(graph.rows(), workspace.partial_solution[fixed_partition], partition, candidates);
        if (candidates.empty())
            return false;
    }
    return true;
}

ygg::uint_t KCKP::choose_next_partition(ygg::uint_t first_partition, const Workspace& workspace) const noexcept
{
    for (auto partition = first_partition; partition < m_graph->num_parts(); ++partition)
        if (!workspace.partition_bits.contains(partition))
            return partition;
    return std::numeric_limits<ygg::uint_t>::max();
}

bool KCKP::update_candidates(ygg::uint_t first_partition, Vertex source, size_t depth, Workspace& workspace) const
{
    for (auto partition = first_partition; partition < m_graph->num_parts(); ++partition)
    {
        if (workspace.partition_bits.contains(partition))
            continue;

        auto& next = workspace.candidates_at(depth + 1, partition);
        urd::intersect_neighbours(m_graph->rows(), source, partition, workspace.candidates_at(depth, partition), next);
        if (next.empty())
            return false;
    }
    return true;
}

}
