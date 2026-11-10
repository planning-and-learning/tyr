/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef TYR_DATALOG_DATABASE_SEMANTICS_BOUND_QUERY_EVALUATION_HPP_
#define TYR_DATALOG_DATABASE_SEMANTICS_BOUND_QUERY_EVALUATION_HPP_

#include "yggdrasil/core/config.hpp"
#include <urd/config.hpp>
#include <urd/repr/sequences/segmented.hpp>
#include "tyr/datalog/lifted/database/semantics/bound/conjunctive_plan.hpp"
#include "tyr/datalog/lifted/database/semantics/bound/union_normal_form.hpp"
#include "tyr/datalog/lifted/database/semantics/join_index.hpp"
#include "tyr/datalog/lifted/database/semantics/operations.hpp"
#include "tyr/datalog/lifted/database/semantics/relation_view.hpp"

#include <algorithm>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <ranges>
#include <span>
#include <stdexcept>
#include <urd/memory.hpp>
#include <urd/repr/maps/postings.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <utility>
#include <vector>

namespace tyr::datalog::database::bound
{

/// Evaluation of a query with some of its output columns bound to given values, over owned snapshots of its inputs.
///
/// The query is compiled into union normal form (union_normal_form.hpp). Each set of bound columns gets one
/// ConjunctivePlan per branch (conjunctive_plan.hpp), evaluated by index nested-loop joins over a hash index of each
/// input on the columns bound at its step; a negation rejects a row if one of its branches matches. replace() keeps the
/// indexes of a changed input current row by row, as the incremental join's indexes are (incremental/join.hpp).
///
/// Inputs are borrowed during bind() only. incremental::bound::QueryEvaluator wraps one to keep it bound across input
/// changes, and runs its plans with variables bound by changed rows (plans(), bind_atom(), run_plan()). The snapshots
/// are builders of `layer`, read through it; it must outlive the evaluator.
class QueryEvaluator
{
public:
    /// `static_slots` marks inputs that never change after the first bind().
    QueryEvaluator(const ygg::FreeRelationRepository& layer, QueryView<> query, const std::vector<bool>& static_slots = {}) :
        m_layer(&layer),
        m_query(union_normal_form(query)),
        m_columns(query.columns().begin(), query.columns().end())
    {
        m_frame.resize(m_query.variables.size());
        m_static.assign(input_count(), false);
        for (size_t slot = 0; slot < std::min(static_slots.size(), m_static.size()); ++slot)
            m_static[slot] = static_slots[slot];
        for (const auto& columns : m_query.input_columns)
            m_inputs.push_back(layer.get_relation_builder<ygg::Schema>(std::span<const ygg::ColumnLayout>(columns)));
        m_bound.assign(input_count(), false);
    }
    // Its snapshots are its own: shared with no copy.
    QueryEvaluator(const QueryEvaluator&) = delete;
    QueryEvaluator& operator=(const QueryEvaluator&) = delete;
    QueryEvaluator(QueryEvaluator&&) noexcept = default;
    QueryEvaluator& operator=(QueryEvaluator&&) noexcept = default;
    ~QueryEvaluator() = default;

    [[nodiscard]] size_t input_count() const noexcept { return m_query.input_columns.size(); }
    [[nodiscard]] std::span<const ygg::ColumnLayout> columns() const noexcept { return m_columns; }
    [[nodiscard]] ygg::FreeBuilderRelationView input(size_t slot) const { return m_layer->view(*m_inputs.at(slot)); }
    [[nodiscard]] bool bound() const noexcept { return std::ranges::find(m_bound, false) == m_bound.end(); }
    /// Increases whenever an input changes.
    [[nodiscard]] uint64_t version() const noexcept { return m_version; }

    [[nodiscard]] bool is_bound(size_t slot) const noexcept { return m_bound[slot]; }
    [[nodiscard]] bool is_static(size_t slot) const noexcept { return m_static[slot]; }

    /// Binds the current inputs: input(slot) returns slot's relation view. Static slots are read on the first bind
    /// only.
    template<SlotRelations Input>
    void bind(const Input& input)
    {
        for (size_t slot = 0; slot < input_count(); ++slot)
            if (!m_bound[slot] || !m_static[slot])
                bind_slot(slot, input(slot));
    }

    /// Replaces the rows of one input slot.
    template<RelationViewConcept V>
    void bind_slot(size_t slot, const V& rows)
    {
        assert(std::ranges::equal(rows.columns(), m_query.input_columns[slot]));
        auto& snapshot = *m_inputs[slot];
        snapshot.clear();
        for (size_t r = 0; r < rows.size(); ++r)
            snapshot.insert(rows.row(r).get_values());
        m_bound[slot] = true;
        invalidate(slot);
    }

