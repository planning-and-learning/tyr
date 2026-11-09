/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef TYR_DATALOG_DATABASE_SEMANTICS_SEARCH_A_STAR_HPP_
#define TYR_DATALOG_DATABASE_SEMANTICS_SEARCH_A_STAR_HPP_

#include <urd/repr/sequences/sequence.hpp>
#include <urd/repr/pools/pools.hpp>
#include <urd/repr/queues/queue.hpp>
#include <urd/store.hpp>
#include "tyr/datalog/lifted/database/semantics/search/backward_frontier.hpp"
#include "tyr/datalog/lifted/database/semantics/search/concepts.hpp"
#include "tyr/datalog/lifted/database/semantics/search/search_tree.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <ranges>
#include <span>
#include <vector>

namespace tyr::datalog::database
{

/// Counters of one A* search.
struct AStarCounters
{
    size_t tree_hits = 0;          ///< Searches ended on a stored path.
    size_t backward_vertices = 0;  ///< Vertices the backward frontier found.

    AStarCounters& operator+=(const AStarCounters& other) noexcept
    {
        tree_hits += other.tree_hits;
        backward_vertices += other.backward_vertices;
        return *this;
    }
};

/// An entry of the open list of A*: popped by smallest f, then largest g, then latest push, the order of a two-level
/// bucket queue whose buckets are stacks. Preferring large g (and the latest of equals) among equal f is the
/// tie-breaking that leaves plateaus towards the targets (Asai & Fukunaga, "Tiebreaking Strategies for A* Search: How
/// to Explore the Final Frontier", JAIR 2017).
struct OpenEntry
{
    ygg::uint_t f = 0;
    ygg::uint_t g = 0;
    ygg::uint_t push = 0;               ///< Push order.
    VertexSet::handle_type index;  ///< The generated vertex.
};

/// Whether lhs is popped before rhs: a total order, since pushes are unique.
struct PoppedFirst
{
    bool operator()(const OpenEntry& lhs, const OpenEntry& rhs) const noexcept
    {
        if (lhs.f != rhs.f)
            return lhs.f < rhs.f;
        if (lhs.g != rhs.g)
            return lhs.g > rhs.g;
        return lhs.push > rhs.push;
    }
};

template<typename Oracle, DistanceHeuristic H>
    requires SuccessorOracle<Oracle> && PredecessorOracle<Oracle> && EdgeCheck<Oracle>
class AStar;

/// The generated vertices of one A* search and its open list, owned by the caller and reused across searches, so that
/// a search allocates only when it generates more vertices than every earlier one. Opaque to the caller: m_nodes is
/// addressed by m_vertices' handles, which only AStar maintains.
class AStarStorage
{
public:
    AStarStorage() : m_vertices() {}

private:
    template<typename Oracle, DistanceHeuristic H>
        requires SuccessorOracle<Oracle> && PredecessorOracle<Oracle> && EdgeCheck<Oracle>
    friend class AStar;

    /// One generated vertex.
    struct Node
    {
        ygg::uint_t g = 0;
        ygg::uint_t h = 0;        ///< The A* estimate when queued.
        ygg::uint_t h_learn = 0;  ///< max(heuristic, learned): the estimate learning may rely on.
        VertexSet::handle_type parent;
        VertexSet::handle_type stored;  ///< The tree's vertex, or none if the vertex was not stored when last looked up.
        bool closed = false;
    };

    void clear() noexcept
    {
        m_vertices.clear();
        m_nodes.clear();
        m_open_list.clear();
        m_pushes = 0;
    }

