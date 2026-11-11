/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef TYR_DATALOG_DATABASE_OPTIMIZATION_DETAILS_DPCCP_HPP_
#define TYR_DATALOG_DATABASE_OPTIMIZATION_DETAILS_DPCCP_HPP_

#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <stdexcept>
#include <urd/bits.hpp>
#include <utility>

namespace tyr::datalog::database::detail
{
/// The most vertices the 64-bit vertex sets represent.
constexpr size_t max_dpccp_vertices = 63;

template<typename Callback>
class DpccpEnumerator
{
    std::span<const std::uint64_t> m_adjacency;
    Callback* m_emit;

    [[nodiscard]] std::uint64_t neighbors(std::uint64_t vertices) const
    {
        std::uint64_t result = 0;
        urd::bits::for_each_set_bit(vertices, [&](size_t vertex) { result |= m_adjacency[vertex]; });
        return result & ~vertices;
    }

    static std::uint64_t through(size_t vertex) { return urd::bits::lo_set<std::uint64_t>[vertex + 1]; }

    bool expand_complement(std::uint64_t left, std::uint64_t right, std::uint64_t excluded)
    {
        const auto next = neighbors(right) & ~excluded;
        // Ascending masks visit every subset before its supersets.
        for (auto subset = (-next) & next; subset; subset = (subset - next) & next)
            if (!std::invoke(*m_emit, left, right | subset))
                return false;
        for (auto subset = (-next) & next; subset; subset = (subset - next) & next)
            if (!expand_complement(left, right | subset, excluded | next))
                return false;
        return true;
    }

    bool emit_connected(std::uint64_t left)
    {
        const auto excluded = left | through(std::countr_zero(left));
        const auto next = neighbors(left) & ~excluded;
        for (auto rest = next; rest;)
        {
            const auto vertex = std::bit_width(rest) - 1;
            const auto right = std::uint64_t { 1 } << vertex;
            rest &= ~right;
            if (!std::invoke(*m_emit, left, right) || !expand_complement(left, right, excluded | (next & through(vertex))))
                return false;
        }
        return true;
    }

    bool expand_connected(std::uint64_t vertices, std::uint64_t excluded)
    {
        const auto next = neighbors(vertices) & ~excluded;
        for (auto subset = (-next) & next; subset; subset = (subset - next) & next)
            if (!emit_connected(vertices | subset))
                return false;
        for (auto subset = (-next) & next; subset; subset = (subset - next) & next)
            if (!expand_connected(vertices | subset, excluded | next))
                return false;
        return true;
    }

public:
    DpccpEnumerator(std::span<const std::uint64_t> adjacency, Callback& emit) : m_adjacency(adjacency), m_emit(&emit) {}

    bool run()
    {
        for (size_t vertex = m_adjacency.size(); vertex-- > 0;)
        {
            const auto singleton = std::uint64_t { 1 } << vertex;
            if (!emit_connected(singleton) || !expand_connected(singleton, through(vertex)))
                return false;
        }
        return true;
    }
};

/// DPccp connected-subgraph/complement enumeration, Moerkotte & Neumann (2006),
/// https://www.vldb.org/conf/2006/p930-moerkotte.pdf, with the corrected exclusion
/// set from https://www.vldb.org/pvldb/vol11/p1069-meister.pdf (2018).
/// Each unordered, connected, disjoint, adjacent pair appears once, with the
/// lowest vertex on the left. All partitions of either child appear before use.
/// Initialize singleton plans first; no Cartesian products are enumerated.
/// A false callback return stops immediately and makes this function return false.
/// Vertex indices are mask bits; arbitrary numbering and disconnected graphs work.
template<typename Callback>
    requires std::predicate<Callback&, std::uint64_t, std::uint64_t>
bool enumerate_connected_pairs(std::span<const std::uint64_t> adjacency, Callback emit)
{
    if (adjacency.size() > max_dpccp_vertices)
        throw std::invalid_argument("DPccp: at most 63 vertices are supported.");
    const auto all = urd::bits::lo_set<std::uint64_t>[adjacency.size()];
    for (size_t vertex = 0; vertex < adjacency.size(); ++vertex)
    {
        const auto singleton = std::uint64_t { 1 } << vertex;
        if ((adjacency[vertex] & ~all) || (adjacency[vertex] & singleton))
            throw std::invalid_argument("DPccp: adjacency contains an invalid vertex or self edge.");
        urd::bits::for_each_set_bit(adjacency[vertex],
                                    [&](size_t neighbor)
                                    {
                                        if (!(adjacency[neighbor] & singleton))
                                            throw std::invalid_argument("DPccp: adjacency must be symmetric.");
                                    });
    }
    return DpccpEnumerator(adjacency, emit).run();
}
}  // namespace tyr::datalog::database::detail

#endif
