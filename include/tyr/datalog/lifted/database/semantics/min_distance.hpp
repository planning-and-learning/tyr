/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef TYR_DATALOG_DATABASE_SEMANTICS_MIN_DISTANCE_HPP_
#define TYR_DATALOG_DATABASE_SEMANTICS_MIN_DISTANCE_HPP_

#include "tyr/datalog/lifted/database/semantics/bound/query_evaluation.hpp"
#include "tyr/datalog/lifted/database/semantics/details/min_distance.hpp"
#include "tyr/datalog/lifted/database/semantics/distance.hpp"
#include "tyr/datalog/lifted/database/semantics/search/a_star.hpp"
#include "tyr/datalog/lifted/database/semantics/search/backward_frontier.hpp"
#include "tyr/datalog/lifted/database/semantics/search/concepts.hpp"
#include "tyr/datalog/lifted/database/semantics/search/search_tree.hpp"

#include <cassert>
#include <cstddef>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <urd/repr/values/dense_set.hpp>
#include <urd/store.hpp>
#include <utility>

namespace tyr::datalog::database
{

/// The counters of all searches (AStarCounters) and of the repairs between them.
struct MinDistanceStatistics : AStarCounters
{
    size_t repaired = 0;  ///< Vertices whose learned distance dropped after added edges.
};

/// Exact minimum distance from any source to any target, or uint_t's maximum if none is reachable, from the inputs of
/// one call. Edges are given by a query over inputs, evaluated on demand and never materialized, or as a relation,
/// which is a query of one input.
///
/// Method:
///  1. Edges. The edge query is translated into union normal form (bound/union_normal_form.hpp), and a
///     bound::QueryEvaluator (bound/query_evaluation.hpp) evaluates it over the call's inputs with some columns bound:
///     a vertex's source columns for its successors, its target columns for its predecessors, both for an edge check.
///  2. Search. A* (search/a_star.hpp) with a binary heap ordered by f, then g. The estimate is the maximum of the
///     heuristic H, the distances the search learned, and the backward frontier's bound.
///  3. Bidirectionality. The backward frontier (search/backward_frontier.hpp) advances whenever it is not larger than
///     the forward open list. Its levels are exact distances; once it is exhausted, sources outside it are
///     unreachable. A search ends at a vertex on a valid chain to a target (search/search_tree.hpp).
///
/// Nothing but compiled plans and static inputs is kept across calls; incremental::MinDistance keeps the search tree
/// and updates it from input changes. Inputs are borrowed during min_distance() only.
template<DistanceHeuristic H = BlindDistanceHeuristic>
class MinDistance
{
public:
    /// Edges given by a query over inputs. `edges` has the plan's edge columns; `static_slots` marks its inputs that
    /// never change, which are then read on the first call only.
    /// Its input snapshots are builders of `layer`, which must outlive it.
    MinDistance(const ygg::FreeRelationRepository& layer,
                QueryView<> edges,
                const DistancePlan& plan,
                const urd::Store<urd::set<size_t>, urd::dense>& static_slots = {}) :
        MinDistance(bound::QueryEvaluator(layer, detail::checked_edges(edges, plan), static_slots), plan)
    {
    }

    /// Edges given as a relation with the plan's edge columns.
    MinDistance(const ygg::FreeRelationRepository& layer, const DistancePlan& plan, bool static_edges = false) :
        MinDistance(detail::relation_edges<bound::QueryEvaluator>(layer, plan, static_edges), plan)
    {
    }

    [[nodiscard]] size_t input_count() const noexcept { return m_edges.evaluator().input_count(); }

    /// The minimum distance over the edges of the inputs: input(slot) returns slot's relation view.
    template<SlotRelations Input, RelationViewConcept S, RelationViewConcept T>
    ygg::uint_t min_distance(const Input& input, const S& sources, const T& targets, const H& heuristic = {})
    {
        assert(std::ranges::equal(sources.columns(), m_plan.source_columns()));
        assert(std::ranges::equal(targets.columns(), m_plan.target_columns()));
        m_edges.evaluator().bind(input);
        m_tree.clear();
        m_targets.assign(targets);
        m_backward.start(m_targets.ensure_in(m_tree, heuristic));
        auto edges = m_edges.with_key({});
        auto search = AStar<typename Edges::Keyed, H>(edges, m_tree, m_backward, heuristic, m_targets.vertices(), m_search, m_edges.evaluator().version());
        const auto distance = search.run(sources);
        m_statistics += search.counters();
        return distance;
    }

    [[nodiscard]] const MinDistanceStatistics& statistics() const noexcept { return m_statistics; }

private:
    using Edges = detail::EdgeQuery<bound::QueryEvaluator>;

    MinDistance(bound::QueryEvaluator edges, const DistancePlan& plan) :
        m_plan(plan),
        m_edges(std::move(edges), plan.edge_columns(), 0, plan.arity()),
        m_tree(),
        m_targets(),
        m_search()
    {
    }

    DistancePlan m_plan;
    Edges m_edges;
    SearchTree m_tree;
    BackwardFrontier m_backward;
    detail::TargetSet m_targets;
    AStarStorage m_search;
    MinDistanceStatistics m_statistics;
};

}  // namespace tyr::datalog::database

#endif
