/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef TYR_DATALOG_DATABASE_DATALOG_DETAIL_GOAL_SEARCH_HPP_
#define TYR_DATALOG_DATABASE_DATALOG_DETAIL_GOAL_SEARCH_HPP_

#include "tyr/datalog/lifted/database/datalog/detail/program.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <urd/repr/maps/flat_map.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <utility>

namespace tyr::datalog::database::datalog::detail
{
class Executor;

/// A rule run from values of some of its terms (Executor::run_bound).
struct BoundRule
{
    CompiledRule rule;
    urd::Store<urd::sequence<Term>> inputs;
    Plan plan;
};

/// The search of a weighted relation R that only goals read (WeightedStratum): the stratum's one table, rules that read
/// it once and add weights only (+), and goals whose other literals give the targets, the keys of R they accept.
struct GoalSearch
{
    size_t table;
    /// The recursive rules, each with its one read of R.
    urd::Store<urd::sequence<std::pair<size_t, size_t>>> rules;
    /// The key columns of R that every goal's other literals bind, and per goal those literals with these columns of
    /// its read's key as head.
    urd::Store<urd::sequence<size_t>> target_columns;
    urd::Store<urd::sequence<CompiledRule>> targets;
    /// Per goal, the key columns of its read of R that are its key.
    urd::Store<urd::sequence<urd::Store<urd::sequence<size_t>>>> goal_columns;
    /// The tables that the targets' literals read: the targets change only with them.
    urd::Store<urd::sequence<size_t>> target_tables;
    /// Whether no weight of the goals' heads can be observed (set by the translation): then a goal holds with any
    /// weight a derivation gives, a search may stop at the first target it reaches, and its row is derived at weight 0.
    bool any_path = false;

    /// The abstract edges with the literals over the current state that reverse() keeps as filters too, and those
    /// literals' tables.
    struct StateEdges
    {
        urd::Store<urd::sequence<BoundRule>> edges;
        urd::Store<urd::sequence<size_t>> tables;
    };

    /// None unless the stratum has this shape.
    static std::optional<GoalSearch> make(CompiledProgram& program, const StratumInfo& stratum, std::span<const size_t> goals);
    /// The recursive rules reversed, over the key columns `columns`: run from a head row's `columns`, they derive the
    /// read row's `columns` with the edge's weight (the read row's weight taken as 0). With `abstract`, only literals over
    /// static inputs (and what they compute) are kept, so that the derivations include the concrete ones in every state,
    /// and rules that keep every column of `columns` (self-loops there) are left out; otherwise every literal. Literals
    /// left unbound are dropped, which only adds derivations. With `abstract` and `state`, also the same edges with the
    /// other literals that those bind completely as filters (read in the current state) into `state`. None if a rule
    /// cannot bind the read row's `columns` and the weight.
    [[nodiscard]] std::optional<urd::Store<urd::sequence<BoundRule>>>
    reverse(CompiledProgram& program, std::span<const size_t> columns, bool abstract, StateEdges* state = nullptr) const;
    /// Adds the targets of the current state, rows of `target_columns`.
    void read_targets(Executor& executor, urd::Store<urd::sequence<Version>>& reads, RowSet& keys) const;
};

/// A* for a GoalSearch, with a heuristic derived from its rules (Felzenszwalb & McAllester 2007, §5.1), per goal key to
/// its own targets where the goal's key columns are target columns (pooled targets bound the distance to the nearest
/// target of any key, much weaker): an abstraction
/// keeps of each recursive rule only its literals over static inputs (and what they compute), so that its derivations
/// include the concrete ones at the same weights, and of the key only one column (a single-column pattern database,
/// Culberson & Schaeffer 1998). h(key) is the maximum over the target columns of the least
/// abstract weight from the key's column to a target's; a maximum of consistent heuristics is consistent (Holte et al.,
/// AIJ 2006). Each column's weights are computed for every value at once, by Dijkstra's algorithm backward from the
/// targets over its reversed rules, once per set of targets on the first estimate that needs them, and kept for as long
/// as the static inputs stay the same (implementation decision: the targets of a policy's states repeat).
class GoalHeuristic
{
public:
    /// None unless some target column has reversed abstract rules. With `proofs`, also the abstraction of the current
    /// state that unreachable() searches (for a search that calls it).
    static std::unique_ptr<GoalHeuristic> make(CompiledProgram& program, const GoalSearch& search, bool proofs);

