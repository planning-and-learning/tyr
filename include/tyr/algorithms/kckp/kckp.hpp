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

#include <cassert>
#include <cstddef>
#include <limits>
#include <optional>
#include <span>
#include <urd/elements/concepts.hpp>
#include <urd/graphs.hpp>
#include <urd/key_range.hpp>
#include <urd/repr/graphs/bit_matrix.hpp>
#include <urd/repr/graphs/bit_rows.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/repr/values/dense_set.hpp>
#include <urd/store.hpp>
#include <yggdrasil/core/config.hpp>

namespace tyr::kckp
{

struct Vertex
{
    ygg::uint_t index;

    constexpr Vertex() noexcept : index(std::numeric_limits<ygg::uint_t>::max()) {}
    constexpr explicit Vertex(ygg::uint_t index_) noexcept : index(index_) {}

    static constexpr auto identifying_members() noexcept { return urd::Members<&Vertex::index> {}; }

    friend constexpr bool operator==(Vertex, Vertex) noexcept = default;
};

/// A set of vertices: over the window of one part (Graph::empty_parts), or over all vertices.
using VertexSet = urd::Store<urd::set<Vertex>, urd::dense>;
/// A vertex set per part, say, each kept whole.
using VertexSets = urd::Store<urd::sequence<VertexSet>, urd::retains>;
/// The pairs of parts whose vertices are all adjacent: first * num_parts + second, both ways.
using PartPairs = urd::Store<urd::set<size_t>, urd::dense>;
/// The compatible pairs of a k-partite graph as they are found: rows of bits that grow.
using CompatibilityMatrix = urd::Store<urd::graph<Vertex>, urd::monotone, urd::partitioned, urd::undirected, urd::constant_adjacency>;
/// The compatible pairs once pruned: equal rows stored once.
using StaticRows = urd::Store<urd::graph<Vertex>, urd::frozen, urd::partitioned, urd::undirected, urd::constant_adjacency>;

/// A k-partite graph of compatible pairs, pruned to arc consistency, whose vertices carry labels (objects): a part's
/// vertices are numbered in the order of their labels.
class Graph
{
public:
    Graph();

    /// The graph of `matrix`, whose vertex v has label labels[v], pruned to arc consistency. The pairs of parts in
    /// `universal` are taken as all adjacent, whatever matrix holds of them.
    static Graph create(bool satisfiable, const CompatibilityMatrix& matrix, std::span<const ygg::uint_t> labels, const PartPairs& universal);

    [[nodiscard]] bool is_satisfiable() const noexcept { return m_satisfiable; }
    [[nodiscard]] size_t num_vertices() const { return m_rows.num_vertices(); }
    [[nodiscard]] size_t num_parts() const { return m_rows.num_parts(); }
    [[nodiscard]] ygg::uint_t part_of(Vertex v) const { return static_cast<ygg::uint_t>(m_rows.part_of(v)); }
    [[nodiscard]] auto part(size_t p) const { return m_rows.part(p); }
    [[nodiscard]] ygg::uint_t label(Vertex v) const { return m_labels[v.index]; }
    /// The vertex with `label` in part p, if any.
    [[nodiscard]] std::optional<Vertex> find_vertex(size_t p, ygg::uint_t label) const;
    [[nodiscard]] bool adjacent(Vertex u, Vertex v) const { return m_rows.adjacent(u, v); }
    [[nodiscard]] const StaticRows& rows() const noexcept { return m_rows; }
    /// The vertices of part p.
    [[nodiscard]] const VertexSet& all(size_t p) const { return m_all[p]; }
    /// An empty vertex set per part, over the part's window.
    [[nodiscard]] VertexSets empty_parts() const;
    /// The first vertex of each part, and one past the last vertex.
    [[nodiscard]] std::span<const size_t> bounds() const noexcept { return m_bounds; }

private:
    bool m_satisfiable { true };
    StaticRows m_rows;
    urd::Store<urd::sequence<ygg::uint_t>> m_labels;
    urd::Store<urd::sequence<size_t>> m_bounds;
    VertexSets m_all;
};

struct Workspace
{
    VertexSets candidates;     ///< the candidates at each depth in each part: depth * num_parts + part
    VertexSets anchor_source;  ///< per part, the rows of the prepared anchor source, see DeltaKCKP::prepare_anchor_source
    VertexSets scratch;        ///< per part
    urd::Store<urd::set<ygg::uint_t>, urd::dense> partition_bits;
    urd::Store<urd::sequence<Vertex>> partial_solution;
    ygg::uint_t partial_solution_size { 0 };
    ygg::uint_t anchor_pi { std::numeric_limits<ygg::uint_t>::max() };
    ygg::uint_t anchor_pj { std::numeric_limits<ygg::uint_t>::max() };
    urd::Store<urd::sequence<size_t>> bounds;  ///< the part bounds of the graph the sets are shaped for (Graph::bounds)

