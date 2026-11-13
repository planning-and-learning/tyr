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

#include <cassert>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <span>
#include <type_traits>
#include <urd/graphs.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/repr/values/dense_set.hpp>
#include <urd/store.hpp>
#include <utility>

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

/// Anchor of a clique whose first new element is a new atom of a literal the graph does not encode:
/// the clique fixes the atom's vertices and contains no delta vertex or edge, which belong to edge anchors.
struct Hyperedge
{
};

enum class AdjacencyKind : std::uint8_t
{
    IMPLICIT,
    STATIC_ONLY,
    RUNTIME,
};

/// What adjacency each pair of parts has, and for the runtime pairs, the number of each vertex's row into each.
class PartitionedAdjacencyLayout
{
public:
    PartitionedAdjacencyLayout() = default;
    PartitionedAdjacencyLayout(const Graph& graph, std::span<const AdjacencyKind> adjacency_kinds);

    friend bool operator==(const PartitionedAdjacencyLayout&, const PartitionedAdjacencyLayout&) noexcept = default;

    [[nodiscard]] AdjacencyKind kind(size_t source_partition, size_t target_partition) const noexcept
    { return m_kinds[(source_partition * m_num_partitions) + target_partition]; }
    [[nodiscard]] bool is_implicit(size_t source_partition, size_t target_partition) const noexcept
    { return kind(source_partition, target_partition) == AdjacencyKind::IMPLICIT; }
    [[nodiscard]] bool is_static_only(size_t source_partition, size_t target_partition) const noexcept
    { return kind(source_partition, target_partition) == AdjacencyKind::STATIC_ONLY; }
    [[nodiscard]] bool is_runtime(size_t source_partition, size_t target_partition) const noexcept
    { return kind(source_partition, target_partition) == AdjacencyKind::RUNTIME; }

    /// The row of `vertex` (in `source_partition`) into the runtime pair's `target_partition`.
    [[nodiscard]] size_t row(Vertex vertex, size_t source_partition, size_t target_partition) const noexcept
    {
        assert(is_runtime(source_partition, target_partition));
        return m_first_rows[vertex.index] + m_pair_rows[(source_partition * m_num_partitions) + target_partition];
    }

private:
    size_t m_num_partitions { 0 };
    urd::Store<urd::sequence<AdjacencyKind>> m_kinds;
    urd::Store<urd::sequence<size_t>> m_pair_rows;   ///< per pair, the position of its row among the source vertex's rows
    urd::Store<urd::sequence<size_t>> m_first_rows;  ///< per vertex, the number of its first row
};

/// The adjacency of a graph that changes from state to state, on the vertices `affected` holds: per pair of parts,
/// none constrains it (IMPLICIT), the static graph alone (STATIC_ONLY), or rows of edges added at runtime (RUNTIME).
/// Vertices in `delta` are adjacent to all others; one not in it only to those in it, so that the delta graph has the
/// edges with a new endpoint. The full graph's `delta` is its `affected`.
class PartitionedAdjacencyMatrix
{
public:
    PartitionedAdjacencyMatrix(const Graph& graph, const PartitionedAdjacencyLayout& adjacency_layout, const VertexSets& affected, const VertexSets& delta);

    /// Adds the edge between u and v, of a runtime pair of parts.
    void add_edge(Vertex u, Vertex v);
    /// The runtime neighbours of `vertex` in `partition`.
    [[nodiscard]] const VertexSet& runtime_neighbours(Vertex vertex, size_t partition) const
    { return m_rows[m_adjacency_layout->row(vertex, m_graph->part_of(vertex), partition)]; }

    void copy_adjacency_to(Vertex vertex, size_t partition, VertexSet& destination) const;
    /// Sets `destination` to the vertices of `source` adjacent to `vertex` in `partition`; `source` may be `destination`.
    void intersect_adjacency(Vertex vertex, size_t partition, const VertexSet& source, VertexSet& destination) const;
    /// Removes the neighbours of `vertex` in `partition` from `destination`; `scratch` is a set over the partition's window.
    void subtract_adjacency_from(Vertex vertex, size_t partition, VertexSet& destination, VertexSet& scratch) const;
    [[nodiscard]] bool is_adjacent(Vertex vertex, Vertex target) const;

    template<typename Callback>
    void for_each_vertex(Callback callback) const
    {
        for (const auto& vertices : *m_affected)
            for (const auto vertex : vertices)
                callback(vertex);
    }

    template<std::invocable<Edge> Callback>
    void for_each_edge(Callback callback) const
    {
        for_each_edge([](Vertex, size_t) noexcept { return true; }, std::move(callback));
    }

