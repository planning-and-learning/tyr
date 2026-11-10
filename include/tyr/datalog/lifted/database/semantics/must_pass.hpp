/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef TYR_DATALOG_DATABASE_SEMANTICS_MUST_PASS_HPP_
#define TYR_DATALOG_DATABASE_SEMANTICS_MUST_PASS_HPP_

#include <urd/repr/sequences/sequence.hpp>
#include <urd/repr/pools/pools.hpp>
#include <urd/store.hpp>
#include "yggdrasil/core/config.hpp"
#include <urd/config.hpp>
#include "tyr/datalog/lifted/database/semantics/bound/query_evaluation.hpp"
#include "tyr/datalog/lifted/database/semantics/details/min_distance.hpp"
#include "tyr/datalog/lifted/database/semantics/must_pass_plan.hpp"
#include "tyr/datalog/lifted/database/semantics/operations.hpp"
#include "tyr/datalog/lifted/database/semantics/relation_view.hpp"
#include "tyr/datalog/lifted/database/semantics/search/a_star.hpp"
#include "tyr/datalog/lifted/database/semantics/search/backward_frontier.hpp"
#include "tyr/datalog/lifted/database/semantics/search/concepts.hpp"
#include "tyr/datalog/lifted/database/semantics/search/search_tree.hpp"
#include "tyr/datalog/lifted/database/syntax/query.hpp"

#include <cassert>
#include <algorithm>
#include <cstddef>
#include <ranges>
#include <span>
#include <utility>
#include <vector>

namespace tyr::datalog::database
{

/// What the explorations of all calls visited, with the counters of their searches (AStarCounters).
struct MustPassStatistics : AStarCounters
{
    size_t keys = 0;      ///< Keys whose targets were reachable.
    size_t vertices = 0;  ///< Expanded vertices.
    size_t edges = 0;     ///< Explored edges.
    size_t removals = 0;  ///< Reachability tests without a probe object.
    size_t backward = 0;  ///< Vertices expanded backward.
};

/// Must-pass objects: for each key k with sources S_k, targets T_k and edges E_k, the probe objects x such that every
/// path from S_k to T_k passes a vertex containing x, i.e. T_k is unreachable once every vertex containing x is removed.
/// A key whose targets are unreachable has none. These are landmarks of the key's graph (Hoffmann, Porteous &
/// Sebastia, "Ordered Landmarks in Planning", JAIR 2004), verified as there and in domain transition graphs (Richter,
/// Helmert & Westphal, "Landmarks Revisited", AAAI 2008) by testing reachability without the candidate; an object, not
/// a single vertex, is the unit, since a path may pass an object in many different vertices.
///
/// Method, per key:
///  1. Edges. As in MinDistance, a bound::QueryEvaluator evaluates the edge query over the call's inputs with the key
///     and a vertex bound, so E_k is explored on demand from S_k and never materialized or kept.
///  2. Witness. A* (search/a_star.hpp, as in MinDistance) finds a shortest path from S_k to T_k. Only objects on this
///     path can be must-pass: any other object is avoided by it. A short witness passes few objects, so few need
///     verification.
///  3. Verification. An object contained in every source or every target is passed trivially, as initial facts and
///     goals are landmarks. For each other probe object on the witness, the same A* tests whether T_k is reachable in
///     the key's graph without the vertices containing it.
///  4. Bidirectionality. As in MinDistance, every search also runs breadth-first backward from T_k (BackwardFrontier),
///     interleaved by Pohl's cardinality comparison; once that side is exhausted without meeting a source, T_k is
///     unreachable ("Bi-directional Search", Machine Intelligence 6, 1971). Unreachability, the expensive case, is so
///     proven from the smaller side. The heuristic stays consistent on the remaining graph, so the searches are
///     goal-directed; only an object that must be passed costs an exhaustive search. Each search starts with an empty
///     search tree: distances learned without one object do not bound those without another.
///
/// The heuristic H estimates the distance to the key's targets from the values of a key followed by a vertex (the bound
/// columns of a successor query); it must be consistent (DistanceHeuristic) for every key, and infinity prunes a
/// vertex. If it has exclude(key, object), it is told before each search which object the search avoids (an empty
/// span: none), so it may estimate on the smaller graph; an abstraction in which the targets become unreachable then
/// proves a must-pass object without a search.
///
/// An edge query that projects vertices onto fewer columns defines an abstraction: every path of the original graph
/// maps to a path of the projection, so objects that must be passed in the projection must be passed by the original's
/// paths too.
///
/// Nothing but compiled plans and static inputs is kept across calls. Inputs are borrowed during must_pass() only.
template<DistanceHeuristic H = BlindDistanceHeuristic>
class MustPass
{
public:
    /// Edges given by a query over inputs with the plan's edge columns; `static_slots` marks its inputs that never
    /// change, which are then read on the first call only. Its input snapshots are builders of `layer`, which must outlive
    /// it.
    MustPass(const ygg::FreeRelationRepository& layer, QueryView<> edges, const MustPassPlan& plan, const std::vector<bool>& static_slots = {}) :
        m_plan(plan),
        m_probe_positions(plan.probe_positions()),
        m_edges(bound::QueryEvaluator(layer, checked(edges, plan), static_slots), plan.edge_columns(), plan.keys(), plan.arity()),
        m_keys(),
        m_probes(),
        m_starts(),
        m_ends(),
        m_tree(),
        m_targets(),
        m_search(),
        m_estimated(plan.key_size() + plan.vertex_size())
    {
    }

