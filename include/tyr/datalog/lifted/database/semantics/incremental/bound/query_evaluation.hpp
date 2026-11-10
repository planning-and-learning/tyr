/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef TYR_DATALOG_DATABASE_SEMANTICS_INCREMENTAL_BOUND_QUERY_EVALUATION_HPP_
#define TYR_DATALOG_DATABASE_SEMANTICS_INCREMENTAL_BOUND_QUERY_EVALUATION_HPP_

#include "tyr/datalog/lifted/database/semantics/bound/query_evaluation.hpp"
#include "tyr/datalog/lifted/database/semantics/operations.hpp"
#include "tyr/datalog/lifted/database/semantics/relation_view.hpp"

#include <cassert>
#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <urd/repr/maps/std_map.hpp>
#include <urd/repr/maps/unordered_flat_map.hpp>
#include <urd/repr/values/dense_set.hpp>
#include <urd/repr/values/unordered_flat_set.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <utility>
#include <vector>

namespace tyr::datalog::database::incremental::bound
{

/// The rows bind() added to and removed from an input slot.
struct InputChange
{
    size_t slot = 0;
    urd::builder<ygg::Relation<ygg::Schema>> added;
    urd::builder<ygg::Relation<ygg::Schema>> removed;
    SetPlan plan;  ///< Of the slot's columns: what diffs the slot's rows.
};

/// A bound::QueryEvaluator kept across input changes. initialize() binds the first inputs; update() takes a caller's
/// exact change, or bind() diffs the inputs against the bound snapshots; and seed() finds the rows a change can add by
/// delta rules for unions of conjunctive queries (Gupta & Mumick, "Maintenance of Materialized Views: Problems,
/// Techniques, and Applications", IEEE Data Eng. Bull. 1995): a branch is evaluated with the changed atom's variables
/// bound to each added row of a positive occurrence, or to each removed row of a negated one. The result is a superset
/// of the added rows, each of which exists in the new state.
///
/// version() increases only on changes that can remove output rows: removed rows of an input that occurs positively, or
/// added rows of one that occurs negated (under an odd number of negations). Unions of conjunctive queries with
/// negation are monotone in their positive atoms and antitone in their negated ones (Abiteboul, Hull & Vianu,
/// "Foundations of Databases", 1995), so other changes keep every output row, and checks of earlier rows stay valid.
class QueryEvaluator
{
public:
    /// Its builders are read through `layer`, which must outlive it.
    QueryEvaluator(const ygg::FreeRelationRepository& layer, QueryView<> query, const urd::Store<urd::set<size_t>, urd::dense>& static_slots = {}) :
        m_layer(&layer),
        m_base(layer, query, static_slots)
    {
        for (size_t slot = 0; slot < input_count(); ++slot)
        {
            const auto columns = std::span<const ygg::ColumnLayout>(m_base.query().input_columns[slot]);
            m_changes.push_back(InputChange { slot, urd::builder<ygg::Relation<ygg::Schema>>(columns), urd::builder<ygg::Relation<ygg::Schema>>(columns), SetPlan(columns, columns) });
        }
        m_changed.reserve(input_count());
        const auto mark = [&](auto&& self, const database::bound::ConjunctiveBranch& branch, bool negated) -> void
        {
            for (const auto& atom : branch.atoms)
                (negated ? m_negative : m_positive).insert(atom.slot);
            for (const auto& negation : branch.negations)
                for (const auto& inner : negation.branches)
                    self(self, inner, !negated);
        };
        for (const auto& branch : m_base.query().branches)
            mark(mark, branch, false);
    }

