/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef TYR_DATALOG_DATABASE_DATALOG_DETAIL_STRATA_HPP_
#define TYR_DATALOG_DATABASE_DATALOG_DETAIL_STRATA_HPP_

#include "tyr/datalog/lifted/database/datalog/detail/table.hpp"
#include "tyr/datalog/lifted/database/datalog/repository.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <urd/memory.hpp>
#include <urd/repr/maps/flat_map.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>

namespace tyr::datalog::database::datalog::detail
{
struct CompiledProgram;
struct CompiledRule;
class Executor;
class GoalHeuristic;
class GoalFrontier;
class UnitSearch;

/// The relations of a stratum, the rules that derive them, and the tables they read.
struct StratumInfo
{
    urd::Store<urd::sequence<size_t>> tables;
    urd::Store<urd::sequence<size_t>> rules;
    /// The tables its rules read (for an aggregate, its body's table).
    urd::Store<urd::sequence<size_t>> reads;
    [[nodiscard]] bool contains(size_t table) const { return std::ranges::find(tables, table) != tables.end(); }
    /// The position of one of its tables in `tables`.
    [[nodiscard]] size_t position(size_t table) const { return static_cast<size_t>(std::ranges::find(tables, table) - tables.begin()); }
};

/// One stratum of a program, evaluated after the strata it reads. Its tables' `current` rows are its relations; an
/// update reads the changes (`added`, `removed`) of the tables it reads and records the changes of its own.
class Stratum
{
public:
    explicit Stratum(StratumInfo info) : m_info(std::move(info)) {}
    Stratum(const Stratum&) = delete;
    Stratum& operator=(const Stratum&) = delete;
    Stratum(Stratum&&) = delete;
    Stratum& operator=(Stratum&&) = delete;
    virtual ~Stratum() = default;

    [[nodiscard]] const StratumInfo& info() const noexcept { return m_info; }
    /// Its relations from the current rows of the tables it reads, without keeping what update() needs.
    virtual void evaluate(CompiledProgram& program, Executor& executor) = 0;
    /// As evaluate(), keeping what update() needs.
    virtual void initialize(CompiledProgram& program, Executor& executor) { evaluate(program, executor); }
    /// Applies the changes of the tables it reads, after initialize().
    virtual void update(CompiledProgram& program, Executor& executor) = 0;
    [[nodiscard]] size_t memory_usage() const noexcept { return urd::memory_usage(m_reads) + state_memory_usage(); }

protected:
    /// Every literal of the rule reads `all`; the caller may change single literals before the run.
    urd::Store<urd::sequence<Version>>& reads(const CompiledRule& rule, Version all);

private:
    StratumInfo m_info;
    /// What each literal of a rule reads in the next run, rebuilt before each run.
    urd::Store<urd::sequence<Version>> m_reads;

    /// The memory of what the stratum maintains.
    [[nodiscard]] virtual size_t state_memory_usage() const noexcept = 0;
};

/// A stratum without recursion: one pass over its rules, and for updates the derivation count of each row, changed by
/// the rule with one changed literal reading its added or removed rows, earlier literals the new and later ones the old
/// relations [Gupta93, counting] (plain.cpp).
class PlainStratum final : public Stratum
{
public:
    PlainStratum(StratumInfo stratum, const CompiledProgram& program);

    void evaluate(CompiledProgram& program, Executor& executor) override;
    void initialize(CompiledProgram& program, Executor& executor) override;
    void update(CompiledProgram& program, Executor& executor) override;

private:
    [[nodiscard]] size_t state_memory_usage() const noexcept override;

    /// Per table of the stratum, the derivation count of each row and the changes staged by one update.
    struct Counts
    {
        RowSet rows;
        urd::Store<urd::sequence<int64_t>> counts;
        RowSet pending_rows;
        urd::Store<urd::sequence<int64_t>> pending;
        explicit Counts(size_t arity) : rows(arity), pending_rows(arity) {}
    };
    urd::Store<urd::sequence<Counts>> m_counts;

    void derive(CompiledProgram& program, Executor& executor, bool counting);
    void count(size_t table, std::span<const ygg::uint_t> row, int64_t change);
    /// Applies the staged count changes; a row whose count leaves or reaches 0 is added or removed (and, if `record`,
    /// recorded as a change).
    void apply_counts(Table& table, Counts& counts, bool record);
};

/// A recursive stratum: semi-naive evaluation, and Delete and Rederive for updates [Gupta93]: delete every row with a
/// derivation through a removed row (with the old relations), rederive the deleted rows that the remaining rows still
/// derive (the rule with its head bound), then insert semi-naively from the additions and the rederived rows
/// (recursive.cpp).
class RecursiveStratum final : public Stratum
{
public:
    RecursiveStratum(StratumInfo stratum, const CompiledProgram& program);