    /// Reads the targets of the current state unless `changed` is false and they were read, and notes the state that
    /// unreachable() reads if a state table changed; the abstract weights are computed on first use.
    void read_targets(CompiledProgram& program, Executor& executor, bool changed);
    /// The heuristic of the key; Arithmetic::infinity if no target is reachable from it in a projection.
    [[nodiscard]] ygg::uint_t operator()(std::span<const ygg::uint_t> key);
    /// Whether no target of the key's goal key is reachable from it in the abstraction that also keeps the literals over
    /// the current state that it binds completely (a proof for the current state only: its tables are computed per
    /// state, on use): an abstraction in which the targets are unreachable proves it without a search.
    [[nodiscard]] bool unreachable(std::span<const ygg::uint_t> key);
    /// About what unreachable() costs: the number of abstract values its backward searches can visit, by the static
    /// projections of their columns (unbounded without literals over the state).
    [[nodiscard]] size_t proof_cost() const noexcept;
    /// Changes whenever read_targets() changes the function for the goal key (the goal's key columns of a key of R), so
    /// that bounds learned with it can be discarded.
    [[nodiscard]] uint64_t generation(std::span<const ygg::uint_t> goal_key) const;
    [[nodiscard]] size_t memory_usage() const noexcept;

private:
    struct Projection
    {
        urd::Store<urd::sequence<size_t>> columns;
        urd::Store<urd::sequence<BoundRule>> edges;
        RowSet keys;
        /// Per set of projected targets (its sorted rows, flattened), the abstract weight to them of each key in
        /// `keys`, infinity for keys beyond its size.
        urd::Store<urd::map<urd::Store<urd::sequence<ygg::uint_t>>, urd::Store<urd::sequence<ygg::uint_t>>>, urd::sorted> weights;
        /// Per goal key, its current targets, their key of `weights` (empty without targets) and their weights once
        /// computed.
        urd::Store<urd::sequence<RowSet>> targets;
        urd::Store<urd::sequence<urd::Store<urd::sequence<ygg::uint_t>>>> wanted;
        urd::Store<urd::sequence<const urd::Store<urd::sequence<ygg::uint_t>>*>> current;
        /// For a state projection, the static projection of its column, whose targets it uses (and its key there,
        /// followed by the state's id); npos otherwise.
        size_t domain = npos;
    };
    static constexpr size_t max_weights = 64;
    GoalSearch m_search;
    urd::Store<urd::sequence<Projection>> m_projections;
    /// The projections with the literals over the current state, the tables they read, and an id per distinct state of
    /// those (the latest few), the current one's in m_state.
    urd::Store<urd::sequence<Projection>> m_state_projections;
    urd::Store<urd::sequence<size_t>> m_state_tables;
    urd::Store<urd::map<urd::Store<urd::sequence<ygg::uint_t>>, ygg::uint_t>, urd::sorted> m_states;
    ygg::uint_t m_state = 0;
    ygg::uint_t m_next_state = 0;
    /// With one goal whose key columns are target columns, their positions in target_columns: each goal key has
    /// weights to its own targets. Otherwise empty, and one goal key, (0), has every target.
    urd::Store<urd::sequence<size_t>> m_goal_positions;
    urd::Store<urd::sequence<size_t>> m_goal_columns;
    urd::Store<urd::sequence<ygg::uint_t>> m_no_goal_key { 0 };
    RowSet m_goal_keys;
    /// Per goal key, its targets (indices into m_target_keys) and its generation.
    urd::Store<urd::sequence<urd::Store<urd::sequence<ygg::uint_t>>>> m_members;
    urd::Store<urd::sequence<uint64_t>> m_generations;
    Executor* m_executor = nullptr;
    bool m_read = false;
    RowSet m_target_keys;
    urd::Store<urd::sequence<Version>> m_reads;
    urd::Store<urd::sequence<urd::Store<urd::sequence<ygg::uint_t>>>> m_derived;
    urd::Store<urd::sequence<ygg::uint_t>> m_key;