    [[nodiscard]] size_t input_count() const noexcept { return m_base.input_count(); }
    [[nodiscard]] std::span<const ygg::ColumnLayout> columns() const noexcept { return m_base.columns(); }
    [[nodiscard]] ygg::FreeBuilderRelationView input(size_t slot) const { return m_base.input(slot); }
    [[nodiscard]] bool bound() const noexcept { return m_base.bound(); }
    [[nodiscard]] bool is_static(size_t slot) const noexcept { return m_base.is_static(slot); }
    /// Increases whenever a change can remove output rows (see the class comment).
    [[nodiscard]] uint64_t version() const noexcept { return m_version; }
    size_t prepare(urd::Store<urd::sequence<size_t>> positions) { return m_base.prepare(std::move(positions)); }
    template<RowPredicate OnRow>
    void evaluate(size_t binding, std::span<const ygg::Value> values, const OnRow& on_row)
    { m_base.evaluate(binding, values, on_row); }

    /// Binds every input, static ones included, as a new baseline: input(slot) returns slot's relation view.
    template<SlotRelations Input>
    void initialize(const Input& input)
    {
        m_changed.clear();
        for (size_t slot = 0; slot < input_count(); ++slot)
            m_base.bind_slot(slot, input(slot));
        ++m_version;
    }

    /// Diffs the current inputs against the bound snapshots: input(slot) returns slot's relation view. Slots not bound
    /// yet are bound, as by initialize(); static slots are read then only. After that, where `maybe_changed` is given,
    /// the slots not in it are assumed unchanged and are not read either. Returns the slots bound before that changed,
    /// whose changes change(slot) holds; both are valid until the next bind().
    template<SlotRelations Input>
    std::span<const size_t> bind(const Input& input, const urd::Store<urd::set<size_t>, urd::dense>* maybe_changed = nullptr)
    {
        m_changed.clear();
        for (size_t slot = 0; slot < input_count(); ++slot)
        {
            if (!m_base.is_bound(slot))
            {
                m_base.bind_slot(slot, input(slot));
                continue;
            }
            if (m_base.is_static(slot) || (maybe_changed && !maybe_changed->contains(slot)))
                continue;
            auto& change = m_changes[slot];
            diff(input(slot), change);
            if (change.added.empty() && change.removed.empty())
                continue;
            apply(slot, m_layer->view(change.added), m_layer->view(change.removed));
            m_changed.push_back(slot);
        }
        return m_changed;
    }
    /// The (added, removed) change of a slot that the last bind() returned, as views.
    [[nodiscard]] std::pair<ygg::FreeBuilderRelationView, ygg::FreeBuilderRelationView> change(size_t slot) const
    {
        const auto& change = m_changes.at(slot);
        return { m_layer->view(change.added), m_layer->view(change.removed) };
    }

    /// Applies an exact change to a bound, non-static slot: `removed` rows are in it, `added` rows are not. A change
    /// that throws leaves the slot and version() unchanged (bound::QueryEvaluator::replace).
    template<RelationViewConcept A, RelationViewConcept R>
    void update(size_t slot, const A& added, const R& removed)
    {
        if (slot >= input_count())
            throw std::invalid_argument("incremental::bound::QueryEvaluator: slot is out of range.");
        assert(std::ranges::equal(added.columns(), m_base.query().input_columns[slot]));
        assert(std::ranges::equal(removed.columns(), m_base.query().input_columns[slot]));
        if (!m_base.is_bound(slot) || m_base.is_static(slot))
            throw std::invalid_argument("incremental::bound::QueryEvaluator: changes apply to bound, non-static slots only.");
        apply(slot, added, removed);
    }