    /// for_each_edge, but each source vertex and target partition are first passed to visit_source, which can skip their edges.
    template<std::predicate<Vertex, size_t> VisitSource, std::invocable<Edge> Callback>
    void for_each_edge(VisitSource visit_source, Callback callback) const
    {
        const auto num_parts = m_graph->num_parts();
        for (size_t source_partition = 0; source_partition < num_parts; ++source_partition)
            for (const auto source : (*m_affected)[source_partition])
                for (auto target_partition = source_partition + 1; target_partition < num_parts; ++target_partition)
                {
                    if (!visit_source(source, target_partition))
                        continue;
                    const auto& targets = (*m_affected)[target_partition];
                    for_each_adjacent_vertex(source,
                                             target_partition,
                                             [&](Vertex target)
                                             {
                                                 if (targets.contains(target))
                                                     callback(Edge(source, target));
                                             });
                }
    }

    /// Clears the rows added to since the last reset.
    void reset() noexcept;

    [[nodiscard]] const Graph& graph() const noexcept { return *m_graph; }
    [[nodiscard]] const VertexSets& affected_partitions() const noexcept { return *m_affected; }
    [[nodiscard]] const VertexSets& delta_partitions() const noexcept { return *m_delta; }

private:
    /// The vertices of `partition` that `vertex` may be adjacent to: all affected ones if it is new, else the new ones.
    [[nodiscard]] const VertexSet& context(Vertex vertex, size_t partition) const
    { return (*m_delta)[m_graph->part_of(vertex)].contains(vertex) ? (*m_affected)[partition] : (*m_delta)[partition]; }

    template<typename Callback>
    void for_each_adjacent_vertex(Vertex vertex, size_t partition, Callback callback) const
    {
        switch (m_adjacency_layout->kind(m_graph->part_of(vertex), partition))
        {
            case AdjacencyKind::RUNTIME:
                for (const auto target : runtime_neighbours(vertex, partition))
                    callback(target);
                return;
            case AdjacencyKind::IMPLICIT:
                for (const auto target : context(vertex, partition))
                    callback(target);
                return;
            case AdjacencyKind::STATIC_ONLY:
                urd::for_each_neighbour_in(m_graph->rows(), vertex, partition, context(vertex, partition), callback);
                return;
        }
    }

    const Graph* m_graph;
    const PartitionedAdjacencyLayout* m_adjacency_layout;
    const VertexSets* m_affected;
    const VertexSets* m_delta;
    VertexSets m_rows;                                   ///< the runtime rows, numbered by the layout
    urd::Store<urd::set<size_t>, urd::dense> m_touched;  ///< the rows added to since the last reset
};

struct FullGraph
{
    FullGraph(const Graph& graph, const PartitionedAdjacencyLayout& adjacency_layout) :
        affected_partitions(graph.empty_parts()),
        matrix(graph, adjacency_layout, affected_partitions, affected_partitions)
    {
    }

    FullGraph(const FullGraph&) = delete;
    FullGraph& operator=(const FullGraph&) = delete;
    FullGraph(FullGraph&&) = delete;
    FullGraph& operator=(FullGraph&&) = delete;
    ~FullGraph() = default;

    void reset() noexcept
    {
        for (auto& vertices : affected_partitions)
            vertices.clear();
        matrix.reset();
    }

    VertexSets affected_partitions;
    PartitionedAdjacencyMatrix matrix;
};

struct DeltaGraph
{
    DeltaGraph(const Graph& graph, const PartitionedAdjacencyLayout& adjacency_layout) :
        affected_partitions(graph.empty_parts()),
        delta_partitions(graph.empty_parts()),
        scratch(graph.empty_parts()),
        matrix(graph, adjacency_layout, affected_partitions, delta_partitions)
    {
    }

    DeltaGraph(const DeltaGraph&) = delete;
    DeltaGraph& operator=(const DeltaGraph&) = delete;
    DeltaGraph(DeltaGraph&&) = delete;
    DeltaGraph& operator=(DeltaGraph&&) = delete;
    ~DeltaGraph() = default;

    void reset() noexcept
    {
        for (auto& vertices : affected_partitions)
            vertices.clear();
        for (auto& vertices : delta_partitions)
            vertices.clear();
        matrix.reset();
    }