    VertexSet m_vertices;
    urd::Store<urd::sequence<Node, VertexSet::handle_type>> m_nodes;
    /// A binary heap in one vector, so clear() keeps its capacity and a reused open list does not allocate.
    urd::Store<urd::queue<OpenEntry>, urd::by_priority, urd::order_by<PoppedFirst>> m_open_list;
    ygg::uint_t m_pushes = 0;
    urd::Store<urd::sequence<ygg::Value>> m_vertex;  ///< A copy of the expanded vertex: generating successors may move m_vertices.
    urd::Store<urd::sequence<ygg::Value>> m_source;  ///< A source's values, read from its row.
};

/// One best-first search from sources to the targets with unit edge costs (Hart, Nilsson & Raphael, "A Formal Basis
/// for the Heuristic Determination of Minimum Cost Paths", IEEE Trans. SSC 1968). The estimate is the maximum of the
/// caller's heuristic h, the tree's learned distance and the backward frontier's bound; all three are consistent, so a
/// vertex is closed once. Within an f, larger g is expanded first (OpenEntry).
///
/// The search is bidirectional: before each expansion the backward frontier advances while it is not larger than the
/// forward open list (Pohl's cardinality comparison, "Bi-directional Search", Machine Intelligence 6, 1971). It ends at
/// a target, at a stored vertex on a valid tree path (MPGAA*'s goal condition), or when the exhausted backward frontier
/// excludes every source.
///
/// Learning (GAA*: h(s) := m - g(s) for closed s) uses m = min(d, the smallest g + h_learn of an open or dropped
/// vertex) instead of the distance d, where h_learn = max(h, learned) omits the backward bound: a search that ends
/// early on a backward level, or on an exhausted frontier, then still learns bounds that are consistent without the
/// backward frontier, which is discarded when added edges break its bounds. [Our addition.]
template<typename Oracle, DistanceHeuristic H>
    requires SuccessorOracle<Oracle> && PredecessorOracle<Oracle> && EdgeCheck<Oracle>
class AStar
{
public:
    AStar(Oracle& oracle, SearchTree& tree, BackwardFrontier& backward, const H& heuristic, const VertexSet& targets, AStarStorage& storage, uint64_t version) :
        m_oracle(&oracle),
        m_tree(&tree),
        m_backward(&backward),
        m_heuristic(&heuristic),
        m_targets(&targets),
        m_storage(&storage),
        m_version(version)
    { m_storage->clear(); }
    AStar(const AStar&) = delete;
    AStar& operator=(const AStar&) = delete;
    AStar(AStar&&) = delete;  // One search, run in place.
    AStar& operator=(AStar&&) = delete;
    ~AStar() = default;

    /// The distance from the nearest source.
    template<VertexRange Sources>
    ygg::uint_t run(const Sources& sources)
    {
        m_tree->begin_search();
        if (closed_off(sources))
            return infinity;
        if (const auto distance = reuse(sources))
            return *distance;
        for (size_t i = 0; i < sources.size(); ++i)
            push(vertex_values(sources.row(i), m_storage->m_source), 0, {});
        while (!m_storage->m_open_list.empty())
        {
            if (advance_backward(sources))
                return finish(infinity, {});
            const auto entry = m_storage->m_open_list.top();
            m_storage->m_open_list.pop();
            if (const auto distance = visit(entry.index, entry.f))
                return *distance;
        }
        return finish(infinity, {});
    }

    [[nodiscard]] const AStarCounters& counters() const noexcept { return m_counters; }

    /// Calls on_vertex(v) for each vertex v of the path found by run(), which returned a finite distance: the generated
    /// vertices from a source to where the search ended, then the tree's next chain from there to a target.
    template<typename OnVertex>
    void path(const OnVertex& on_vertex) const
    {
        auto end = m_reused;
        if (!m_last.is_none())
        {
            for (auto index = m_last; !index.is_none(); index = m_storage->m_nodes[index].parent)
                on_vertex(m_storage->m_vertices.view(index));
            end = m_storage->m_nodes[m_last].stored;
        }
        if (end.is_none())
            return;
        if (m_last.is_none())
            on_vertex(m_tree->vertex(end));
        for (auto id = m_tree->next(end); !id.is_none(); id = m_tree->next(id))
            on_vertex(m_tree->vertex(id));
    }

private:
    Oracle* m_oracle;
    SearchTree* m_tree;
    BackwardFrontier* m_backward;
    const H* m_heuristic;
    const VertexSet* m_targets;
    AStarStorage* m_storage;
    uint64_t m_version;
    size_t m_open = 0;
    ygg::uint_t m_dropped = infinity;  ///< The smallest g + h_learn among vertices dropped by the backward bound alone.
    VertexSet::handle_type m_last;    ///< The generated vertex the search ended at, if it ended at one.
    VertexSet::handle_type m_reused;  ///< The stored source whose tree path ended the search without expanding, if any.
    AStarCounters m_counters;

