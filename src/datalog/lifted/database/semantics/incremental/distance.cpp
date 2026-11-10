/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "tyr/datalog/lifted/database/semantics/incremental/distance.hpp"

#include "yggdrasil/core/config.hpp"
#include <urd/config.hpp>

#include <ranges>

namespace tyr::datalog::database::incremental
{
DistanceEvaluator::DistanceEvaluator(const ygg::FreeRelationRepository& layer, DistancePlan plan) :
    m_layer(&layer),
    m_plan(std::move(plan)),
    m_graph(m_plan.tuple_size()),
    m_result(m_plan.output_columns()),
    m_delta(layer, m_plan.output_columns())
{
}
}

namespace tyr::datalog::database::incremental
{
VertexSet::handle_type DistanceEvaluator::require_vertex(std::span<const ygg::Value> row) const
{
    const auto vertex = m_graph.vertices.find(row);
    if (vertex.is_none())
        throw std::invalid_argument("Incremental distance: removed vertex is absent.");
    return vertex;
}
}

namespace tyr::datalog::database::incremental
{
bool DistanceEvaluator::is_source(VertexSet::handle_type vertex) const noexcept
{ return vertex < m_source_slots.position(m_source_slots.end()) && m_source_slots[vertex] != absent; }
}

namespace tyr::datalog::database::incremental
{
bool DistanceEvaluator::is_target(VertexSet::handle_type vertex) const noexcept
{ return vertex < m_target_flags.position(m_target_flags.end()) && m_target_flags[vertex] == Flag::SET; }
}

namespace tyr::datalog::database::incremental
{
void DistanceEvaluator::resize_vertices()
{
    const auto size = m_graph.vertices.size();
    m_source_slots.resize(size, absent);
    m_target_flags.resize(size, Flag::CLEAR);
    m_repair->resize(size);
    m_changed_flags.resize(size, Flag::CLEAR);
    for (size_t i = 0; i < m_active_sources; ++i)
    {
        m_sources[i].distances.resize(size, infinity);
        m_sources[i].supports.resize(size, 0);
    }
}
}

namespace tyr::datalog::database::incremental
{
void DistanceEvaluator::remove_source(VertexSet::handle_type root)
{
    const auto position = m_source_slots[root];
    assert(position < m_active_sources);
    --m_active_sources;
    if (position != m_active_sources)
    {
        std::swap(m_sources[position], m_sources[m_active_sources]);
        m_source_slots[m_sources[position].root] = position;
    }
    m_source_slots[root] = absent;
}
}

namespace tyr::datalog::database::incremental
{
void DistanceEvaluator::insert_result(VertexSet::handle_type source, VertexSet::handle_type target, ygg::uint_t length)
{
    if (length == infinity)
        return;
    const auto row = database::detail::make_distance_row(m_graph, source, target, length, m_row);
    m_result.insert(row);
    m_delta.added.insert(row);
}
}

namespace tyr::datalog::database::incremental
{
void DistanceEvaluator::remove_result(VertexSet::handle_type source, VertexSet::handle_type target, ygg::uint_t length)
{
    if (length == infinity)
        return;
    const auto row = database::detail::make_distance_row(m_graph, source, target, length, m_row);
    const auto position = m_result.find(row);
    URD_ASSUME(position.has_value());  // only rows of the result are retracted
    m_delta.removed.insert(row);
    m_result.erase(*position);
}
}

namespace tyr::datalog::database::incremental
{
void DistanceEvaluator::set_distance(SourceState& state, VertexSet::handle_type vertex, ygg::uint_t length)
{
    if (m_changed_flags[vertex] == Flag::CLEAR)
    {
        m_changed_flags[vertex] = Flag::SET;
        m_changed.emplace_back(vertex, state.distances[vertex]);
    }
    state.distances[vertex] = length;
}
}

namespace tyr::datalog::database::incremental
{
ygg::uint_t DistanceEvaluator::successor_distance(ygg::uint_t distance) noexcept { return distance == infinity ? infinity : distance + 1; }
}

namespace tyr::datalog::database::incremental
{
void DistanceEvaluator::update_edge(SourceState& state,
                                    VertexSet::handle_type from,
                                    VertexSet::handle_type to,
                                    std::optional<ygg::uint_t> before,
                                    std::optional<ygg::uint_t> after)
{
    // Presence is separate from infinity: deleting an unreachable edge still removes it from SP.
    auto& supports = state.supports[to];
    if (before && successor_distance(*before) <= state.distances[to])
    {
        assert(supports != 0);
        --supports;
    }
    if (after && successor_distance(*after) <= state.distances[to])
        ++supports;
    m_repair->set_edge(from, to, after ? successor_distance(*after) : infinity, state.distances[to]);
    m_repair->schedule(to, state.distances[to], supports);
}
}

