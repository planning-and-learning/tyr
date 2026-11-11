/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef TYR_DATALOG_DATABASE_SEMANTICS_INCREMENTAL_DISTANCE_HPP_
#define TYR_DATALOG_DATABASE_SEMANTICS_INCREMENTAL_DISTANCE_HPP_

#include "tyr/datalog/lifted/database/semantics/distance.hpp"
#include "tyr/datalog/lifted/database/semantics/incremental/delta.hpp"
#include "tyr/datalog/lifted/database/semantics/incremental/details/distance_repair.hpp"
#include "yggdrasil/core/config.hpp"

#include <algorithm>
#include <cassert>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <urd/config.hpp>
#include <urd/memory.hpp>
#include <urd/repr/maps/ordered_map.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/repr/values/sparse_set.hpp>
#include <urd/store.hpp>
#include <utility>
#include <vector>

namespace tyr::datalog::database::incremental
{

/// Maintains every source's distances using Ramalingam and Reps, "An Incremental
/// Algorithm for a Generalization of the Shortest-Path Problem" (1996), Section 4,
/// Figure 4 (DynamicSSF-G), https://doi.org/10.1006/jagm.1996.0046.
/// All edge changes are seeded before repair. Counts represent the paper's support
/// sets; temporary heaps retain only improving edges. See docs/database.md for the
/// unit-edge specialization and the costs outside the per-source repair algorithm.
///
/// Inputs are borrowed during calls only. Added/removed inputs describe actual,
/// disjoint set changes; swapping each pair undoes an update. Invalid changes are
/// rejected before mutation and preserve the previous result. A failure during
/// mutation requires initialize().
/// Results and deltas are borrowed until the next mutation. Tuple IDs are retained
/// until initialize(), so memory follows historical vertices and peak source count.
class DistanceEvaluator
{
    static constexpr ygg::uint_t infinity = database::detail::distance_infinity;
    static constexpr size_t absent = std::numeric_limits<size_t>::max();

    enum class Flag : unsigned char
    {
        CLEAR,
        SET
    };

    struct SourceState
    {
        VertexSet::handle_type root;
        urd::Store<urd::sequence<ygg::uint_t, VertexSet::handle_type>> distances;
        urd::Store<urd::sequence<size_t, VertexSet::handle_type>> supports;

        /// The members it owns memory through (urd::memory_of).
        static constexpr auto memory_members() noexcept { return urd::Members<&SourceState::distances, &SourceState::supports> {}; }
    };

    const ygg::FreeRelationRepository* m_layer;
    DistancePlan m_plan;
    database::detail::DistanceGraph m_graph;
    urd::Store<urd::map<VertexSet::handle_type, VertexSet::handle_type>, urd::multi, urd::retains> m_reverse;
    urd::Store<urd::sequence<SourceState>> m_sources;
    size_t m_active_sources = 0;
    urd::Store<urd::sequence<size_t, VertexSet::handle_type>> m_source_slots;
    urd::Store<urd::sequence<VertexSet::handle_type>> m_targets;
    urd::Store<urd::sequence<Flag, VertexSet::handle_type>> m_target_flags;
    urd::builder<ygg::Relation<ygg::Schema>> m_result;
    Delta m_delta;
    bool m_initialized = false;

    // One scratch area serves all source trees sequentially.
    urd::Store<urd::sequence<VertexSet::handle_type>> m_queue;
    std::unique_ptr<detail::DistanceRepairWorkspace> m_repair = std::make_unique<detail::DistanceRepairWorkspace>();
    /// The distance before of each vertex whose distance repair() set, in the order they were set.
    urd::Store<urd::map<VertexSet::handle_type, ygg::uint_t>, urd::dense, urd::insertion_ordered> m_changed;
    urd::Store<urd::sequence<ygg::Value>> m_row;
    mutable urd::Store<urd::sequence<ygg::Value>> m_values;  ///< An input row's values, as read.

    urd::Store<urd::sequence<VertexSet::handle_type>> m_sources_added;
    urd::Store<urd::sequence<VertexSet::handle_type>> m_sources_removed;
    urd::Store<urd::sequence<VertexSet::handle_type>> m_targets_added;
    urd::Store<urd::sequence<VertexSet::handle_type>> m_targets_removed;
    urd::Store<urd::sequence<std::pair<VertexSet::handle_type, VertexSet::handle_type>>> m_edges_added;
    urd::Store<urd::sequence<std::pair<VertexSet::handle_type, VertexSet::handle_type>>> m_edges_removed;