    [[nodiscard]] size_t input_count() const noexcept { return m_edges.evaluator().input_count(); }
    [[nodiscard]] const MustPassPlan& plan() const noexcept { return m_plan; }

    /// Appends to `out`, which has the plan's output columns, the must-pass probe objects of every key of the sources and
    /// targets over the edges of the inputs: input(slot) returns slot's relation view.
    template<SlotRelations Input, RelationViewConcept S, RelationViewConcept T, RelationViewConcept P>
    void must_pass(const Input& input, const S& sources, const T& targets, const P& probes, urd::builder<ygg::Relation<ygg::Schema>>& out, H& heuristic)
    {
        m_heuristic = &heuristic;
        assert(std::ranges::equal(sources.columns(), m_plan.source_columns()));
        assert(std::ranges::equal(targets.columns(), m_plan.target_columns()));
        assert(std::ranges::equal(probes.columns(), m_plan.probe_columns()));
        assert(std::ranges::equal(out.columns(), m_plan.output_columns()));
        m_edges.evaluator().bind(input);
        m_probes.clear();
        for (size_t r = 0; r < probes.size(); ++r)
            m_probes.insert(vertex_values(probes.row(r), m_values));
        if (m_probes.empty())
            return;
        group(sources, targets);
        for (auto it = m_key_sources.begin(); it != m_key_sources.end(); ++it)
            if (const auto key = m_key_sources.position(it); !m_key_sources[key].empty() && !m_key_targets[key].empty())
                verify(sources, targets, key, out);
    }

    template<SlotRelations Input, RelationViewConcept S, RelationViewConcept T, RelationViewConcept P>
        requires std::default_initializable<H>
    void must_pass(const Input& input, const S& sources, const T& targets, const P& probes, urd::builder<ygg::Relation<ygg::Schema>>& out)
    {
        auto heuristic = H {};
        must_pass(input, sources, targets, probes, out, heuristic);
    }

    [[nodiscard]] const MustPassStatistics& statistics() const noexcept { return m_statistics; }

private:
    /// Vertices of the current key, as a VertexRange.
    struct Vertices
    {
        const VertexSet* set = nullptr;
        std::span<const VertexSet::handle_type> ids;

        [[nodiscard]] size_t size() const noexcept { return ids.size(); }
        [[nodiscard]] std::span<const ygg::Value> row(size_t i) const { return set->at(ids[i]); }
    };

    using Edges = detail::EdgeQuery<bound::QueryEvaluator>;

    /// The current key's edges (Edges::Keyed, evaluated on demand as in MinDistance) without the vertices containing
    /// the excluded probe object: the oracle of AStar and BackwardFrontier.
    class Graph
    {
    public:
        Graph(typename Edges::Keyed edges, MustPass& owner) : m_edges(edges), m_owner(&owner) {}

        template<RowPredicate OnVertex>
        void successors(std::span<const ygg::Value> vertex, const OnVertex& on_vertex)
        {
            ++m_owner->m_statistics.vertices;
            m_edges.successors(vertex, allowed_only(on_vertex));
        }
        template<RowPredicate OnVertex>
        void predecessors(std::span<const ygg::Value> vertex, const OnVertex& on_vertex)
        {
            ++m_owner->m_statistics.backward;
            m_edges.predecessors(vertex, allowed_only(on_vertex));
        }
        bool edge_exists(std::span<const ygg::Value> from, std::span<const ygg::Value> to)
        { return m_owner->allowed(from) && m_owner->allowed(to) && m_edges.edge_exists(from, to); }

    private:
        /// on_vertex for allowed vertices; the others are skipped.
        template<RowPredicate OnVertex>
        auto allowed_only(const OnVertex& on_vertex)
        {
            return [this, &on_vertex](std::span<const ygg::Value> next)
            {
                ++m_owner->m_statistics.edges;
                return !m_owner->allowed(next) || on_vertex(next);
            };
        }

        typename Edges::Keyed m_edges;
        MustPass* m_owner;
    };