    /// The larger lower bound of a stored vertex; exact once its next chain is valid.
    [[nodiscard]] ygg::uint_t value(VertexSet::handle_type id) const noexcept
    {
        const auto level = m_backward->level(id);
        return level == none ? m_tree->learned(id) : std::max(m_tree->learned(id), level);
    }

    bool on_valid_path(VertexSet::handle_type id)
    {
        return m_tree->on_valid_path(
            id,
            [&](VertexSet::handle_type u) { return value(u); },
            [&](VertexSet::handle_type u) { return m_targets->contains(m_tree->vertex(u)); },
            *m_oracle,
            m_version);
    }

    /// Whether the exhausted backward frontier found no source.
    template<VertexRange Sources>
    [[nodiscard]] bool closed_off(const Sources& sources) const
    {
        return m_backward->exhausted()
               && std::ranges::none_of(std::views::iota(static_cast<size_t>(0), sources.size()),
                                       [&](size_t i) { return m_backward->level(m_tree->find(vertex_values(sources.row(i), m_storage->m_source))) != none; });
    }

    /// A single stored source on a valid tree path has an exact value; a learned infinity proves it dead.
    template<VertexRange Sources>
    std::optional<ygg::uint_t> reuse(const Sources& sources)
    {
        if (sources.size() != 1)
            return std::nullopt;
        const auto stored = m_tree->find(vertex_values(sources.row(0), m_storage->m_source));
        if (stored.is_none())
            return std::nullopt;
        if (m_tree->learned(stored) == infinity)
            return infinity;
        if (!on_valid_path(stored))
            return std::nullopt;
        ++m_counters.tree_hits;
        m_reused = stored;
        return value(stored);
    }

    /// Cardinality comparison; returns whether the exhausted backward frontier excludes every source.
    template<VertexRange Sources>
    bool advance_backward(const Sources& sources)
    {
        while (!m_backward->exhausted() && m_backward->frontier_size() <= m_open)
        {
            m_counters.backward_vertices += m_backward->step(*m_tree, *m_oracle, *m_heuristic);
            if (closed_off(sources))
                return true;
        }
        return false;
    }

    AStarStorage::Node& node(VertexSet::handle_type index) noexcept { return m_storage->m_nodes[index]; }

    void enqueue(VertexSet::handle_type index)
    { m_storage->m_open_list.push(OpenEntry { node(index).g + node(index).h, node(index).g, m_storage->m_pushes++, index }); }

    /// The tree's vertex of a generated vertex, or none. A vertex not stored when generated is looked up again: the
    /// backward frontier may have stored it since.
    VertexSet::handle_type stored(VertexSet::handle_type index)
    {
        auto& id = node(index).stored;
        if (id.is_none())
            id = m_tree->find(m_storage->m_vertices.view(index));
        return id;
    }