namespace tyr::datalog::database::incremental
{
void DistanceEvaluator::rebuild_incoming(SourceState& state, VertexSet::handle_type vertex)
{
    assert(m_repair->settled(vertex));
    auto& supports = state.supports[vertex];
    supports = vertex == state.root ? 1 : 0;
    for (const auto [first, last] = m_reverse.equal_range(vertex); const auto from : std::ranges::subrange(first, last) | std::views::values)
    {
        detail::count_distance_work(&detail::DistanceWork::incoming_edges);
        const auto candidate = successor_distance(state.distances[from]);
        if (candidate <= state.distances[vertex])
            ++supports;
        m_repair->set_edge(from, vertex, candidate, state.distances[vertex]);
    }
    m_repair->schedule(vertex, state.distances[vertex], supports);
}
}

namespace tyr::datalog::database::incremental
{
void DistanceEvaluator::repair(SourceState& state)
{
    assert(m_repair->empty());
    // Figure 4: seed the entire mixed batch before extracting any inconsistent vertex.
    for (const auto& [from, to] : m_edges_removed)
        update_edge(state, from, to, state.distances[from], std::nullopt);
    for (const auto& [from, to] : m_edges_added)
        update_edge(state, from, to, std::nullopt, state.distances[from]);

    while (m_repair->scheduled())
    {
        const auto [key, vertex] = m_repair->pop();
        detail::count_distance_work(&detail::DistanceWork::processed_vertices);
        const auto before = state.distances[vertex];
        const auto after = key < before ? key : infinity;
        set_distance(state, vertex, after);
        if (key < before)
            state.supports[vertex] = m_repair->accept_improvement(vertex, key);
        else
            rebuild_incoming(state, vertex);
        for (const auto [first, last] = m_graph.outgoing.equal_range(vertex); const auto to : std::ranges::subrange(first, last) | std::views::values)
        {
            detail::count_distance_work(&detail::DistanceWork::outgoing_edges);
            update_edge(state, vertex, to, before, after);
        }
    }
    assert(m_repair->empty());
    for (const auto& [target, before] : m_changed)
    {
        m_changed_flags[target] = Flag::CLEAR;
        if (is_target(target) && before != state.distances[target])
        {
            remove_result(state.root, target, before);
            insert_result(state.root, target, state.distances[target]);
        }
    }
    m_changed.clear();
}
}

namespace tyr::datalog::database::incremental
{
void DistanceEvaluator::clear_changes()
{
    m_sources_added.clear();
    m_sources_removed.clear();
    m_targets_added.clear();
    m_targets_removed.clear();
    m_edges_added.clear();
    m_edges_removed.clear();
}
}

namespace tyr::datalog::database::incremental
{
ygg::FreeBuilderRelationView DistanceEvaluator::get_result() const&
{
    detail::require_initialized(m_initialized);
    return m_layer->view(m_result);
}
}

namespace tyr::datalog::database::incremental
{
const Delta& DistanceEvaluator::get_delta() const&
{
    detail::require_initialized(m_initialized);
    return m_delta;
}
}

namespace tyr::datalog::database::incremental
{
size_t DistanceEvaluator::memory_usage() const noexcept
{
    size_t bytes =
        m_graph.memory_usage() + urd::memory_usage(m_reverse) + m_result.memory_usage() + m_delta.memory_usage() + (m_repair ? m_repair->memory_usage() : 0);
    bytes += m_sources.capacity() * sizeof(SourceState) + m_source_slots.capacity() * sizeof(size_t);
    for (const auto& source : m_sources)
        bytes += source.distances.capacity() * sizeof(ygg::uint_t) + source.supports.capacity() * sizeof(size_t);
    bytes += (m_targets.capacity() + m_queue.capacity() + m_sources_added.capacity() + m_sources_removed.capacity() + m_targets_added.capacity()
              + m_targets_removed.capacity())
             * sizeof(VertexSet::handle_type);
    bytes += m_changed.capacity() * sizeof(std::pair<VertexSet::handle_type, ygg::uint_t>)
             + (m_edges_added.capacity() + m_edges_removed.capacity()) * sizeof(std::pair<VertexSet::handle_type, VertexSet::handle_type>);
    bytes += (m_target_flags.capacity() + m_changed_flags.capacity()) * sizeof(Flag) + (m_row.capacity() + m_values.capacity()) * sizeof(ygg::Value);
    return bytes;
}
}

namespace tyr::datalog::database::incremental
{
void SourceMinDistance::count(ygg::FreeBuilderRelationView rows, bool add)
{
    for (size_t i = 0; i < rows.size(); ++i)
    {
        const auto distance = static_cast<ygg::uint_t>(rows.row(i).value(m_position));
        if (distance >= m_counts.size())
            m_counts.resize(distance + 1, 0);
        assert(add || m_counts[distance] > 0);
        m_counts[distance] = add ? m_counts[distance] + 1 : m_counts[distance] - 1;
    }
}
}
