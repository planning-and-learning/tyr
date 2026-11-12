/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef TYR_DATALOG_DATABASE_SEMANTICS_SEARCH_A_STAR_HPP_
#define TYR_DATALOG_DATABASE_SEMANTICS_SEARCH_A_STAR_HPP_

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
#include <urd/repr/pools/pools.hpp>
#include <urd/repr/queues/queue.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
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
    ygg::uint_t push = 0;          ///< Push order.
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
        ygg::uint_t h = 0;  ///< The A* estimate when queued; infinity once the backward bound excludes the vertex.
        VertexSet::handle_type parent {};
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
/// caller's heuristic h, the tree's learned distance and the backward frontier's bound, all admissible; a closed
/// vertex reached again more cheaply is reopened, as A* with an admissible estimate needs, which never happens while
/// the estimate is consistent. Within an f, larger g is expanded first (OpenEntry).
///
/// The search is bidirectional: before each expansion the backward frontier advances while it is not larger than the
/// forward open list (Pohl's cardinality comparison, "Bi-directional Search", Machine Intelligence 6, 1971). It ends at
/// a target, at a stored vertex on a valid tree path whose length meets the vertex's lower bound (MPGAA*'s goal
/// condition, with the path's length as an upper bound apart from the learned lower bound), once the least f reaches
/// the shortest path through a stored vertex on a valid tree path seen (an incumbent), or when the exhausted backward
/// frontier excludes every source, or the heuristic proves every source unreachable (ProvesUnreachable, which also
/// prunes the search once it is as large as the proof). With `any_path`, a caller that needs only whether a target is
/// reachable gets the length of some path: the search also ends at the first stored vertex on a valid tree path, a
/// source included.
///
/// Learning (GAA*: h(s) := m - g(s) for closed s) uses m = min(d, the smallest f of an open vertex) instead of the
/// distance d, with the whole estimate, the backward bound included: a consistent heuristic of its own, so the learned
/// values stay consistent with the estimates of unstored vertices, which TreeRepair relies on, and where the backward
/// frontier ended the search they are the path's exact distances. They hold for as long as the bound does: a new
/// backward frontier forgets them (BackwardFrontier::start). [Our addition.]
template<typename Oracle, DistanceHeuristic H>
    requires SuccessorOracle<Oracle> && PredecessorOracle<Oracle> && EdgeCheck<Oracle>
class AStar
{
public:
    AStar(Oracle& oracle,
          SearchTree& tree,
          BackwardFrontier& backward,
          const H& heuristic,
          const VertexSet& targets,
          AStarStorage& storage,
          uint64_t version,
          bool any_path = false) :
        m_oracle(&oracle),
        m_tree(&tree),
        m_backward(&backward),
        m_heuristic(&heuristic),
        m_targets(&targets),
        m_storage(&storage),
        m_version(version),
        m_any_path(any_path)
    { m_storage->clear(); }
    AStar(const AStar&) = delete;
    AStar& operator=(const AStar&) = delete;
    AStar(AStar&&) = delete;  // One search, run in place.
    AStar& operator=(AStar&&) = delete;
    ~AStar() = default;