    /// Applies a change to a bound input slot: `removed` rows are in it, `added` rows are not. Built indexes of the slot
    /// are updated in place. Every row is checked before any is applied, so a change that throws leaves the slot, its
    /// indexes and version() unchanged; an empty change does not increase version().
    template<RelationViewConcept A, RelationViewConcept R>
    void replace(size_t slot, const A& added, const R& removed)
    {
        if (added.empty() && removed.empty())
            return;
        auto& snapshot = *m_inputs[slot];
        const auto stored = m_layer->view(snapshot);
        const auto columns = std::span<const ygg::ColumnLayout>(m_query.input_columns[slot]);
        for (size_t r = 0; r < removed.size(); ++r)
            if (!snapshot.contains(removed.row(r).get_values()))
                throw std::invalid_argument("bound::QueryEvaluator: removed row is absent from the input.");
        for (size_t r = 0; r < added.size(); ++r)
            if (snapshot.contains(added.row(r).get_values()))
                throw std::invalid_argument("bound::QueryEvaluator: added row is already present in the input.");
        for (size_t r = 0; r < removed.size(); ++r)
        {
            const auto found = snapshot.find(removed.row(r).get_values());
            URD_ASSUME(found.has_value());  // every removed row was checked present above
            const auto position = *found;
            const auto last = ygg::to_uint_t(snapshot.size() - 1);
            const auto erase = [](auto& rows, ygg::hash_t key, ygg::uint_t value)
            {
                const auto matches = rows.equal_range(key);
                const auto found = std::find_if(matches.first, matches.second, [&](const auto& pair) { return pair.second == value; });
                URD_ASSUME(found != matches.second);  // every indexed row is in its key's entries
                rows.erase(found);
            };
            for_each_index(slot, [&](KeyIndex& entry) { erase(entry.rows, database::detail::join_key_hash(stored.row(position).get_values(), entry.keys), position); });
            snapshot.erase(position);
            if (position != last)
                for_each_index(slot,
                               [&](KeyIndex& entry)
                               {
                                   // The last row moved into the erased one's position.
                                   const auto key = database::detail::join_key_hash(stored.row(position).get_values(), entry.keys);
                                   erase(entry.rows, key, last);
                                   entry.rows.insert({ key, position });
                               });
        }
        for (size_t r = 0; r < added.size(); ++r)
        {
            const auto position = snapshot.insert(added.row(r).get_values());
            for_each_index(slot, [&](KeyIndex& entry) { entry.rows.insert({ database::detail::join_key_hash(added.row(r).get_values(), entry.keys), position }); });
        }
        ++m_version;
    }

    /// Evaluation with the output columns at `positions` bound, in that order; returns its handle. The plans are
    /// compiled at the first evaluation, when the inputs' sizes are known.
    size_t prepare(urd::Store<urd::sequence<size_t>> positions)
    {
        for (size_t binding = 0; binding < m_bindings.size(); ++binding)
            if (m_bindings[binding].positions == positions)
                return binding;
        m_bindings.push_back(Binding { std::move(positions), {} });
        return m_bindings.size() - 1;
    }

    /// Calls on_row(row) for each output row whose prepared columns hold `values` (their canonical values, in the
    /// prepared order); stops when on_row returns false.
    template<RowPredicate OnRow>
    void evaluate(size_t binding, std::span<const ygg::Value> values, const OnRow& on_row)
    {
        auto& prepared = m_bindings.at(binding);
        if (prepared.plans.empty() && !m_query.branches.empty())
            prepared.plans = plans(bound_variables(prepared.positions));
        for (const auto& plan : prepared.plans)
        {
            for (size_t i = 0; i < prepared.positions.size(); ++i)
                m_frame[m_query.output[prepared.positions[i]]] = values[i];
            if (!run_plan(plan, on_row))
                return;
        }
    }

    [[nodiscard]] const UnionNormalForm& query() const noexcept { return m_query; }

    /// One plan per branch, with the variables marked in `bound` bound on arrival, for the current inputs' sizes.
    std::vector<ConjunctivePlan> plans(const std::vector<bool>& bound);  // std::vector: of an incomplete type (a Store resolves its element type at once)

    /// Binds the atom's variables to an input row; false if the row repeats a variable with different values.
    bool bind_atom(const ConjunctiveAtom& atom, const ygg::RowPool::row_type& row);

    /// Runs a plan whose initially bound variables are set, calling on_row(row) for each output row; returns false
    /// once on_row returned false.
    template<RowPredicate OnRow>
    bool run_plan(const ConjunctivePlan& plan, const OnRow& on_row)
    {
        return !accepts(plan, plan.filters) || run(plan, 0, [&] { return on_row(output_row()); });
    }

private:
    struct Binding
    {
        urd::Store<urd::sequence<size_t>> positions;
        std::vector<ConjunctivePlan> plans;  // std::vector: of an incomplete type (a Store resolves its element type at once)
    };
    /// The rows of an input slot keyed by some of its columns; with no key columns, a scan.
    struct KeyIndex
    {
        size_t slot = 0;
        urd::Store<urd::sequence<size_t>> positions;
        urd::Store<urd::sequence<ColumnSlice>> keys;           ///< The key columns in the slot's rows.
        urd::Store<urd::map<ygg::hash_t, ygg::uint_t>, urd::multi, urd::retains> rows;  ///< Row positions by key hash, once built.
        bool built = false;
    };
    /// A step's lookup: its index and where the key's variables are in the frame.
    struct Lookup
    {
        size_t index = 0;
        urd::Store<urd::sequence<ColumnSlice>> probe;
    };

