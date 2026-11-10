/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef TYR_DATALOG_DATABASE_SEMANTICS_SEARCH_SEARCH_TREE_HPP_
#define TYR_DATALOG_DATABASE_SEMANTICS_SEARCH_SEARCH_TREE_HPP_

#include <urd/repr/sequences/sequence.hpp>
#include <urd/repr/pools/pools.hpp>
#include <urd/store.hpp>
#include "tyr/datalog/lifted/database/semantics/search/concepts.hpp"

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <vector>

namespace tyr::datalog::database
{

/// The stored vertices of searches towards fixed targets: per vertex a lower bound on its distance (learned) and one
/// next pointer along a found path. A vertex whose next chain reaches a target, with values that drop by one per
/// existing edge, has an exact value: the goal condition of Multipath Generalized Adaptive A* (Hernández, Asín &
/// Baier, "Reusing Previously Found A* Paths for Fast Goal-Directed Navigation in Dynamic Terrain", AAAI 2015).
/// Edges of a chain are checked lazily, and a check is kept until the graph's version changes.
///
/// The tree knows edges only through EdgeCheck; the caller supplies the value used along a chain (e.g. the larger of
/// the learned distance and an exact backward level). incremental::MinDistance keeps it across searches and repairs it
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
    /// A stored vertex, or none.
    [[nodiscard]] VertexSet::handle_type find(std::span<const ygg::Value> vertex) const { return m_vertices.find(vertex); }
    [[nodiscard]] std::span<const ygg::Value> vertex(VertexSet::handle_type id) const { return m_vertices.at(id); }

    /// A stored vertex, inserted with its heuristic estimate as learned distance. The estimate is computed only on
    /// insertion.
    template<DistanceHeuristic H>
    VertexSet::handle_type ensure(std::span<const ygg::Value> vertex, const H& heuristic)
    {
        if (const auto stored = m_vertices.find(vertex); !stored.is_none())
            return stored;
        const auto estimate = heuristic.estimate(vertex);
        m_nodes.push_back(Node { estimate });
        return m_vertices.insert(vertex).first;
    }
    /// The learned distance of a vertex: its stored value, or the heuristic's estimate if it is not stored.
    template<DistanceHeuristic H>
    [[nodiscard]] ygg::uint_t value(std::span<const ygg::Value> vertex, const H& heuristic) const
    {
        const auto stored = m_vertices.find(vertex);
        return stored.is_none() ? heuristic.estimate(vertex) : learned(stored);
    }

    [[nodiscard]] ygg::uint_t learned(VertexSet::handle_type id) const noexcept { return m_nodes[id].learned; }
    /// Raises the learned distance to a bound a search established (Generalized Adaptive A*).
    void learn(VertexSet::handle_type id, ygg::uint_t bound) noexcept { m_nodes[id].learned = std::max(m_nodes[id].learned, bound); }
    /// Lowers the learned distance and drops the next pointer, after edges were added.
    void lower(VertexSet::handle_type id, ygg::uint_t bound) noexcept
    {
        m_nodes[id].learned = bound;
        set_next(id, {});
    }
    /// The next vertex along a found path, or none.
    [[nodiscard]] VertexSet::handle_type next(VertexSet::handle_type id) const noexcept { return m_nodes[id].next; }
    void link(VertexSet::handle_type from, VertexSet::handle_type to) noexcept { set_next(from, to); }

    /// Starts a new search: chain checks of earlier searches are forgotten.
    void begin_search() noexcept { ++m_epoch; }

    /// Whether the next chain from `start` reaches a target with values that drop by one per existing edge. Edge
    /// checks hold until `version` changes.
    template<typename Value, typename IsTarget, EdgeCheck Edges>
        requires std::invocable<const Value&, VertexSet::handle_type> && std::predicate<const IsTarget&, VertexSet::handle_type>
    bool on_valid_path(VertexSet::handle_type start, const Value& value, const IsTarget& is_target, Edges& edges, uint64_t version)
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
            if (value(u) == 0 && is_target(u))
            {
                result = true;
                break;
            }
            const auto v = m_nodes[u].next;
            if (v.is_none() || value(v) == infinity || value(u) != value(v) + 1)
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
        VertexSet::handle_type next;
        uint64_t checked = 0;               ///< Epoch of the last chain check.
        bool valid = false;                 ///< Its result.
        uint64_t edge_version = unchecked;  ///< Graph version at which the edge to next was checked.
    };

    /// A new next pointer has an unchecked edge.
    void set_next(VertexSet::handle_type id, VertexSet::handle_type next) noexcept
    {
        m_nodes[id].next = next;
        m_nodes[id].edge_version = unchecked;
    }

    VertexSet m_vertices;
    urd::Store<urd::sequence<Node, VertexSet::handle_type>> m_nodes;
    uint64_t m_epoch = 0;
    urd::Store<urd::sequence<VertexSet::handle_type>> m_chain;  ///< Scratch of on_valid_path(), kept to avoid allocations.
};

}  // namespace tyr::datalog::database

#endif