    explicit GoalHeuristic(const GoalSearch& search) : m_search(search), m_goal_keys(1), m_target_keys(search.target_columns.size()) {}
    /// The weights of the goal key's current targets, computed on first use; null without targets.
    const urd::Store<urd::sequence<ygg::uint_t>>* weights(Projection& projection, ygg::uint_t group);
    /// The projection's weight of a key of R, of goal key `group`; infinity without one.
    ygg::uint_t weight(Projection& projection, ygg::uint_t group, std::span<const ygg::uint_t> key);
    /// The abstract weights to the projected targets, by Dijkstra's algorithm backward from them.
    urd::Store<urd::sequence<ygg::uint_t>> backward(Executor& executor, Projection& projection, const RowSet& targets);
};

/// The other side of a bidirectional search for a GoalSearch with one goal whose targets bind every key column: per key
/// of the goal, a breadth-first search backward from the key's targets over the rules reversed with all their
/// literals, in the current state, so that its rows are keys of R from which a target is derivable. It advances by one
/// row per row the stratum settles for the goal key (bidirectional search, Pohl 1971; whichever side is smaller is
/// exhausted first). A source among its rows ends it: the stratum's search reaches a target.
/// Exhausted without one, it proves that the goal key's goal has no row (implementation decisions).
class GoalFrontier
{
public:
    /// None unless the stratum has one goal, its targets bind every key column and every rule can be reversed.
    static std::unique_ptr<GoalFrontier> make(CompiledProgram& program, const GoalSearch& search);

    /// Starts anew with the targets of the current state.
    void read_targets(Executor& executor);
    /// A source: a key of R that the stratum's base rules derive.
    void add_source(std::span<const ygg::uint_t> key);
    /// Advances the search of the key's goal key by one row.
    void advance(Executor& executor, std::span<const ygg::uint_t> key);
    /// Whether the key's goal key is known to have no target that the stratum reaches.
    [[nodiscard]] bool excluded(std::span<const ygg::uint_t> key);
    [[nodiscard]] size_t memory_usage() const noexcept;

private:
    enum class Status : uint8_t
    {
        SEARCHING,
        REACHABLE,
        UNREACHABLE,
    };
    /// The backward search of one goal key: its rows (indices into m_rows) in the order found.
    struct Side
    {
        urd::Store<urd::sequence<ygg::uint_t>> rows;
        size_t next = 0;
        Status status = Status::SEARCHING;
    };
    GoalSearch m_search;
    urd::Store<urd::sequence<size_t>> m_goal_columns;
    urd::Store<urd::sequence<BoundRule>> m_edges;
    /// The goal keys with targets, each with its side; the rows found backward; the sources.
    RowSet m_goal_keys;
    urd::Store<urd::sequence<Side>> m_sides;
    RowSet m_rows;
    RowSet m_sources;
    urd::Store<urd::sequence<Version>> m_reads;
    urd::Store<urd::sequence<urd::Store<urd::sequence<ygg::uint_t>>>> m_derived;
    urd::Store<urd::sequence<ygg::uint_t>> m_key;