    /// The distance from the nearest source; with `any_path`, the length of a path from a source.
    template<VertexRange Sources>
    ygg::uint_t run(const Sources& sources)
    {
        m_tree->begin_search();
        if (closed_off(sources))
            return infinity;
        if (const auto distance = reuse(sources))
            return *distance;
        for (size_t i = 0; i < sources.size(); ++i)
            push(source(sources, i), 0, {});
        while (!m_storage->m_open_list.empty())
        {
            if (advance_backward(sources))
                return finish_incumbent();
            const auto entry = m_storage->m_open_list.top();
            m_storage->m_open_list.pop();
            if (entry.f >= m_incumbent)
                return finish_incumbent();
            if (const auto distance = visit(entry.index, entry.f))
                return *distance;
            if (proven_unreachable(sources))
                return finish(infinity, {});
        }
        return finish_incumbent();
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
                on_vertex(m_storage->m_vertices.at(index));
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
    bool m_any_path;
    size_t m_open = 0;
    VertexSet::handle_type m_last;             ///< The generated vertex the search ended at, if it ended at one.
    VertexSet::handle_type m_reused;           ///< The stored source whose tree path ended the search without expanding, if any.
    ygg::uint_t m_incumbent = infinity;        ///< The shortest g + tree path length of a generated vertex seen.
    VertexSet::handle_type m_incumbent_index;  ///< That vertex.
    size_t m_expanded = 0;
    bool m_pruning = false;
    ygg::uint_t m_dropped = infinity;  ///< The smallest g + h of a vertex pruned as unreachable in the current inputs only.
    AStarCounters m_counters;

    /// The values of the i-th source, borrowed until the next call.
    template<VertexRange Sources>
    [[nodiscard]] std::span<const ygg::Value> source(const Sources& sources, size_t i) const
    { return vertex_values(sources.row(i), m_storage->m_source); }

    /// The larger lower bound of a stored vertex; exact once its next chain is valid.
    [[nodiscard]] ygg::uint_t value(VertexSet::handle_type id) const noexcept
    {
        const auto level = m_backward->level(id);
        return level == none ? m_tree->learned(id) : std::max(m_tree->learned(id), level);
    }

    bool on_valid_path(VertexSet::handle_type id)
    {
        return m_tree->on_valid_path(id, [&](VertexSet::handle_type u) { return m_targets->contains(m_tree->vertex(u)); }, *m_oracle, m_version);
    }

    /// Ends with the incumbent, or without a path.
    ygg::uint_t finish_incumbent()
    {
        if (m_incumbent == infinity)
            return finish(infinity, {});
        ++m_counters.tree_hits;
        return finish(m_incumbent, m_incumbent_index);
    }

    /// Whether the exhausted backward frontier found no source.
    template<VertexRange Sources>
    [[nodiscard]] bool closed_off(const Sources& sources) const
    {
        return m_backward->exhausted()
               && std::ranges::none_of(std::views::iota(static_cast<size_t>(0), sources.size()),
                                       [&](size_t i) { return m_backward->level(m_tree->find(source(sources, i))) != none; });
    }

    /// A single stored source on a valid tree path whose length meets its lower bound has that distance; a learned
    /// infinity proves it dead. With `any_path`, any stored source on a valid tree path ends the search.
    template<VertexRange Sources>
    std::optional<ygg::uint_t> reuse(const Sources& sources)
    {
        if (m_any_path)
            for (size_t i = 0; i < sources.size(); ++i)
                if (const auto stored = m_tree->find(source(sources, i)); !stored.is_none() && on_valid_path(stored))
                {
                    ++m_counters.tree_hits;
                    m_reused = stored;
                    return m_tree->path(stored);
                }
        if (sources.size() != 1)
            return std::nullopt;
        const auto vertex = source(sources, 0);
        const auto stored = m_tree->find(vertex);
        if (stored.is_none())
            return std::nullopt;
        if (m_tree->learned(stored) == infinity)
            return infinity;
        const auto lower = std::max({ value(stored), m_heuristic->estimate(vertex), m_backward->bound(stored) });
        if (!on_valid_path(stored) || m_tree->path(stored) != lower)
            return std::nullopt;
        ++m_counters.tree_hits;
        m_reused = stored;
        return lower;
    }

    /// Cardinality comparison; returns whether the exhausted backward frontier excludes every source.
    template<VertexRange Sources>
    bool advance_backward(const Sources& sources)
    {
        while (!m_backward->exhausted() && m_backward->frontier_size() <= m_open)
        {
            const auto found = m_backward->step(*m_tree, *m_oracle, *m_heuristic);
            m_counters.backward_vertices += found;
            if (found != 0)
                relinked();
            if (closed_off(sources))
                return true;
        }
        return false;
    }

    /// The backward frontier links the vertices it finds, which may change stored chains: the chain checks made so far
    /// are discarded, and the incumbent is kept only while its chain still holds.
    void relinked()
    {
        m_tree->begin_search();
        if (m_incumbent_index.is_none())
            return;
        const auto id = stored(m_incumbent_index);
        if (on_valid_path(id))
            m_incumbent = node(m_incumbent_index).g + m_tree->path(id);
        else
        {
            m_incumbent = infinity;
            m_incumbent_index = {};
        }
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
            id = m_tree->find(m_storage->m_vertices.at(index));
        return id;
    }

    void push(std::span<const ygg::Value> vertex, ygg::uint_t cost, VertexSet::handle_type from)
    {
        auto index = m_storage->m_vertices.find(vertex);
        if (!index.is_none() && node(index).g <= cost)
            return;
        if (!index.is_none() && node(index).closed)
        {
            node(index).closed = false;
            ++m_open;
        }
        if (index.is_none())
        {
            auto estimate = m_heuristic->estimate(vertex);
            assert(from.is_none() || estimate == infinity || m_heuristic->estimate(m_storage->m_vertices.at(from)) <= estimate + 1);  // Consistency.
            const auto stored = m_tree->find(vertex);
            if (!stored.is_none())
                estimate = std::max(estimate, m_tree->learned(stored));
            if (estimate == infinity)
                return;
            const auto bound = m_backward->bound(stored);
            if (bound == infinity || pruned(vertex, cost + std::max(estimate, bound)))
                return;
            index = m_storage->m_vertices.insert(vertex).first;
            m_storage->m_nodes.push_back(AStarStorage::Node { .h = std::max(estimate, bound), .stored = stored });
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
        if (m_targets->contains(m_storage->m_vertices.at(index)))
            return finish(f, index);
        if (pruned(m_storage->m_vertices.at(index), current.g + current.h))
        {
            current.closed = true;
            --m_open;
            return std::nullopt;
        }
        const auto id = stored(index);
        // The backward frontier may have raised its bound since the vertex was queued.
        const auto bound = m_backward->bound(id);
        if (bound == infinity)
        {
            current.closed = true;
            current.h = infinity;
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
            const auto length = m_tree->path(id);
            if (length == current.h)
            {
                ++m_counters.tree_hits;
                return finish(current.g + length, index);
            }
            if (m_any_path)
            {
                ++m_counters.tree_hits;
                const auto last = last_generated(index);
                return finish(node(last).g + m_tree->path(stored(last)), last);
            }
            if (current.g + length < m_incumbent)
            {
                m_incumbent = current.g + length;
                m_incumbent_index = index;
            }
        }
        expand(index);
        return std::nullopt;
    }

    /// The last generated vertex on the tree path from the stored vertex `index`, or `index`: a path that is not a
    /// shortest one may meet its own generated vertices on the tree path, and ending at the last one keeps it simple,
    /// so that linking it into the tree makes no cycle.
    VertexSet::handle_type last_generated(VertexSet::handle_type index)
    {
        auto result = index;
        for (auto id = m_tree->next(stored(index)); !id.is_none(); id = m_tree->next(id))
            if (const auto found = m_storage->m_vertices.find(m_tree->vertex(id)); !found.is_none())
                result = found;
        return result;
    }

    /// ProvesUnreachable: once the search has expanded proof_cost() vertices, it prunes the vertices that the heuristic
    /// proves unreachable in the current inputs, and returns whether it proves every source so. Learning counts the
    /// pruned vertices with their estimates (m_dropped): in other inputs they may reach a target.
    template<VertexRange Sources>
    bool proven_unreachable(const Sources& sources)
    {
        if constexpr (ProvesUnreachable<const H>)
        {
            if (m_pruning || m_expanded < m_heuristic->proof_cost())
                return false;
            m_pruning = true;
            return std::ranges::all_of(std::views::iota(static_cast<size_t>(0), sources.size()),
                                       [&](size_t i) { return m_heuristic->unreachable(source(sources, i)); });
        }
        else
            return false;
    }

    /// Whether pruning is on and the vertex, reached at f = `f`, is proven unreachable; then it counts for learning.
    bool pruned(std::span<const ygg::Value> vertex, ygg::uint_t f)
    {
        if constexpr (ProvesUnreachable<const H>)
        {
            if (!m_pruning || !m_heuristic->unreachable(vertex))
                return false;
            m_dropped = std::min(m_dropped, f);
            return true;
        }
        else
            return false;
    }

    void expand(VertexSet::handle_type index)
    {
        ++m_expanded;
        node(index).closed = true;
        --m_open;
        const auto vertex = m_storage->m_vertices.at(index);
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
    /// learn more than h are stored: an unstored vertex's value is the estimate, so storing one that learns nothing
    /// would record that estimate again. The stored values remain consistent: a closed vertex x that learns nothing has
    /// m <= g(x) + h(x), so a closed predecessor p learns m - g(p) <= 1 + h(x). As in LRTA*'s table of updated values
    /// (Korf, "Real-Time Heuristic Search", AIJ 1990), the tree holds only what was learned.
    ygg::uint_t finish(ygg::uint_t distance, VertexSet::handle_type last)
    {
        const auto& nodes = m_storage->m_nodes;
        auto m = std::min(distance, m_dropped);
        for (const auto& open : nodes)
            if (!open.closed && open.h != infinity)
                m = std::min(m, open.g + open.h);
        for (auto it = nodes.begin(); it != nodes.end(); ++it)
            if (it->closed && m > it->g)
                if (const auto learned = m == infinity ? infinity : m - it->g; learned > it->h)
                    m_tree->learn(ensure(nodes.position(it)), learned);
        m_last = last;
        if (!last.is_none())
        {
            // The path's lengths from where it ends: 0 at a target, or the stored chain's.
            if (m_targets->contains(m_storage->m_vertices.at(last)))
                m_tree->mark_target(ensure(last));
            for (auto child = last; !nodes[child].parent.is_none(); child = nodes[child].parent)
                m_tree->link(ensure(nodes[child].parent), ensure(child));
        }
        return distance;
    }

    VertexSet::handle_type ensure(VertexSet::handle_type index)
    {
        if (const auto id = stored(index); !id.is_none())
            return id;
        return node(index).stored = m_tree->ensure(m_storage->m_vertices.at(index), *m_heuristic);
    }
};

}  // namespace tyr::datalog::database

#endif
