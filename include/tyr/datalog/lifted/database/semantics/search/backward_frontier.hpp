/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef TYR_DATALOG_DATABASE_SEMANTICS_SEARCH_BACKWARD_FRONTIER_HPP_
#define TYR_DATALOG_DATABASE_SEMANTICS_SEARCH_BACKWARD_FRONTIER_HPP_

#include "tyr/datalog/lifted/database/semantics/search/concepts.hpp"
#include "tyr/datalog/lifted/database/semantics/search/search_tree.hpp"

#include <cstddef>
#include <optional>
#include <span>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <utility>
#include <vector>

namespace tyr::datalog::database
{

/// A breadth-first search backwards from the targets over the vertices of a SearchTree, grown one level at a time
/// while the forward search runs (the caller interleaves it by Pohl's cardinality comparison). Found vertices get their
/// exact distance as level and a next pointer to their BFS parent. Vertices not yet found are at least depth + 1 away,
/// and once the frontier is exhausted no other vertex can reach a target.
///
/// The found vertices form a perimeter around the targets as in perimeter search (Dillenburg & Nelson, "Perimeter
/// Search", Artificial Intelligence 1994; Manzini, "BIDA*", Artificial Intelligence 1995), but the perimeter grows during
/// the search, and only its depth bound is used outside it, not the front-to-front estimate min over perimeter
/// vertices p of h(v, p) + d(p).
///
/// These bounds form a consistent heuristic of their own. Edges removed after a level keep the levels lower bounds;
/// added edges invalidate them, and the caller restarts the search.
class BackwardFrontier
{
public:
    /// Starts from the targets (stored vertices), which become their own paths of length 0 (SearchTree::mark_target).
    /// The tree's learned distances are forgotten: A* learns them with the bounds of this frontier.
    void start(SearchTree& tree, std::span<const VertexSet::handle_type> targets);

    /// Advances one level; returns the number of vertices found.
    template<PredecessorOracle Predecessors, DistanceHeuristic H>
    size_t step(SearchTree& tree, Predecessors& predecessors, const H& heuristic)
    {
        std::swap(m_frontier, m_expanding);
        m_frontier.clear();
        const auto level = m_depth + 1;
        for (const auto parent : m_expanding)
        {
            // A copy: inserting predecessors into the tree may move its vertices.
            m_vertex.assign(tree.vertex(parent).begin(), tree.vertex(parent).end());
            predecessors.predecessors(m_vertex,
                                      [&](std::span<const ygg::Value> from)
                                      {
                                          const auto id = tree.ensure(from, heuristic);
                                          auto& found = level_slot(id);
                                          if (found != none)
                                              return true;
                                          found = level;
                                          tree.link(id, parent);
                                          m_frontier.push_back(id);
                                          return true;
                                      });
        }
        m_depth = level;
        m_exhausted = m_frontier.empty();
        return m_frontier.size();
    }

    [[nodiscard]] bool exhausted() const noexcept { return m_exhausted; }
    [[nodiscard]] size_t frontier_size() const noexcept { return m_frontier.size(); }
    /// The distance of a found vertex, or none: exact on the edges of the search that found it, a lower bound after
    /// edges are removed.
    [[nodiscard]] ygg::uint_t level(VertexSet::handle_type id) const noexcept { return id < m_level.position(m_level.end()) ? m_level[id] : none; }
    /// A lower bound on any vertex's distance (none: one not stored): its level if found, otherwise one beyond the last
    /// level, or infinity once the frontier is exhausted.
    [[nodiscard]] ygg::uint_t bound(VertexSet::handle_type id) const noexcept
    {
        if (const auto found = level(id); found != none)
            return found;
        return m_exhausted ? infinity : m_depth + 1;
    }

private:
    urd::Store<urd::sequence<ygg::uint_t, VertexSet::handle_type>> m_level;  ///< By the tree's vertex.
    urd::Store<urd::sequence<VertexSet::handle_type>> m_frontier;
    urd::Store<urd::sequence<VertexSet::handle_type>> m_expanding;  ///< The level step() expands; kept with m_vertex to avoid allocations.
    urd::Store<urd::sequence<ygg::Value>> m_vertex;
    ygg::uint_t m_depth = 0;
    bool m_exhausted = false;

    ygg::uint_t& level_slot(VertexSet::handle_type id)
    {
        while (!(id < m_level.position(m_level.end())))
            m_level.push_back(none);
        return m_level[id];
    }
};

}  // namespace tyr::datalog::database

#endif