    GoalFrontier(const GoalSearch& search, urd::Store<urd::sequence<size_t>> goal_columns, urd::Store<urd::sequence<BoundRule>> edges);
    /// The goal key of a key of R; a goal without key columns has one goal key, (0).
    std::span<const ygg::uint_t> goal_key(std::span<const ygg::uint_t> key);
};
/// The search of a unit-cost GoalSearch (base rules of weight 0, recursive rules adding 1) whose targets bind every key
/// column, with the stratum's rules as edges: per goal
/// key, A* (search/a_star.hpp) from the key's sources, estimating with the maximum of the GoalHeuristic, the distances
/// learned by earlier searches (Generalized Adaptive A*, Sun, Koenig & Yeoh 2008) and the bound of a backward frontier
/// (Pohl's cardinality comparison), ending on a stored path that still holds (Multipath GAA*, Hernández, Asín & Baier
/// 2015). Per goal key the search tree and the backward frontier are kept while the key's targets stay the same:
/// removed edges keep the learned distances admissible, and the edges that an update adds (the rules run with a changed
/// literal reading its change) are repaired (TreeRepair) or restart the frontier; a change these rules cannot express
/// discards the trees. A search derives only the row of the target it reaches, at the distance: the goals read nothing
/// else. With GoalSearch::any_path, it ends at the first path it finds (AStar's any_path) and derives the row at weight 0.
class UnitSearch
{
public:
    /// None unless the stratum has that shape and every rule can be reversed with all its literals.
    static std::unique_ptr<UnitSearch> make(CompiledProgram& program, const StratumInfo& stratum, const GoalSearch& search);
    UnitSearch(const UnitSearch&) = delete;
    UnitSearch& operator=(const UnitSearch&) = delete;
    UnitSearch(UnitSearch&&) = delete;
    UnitSearch& operator=(UnitSearch&&) = delete;
    ~UnitSearch();

    /// Discards the stored searches (the inputs were not changed incrementally).
    void reset() noexcept;
    /// Records the edges that the changes of the inputs (their added and removed rows) add.
    void changed(CompiledProgram& program, Executor& executor);
    /// Searches each goal key from its sources (rows of R derived by the base rules) and derives the reached target's
    /// row into R's current rows. `heuristic` estimates for the current targets, or is null.
    /// `targets_changed` false: the targets are those of the last run.
    void run(CompiledProgram& program,
             Executor& executor,
             std::span<const urd::Store<urd::sequence<ygg::uint_t>>> sources,
             GoalHeuristic* heuristic,
             bool targets_changed);
    [[nodiscard]] size_t memory_usage() const noexcept;

private:
    struct Key;
    struct Oracle;
    /// A rule run with one literal reading its change, deriving the added edges: the read row's key, then the head's,
    /// each in the key columns `columns`. Goal key columns that the rule leaves unbound are missing: the rules keep the
    /// goal key, so the edge exists for every goal key.
    struct Delta
    {
        CompiledRule rule;
        size_t literal;
        Version change;
        urd::Store<urd::sequence<size_t>> columns;
    };
    GoalSearch m_search;
    urd::Store<urd::sequence<size_t>> m_goal_columns;
    /// The recursive rules without their read of R, run from a key at weight 0, and reversed; `m_edges` are the
    /// forward rules run with the head's key bound too, which check one edge.
    urd::Store<urd::sequence<BoundRule>> m_forward;
    urd::Store<urd::sequence<BoundRule>> m_edges;
    urd::Store<urd::sequence<BoundRule>> m_reversed;
    urd::Store<urd::sequence<Delta>> m_deltas;
    /// The tables whose changes the delta rules cannot express, and those that the recursive rules read.
    urd::Store<urd::sequence<size_t>> m_opaque;
    urd::Store<urd::sequence<size_t>> m_edge_tables;
    RowSet m_goal_keys;
    urd::Store<urd::sequence<std::unique_ptr<Key>>> m_keys;
    /// Per goal key, its targets as vertices, from the last run that read them.
    urd::Store<urd::sequence<urd::Store<urd::sequence<urd::Store<urd::sequence<ygg::uint_t>>>>>> m_key_targets;
    bool m_read = false;
    uint64_t m_version = 0;
    urd::Store<urd::sequence<Version>> m_reads;
    urd::Store<urd::sequence<urd::Store<urd::sequence<ygg::uint_t>>>> m_derived;
    urd::Store<urd::sequence<ygg::uint_t>> m_key;

    UnitSearch(const GoalSearch& search,
               urd::Store<urd::sequence<BoundRule>> forward,
               urd::Store<urd::sequence<BoundRule>> edges,
               urd::Store<urd::sequence<BoundRule>> reversed);
    std::span<const ygg::uint_t> goal_key(std::span<const ygg::uint_t> key);
    /// The index of the goal key, with its search made on first use.
    ygg::uint_t key_search(std::span<const ygg::uint_t> goal_key);
};
}  // namespace tyr::datalog::database::datalog::detail

#endif