    void evaluate(CompiledProgram& program, Executor& executor) override;
    void update(CompiledProgram& program, Executor& executor) override;

private:
    [[nodiscard]] size_t state_memory_usage() const noexcept override;

    /// Per table of the stratum: the rows found by the current semi-naive iteration, and its rows before it is
    /// evaluated anew.
    urd::Store<urd::sequence<IndexedTuples>> m_next;
    urd::Store<urd::sequence<RowSet>> m_snapshot;

    /// The relations from the rules alone, into empty tables.
    void derive(CompiledProgram& program, Executor& executor);
    /// Moves the rows found last into the frontier; `add` decides each row's fate. Whether any row was found.
    template<typename Add>
    bool merge(CompiledProgram& program, const Add& add);
    /// Semi-naive iterations from the frontier: each rule once per literal of the stratum, which reads the frontier,
    /// the others `others`; rows that `keep(table, staged, row)` accepts are staged and merged by `add`.
    template<typename Keep, typename Add>
    void iterate(CompiledProgram& program, Executor& executor, Version others, const Keep& keep, const Add& add);
    /// Every rule once per changed literal outside the stratum, which reads `change(kind)`, the stratum's own relations
    /// their current rows and the others `others`; stages the rows `keep` accepts.
    template<typename Change, typename Keep>
    void seed(CompiledProgram& program, Executor& executor, Version others, const Change& change, const Keep& keep);
    /// The over-deletion's semi-naive iterations: a literal of the stratum reads the rows deleted last, the others the
    /// old relations. False once too many rows are deleted.
    template<typename Keep, typename Add>
    bool iterate_deletions(CompiledProgram& program, Executor& executor, const Keep& keep, const Add& add);
    [[nodiscard]] bool overdeleted_too_many(const CompiledProgram& program) const;
    /// Evaluates the stratum anew and records the difference to its old rows as its changes.
    void recompute(CompiledProgram& program, Executor& executor);
};

/// A stratum of weighted relations: each keeps, per row of its other columns, the least weight derivable by rules whose
/// head weight is the sum or maximum of their weighted atoms' weights and other terms. Evaluated by Knuth's lightest
/// derivation algorithm (Felzenszwalb & McAllester 2007, §3): rows are settled in order of weight from a priority queue,
/// each settled row expanded by the rules that read it with the settled rows (weighted.cpp). If only goal rules read the
/// stratum (a min head over one of its relations and the targets), it stops once every goal holds: the first settled
/// row a goal reads has the least weight. A goal with key columns holds per key, and the rows of a key whose goal holds
/// are no longer queued or settled (the rules keep the key). With a heuristic derived from its rules (goal_search.hpp), rows
/// settle in order of weight plus the heuristic (A*, Felzenszwalb & McAllester 2007, §4), which keeps that order
/// correct since the heuristic is monotone. An update evaluates the stratum anew and records the difference to its
/// old rows.
class WeightedStratum final : public Stratum
{
public:
    /// `goals`: the rules of higher strata that read the stratum, if all of them are goals; empty, the stratum is
    /// evaluated completely.
    WeightedStratum(StratumInfo stratum,
                    const CompiledProgram& program,
                    urd::Store<urd::sequence<size_t>> goals,
                    std::unique_ptr<GoalHeuristic> heuristic,
                    std::unique_ptr<GoalFrontier> frontier,
                    std::unique_ptr<UnitSearch> unit,
                    urd::Store<urd::sequence<size_t>> target_tables);
    WeightedStratum(const WeightedStratum&) = delete;
    WeightedStratum& operator=(const WeightedStratum&) = delete;
    WeightedStratum(WeightedStratum&&) = delete;
    WeightedStratum& operator=(WeightedStratum&&) = delete;
    ~WeightedStratum() override;

    void evaluate(CompiledProgram& program, Executor& executor) override;
    void update(CompiledProgram& program, Executor& executor) override;

private:
    [[nodiscard]] size_t state_memory_usage() const noexcept override;

