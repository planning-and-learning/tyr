/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef TYR_DATALOG_DATABASE_SEMANTICS_SEARCH_SEARCH_TREE_HPP_
#define TYR_DATALOG_DATABASE_SEMANTICS_SEARCH_SEARCH_TREE_HPP_

#include "tyr/datalog/lifted/database/semantics/search/concepts.hpp"

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <urd/repr/pools/pools.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <vector>

namespace tyr::datalog::database
{

/// The stored vertices of searches towards fixed targets: per vertex a lower bound on its distance (learned), one next
/// pointer along a found path, and that path's length (an upper bound while its edges exist). A vertex whose next chain
/// reaches a target with lengths that drop by one per existing edge has a path of that length; where the length meets
/// a lower bound the distance is exact: the goal condition of Multipath Generalized Adaptive A* (Hernández, Asín &
/// Baier, "Reusing Previously Found A* Paths for Fast Goal-Directed Navigation in Dynamic Terrain", AAAI 2015), with
/// the path's length kept apart from the learned bound, which A* may learn below it. Edges of a chain are checked
/// lazily, and a check is kept until the graph's version changes.
///
/// The tree knows edges only through EdgeCheck. Goal searches keep it across searches and repair it
/// (incremental/search/tree_repair.hpp).
class SearchTree
{
public:
    SearchTree() : m_vertices() {}

    void clear()
    {
        m_vertices.clear();
        m_nodes.clear();
    }

    [[nodiscard]] size_t size() const noexcept { return m_nodes.size(); }
    /// The stored vertices; a vertex keeps its handle until clear().
    [[nodiscard]] const VertexSet& vertices() const noexcept { return m_vertices; }
    /// A stored vertex, or none.
    [[nodiscard]] VertexSet::handle_type find(std::span<const ygg::uint_t> vertex) const { return m_vertices.find(vertex); }
    [[nodiscard]] std::span<const ygg::uint_t> vertex(VertexSet::handle_type id) const { return m_vertices.at(id); }

    /// A stored vertex, inserted with its heuristic estimate as learned distance. The estimate is computed only on
    /// insertion.
    template<DistanceHeuristic H>
    VertexSet::handle_type ensure(std::span<const ygg::uint_t> vertex, const H& heuristic)
    {
        if (const auto stored = m_vertices.find(vertex); !stored.is_none())
            return stored;
        const auto estimate = heuristic.estimate(vertex);
        m_nodes.push_back(Node { .learned = estimate, .era = m_era });
        return m_vertices.insert(vertex).first;
    }
    /// The learned distance of a vertex: its stored value, or the heuristic's estimate if it is not stored.
    template<DistanceHeuristic H>
    [[nodiscard]] ygg::uint_t value(std::span<const ygg::uint_t> vertex, const H& heuristic) const
    {
        const auto stored = m_vertices.find(vertex);
        return stored.is_none() ? heuristic.estimate(vertex) : learned(stored);
    }

    /// 0 if forgotten since it was learned.
    [[nodiscard]] ygg::uint_t learned(VertexSet::handle_type id) const noexcept { return m_nodes[id].era == m_era ? m_nodes[id].learned : 0; }
    /// Raises the learned distance to a bound a search established (Generalized Adaptive A*).
    void learn(VertexSet::handle_type id, ygg::uint_t bound) noexcept { set_learned(id, std::max(learned(id), bound)); }
    /// Lowers the learned distance and drops the next pointer, after edges were added.
    void lower(VertexSet::handle_type id, ygg::uint_t bound) noexcept
    {
        set_learned(id, bound);
        set_next(id, {});
    }
    /// Forgets every learned distance: they may rest on bounds that no longer hold.
    void forget_learned() noexcept { ++m_era; }
    /// The next vertex along a found path, or none.
    [[nodiscard]] VertexSet::handle_type next(VertexSet::handle_type id) const noexcept { return m_nodes[id].next; }
    /// The length of the vertex's chain when it was linked; infinity without one.
    [[nodiscard]] ygg::uint_t path(VertexSet::handle_type id) const noexcept { return m_nodes[id].path; }
    /// A target: its chain is itself.
    void mark_target(VertexSet::handle_type id) noexcept
    {
        set_next(id, {});
        m_nodes[id].path = 0;
    }
    /// Links the vertex to the next one of its path, which is one longer than the next one's.
    void link(VertexSet::handle_type from, VertexSet::handle_type to) noexcept
    {
        set_next(from, to);
        m_nodes[from].path = m_nodes[to].path == infinity ? infinity : m_nodes[to].path + 1;
    }

    /// Starts a new search: chain checks of earlier searches are forgotten.
    void begin_search() noexcept { ++m_epoch; }

    /// Whether the next chain from `start` reaches a target with lengths that drop by one per existing edge: a path of
    /// length path(start). Edge checks hold until `version` changes.
    template<typename IsTarget, EdgeCheck Edges>
        requires std::predicate<const IsTarget&, VertexSet::handle_type>
    bool on_valid_path(VertexSet::handle_type start, const IsTarget& is_target, Edges& edges, uint64_t version)
    {
        auto& chain = m_chain;
        chain.clear();
        auto result = false;
        for (auto u = start;;)
        {
            if (m_nodes[u].checked == m_epoch)
            {
                result = m_nodes[u].valid;
                break;
            }
            chain.push_back(u);
            if (m_nodes[u].path == 0 && is_target(u))
            {
                result = true;
                break;
            }
            const auto v = m_nodes[u].next;
            if (v.is_none() || m_nodes[v].path == infinity || m_nodes[u].path != m_nodes[v].path + 1)
                break;
            if (m_nodes[u].edge_version != version)
            {
                if (!edges.edge_exists(vertex(u), vertex(v)))
                {
                    set_next(u, {});
                    break;
                }
                m_nodes[u].edge_version = version;
            }
            u = v;
        }
        for (const auto u : chain)
        {
            m_nodes[u].checked = m_epoch;
            m_nodes[u].valid = result;
        }
        return result;
    }

private:
    static constexpr uint64_t unchecked = std::numeric_limits<uint64_t>::max();

    struct Node
    {
        ygg::uint_t learned = 0;
        VertexSet::handle_type next {};
        ygg::uint_t path = infinity;        ///< The chain's length when it was linked.
        ygg::uint_t era = 0;                ///< Of learned: forget_learned() calls before it was set.
        uint64_t checked = 0;               ///< Epoch of the last chain check.
        bool valid = false;                 ///< Its result.
        uint64_t edge_version = unchecked;  ///< Graph version at which the edge to next was checked.
    };

    void set_learned(VertexSet::handle_type id, ygg::uint_t value) noexcept
    {
        m_nodes[id].learned = value;
        m_nodes[id].era = m_era;
    }
    /// A new next pointer has an unchecked edge; without one, there is no path.
    void set_next(VertexSet::handle_type id, VertexSet::handle_type next) noexcept
    {
        m_nodes[id].next = next;
        m_nodes[id].edge_version = unchecked;
        if (next.is_none())
            m_nodes[id].path = infinity;
    }

    VertexSet m_vertices;
    urd::Store<urd::sequence<Node, VertexSet::handle_type>> m_nodes;
    uint64_t m_epoch = 0;
    ygg::uint_t m_era = 0;
    urd::Store<urd::sequence<VertexSet::handle_type>> m_chain;  ///< Scratch of on_valid_path(), kept to avoid allocations.
};

}  // namespace tyr::datalog::database

#endif