    const ygg::FreeRelationRepository* m_layer;
    UnionNormalForm m_query;
    urd::Store<urd::sequence<ygg::ColumnLayout>> m_columns;
    urd::Store<urd::sequence<ygg::Value>> m_frame;  ///< The variables' values while a branch runs, by variable.
    std::vector<bool> m_static;
    urd::Store<urd::sequence<std::shared_ptr<urd::builder<ygg::Relation<ygg::Schema>>>>> m_inputs;  ///< The input snapshots, from the layer.
    std::vector<bool> m_bound;
    urd::Store<urd::sequence<KeyIndex>, urd::stable_refs> m_indexes;
    urd::Store<urd::sequence<Lookup>, urd::stable_refs> m_lookups;  ///< By ConjunctiveStep::index.
    urd::Store<urd::sequence<Binding>> m_bindings;
    urd::Store<urd::sequence<ygg::Value>> m_row;
    uint64_t m_version = 0;

    std::span<const ygg::Value> output_row()
    {
        m_row.clear();
        for (const auto var : m_query.output)
            m_row.push_back(m_frame[var]);
        return m_row;
    }

    [[nodiscard]] std::vector<bool> bound_variables(std::span<const size_t> positions) const
    {
        auto bound = std::vector<bool>(m_query.variables.size(), false);
        for (const auto position : positions)
            bound[m_query.output[position]] = true;
        return bound;
    }

    /// The slot's rows were replaced: its indexes are rebuilt, in place, when next used.
    void invalidate(size_t slot)
    {
        for (auto& entry : m_indexes)
            if (entry.slot == slot)
                entry.built = false;
        ++m_version;
    }

    /// Calls f on each built index of the slot that has key columns.
    template<typename F>
        requires std::invocable<const F&, KeyIndex&>
    void for_each_index(size_t slot, const F& f)
    {
        for (auto& entry : m_indexes)
            if (entry.slot == slot && entry.built && !entry.positions.empty())
                f(entry);
    }

    // ---- Plans --------------------------------------------------------------------------------------------------

    size_t index_id(size_t slot, std::span<const std::pair<size_t, uint32_t>> key);

    /// Resolves each step's lookup structure, recursively into negations.
    void resolve(ConjunctivePlan& plan);

    // ---- Execution ----------------------------------------------------------------------------------------------

    bool accepts(const ConjunctivePlan& plan, const ConjunctiveFilters& filters);

    /// Depth-first over the plan's steps; returns false once `emit` asked to stop. An existence step stops at its first
    /// row that reaches the rest of the plan.
    template<typename Emit>
        requires std::predicate<const Emit&>
    bool run(const ConjunctivePlan& plan, size_t step_index, const Emit& emit)
    {
        if (step_index == plan.steps.size())
            return emit();
        const auto& step = plan.steps[step_index];
        assert(step.index != ConjunctiveStep::unresolved);
        const auto rows = m_layer->view(*m_inputs[step.slot]);
        auto proceed = true;  // False once emit asked to stop.
        const auto visit = [&](size_t r)
        {
            const auto row = rows.row(r).get_values();
            for (const auto& [c, var] : step.assign)
                m_frame[var] = row.get(c);
            for (const auto& [c, var] : step.compare)
                if (row.get(c) != m_frame[var])
                    return true;
            if (!accepts(plan, step.filters))
                return true;
            proceed = run(plan, step_index + 1, emit);
            return proceed && !step.exists;
        };
        const auto& lookup = m_lookups[step.index];
        auto& entry = m_indexes[lookup.index];
        if (entry.positions.empty())
        {
            for (size_t r = 0; r < rows.size(); ++r)
                if (!visit(r))
                    return proceed;
            return true;
        }
        if (!entry.built)
        {
            entry.rows.clear();
            entry.rows.reserve(rows.size());
            for (size_t r = 0; r < rows.size(); ++r)
                entry.rows.insert({ database::detail::join_key_hash(rows.row(r).get_values(), entry.keys), ygg::to_uint_t(r) });
            entry.built = true;
        }
        // The key's variables stay bound while this step iterates, so the frame is the probe.
        const auto probe = std::span<const ygg::Value>(m_frame);
        for (const auto [first, last] = entry.rows.equal_range(database::detail::join_key_hash(probe, lookup.probe)); const auto r : std::ranges::subrange(first, last) | std::views::values)
            if (database::detail::join_keys_equal(rows.row(r).get_values(), entry.keys, probe, lookup.probe) && !visit(r))
                return proceed;
        return true;
    }
};

}  // namespace tyr::datalog::database::bound

#endif