    /// A derived row of a table of the stratum, waiting to be settled, by its weight plus the heuristic.
    struct Candidate
    {
        ygg::uint_t priority = 0;
        size_t position = 0;
        urd::Store<urd::sequence<ygg::uint_t>> row;
        friend bool operator>(const Candidate& lhs, const Candidate& rhs) noexcept { return lhs.priority > rhs.priority; }
        static constexpr auto memory_members() noexcept { return urd::Members<&Candidate::row> {}; }
    };
    /// Per table of the stratum: the rows of the other columns settled, and the least weight queued per such row.
    urd::Store<urd::sequence<RowSet>> m_settled;
    urd::Store<urd::sequence<RowSet>> m_queued;
    urd::Store<urd::sequence<urd::Store<urd::sequence<ygg::uint_t>>>> m_best;
    urd::Store<urd::sequence<Candidate>> m_queue;
    urd::Store<urd::sequence<RowSet>> m_snapshot;
    urd::Store<urd::sequence<size_t>> m_goals;
    /// Per goal without key columns, whether it holds.
    urd::Store<urd::sequence<bool>> m_holds;
    /// Per goal, the table of the stratum it reads, the columns of the read row that are its key, and the keys whose
    /// goal holds.
    struct GoalKeys
    {
        size_t position = 0;
        urd::Store<urd::sequence<size_t>> columns;
        RowSet done;
    };
    urd::Store<urd::sequence<GoalKeys>> m_goal_keys;
    urd::Store<urd::sequence<ygg::uint_t>> m_key;
    std::unique_ptr<GoalHeuristic> m_heuristic;
    std::unique_ptr<GoalFrontier> m_frontier;
    /// With unit costs, the searches of UnitSearch replace the queue (goal_search.hpp).
    std::unique_ptr<UnitSearch> m_unit;
    /// The tables the goals' targets read (GoalSearch::target_tables).
    urd::Store<urd::sequence<size_t>> m_target_tables;
    /// The rows a run derived, offered after it.
    urd::Store<urd::sequence<std::pair<size_t, urd::Store<urd::sequence<ygg::uint_t>>>>> m_derived;

    /// Records the goals that the settled row (the frontier of its table) makes hold; whether every goal holds.
    bool goals_hold(CompiledProgram& program, Executor& executor, size_t table);
    /// Whether the goals that read the table at `position` hold for the row's keys; false for goals without keys.
    [[nodiscard]] bool finished(size_t position, std::span<const ygg::uint_t> row);
    /// Queues a derived row of the stratum's table at `position` unless a lighter one is settled or queued, or the
    /// heuristic shows that it reaches no target.
    void offer(size_t position, std::span<const ygg::uint_t> row);
    void offer_derived();
    /// `fresh`: the inputs were read anew, without changes.
    void derive(CompiledProgram& program, Executor& executor, bool fresh);
};

/// The groups of an aggregate, kept from its body's table (its satisfying assignments) (aggregate.cpp).
struct AggregateTable
{
    size_t table = npos;
    size_t source = npos;
    AggregateFunction function = AggregateFunction::COUNT;
    urd::Store<urd::sequence<size_t>> group;
    size_t value = 0;
};

/// The stratum of one aggregate: per group of its body's rows, the count, sum and value counts, from which its table
/// holds one row per group with its value; updates change the groups of the body's changed rows.
class AggregateStratum final : public Stratum
{
public:
    AggregateStratum(StratumInfo stratum, AggregateTable aggregate);

    void evaluate(CompiledProgram& program, Executor& executor) override;
    void update(CompiledProgram& program, Executor& executor) override;

private:
    [[nodiscard]] size_t state_memory_usage() const noexcept override;

    struct Group
    {
        size_t count = 0;
        uint64_t sum = 0;
        urd::Store<urd::map<ygg::uint_t, size_t>, urd::sorted> values;

        static constexpr auto memory_members() noexcept { return urd::Members<&Group::values> {}; }
    };
    AggregateTable m_aggregate;
    RowSet m_groups;
    urd::Store<urd::sequence<Group>> m_states;
    urd::Store<urd::sequence<ygg::uint_t>> m_key;
    urd::Store<urd::sequence<ygg::uint_t>> m_row;

    [[nodiscard]] std::optional<ygg::uint_t> value_of(const Group& state) const;
    /// The state of the group of a body row, created if absent.
    std::pair<ygg::uint_t, Group&> group_of(std::span<const ygg::uint_t> row);
    void change_group(Group& state, std::span<const ygg::uint_t> row, bool add) const;
    std::span<const ygg::uint_t> group_row(std::span<const ygg::uint_t> group, ygg::uint_t value);
};
}  // namespace tyr::datalog::database::datalog::detail

#endif