    /// The heuristic on a vertex of the current key: H estimates from the key followed by the vertex.
    class KeyedHeuristic
    {
    public:
        explicit KeyedHeuristic(MustPass& owner) : m_owner(&owner) {}

        [[nodiscard]] ygg::uint_t estimate(std::span<const ygg::Value> vertex) const { return m_owner->estimate(vertex); }

    private:
        MustPass* m_owner;
    };

    static QueryView<> checked(QueryView<> edges, const MustPassPlan& plan)
    {
        if (!std::ranges::equal(edges.columns(), plan.edge_columns()))
            throw std::invalid_argument("Relational operation: input schema does not match prepared plan.");
        return edges;
    }

    [[nodiscard]] std::span<const ygg::Value> key_of(std::span<const ygg::Value> row) const noexcept { return row.first(m_plan.key_size()); }
    [[nodiscard]] std::span<const ygg::Value> vertex_of(std::span<const ygg::Value> row) const noexcept
    { return row.subspan(m_plan.key_size(), m_plan.vertex_size()); }

    /// The row positions of the sources and targets of each key; keys are numbered by their first source. Without key
    /// columns every row has the one empty key.
    template<RelationViewConcept S, RelationViewConcept T>
    void group(const S& sources, const T& targets)
    {
        m_keys.clear();
        for (size_t r = 0; r < sources.size(); ++r)
            m_keys.insert(key_of(vertex_values(sources.row(r), m_values)));
        m_key_sources.resize(m_keys.size());
        m_key_targets.resize(m_keys.size());
        for (auto& rows : m_key_sources)
            rows.clear();
        for (auto& rows : m_key_targets)
            rows.clear();
        for (size_t r = 0; r < sources.size(); ++r)
        {
            const auto key = m_keys.find(key_of(vertex_values(sources.row(r), m_values)));
            URD_ASSUME(!key.is_none());  // every source's key was inserted above
            m_key_sources[key].push_back(r);
        }
        for (size_t r = 0; r < targets.size(); ++r)
            if (const auto key = m_keys.find(key_of(vertex_values(targets.row(r), m_values))); !key.is_none())  // A key without sources has no paths.
                m_key_targets[key].push_back(r);
    }

    /// Calls on_probe with each probe object a vertex contains (an object at several positions once per position).
    template<typename OnProbe>
    void probes_of(std::span<const ygg::Value> vertex, const OnProbe& on_probe) const
    {
        for (const auto position : m_probe_positions)
            if (const auto probe = m_probes.find(vertex.subspan(position, m_plan.probe_size())); !probe.is_none())
                on_probe(probe);
    }
    [[nodiscard]] bool contains(std::span<const ygg::Value> vertex, VertexSet::handle_type probe) const
    {
        const auto object = m_probes.at(probe);
        return std::ranges::any_of(m_probe_positions,
                                   [&](size_t position) { return std::ranges::equal(vertex.subspan(position, m_plan.probe_size()), object); });
    }
    /// Whether the current search may visit a vertex: it does not contain the excluded probe object.
    [[nodiscard]] bool allowed(std::span<const ygg::Value> vertex) const { return m_excluded.is_none() || !contains(vertex, m_excluded); }

    /// The heuristic's estimate of a vertex of the current key.
    ygg::uint_t estimate(std::span<const ygg::Value> vertex)
    {
        std::ranges::copy(vertex, m_estimated.begin() + static_cast<std::ptrdiff_t>(m_plan.key_size()));
        return m_heuristic->estimate(m_estimated);
    }

    /// Whether a target of the current key is reachable without the vertices containing `excluded` (none: any). The
    /// found path's vertices are passed to on_path.
    template<typename OnPath>
    bool search(VertexSet::handle_type excluded, const OnPath& on_path)
    {
        m_excluded = excluded;
        if constexpr (requires(H& h, std::span<const ygg::Value> values) { h.exclude(values, values); })
        {
            m_heuristic->exclude(m_key, excluded.is_none() ? std::span<const ygg::Value>() : std::span<const ygg::Value>(m_probes.at(excluded)));
        }
        const auto keep_allowed = [&](const VertexSet& vertices,
                                      const urd::Store<urd::sequence<VertexSet::handle_type>>& ids,
                                      urd::Store<urd::sequence<VertexSet::handle_type>>& out)
        {
            out.clear();
            for (const auto id : ids)
                if (allowed(vertices.at(id)))
                    out.push_back(id);
        };
        keep_allowed(m_starts, m_start_ids, m_allowed_starts);
        keep_allowed(m_ends, m_end_ids, m_allowed_ends);
        auto graph = Graph(m_edges.with_key(m_key), *this);
        const auto heuristic = KeyedHeuristic(*this);
        m_tree.clear();
        m_targets.assign(Vertices { &m_ends, m_allowed_ends });
        m_frontier.start(m_targets.ensure_in(m_tree, heuristic));
        auto search = AStar<Graph, KeyedHeuristic>(graph, m_tree, m_frontier, heuristic, m_targets.vertices(), m_search, m_edges.evaluator().version());
        const auto reachable = search.run(Vertices { &m_starts, m_allowed_starts }) != infinity;
        m_statistics += search.counters();
        if (reachable)
            search.path(on_path);
        return reachable;
    }