    [[nodiscard]] VertexSet::handle_type require_vertex(std::span<const ygg::Value> row) const;
    [[nodiscard]] bool is_source(VertexSet::handle_type vertex) const noexcept;
    [[nodiscard]] bool is_target(VertexSet::handle_type vertex) const noexcept;
    void resize_vertices();
    SourceState& add_source(VertexSet::handle_type root);
    void remove_source(VertexSet::handle_type root);
    void insert_result(VertexSet::handle_type source, VertexSet::handle_type target, ygg::uint_t length);
    void remove_result(VertexSet::handle_type source, VertexSet::handle_type target, ygg::uint_t length);
    void set_distance(SourceState& state, VertexSet::handle_type vertex, ygg::uint_t length);
    static ygg::uint_t successor_distance(ygg::uint_t distance) noexcept;
    void update_edge(SourceState& state,
                     VertexSet::handle_type from,
                     VertexSet::handle_type to,
                     std::optional<ygg::uint_t> before,
                     std::optional<ygg::uint_t> after);
    void rebuild_incoming(SourceState& state, VertexSet::handle_type vertex);
    void repair(SourceState& state);
    void clear_changes();
    /// Throws unless the changes describe actual set changes of the current inputs.
    template<RelationChange S, RelationChange E, RelationChange T>
    void require_actual(const S& sources, const E& edges, const T& targets) const;
    /// Applies changes that are known to be valid.
    template<RelationChange S, RelationChange E, RelationChange T>
    void apply(const S& sources, const E& edges, const T& targets);

    friend struct detail::NodeRules<QueryDistanceTag>;

public:
    /// Its builders are read through `layer`, which must outlive it.
    DistanceEvaluator(const ygg::FreeRelationRepository& layer, DistancePlan plan);

    /// Replace all inputs, retaining buffers, and clear the last output delta.
    template<RelationViewConcept S, RelationViewConcept E, RelationViewConcept T>
    void initialize(const S& sources, const E& edges, const T& targets);

    /// Publish only the net result change of this complete batch.
    template<RelationChange S, RelationChange E, RelationChange T>
    void update(const S& sources, const E& edges, const T& targets);