    VertexSets affected_partitions;
    VertexSets delta_partitions;
    VertexSets scratch;  ///< per part, for whoever builds the graph
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
    ~DeltaKCKP() = default;

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
    void for_each_clique(Callback callback, Workspace& workspace) const
    {
        workspace.prepare(*m_graph);
        const auto num_partitions = m_graph->num_parts();
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
            complete_from_seed<void>(std::move(callback), 0, workspace);
        }
    }

    template<typename Callback>
    void for_each_delta_clique(Callback callback, Workspace& workspace) const
    {
        workspace.prepare(*m_graph);
        if (m_iteration == 1)
        {
            for_each_clique(std::move(callback), workspace);
            return;
        }

        const auto num_partitions = m_graph->num_parts();
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

        // The rows of an anchor's source are shared by its edges into a target partition, and if one is empty, no edge seeds.
        m_delta_graph.matrix.for_each_edge([&](Vertex source, size_t target_partition)
                                           { return prepare_anchor_source(source, static_cast<ygg::uint_t>(target_partition), workspace); },
                                           [&](Edge edge)
                                           {
                                               if (seed_from_prepared_anchor(edge, workspace))
                                                   complete_from_seed<Edge>(std::ref(callback), 0, workspace);
                                           });
    }

    /// Cliques that contain all anchor vertices (at most one per partition) and no delta vertex or edge.
    template<typename Callback>
    void for_each_hyperedge_clique(std::span<const Vertex> anchor, Callback callback, Workspace& workspace) const
    {
        if (!seed_from_vertices(anchor, workspace))
            return;
        if (workspace.partial_solution_size == m_graph->num_parts())
            callback(workspace.partial_solution);
        else
            complete_from_seed<Hyperedge>(std::move(callback), 0, workspace);
    }

    const urd::Store<urd::sequence<Edge>>& materialize_delta_edges();
    void seed_without_anchor(Workspace& workspace) const;
    bool seed_from_anchor(const Edge& edge, Workspace& workspace) const;
    /// Computes the rows of the anchor source shared by its edges into the target partition: in every other partition, the
    /// source's neighbours, without its delta neighbours before the target partition. Returns false if a row is empty.
    bool prepare_anchor_source(Vertex source, ygg::uint_t target_partition, Workspace& workspace) const;
    /// seed_from_anchor for an edge from the source last passed to prepare_anchor_source.
    bool seed_from_prepared_anchor(const Edge& edge, Workspace& workspace) const;
    bool seed_from_vertices(std::span<const Vertex> vertices, Workspace& workspace) const;

    template<typename Anchor, typename Callback>
    void complete_from_seed(Callback callback, size_t depth, Workspace& workspace) const
    {
        assert(depth < m_graph->num_parts());
        auto select_partition = [&](size_t current_depth, const Workspace& current) { return choose_best_partition(current_depth, current); };
        auto update_candidates = [&](Vertex source, size_t current_depth, Workspace& current)
        { return update_candidates_for<Anchor>(source, current_depth, current); };
        detail::complete_clique(*m_graph, depth, workspace, select_partition, update_candidates, callback);
    }

    [[nodiscard]] const Graph& get_graph() const noexcept { return *m_graph; }
    [[nodiscard]] const DeltaGraph& get_delta_graph() const noexcept { return m_delta_graph; }
    [[nodiscard]] const FullGraph& get_full_graph() const noexcept { return m_full_graph; }
    [[nodiscard]] size_t get_iteration() const noexcept { return m_iteration; }

private:
    [[nodiscard]] ygg::uint_t choose_best_partition(size_t depth, const Workspace& workspace) const;

    template<typename Anchor>
    bool update_candidates_for(Vertex source, size_t depth, Workspace& workspace) const
    {
        const auto source_partition = m_graph->part_of(source);
        assert(source_partition != workspace.anchor_pi);
        assert(source_partition != workspace.anchor_pj);

        for (ygg::uint_t partition = 0; partition < m_graph->num_parts(); ++partition)
        {
            if (workspace.partition_bits.contains(partition))
                continue;

            auto& next = workspace.candidates_at(depth + 1, partition);
            m_full_graph.matrix.intersect_adjacency(source, partition, workspace.candidates_at(depth, partition), next);
            if (next.empty())
                return false;

            if constexpr (std::is_same_v<Anchor, Edge>)
            {
                if (source_partition < workspace.anchor_pi || partition < workspace.anchor_pi)
                {
                    m_delta_graph.matrix.subtract_adjacency_from(source, partition, next, workspace.scratch[partition]);
                    if (next.empty())
                        return false;
                }
            }
            else if constexpr (std::is_same_v<Anchor, Hyperedge>)
            {
                m_delta_graph.matrix.subtract_adjacency_from(source, partition, next, workspace.scratch[partition]);
                if (next.empty())
                    return false;
            }
        }
        return true;
    }

    const Graph* m_graph;
    size_t m_iteration { 0 };
    DeltaGraph m_delta_graph;
    FullGraph m_full_graph;
    urd::Store<urd::sequence<Edge>> m_delta_edges;
};

}

#endif