    template<RelationViewConcept S, RelationViewConcept T>
    void verify(const S& sources, const T& targets, VertexSet::handle_type key, urd::builder<ygg::Relation<ygg::Schema>>& out)
    {
        const auto key_values = key_of(vertex_values(sources.row(m_key_sources[key].front()), m_values));
        m_key_values.assign(key_values.begin(), key_values.end());
        m_key = m_key_values;
        std::ranges::copy(m_key, m_estimated.begin());
        const auto intern = [&](VertexSet& vertices, urd::Store<urd::sequence<VertexSet::handle_type>>& ids, const auto& rows, const auto& positions)
        {
            vertices.clear();
            ids.clear();
            for (const auto r : positions)
                if (const auto [id, added] = vertices.insert(vertex_of(vertex_values(rows.row(r), m_values))); added)
                    ids.push_back(id);
        };
        intern(m_starts, m_start_ids, sources, m_key_sources[key]);
        intern(m_ends, m_end_ids, targets, m_key_targets[key]);
        m_candidates.clear();
        const auto on_witness = [&](std::span<const ygg::Value> vertex)
        {
            probes_of(vertex,
                      [&](VertexSet::handle_type probe)
                      {
                          if (std::ranges::find(m_candidates, probe) == m_candidates.end())
                              m_candidates.push_back(probe);
                      });
        };
        if (!search({}, on_witness))
            return;
        ++m_statistics.keys;
        m_row.assign(m_key.begin(), m_key.end());
        m_row.resize(m_plan.key_size() + m_plan.probe_size());
        const auto in_all = [&](const VertexSet& vertices, const urd::Store<urd::sequence<VertexSet::handle_type>>& ids, VertexSet::handle_type probe)
        { return std::ranges::all_of(ids, [&](VertexSet::handle_type id) { return contains(vertices.at(id), probe); }); };
        for (const auto probe : m_candidates)
            // An object in every source or every target is passed trivially, as initial facts and goals are landmarks.
            if (in_all(m_starts, m_start_ids, probe) || in_all(m_ends, m_end_ids, probe)
                || (++m_statistics.removals, !search(probe, [](std::span<const ygg::Value>) {})))
            {
                std::ranges::copy(m_probes.at(probe), m_row.begin() + static_cast<std::ptrdiff_t>(m_plan.key_size()));
                out.insert(std::span<const ygg::Value>(m_row));
            }
    }

    MustPassPlan m_plan;
    urd::Store<urd::sequence<size_t>> m_probe_positions;
    Edges m_edges;
    VertexSet m_keys;    ///< Interned keys; the empty key without key columns.
    VertexSet m_probes;  ///< Interned probe objects.
    urd::Store<urd::sequence<urd::Store<urd::sequence<size_t>>, VertexSet::handle_type>> m_key_sources;  ///< By key.
    urd::Store<urd::sequence<urd::Store<urd::sequence<size_t>>, VertexSet::handle_type>> m_key_targets;  ///< By key.
    // The current key.
    urd::Store<urd::sequence<ygg::Value>> m_key_values;
    std::span<const ygg::Value> m_key;  ///< m_key_values.
    VertexSet m_starts;
    VertexSet m_ends;
    urd::Store<urd::sequence<VertexSet::handle_type>> m_start_ids;  ///< Of m_starts, as inserted.
    urd::Store<urd::sequence<VertexSet::handle_type>> m_end_ids;    ///< Of m_ends, as inserted.
    // Searches.
    H* m_heuristic = nullptr;
    VertexSet::handle_type m_excluded;  ///< The probe object whose vertices the current search avoids, or none.
    urd::Store<urd::sequence<VertexSet::handle_type>> m_allowed_starts;
    urd::Store<urd::sequence<VertexSet::handle_type>> m_allowed_ends;
    SearchTree m_tree;
    BackwardFrontier m_frontier;
    detail::TargetSet m_targets;
    AStarStorage m_search;
    urd::Store<urd::sequence<VertexSet::handle_type>> m_candidates;  ///< Probe objects on the witness.
    urd::Store<urd::sequence<ygg::Value>> m_estimated;  ///< The key and a vertex, as given to the heuristic.
    urd::Store<urd::sequence<ygg::Value>> m_row;
    urd::Store<urd::sequence<ygg::Value>> m_values;  ///< A row's values, as read from an input.
    MustPassStatistics m_statistics;
};

}  // namespace tyr::datalog::database

#endif