    Workspace() = default;
    explicit Workspace(const Graph& graph) { prepare(graph); }

    /// Shapes the sets after the parts of `graph`, unless they already are.
    void prepare(const Graph& graph);

    [[nodiscard]] VertexSet& candidates_at(size_t depth, size_t part) noexcept { return candidates[depth * partial_solution.size() + part]; }
    [[nodiscard]] const VertexSet& candidates_at(size_t depth, size_t part) const noexcept { return candidates[depth * partial_solution.size() + part]; }
};

namespace detail
{
template<typename SelectPartition, typename UpdateCandidates, typename Callback>
void complete_clique(const Graph& graph,
                     size_t depth,
                     Workspace& workspace,
                     SelectPartition& select_partition,
                     UpdateCandidates& update_candidates,
                     Callback& callback)
{
    const auto partition = select_partition(depth, workspace);
    if (partition == std::numeric_limits<ygg::uint_t>::max())
        return;

    for (const auto vertex : workspace.candidates_at(depth, partition))
    {
        workspace.partial_solution[partition] = vertex;
        ++workspace.partial_solution_size;

        if (workspace.partial_solution_size == graph.num_parts())
            callback(workspace.partial_solution);
        else
        {
            workspace.partition_bits.insert(partition);
            if (update_candidates(vertex, depth, workspace))
                complete_clique(graph, depth + 1, workspace, select_partition, update_candidates, callback);
            workspace.partition_bits.erase(partition);
        }

        --workspace.partial_solution_size;
    }
}
}

class KCKP
{
public:
    explicit KCKP(const Graph& graph) : m_graph(&graph) {}
    KCKP(Graph&&) = delete;

    template<typename Callback>
    void for_each_clique(Callback callback, Workspace& workspace) const
    {
        if (!m_graph->is_satisfiable() || !seed_from_prefix(std::span<const Vertex> {}, workspace))
            return;

        if (m_graph->num_parts() == 0)
        {
            callback(workspace.partial_solution);
            return;
        }

        complete_from_prefix(0, workspace, callback);
    }

    template<typename Callback>
    void for_each_compatible_extension(std::span<const Vertex> prefix, Workspace& workspace, Callback callback) const
    {
        assert(prefix.size() <= m_graph->num_parts());
        if (!m_graph->is_satisfiable() || !seed_from_prefix(prefix, workspace))
            return;

        if (prefix.size() == m_graph->num_parts())
        {
            callback(std::span<const Vertex> {});
            return;
        }

        const auto first_partition = static_cast<ygg::uint_t>(prefix.size());
        auto emit = [&](const urd::Store<urd::sequence<Vertex>>& clique) { callback(std::span<const Vertex>(clique).subspan(first_partition)); };
        complete_from_prefix(first_partition, workspace, emit);
    }

    [[nodiscard]] const Graph& get_graph() const noexcept { return *m_graph; }

private:
    bool seed_from_prefix(std::span<const Vertex> prefix, Workspace& workspace) const;

    [[nodiscard]] ygg::uint_t choose_next_partition(ygg::uint_t first_partition, const Workspace& workspace) const noexcept;

    bool update_candidates(ygg::uint_t first_partition, Vertex source, size_t depth, Workspace& workspace) const;

    template<typename Callback>
    void complete_from_prefix(ygg::uint_t first_partition, Workspace& workspace, Callback& callback) const
    {
        auto select_partition = [&](size_t, const Workspace& current) { return choose_next_partition(first_partition, current); };
        auto update_compatible = [&](Vertex source, size_t depth, Workspace& current) { return update_candidates(first_partition, source, depth, current); };
        detail::complete_clique(*m_graph, 0, workspace, select_partition, update_compatible, callback);
    }

    const Graph* m_graph;
};

}

#endif