    [[nodiscard]] ygg::FreeBuilderRelationView get_result() const&;
    [[nodiscard]] ygg::FreeBuilderRelationView get_result() const&& = delete;
    [[nodiscard]] const Delta& get_delta() const&;
    [[nodiscard]] const Delta& get_delta() const&& = delete;
    /// Retained storage, including inactive reusable source arrays; excludes the plan.
    [[nodiscard]] size_t memory_usage() const noexcept;
};

inline auto DistanceEvaluator::add_source(VertexSet::handle_type root) -> SourceState&
{
    if (m_active_sources == m_sources.size())
        m_sources.emplace_back();
    auto& state = m_sources[m_active_sources];
    state.root = root;
    database::detail::breadth_first_search(m_graph, root, state.distances, m_queue);
    state.supports.assign(m_graph.vertices.size(), 0);
    state.supports[root] = 1;  // The root has a constant zero-distance contribution.
    // SP includes infinite contributions to unreachable vertices (infinity <= infinity).
    for (const auto& [to, from] : m_reverse)
        if (successor_distance(state.distances[from]) <= state.distances[to])
            ++state.supports[to];
    m_source_slots[root] = m_active_sources++;
    return state;
}

template<RelationViewConcept S, RelationViewConcept E, RelationViewConcept T>
void DistanceEvaluator::initialize(const S& sources, const E& edges, const T& targets)
{
    assert(std::ranges::equal(sources.columns(), m_plan.source_columns()));
    assert(std::ranges::equal(edges.columns(), m_plan.edge_columns()));
    assert(std::ranges::equal(targets.columns(), m_plan.target_columns()));
    detail::require_unaliased(sources, m_result, m_delta);
    detail::require_unaliased(edges, m_result, m_delta);
    detail::require_unaliased(targets, m_result, m_delta);
    m_initialized = false;
    m_graph.clear();
    m_reverse.clear();
    m_active_sources = 0;
    m_source_slots.clear();
    m_targets.clear();
    m_target_flags.clear();
    m_result.clear();
    m_delta.clear();
    m_queue.clear();
    m_repair->clear();
    m_changed.clear();
    clear_changes();
    m_graph.outgoing.reserve(edges.size());
    m_reverse.reserve(edges.size());
    for (size_t i = 0; i < edges.size(); ++i)
    {
        const auto [from, to] = m_graph.insert_endpoints(edges.row(i).get_values());
        m_graph.outgoing.insert({ from, to });
        m_reverse.insert({ to, from });
    }
    for (size_t i = 0; i < sources.size(); ++i)
        m_graph.insert_vertex(sources.row(i).get_values());
    for (size_t i = 0; i < targets.size(); ++i)
        m_targets.push_back(m_graph.insert_vertex(targets.row(i).get_values()));
    resize_vertices();
    for (const auto target : m_targets)
        m_target_flags[target] = Flag::SET;
    for (size_t i = 0; i < sources.size(); ++i)
    {
        const auto source = m_graph.find_vertex(sources.row(i).get_values());
        URD_ASSUME(!source.is_none());  // every source was inserted above
        const auto& state = add_source(source);
        for (const auto target : m_targets)
            if (state.distances[target] != infinity)
                m_result.insert(database::detail::make_distance_row(m_graph, state.root, target, state.distances[target], m_row));
    }
    m_initialized = true;
}

template<RelationChange S, RelationChange E, RelationChange T>
void DistanceEvaluator::update(const S& sources, const E& edges, const T& targets)
{
    detail::require_initialized(m_initialized);
    detail::require_change(sources, m_plan.source_columns(), m_result, m_delta);
    detail::require_change(edges, m_plan.edge_columns(), m_result, m_delta);
    detail::require_change(targets, m_plan.target_columns(), m_result, m_delta);
    require_actual(sources, edges, targets);
    apply(sources, edges, targets);
}

template<RelationChange S, RelationChange E, RelationChange T>
void DistanceEvaluator::require_actual(const S& sources, const E& edges, const T& targets) const
{
    const auto& [sources_added, sources_removed] = sources;
    const auto& [edges_added, edges_removed] = edges;
    const auto& [targets_added, targets_removed] = targets;
    const auto contains_edge = [&](const ygg::RowPool::row_type& row)
    {
        const auto values = vertex_values(row, m_values);
        const auto from = m_graph.vertices.find(values.first(m_plan.tuple_size()));
        const auto to = m_graph.vertices.find(values.subspan(m_plan.tuple_size()));
        return !from.is_none() && !to.is_none() && m_graph.contains_edge(from, to);
    };
    const auto require_members = [&](const auto& rows, auto is_member, bool removing)
    {
        for (size_t i = 0; i < rows.size(); ++i)
        {
            const auto vertex = m_graph.vertices.find(vertex_values(rows.row(i), m_values));
            if ((!vertex.is_none() && (this->*is_member)(vertex)) != removing)
                throw std::invalid_argument(removing ? "Incremental distance: removed vertex is absent." :
                                                       "Incremental distance: added vertex is already present.");
        }
    };
    require_members(sources_removed, &DistanceEvaluator::is_source, true);
    require_members(sources_added, &DistanceEvaluator::is_source, false);
    require_members(targets_removed, &DistanceEvaluator::is_target, true);
    require_members(targets_added, &DistanceEvaluator::is_target, false);
    for (size_t i = 0; i < edges_removed.size(); ++i)
        if (!contains_edge(edges_removed.row(i).get_values()))
            throw std::invalid_argument("Incremental distance: removed edge is absent.");
    for (size_t i = 0; i < edges_added.size(); ++i)
        if (contains_edge(edges_added.row(i).get_values()))
            throw std::invalid_argument("Incremental distance: added edge is already present.");
}

template<RelationChange S, RelationChange E, RelationChange T>
void DistanceEvaluator::apply(const S& sources, const E& edges, const T& targets)
{
    const auto& [sources_added, sources_removed] = sources;
    const auto& [edges_added, edges_removed] = edges;
    const auto& [targets_added, targets_removed] = targets;
    m_initialized = false;
    m_delta.clear();
    clear_changes();

    for (size_t i = 0; i < sources_removed.size(); ++i)
    {
        const auto id = require_vertex(vertex_values(sources_removed.row(i), m_values));
        assert(is_source(id));
        m_sources_removed.push_back(id);
    }
    for (size_t i = 0; i < targets_removed.size(); ++i)
    {
        const auto id = require_vertex(vertex_values(targets_removed.row(i), m_values));
        assert(is_target(id));
        m_targets_removed.push_back(id);
    }
    for (size_t i = 0; i < edges_removed.size(); ++i)
    {
        const auto row = vertex_values(edges_removed.row(i), m_values);
        const auto from = require_vertex(row.first(m_plan.tuple_size()));
        const auto to = require_vertex(row.subspan(m_plan.tuple_size()));
        assert(m_graph.contains_edge(from, to));
        m_edges_removed.emplace_back(from, to);
    }
    for (size_t i = 0; i < sources_added.size(); ++i)
    {
        const auto id = m_graph.insert_vertex(sources_added.row(i).get_values());
        assert(!is_source(id));
        m_sources_added.push_back(id);
    }
    for (size_t i = 0; i < targets_added.size(); ++i)
    {
        const auto id = m_graph.insert_vertex(targets_added.row(i).get_values());
        assert(!is_target(id));
        m_targets_added.push_back(id);
    }
    for (size_t i = 0; i < edges_added.size(); ++i)
    {
        const auto [from, to] = m_graph.insert_endpoints(edges_added.row(i).get_values());
        assert(!m_graph.contains_edge(from, to));
        m_edges_added.emplace_back(from, to);
    }

    // Remove old source/target pairs before repairing distances, so simultaneous
    // membership changes cannot publish intermediate rows or duplicate removals.
    for (const auto root : m_sources_removed)
    {
        const auto& state = m_sources[m_source_slots[root]];
        for (const auto target : m_targets)
            remove_result(root, target, state.distances[target]);
        remove_source(root);
    }
    for (const auto target : m_targets_removed)
    {
        for (size_t i = 0; i < m_active_sources; ++i)
            remove_result(m_sources[i].root, target, m_sources[i].distances[target]);
        m_target_flags[target] = Flag::CLEAR;
        const auto position = std::ranges::find(m_targets, target);
        assert(position != m_targets.end());
        *position = m_targets.back();
        m_targets.pop_back();
    }
    for (const auto& [from, to] : m_edges_removed)
    {
        const auto erase = [](auto& edges, VertexSet::handle_type key, VertexSet::handle_type value)
        {
            const auto matches = edges.equal_range(key);
            const auto found = std::find_if(matches.first, matches.second, [&](const auto& entry) { return entry.second == value; });
            assert(found != matches.second);
            edges.erase(found);
        };
        erase(m_graph.outgoing, from, to);
        erase(m_reverse, to, from);
    }
    m_graph.outgoing.reserve(m_graph.outgoing.size() + m_edges_added.size());
    m_reverse.reserve(m_reverse.size() + m_edges_added.size());
    for (const auto& [from, to] : m_edges_added)
    {
        m_graph.outgoing.insert({ from, to });
        m_reverse.insert({ to, from });
    }
    resize_vertices();
    if (!m_edges_added.empty() || !m_edges_removed.empty())
        for (size_t i = 0; i < m_active_sources; ++i)
            repair(m_sources[i]);
    for (const auto target : m_targets_added)
    {
        m_target_flags[target] = Flag::SET;
        m_targets.push_back(target);
        for (size_t i = 0; i < m_active_sources; ++i)
            insert_result(m_sources[i].root, target, m_sources[i].distances[target]);
    }
    for (const auto root : m_sources_added)
    {
        const auto& state = add_source(root);
        for (const auto target : m_targets)
            insert_result(root, target, state.distances[target]);
    }
    m_initialized = true;
}

/// Maintains the smallest distance between any source and target. Wraps DistanceEvaluator
/// and counts its result rows per distance value, so deleting the minimum reveals the next one.
class SourceMinDistance
{
    size_t m_position;  ///< The distance column follows the edge columns.
    DistanceEvaluator m_evaluator;
    urd::Store<urd::sequence<size_t>> m_counts;