    /// Calls on_row(row) for every output row the slot's change (applied before) can add, with repetitions and rows
    /// that existed before. Returns false if a changed row binds variables outside its branch's atoms, which delta
    /// rules cannot express; then the added rows are unknown.
    template<RelationViewConcept A, RelationViewConcept R, typename OnRow>
        requires std::invocable<const OnRow&, std::span<const ygg::Value>>
    bool seed(size_t slot, const A& added, const R& removed, const OnRow& on_row)
    {
        const auto each = [&](std::span<const ygg::Value> row)
        {
            on_row(row);
            return true;
        };
        for (size_t b = 0; b < m_base.query().branches.size(); ++b)
            if (!seed_branch(b, slot, added, removed, each))
                return false;
        return true;
    }

private:
    const ygg::FreeRelationRepository* m_layer;
    database::bound::QueryEvaluator m_base;
    /// By the variables bound on arrival, as database::bound::QueryEvaluator::plans() returns them.
    urd::Store<urd::map<database::bound::VariableSet, urd::Store<urd::sequence<database::bound::ConjunctivePlan>>>> m_seed_plans;
    urd::Store<urd::sequence<InputChange>> m_changes;     ///< By slot; reused across bind() calls.
    urd::Store<urd::sequence<size_t>> m_changed;          ///< The slots the last bind() changed.
    urd::Store<urd::set<size_t>, urd::dense> m_positive;  ///< The slots that occur under an even number of negations.
    urd::Store<urd::set<size_t>, urd::dense> m_negative;  ///< The slots that occur under an odd number of negations.
    uint64_t m_version = 0;
    database::bound::VariableSet m_bound;  ///< Scratch of seed_plan() and seed_branch(), kept to avoid allocations.
    database::bound::VariableSet m_top_vars;

    /// Applies a change that is known to be valid (see update()).
    template<RelationViewConcept A, RelationViewConcept R>
    void apply(size_t slot, const A& added, const R& removed)
    {
        m_base.replace(slot, added, removed);
        if ((m_positive.contains(slot) && !removed.empty()) || (m_negative.contains(slot) && !added.empty()))
            ++m_version;
    }

    /// Replaces `change` by the difference of `rows` from the slot's snapshot.
    template<RelationViewConcept V>
    void diff(const V& rows, InputChange& change) const
    {
        const auto snapshot = m_base.input(change.slot);
        database::difference(rows, snapshot, change.plan, change.added);
        database::difference(snapshot, rows, change.plan, change.removed);
    }

    /// The plan of a branch with the atom's variables bound; valid until the next call.
    const database::bound::ConjunctivePlan& seed_plan(size_t branch, const database::bound::ConjunctiveAtom& atom)
    {
        m_bound.clear();
        m_bound.insert(atom.vars.begin(), atom.vars.end());
        auto found = m_seed_plans.find(m_bound);
        if (found == m_seed_plans.end())
            found = m_seed_plans.emplace(m_bound, m_base.plans(m_bound)).first;
        return found->second[branch];
    }

    template<RelationViewConcept A, RelationViewConcept R, RowPredicate OnRow>
    bool seed_branch(size_t b, size_t slot, const A& added, const R& removed, const OnRow& on_row)
    {
        const auto& query = m_base.query();
        const auto& top = query.branches[b];
        auto& top_vars = m_top_vars;
        top_vars.clear();
        for (const auto& atom : top.atoms)
            top_vars.insert(atom.vars.begin(), atom.vars.end());
        const auto seed_atom = [&](const database::bound::ConjunctiveAtom& atom, const auto& rows) -> bool
        {
            if (atom.slot != slot || rows.empty())
                return true;
            if (!std::ranges::all_of(atom.vars, [&](uint32_t var) { return top_vars.contains(var); }))
                return false;
            const auto& plan = seed_plan(b, atom);
            for (size_t r = 0; r < rows.size(); ++r)
                if (m_base.bind_atom(atom, rows.row(r).get_values()))
                    m_base.run_plan(plan, on_row);
            return true;
        };
        const auto walk = [&](auto&& self, const database::bound::ConjunctiveBranch& branch, bool negated) -> bool
        {
            for (const auto& atom : branch.atoms)
                if (!(negated ? seed_atom(atom, removed) : seed_atom(atom, added)))
                    return false;
            for (const auto& negation : branch.negations)
                for (const auto& inner : negation.branches)
                    if (!self(self, inner, !negated))
                        return false;
            return true;
        };
        return walk(walk, top, false);
    }
};

}  // namespace tyr::datalog::database::incremental::bound

#endif
