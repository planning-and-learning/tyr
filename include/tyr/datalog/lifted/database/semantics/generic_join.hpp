/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef TYR_DATALOG_DATABASE_SEMANTICS_GENERIC_JOIN_HPP_
#define TYR_DATALOG_DATABASE_SEMANTICS_GENERIC_JOIN_HPP_

#include "tyr/datalog/lifted/database/semantics/generic_join_plan.hpp"
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include "tyr/datalog/lifted/database/semantics/operations.hpp"

#include <urd/memory.hpp>
#include <cista/containers/vector.h>
#include <cassert>
#include <memory>
#include <tuple>

namespace tyr::datalog::database
{
namespace detail
{
// Test targets enable these counters consistently across all translation units.
struct GenericJoinWork
{
    size_t candidate_keys = 0;
    size_t probes = 0;
    size_t prefixes = 0;
};
inline GenericJoinWork& generic_join_work() noexcept
{
    thread_local GenericJoinWork work;
    return work;
}
void count_generic_join_work(size_t GenericJoinWork::* member) noexcept;

struct GenericJoinTrie;
}  // namespace detail
}  // namespace tyr::datalog::database

/// A trie node: what its memory_usage() counts (its table and children).
template<>
struct urd::memory_of<tyr::datalog::database::detail::GenericJoinTrie>
{
    static std::size_t bytes(const tyr::datalog::database::detail::GenericJoinTrie& trie);
};

namespace tyr::datalog::database
{
namespace detail
{

/// Hash-trie nodes own distinct canonical values; input row movement is irrelevant.
struct GenericJoinTrie
{
    urd::Store<urd::map<ygg::Value, std::unique_ptr<GenericJoinTrie>>> children;
    size_t rows = 0;

    [[nodiscard]] const GenericJoinTrie* find(ygg::Value value) const
    {
        const auto found = children.find(value);
        return found == children.end() ? nullptr : found->second.get();
    }

    [[nodiscard]] bool contains(const ygg::RowPool::row_type& row, std::span<const ColumnSlice> keys) const;

    bool insert(const ygg::RowPool::row_type& row, std::span<const ColumnSlice> keys);

    bool erase(const ygg::RowPool::row_type& row, std::span<const ColumnSlice> keys);

    void clear()
    {
        children.clear();
        rows = 0;
    }

    /// Its table, and each key's bytes and child node.
    [[nodiscard]] size_t memory_usage() const noexcept { return urd::memory_usage(children); }
};
}  // namespace detail
}  // namespace tyr::datalog::database

inline std::size_t urd::memory_of<tyr::datalog::database::detail::GenericJoinTrie>::bytes(const tyr::datalog::database::detail::GenericJoinTrie& trie)
{ return trie.memory_usage(); }

namespace tyr::datalog::database
{

namespace incremental
{
class GenericJoinEvaluator;
}

/// Sequential scratch and hash tries for one prepared plan. Owns indexed values;
/// sources need only remain valid during the call. Moving preserves usability.
class GenericJoinWorkspace
{
    GenericJoinPlan m_plan;
    urd::Store<urd::sequence<detail::GenericJoinTrie>> m_tries;
    urd::Store<urd::sequence<const detail::GenericJoinTrie*>> m_cursors;
    ygg::RowPool::row_builder_type m_row;  ///< An output row, built in the output's widths.

    void enumerate(size_t depth, urd::builder<ygg::Relation<ygg::Schema>>& out);

    /// Appends the full assignments over the tries returned by root(input).
    template<std::invocable<size_t> Root>
    void append(urd::builder<ygg::Relation<ygg::Schema>>& out, Root root)
    {
        for (size_t input = 0; input < m_tries.size(); ++input)
        {
            const detail::GenericJoinTrie& trie = root(input);
            if (trie.rows == 0)
                return;
            m_cursors[input] = &trie;
        }
        m_row.reset(out.widths());
        enumerate(0, out);
    }
    void append(urd::builder<ygg::Relation<ygg::Schema>>& out)
    {
        append(out, [&](size_t input) -> const auto& { return m_tries[input]; });
    }
    /// Appends the assignments that read the replacement trie for the replaced input.
    void append(urd::builder<ygg::Relation<ygg::Schema>>& out, size_t replaced, const detail::GenericJoinTrie& replacement)
    {
        append(out, [&](size_t input) -> const auto& { return input == replaced ? replacement : m_tries[input]; });
    }
    detail::GenericJoinTrie& trie(size_t input) { return m_tries.at(input); }

    friend class incremental::GenericJoinEvaluator;

public:
    explicit GenericJoinWorkspace(const GenericJoinPlan& plan) :
        m_plan(plan),
        m_tries(plan.input_count()),
        m_cursors((plan.variable_order().size() + 1) * plan.input_count())
    {
    }

    [[nodiscard]] bool matches(const GenericJoinPlan& plan) const { return m_plan.matches(plan); }

    /// Indexes the inputs, which match the plan, and appends their join to out.
    template<RelationViewRange R>
    void append_join(const R& inputs, urd::builder<ygg::Relation<ygg::Schema>>& out)
    {
        for (size_t input = 0; input < m_plan.input_count(); ++input)
        {
            auto& trie = m_tries[input];
            trie.clear();
            for (size_t row = 0; row < inputs[input].size(); ++row)
                trie.insert(inputs[input].row(row).get_values(), m_plan.input_keys(input));
        }
        append(out);
    }

    [[nodiscard]] size_t memory_usage() const noexcept
    {
        size_t result =
            m_tries.capacity() * sizeof(detail::GenericJoinTrie) + m_cursors.capacity() * sizeof(typename decltype(m_cursors)::value_type);
        for (const auto& trie : m_tries)
            result += trie.memory_usage();
        return result;
    }
};

/// Generic Join (Ngo, Re, Rudra, 2013, Section 4): intersect one variable at a
/// time, iterating the smallest distinct prefix extension and probing the rest.
/// https://arxiv.org/abs/1310.3314. Hash costs are expected; no ordering is required.
/// Output replaces its rows; an output aliasing an input throws before mutation. The inputs, the output and the
/// workspace are the plan's (asserted in debug builds).
template<RelationViewRange R>
void generic_join(const R& inputs, const GenericJoinPlan& plan, urd::builder<ygg::Relation<ygg::Schema>>& out, GenericJoinWorkspace& workspace)
{
    assert(std::ranges::size(inputs) == plan.input_count() && workspace.matches(plan));
    assert(std::ranges::equal(out.columns(), plan.output_columns()));
    for (size_t input = 0; input < plan.input_count(); ++input)
    {
        assert(std::ranges::equal(inputs[input].columns(), plan.input_columns(input)));
        if (inputs[input].relation_identity() == relation_identity(out))
            throw std::invalid_argument("Generic join: output aliases an input.");
    }
    out.clear();
    workspace.append_join(inputs, out);
}
}  // namespace tyr::datalog::database

#endif