    void count(ygg::FreeBuilderRelationView rows, bool add);

public:
    /// Its builders are read through `layer`, which must outlive it.
    SourceMinDistance(const ygg::FreeRelationRepository& layer, DistancePlan plan) :
        m_position(plan.output_columns().size() - 1),
        m_evaluator(layer, std::move(plan))
    {
    }

    /// Replace all inputs, retaining buffers.
    template<RelationViewConcept S, RelationViewConcept E, RelationViewConcept T>
    void initialize(const S& sources, const E& edges, const T& targets)
    {
        m_evaluator.initialize(sources, edges, targets);
        m_counts.clear();
        count(m_evaluator.get_result(), true);
    }

    /// Apply one complete batch of input changes, as DistanceEvaluator::update.
    template<RelationChange S, RelationChange E, RelationChange T>
    void update(const S& sources, const E& edges, const T& targets)
    {
        m_evaluator.update(sources, edges, targets);
        const auto [added, removed] = m_evaluator.get_delta().change();
        count(removed, false);
        count(added, true);
    }

    /// uint_t's maximum if no target is reachable.
    [[nodiscard]] ygg::uint_t get_result() const noexcept
    {
        const auto it = std::ranges::find_if(m_counts, [](size_t count) { return count > 0; });
        return it == m_counts.end() ? std::numeric_limits<ygg::uint_t>::max() : static_cast<ygg::uint_t>(it - m_counts.begin());
    }

    [[nodiscard]] size_t memory_usage() const noexcept { return m_evaluator.memory_usage() + urd::memory_usage(m_counts); }
};

}  // namespace tyr::datalog::database::incremental

#endif
