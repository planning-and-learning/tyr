/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef TYR_DATALOG_DATABASE_SEMANTICS_INCREMENTAL_SEARCH_TREE_REPAIR_HPP_
#define TYR_DATALOG_DATABASE_SEMANTICS_INCREMENTAL_SEARCH_TREE_REPAIR_HPP_

#include "tyr/datalog/lifted/database/semantics/search/concepts.hpp"
#include "tyr/datalog/lifted/database/semantics/search/search_tree.hpp"

#include <cstddef>
#include <span>
#include <urd/repr/pools/pools.hpp>
#include <urd/repr/queues/queue.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <utility>
#include <vector>

namespace tyr::datalog::database::incremental
{

/// Edges between vertices given by their values; each vertex is stored once.
class EdgeList
{
public:
    EdgeList() : m_vertices() {}

    void add(std::span<const ygg::uint_t> from, std::span<const ygg::uint_t> to)
    { m_edges.emplace_back(m_vertices.insert(from).first, m_vertices.insert(to).first); }
    void clear() noexcept
    {
        m_vertices.clear();
        m_edges.clear();
    }
    [[nodiscard]] bool empty() const noexcept { return m_edges.empty(); }
    [[nodiscard]] size_t size() const noexcept { return m_edges.size(); }
    [[nodiscard]] std::span<const ygg::uint_t> from(size_t i) const { return m_vertices.at(m_edges[i].first); }
    [[nodiscard]] std::span<const ygg::uint_t> to(size_t i) const { return m_vertices.at(m_edges[i].second); }

private:
    VertexSet m_vertices;
    urd::Store<urd::sequence<std::pair<VertexSet::handle_type, VertexSet::handle_type>>> m_edges;
};

/// Generalized Adaptive A*'s ReestablishConsistency (Sun, Koenig & Yeoh, "Generalized Adaptive A*", AAMAS 2008) on a
/// SearchTree kept across searches towards fixed targets, after `added` edges: a Dijkstra-like lowering of learned
/// distances over stored predecessors; unstored vertices keep the heuristic's estimate. Learned bounds stay admissible
/// and consistent with the heuristic, so removed edges keep them valid. A lowered vertex's next pointer is relinked to
/// the vertex that lowered it, as with the support pointers of Multipath Generalized Adaptive A* (Hernández, Asín &
/// Baier, AAAI 2015); SearchTree::on_valid_path() checks the link when it is used. The target of an added edge is
/// stored only if it lowers the edge's source. Owns its queue, so repeated repairs do not allocate.
class TreeRepair
{
public:
    /// Returns the number of lowered vertices.
    template<PredecessorOracle Predecessors, DistanceHeuristic H>
    size_t run(SearchTree& tree, const EdgeList& added, Predecessors& predecessors, const H& heuristic)
    {
        m_queue.clear();
        for (size_t i = 0; i < added.size(); ++i)
            if (const auto from = tree.find(added.from(i)); !from.is_none() && lowers(tree, from, tree.value(added.to(i), heuristic)))
                lower(tree, from, tree.ensure(added.to(i), heuristic));
        auto lowered = static_cast<size_t>(0);
        while (!m_queue.empty())
        {
            const auto [learned, id] = m_queue.top();
            m_queue.pop();
            if (learned != tree.learned(id))
                continue;
            ++lowered;
            m_vertex.assign(tree.vertex(id).begin(), tree.vertex(id).end());
            predecessors.predecessors(m_vertex,
                                      [&](std::span<const ygg::uint_t> from)
                                      {
                                          if (const auto stored = tree.find(from); !stored.is_none() && lowers(tree, stored, tree.learned(id)))
                                              lower(tree, stored, id);
                                          return true;
                                      });
        }
        return lowered;
    }

private:
    urd::Store<urd::queue<std::pair<ygg::uint_t, VertexSet::handle_type>>, urd::by_priority> m_queue;  ///< (learned, vertex), the least first.
    urd::Store<urd::sequence<ygg::uint_t>> m_vertex;  ///< A copy of the lowered vertex: storing vertices may move them.

    static bool lowers(const SearchTree& tree, VertexSet::handle_type from, ygg::uint_t to_value) noexcept
    { return to_value != infinity && tree.learned(from) > to_value + 1; }
    void lower(SearchTree& tree, VertexSet::handle_type from, VertexSet::handle_type to)
    {
        tree.lower(from, tree.learned(to) + 1);
        tree.link(from, to);
        m_queue.emplace(tree.learned(from), from);
    }
};

}  // namespace tyr::datalog::database::incremental

#endif
