/*
 * Copyright (C) 2026 Dominik Drexler
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef TYR_DATALOG_DATABASE_SEMANTICS_OPERATIONS_HPP_
#define TYR_DATALOG_DATABASE_SEMANTICS_OPERATIONS_HPP_

#include "tyr/datalog/lifted/database/semantics/join_index.hpp"
#include "tyr/datalog/lifted/database/semantics/plans.hpp"
#include "tyr/datalog/lifted/database/semantics/relation_view.hpp"

#include <algorithm>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <functional>
#include <ranges>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <urd/elements/equal_to.hpp>
#include <urd/elements/hash.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <utility>
#include <vector>

namespace tyr::datalog::database
{

/// Scratch storage for sequential relational operations. Keep one workspace per evaluator (or borrow one from a urd
/// slots Store); never share it between concurrent or reentrant calls. All buffers retain their capacity.
struct Workspace
{
    ygg::RowPool::row_builder_type row;  ///< An output row, built in the output's widths.
    urd::Store<urd::map<ygg::hash_t, size_t>, urd::multi, urd::retains> join_index;

    Workspace() = default;
    ~Workspace() = default;
    Workspace(const Workspace&) = delete;  ///< Scratch storage is moved, never copied.
    Workspace& operator=(const Workspace&) = delete;
    Workspace(Workspace&&) = default;
    Workspace& operator=(Workspace&&) = default;

    /// Retained scratch storage.
    [[nodiscard]] size_t memory_usage() const noexcept { return urd::memory_usage(join_index); }
};

/// Marks inputs whose row storage remains immutable for the cache lifetime.
struct JoinReuse
{
    bool lhs = false;
    bool rhs = false;
};

/// The operations take their inputs, a plan built from the inputs' columns, and an output builder whose columns are the
/// plan's output columns (initialize it with them); those using scratch storage take a Workspace. Each replaces the
/// output's rows, retaining capacity, and throws std::invalid_argument where the output is an input. Result row order
/// is unspecified. An insertion failure may leave partial output.

/// Keeps the plan's columns in its order and eliminates duplicate result rows.
template<RelationViewConcept V>
void project(const V& input, const ProjectionPlan& plan, urd::builder<ygg::Relation<ygg::Schema>>& out, Workspace& workspace);

/// Replaces the destination's columns and rows by the source's; the destination's own view leaves it as it is.
template<RelationViewConcept V>
urd::builder<ygg::Relation<ygg::Schema>>& assign(urd::builder<ygg::Relation<ygg::Schema>>& destination, const V& source);

/// The input rows the predicate accepts, into an output of the input's columns. The predicate receives a row view
/// (Row<Schema>) of the input; it must not mutate inputs or output. A throwing predicate can leave a partial output
/// result.
template<RelationViewConcept V, typename Predicate>
    requires std::predicate<Predicate&, decltype(std::declval<const V&>().row(0))>
void select(const V& input, Predicate predicate, urd::builder<ygg::Relation<ygg::Schema>>& out);

/// The rows whose two planned columns hold equal values.
template<RelationViewConcept V>
void select_equal_columns(const V& input, const SelectEqualPlan& plan, urd::builder<ygg::Relation<ygg::Schema>>& out);

/// The rows whose planned column holds the plan's value.
template<RelationViewConcept V>
void select_equal_value(const V& input, const SelectValuePlan& plan, urd::builder<ygg::Relation<ygg::Schema>>& out);

/// Natural join: lhs columns followed by rhs-only columns; disjoint schemas produce the Cartesian product. Hashes the
/// smaller input, retaining only hashes and row indices in the transient index; collisions are resolved by comparing
/// the actual values.
template<RelationViewConcept L, RelationViewConcept R>
void join(const L& lhs, const R& rhs, const JoinPlan& plan, urd::builder<ygg::Relation<ygg::Schema>>& out, Workspace& workspace);

/// As join, reusing the cache's index of the one marked input, or of the smaller input when both are marked; with
/// neither marked, an ordinary transient index. Empty inputs and Cartesian products create no cached index.
template<RelationViewConcept L, RelationViewConcept R>
void join(const L& lhs,
          const R& rhs,
          const JoinPlan& plan,
          JoinIndexCache& cache,
          JoinReuse reuse,
          urd::builder<ygg::Relation<ygg::Schema>>& out,
          Workspace& workspace);

/// The rows of either input. The trailing underscore avoids the C++ keyword union.
template<RelationViewConcept L, RelationViewConcept R>
void union_(const L& lhs, const R& rhs, const SetPlan& plan, urd::builder<ygg::Relation<ygg::Schema>>& out);

/// The lhs rows not in rhs.
template<RelationViewConcept L, RelationViewConcept R>
void difference(const L& lhs, const R& rhs, const SetPlan& plan, urd::builder<ygg::Relation<ygg::Schema>>& out);

/// Semijoin lhs ⋉ rhs: the lhs rows with a match in rhs on the plan's keys. Without shared columns, lhs if rhs is
/// nonempty. Hashes rhs. The output has the lhs columns.
template<RelationViewConcept L, RelationViewConcept R>
void semijoin(const L& lhs, const R& rhs, const JoinPlan& plan, urd::builder<ygg::Relation<ygg::Schema>>& out, Workspace& workspace);

/// Antijoin lhs ▷ rhs: the lhs rows without a match in rhs on the plan's keys. Without shared columns, lhs if rhs is
/// empty. Hashes rhs. The output has the lhs columns.
template<RelationViewConcept L, RelationViewConcept R>
void antijoin(const L& lhs, const R& rhs, const JoinPlan& plan, urd::builder<ygg::Relation<ygg::Schema>>& out, Workspace& workspace);

}  // namespace tyr::datalog::database

namespace tyr::datalog::database
{

namespace detail
{
/// Copies the input's fields into the output's columns from `first` on, verbatim (their columns are alike).
inline void copy_fields(ygg::RowPool::row_builder_type& output, size_t first, const ygg::RowPool::row_type& input, std::span<const ColumnSlice> fields) noexcept
{
    for (const auto field : fields)
        output.copy(first++, input, field.position);
}

/// Clears the output, which must be none of the inputs.
template<RelationViewConcept... Vs>
void prepare_output(urd::builder<ygg::Relation<ygg::Schema>>& out, const Vs&... inputs)
{
    if (((inputs.relation_identity() == out.relation_identity()) || ...))
        throw std::invalid_argument("Relational operation: output aliases an input.");
    out.clear();
}
}  // namespace detail

template<RelationViewConcept V>
void project(const V& input, const ProjectionPlan& plan, urd::builder<ygg::Relation<ygg::Schema>>& out, Workspace& workspace)
{
    assert(std::ranges::equal(input.columns(), plan.input_columns()) && std::ranges::equal(out.columns(), plan.output_columns()));
    detail::prepare_output(out, input);
    const auto positions = plan.positions();
    if (positions.empty())
    {
        if (!input.empty())
            out.insert(std::span<const ygg::Value>());
        return;
    }
    auto& row = workspace.row;
    row.reset(out.widths());
    for (size_t i = 0; i < input.size(); ++i)
    {
        detail::copy_fields(row, 0, input.row(i).get_values(), positions);
        out.insert(row.row());
    }
}

template<RelationViewConcept V>
urd::builder<ygg::Relation<ygg::Schema>>& assign(urd::builder<ygg::Relation<ygg::Schema>>& destination, const V& source)
{
    if (source.relation_identity() == destination.relation_identity())
        return destination;
    destination.initialize(source.columns());
    for (size_t i = 0; i < source.size(); ++i)
        destination.insert(source.row(i).get_values());
    return destination;
}

template<RelationViewConcept V, typename Predicate>
    requires std::predicate<Predicate&, decltype(std::declval<const V&>().row(0))>
void select(const V& input, Predicate predicate, urd::builder<ygg::Relation<ygg::Schema>>& out)
{
    assert(std::ranges::equal(input.columns(), out.columns()));
    detail::prepare_output(out, input);
    for (size_t i = 0; i < input.size(); ++i)
        if (const auto row = input.row(i); std::invoke(predicate, row))
            out.insert(row.get_values());
}

template<RelationViewConcept V>
void select_equal_columns(const V& input, const SelectEqualPlan& plan, urd::builder<ygg::Relation<ygg::Schema>>& out)
{
    assert(std::ranges::equal(input.columns(), plan.output_columns()));
    select(input, [lhs = plan.lhs_position(), rhs = plan.rhs_position()](const auto& row) { return row.value(lhs) == row.value(rhs); }, out);
}

template<RelationViewConcept V>
void select_equal_value(const V& input, const SelectValuePlan& plan, urd::builder<ygg::Relation<ygg::Schema>>& out)
{
    assert(std::ranges::equal(input.columns(), plan.output_columns()));
    select(input, [position = plan.position(), value = plan.value()](const auto& row) { return row.value(position) == value; }, out);
}

namespace detail
{
/// Appends joined rows. Lookup values are row positions in the selected build input for a key hash; actual key
/// equality resolves hash collisions.
template<RelationViewConcept L, RelationViewConcept R, typename Index>
void join_rows(const L& lhs,
               const R& rhs,
               const JoinPlan& plan,
               bool build_left,
               const Index& index,  ///< urd::Store<urd::map<hash_t, position>, urd::multi, urd::retains>
               urd::builder<ygg::Relation<ygg::Schema>>& out,
               Workspace& workspace)
{
    if (lhs.empty() || rhs.empty())
        return;

    auto& row = workspace.row;
    row.reset(out.widths());
    const auto emit = [&](const ygg::RowPool::row_type& left, const ygg::RowPool::row_type& right)
    {
        for (size_t c = 0; c < left.size(); ++c)
            row.copy(c, left, c);
        copy_fields(row, left.size(), right, plan.rhs_payload());
        out.insert(row.row());
    };

    if (plan.lhs_keys().empty())
    {
        for (size_t i = 0; i < lhs.size(); ++i)
            for (size_t j = 0; j < rhs.size(); ++j)
                emit(lhs.row(i).get_values(), rhs.row(j).get_values());
        return;
    }

    const auto probe_rows = [&](const RelationViewConcept auto& build,
                                const RelationViewConcept auto& probe,
                                std::span<const ColumnSlice> build_keys,
                                std::span<const ColumnSlice> probe_keys)
    {
        for (size_t i = 0; i < probe.size(); ++i)
        {
            const auto probe_row = probe.row(i).get_values();
            for (const auto [first, last] = index.equal_range(join_key_hash(probe_row, probe_keys));
                 const auto match : std::ranges::subrange(first, last) | std::views::values)
            {
                const auto build_row = build.row(match).get_values();
                if (!join_keys_equal(build_row, build_keys, probe_row, probe_keys))
                    continue;
                const auto left = build_left ? build_row : probe_row;
                const auto right = build_left ? probe_row : build_row;
                emit(left, right);
            }
        }
    };
    if (build_left)
        probe_rows(lhs, rhs, plan.lhs_keys(), plan.rhs_keys());
    else
        probe_rows(rhs, lhs, plan.rhs_keys(), plan.lhs_keys());
}

template<RelationViewConcept L, RelationViewConcept R>
void assert_planned(const L& lhs, const R& rhs, const JoinPlan& plan) noexcept
{
    assert(std::ranges::equal(lhs.columns(), plan.lhs_columns()) && std::ranges::equal(rhs.columns(), plan.rhs_columns()));
    (void) lhs, (void) rhs, (void) plan;  // read only by the assert
}
}  // namespace detail

template<RelationViewConcept L, RelationViewConcept R>
void join(const L& lhs, const R& rhs, const JoinPlan& plan, urd::builder<ygg::Relation<ygg::Schema>>& out, Workspace& workspace)
{
    detail::assert_planned(lhs, rhs, plan);
    assert(std::ranges::equal(out.columns(), plan.output_columns()));
    detail::prepare_output(out, lhs, rhs);
    const bool build_left = lhs.size() <= rhs.size();
    if (!plan.lhs_keys().empty() && !lhs.empty() && !rhs.empty())
    {
        if (build_left)
            detail::build_join_index(lhs, plan.lhs_keys(), workspace.join_index);
        else
            detail::build_join_index(rhs, plan.rhs_keys(), workspace.join_index);
    }
    detail::join_rows(lhs, rhs, plan, build_left, workspace.join_index, out, workspace);
}

template<RelationViewConcept L, RelationViewConcept R>
void join(const L& lhs,
          const R& rhs,
          const JoinPlan& plan,
          JoinIndexCache& cache,
          JoinReuse reuse,
          urd::builder<ygg::Relation<ygg::Schema>>& out,
          Workspace& workspace)
{
    if (!reuse.lhs && !reuse.rhs)
        return join(lhs, rhs, plan, out, workspace);
    detail::assert_planned(lhs, rhs, plan);
    assert(std::ranges::equal(out.columns(), plan.output_columns()));
    // Build on a reused input, the smaller one if both are reused.
    const bool build_left = reuse.lhs && (!reuse.rhs || lhs.size() <= rhs.size());
    detail::prepare_output(out, lhs, rhs);
    if (lhs.empty() || rhs.empty())
        return;
    const auto* index = &workspace.join_index;
    if (!plan.lhs_keys().empty())
        index = &(build_left ? cache.get_or_create(lhs, plan.lhs_keys()) : cache.get_or_create(rhs, plan.rhs_keys())).index();
    detail::join_rows(lhs, rhs, plan, build_left, *index, out, workspace);
}

template<RelationViewConcept L, RelationViewConcept R>
void union_(const L& lhs, const R& rhs, [[maybe_unused]] const SetPlan& plan, urd::builder<ygg::Relation<ygg::Schema>>& out)
{
    assert(std::ranges::equal(lhs.columns(), plan.output_columns()) && std::ranges::equal(rhs.columns(), plan.output_columns()));
    detail::prepare_output(out, lhs, rhs);
    for (size_t i = 0; i < lhs.size(); ++i)
        out.insert(lhs.row(i).get_values());
    for (size_t i = 0; i < rhs.size(); ++i)
        out.insert(rhs.row(i).get_values());
}

template<RelationViewConcept L, RelationViewConcept R>
void difference(const L& lhs, const R& rhs, [[maybe_unused]] const SetPlan& plan, urd::builder<ygg::Relation<ygg::Schema>>& out)
{
    assert(std::ranges::equal(lhs.columns(), plan.output_columns()) && std::ranges::equal(rhs.columns(), plan.output_columns()));
    detail::prepare_output(out, lhs, rhs);
    for (size_t i = 0; i < lhs.size(); ++i)
        if (const auto row = lhs.row(i); !rhs.contains(row))
            out.insert(row.get_values());
}

namespace detail
{
/// Keeps the lhs rows whose match in rhs on the plan's keys exists (`matching`) or does not.
template<RelationViewConcept L, RelationViewConcept R>
void filter_matches(const L& lhs, const R& rhs, const JoinPlan& plan, bool matching, urd::builder<ygg::Relation<ygg::Schema>>& out, Workspace& workspace)
{
    assert_planned(lhs, rhs, plan);
    assert(std::ranges::equal(out.columns(), plan.lhs_columns()));
    prepare_output(out, lhs, rhs);
    const auto lhs_keys = plan.lhs_keys();
    const auto rhs_keys = plan.rhs_keys();
    if (!lhs_keys.empty() && !rhs.empty())
        build_join_index(rhs, rhs_keys, workspace.join_index);
    const auto matched = [&](const ygg::RowPool::row_type& row)
    {
        if (rhs.empty() || lhs_keys.empty())
            return !rhs.empty();
        for (const auto [first, last] = workspace.join_index.equal_range(join_key_hash(row, lhs_keys));
             const auto match : std::ranges::subrange(first, last) | std::views::values)
            if (join_keys_equal(rhs.row(match).get_values(), rhs_keys, row, lhs_keys))
                return true;
        return false;
    };
    for (size_t i = 0; i < lhs.size(); ++i)
        if (const auto row = lhs.row(i).get_values(); matched(row) == matching)
            out.insert(row);
}
}  // namespace detail

template<RelationViewConcept L, RelationViewConcept R>
void semijoin(const L& lhs, const R& rhs, const JoinPlan& plan, urd::builder<ygg::Relation<ygg::Schema>>& out, Workspace& workspace)
{ detail::filter_matches(lhs, rhs, plan, true, out, workspace); }

template<RelationViewConcept L, RelationViewConcept R>
void antijoin(const L& lhs, const R& rhs, const JoinPlan& plan, urd::builder<ygg::Relation<ygg::Schema>>& out, Workspace& workspace)
{ detail::filter_matches(lhs, rhs, plan, false, out, workspace); }

}  // namespace tyr::datalog::database

#endif