    void push(std::span<const ygg::Value> vertex, ygg::uint_t cost, VertexSet::handle_type from)
    {
        auto index = m_storage->m_vertices.find(vertex);
        if (!index.is_none() && (node(index).closed || node(index).g <= cost))
            return;
        if (index.is_none())
        {
            auto estimate = m_heuristic->estimate(vertex);
            assert(from.is_none() || estimate == infinity || m_heuristic->estimate(m_storage->m_vertices.view(from)) <= estimate + 1);  // Consistency.
            const auto stored = m_tree->find(vertex);
            if (!stored.is_none())
                estimate = std::max(estimate, m_tree->learned(stored));
            if (estimate == infinity)
                return;
            const auto bound = m_backward->bound(stored);
            if (bound == infinity)
            {
                m_dropped = std::min(m_dropped, cost + estimate);
                return;
            }
            index = m_storage->m_vertices.insert(vertex).first;
            m_storage->m_nodes.push_back(AStarStorage::Node { .h = std::max(estimate, bound), .h_learn = estimate, .stored = stored });
            ++m_open;
        }
        node(index).g = cost;
        node(index).parent = from;
        enqueue(index);
    }

    /// Pops a vertex queued at f; returns the distance if the search ends there.
    std::optional<ygg::uint_t> visit(VertexSet::handle_type index, ygg::uint_t f)
    {
        auto& current = node(index);
        if (current.closed || current.g + current.h != f)
            return std::nullopt;
        if (m_targets->contains(m_storage->m_vertices.view(index)))
            return finish(f, index);
        const auto id = stored(index);
        // The backward frontier may have raised its bound since the vertex was queued.
        const auto bound = m_backward->bound(id);
        if (bound == infinity)
        {
            m_dropped = std::min(m_dropped, current.g + current.h_learn);
            current.closed = true;
            current.h_learn = infinity;
            --m_open;
            return std::nullopt;
        }
        if (bound > current.h)
        {
            current.h = bound;
            enqueue(index);
            return std::nullopt;
        }
        // It stays open for learning: its successors were not generated.
        if (!id.is_none() && on_valid_path(id))
        {
            ++m_counters.tree_hits;
            return finish(current.g + value(id), index);
        }
        expand(index);
        return std::nullopt;
    }

    void expand(VertexSet::handle_type index)
    {
        node(index).closed = true;
        --m_open;
        const auto vertex = m_storage->m_vertices.view(index);
        m_storage->m_vertex.assign(vertex.begin(), vertex.end());
        const auto cost = node(index).g + 1;
        m_oracle->successors(m_storage->m_vertex,
                             [&](std::span<const ygg::Value> to)
                             {
                                 push(to, cost, index);
                                 return true;
                             });
    }

    /// Learns for closed vertices (see the class comment) and links the found path into the tree. Only vertices that
    /// learn more than h_learn are stored: an unstored vertex's value is the heuristic's estimate, so storing one that
    /// learns nothing would record that estimate again. The stored values remain consistent: a closed vertex x that
    /// learns nothing has m <= g(x) + h(x), so a closed predecessor p learns m - g(p) <= 1 + h(x). As in LRTA*'s
    /// table of updated values (Korf, "Real-Time Heuristic Search", AIJ 1990), the tree holds only what was learned.
    ygg::uint_t finish(ygg::uint_t distance, VertexSet::handle_type last)
    {
        const auto& nodes = m_storage->m_nodes;
        auto m = std::min(distance, m_dropped);
        for (const auto& open : nodes)
            if (!open.closed && open.h_learn != infinity)
                m = std::min(m, open.g + open.h_learn);
        for (auto it = nodes.begin(); it != nodes.end(); ++it)
            if (it->closed && m > it->g)
                if (const auto learned = m == infinity ? infinity : m - it->g; learned > it->h_learn)
                    m_tree->learn(ensure(nodes.position(it)), learned);
        m_last = last;
        if (!last.is_none())
            for (auto child = last; !nodes[child].parent.is_none(); child = nodes[child].parent)
                m_tree->link(ensure(nodes[child].parent), ensure(child));
        return distance;
    }

    VertexSet::handle_type ensure(VertexSet::handle_type index)
    {
        if (const auto id = stored(index); !id.is_none())
            return id;
        return node(index).stored = m_tree->ensure(m_storage->m_vertices.view(index), *m_heuristic);
    }
};

}  // namespace tyr::datalog::database

#endif
