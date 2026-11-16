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

/// Natural join: left columns followed by right-only columns; disjoint schemas produce the Cartesian product. Hashes the
/// smaller input, retaining only hashes and row indices in the transient index; collisions are resolved by comparing
/// the actual values.
template<RelationViewConcept L, RelationViewConcept R>
void join(const L& left_operand, const R& right_operand, const JoinPlan& plan, urd::builder<ygg::Relation<ygg::Schema>>& out, Workspace& workspace);

/// The rows of either input. The trailing underscore avoids the C++ keyword union.
template<RelationViewConcept L, RelationViewConcept R>
void union_(const L& left_operand, const R& right_operand, const SetPlan& plan, urd::builder<ygg::Relation<ygg::Schema>>& out);

/// The left rows not in right.
template<RelationViewConcept L, RelationViewConcept R>
void difference(const L& left_operand, const R& right_operand, const SetPlan& plan, urd::builder<ygg::Relation<ygg::Schema>>& out);

/// Semijoin left ⋉ right: the left rows with a match in right on the plan's keys. Without shared columns, left if right is
/// nonempty. Hashes right. The output has the left columns.
template<RelationViewConcept L, RelationViewConcept R>
void semijoin(const L& left_operand, const R& right_operand, const JoinPlan& plan, urd::builder<ygg::Relation<ygg::Schema>>& out, Workspace& workspace);

/// Antijoin left ▷ right: the left rows without a match in right on the plan's keys. Without shared columns, left if right is
/// empty. Hashes right. The output has the left columns.
template<RelationViewConcept L, RelationViewConcept R>
void antijoin(const L& left_operand, const R& right_operand, const JoinPlan& plan, urd::builder<ygg::Relation<ygg::Schema>>& out, Workspace& workspace);

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
    if ((reads_builder(inputs, out) || ...))
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
    if (reads_builder(source, destination))
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
    select(input, [left = plan.left_position(), right = plan.right_position()](const auto& row) { return row.value(left) == row.value(right); }, out);
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
void join_rows(const L& left_operand,
               const R& right_operand,
               const JoinPlan& plan,
               bool build_left,
               const Index& index,  ///< urd::Store<urd::map<hash_t, position>, urd::multi, urd::retains>
               urd::builder<ygg::Relation<ygg::Schema>>& out,
               Workspace& workspace)
{
    if (left_operand.empty() || right_operand.empty())
        return;

    auto& row = workspace.row;
    row.reset(out.widths());
    const auto emit = [&](const ygg::RowPool::row_type& left_row, const ygg::RowPool::row_type& right_row)
    {
        for (size_t c = 0; c < left_row.size(); ++c)
            row.copy(c, left_row, c);
        copy_fields(row, left_row.size(), right_row, plan.right_payload());
        out.insert(row.row());
    };

    if (plan.left_keys().empty())
    {
        for (size_t i = 0; i < left_operand.size(); ++i)
            for (size_t j = 0; j < right_operand.size(); ++j)
                emit(left_operand.row(i).get_values(), right_operand.row(j).get_values());
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
                const auto left_row = build_left ? build_row : probe_row;
                const auto right_row = build_left ? probe_row : build_row;
                emit(left_row, right_row);
            }
        }
    };
    if (build_left)
        probe_rows(left_operand, right_operand, plan.left_keys(), plan.right_keys());
    else
        probe_rows(right_operand, left_operand, plan.right_keys(), plan.left_keys());
}

template<RelationViewConcept L, RelationViewConcept R>
void assert_planned(const L& left_operand, const R& right_operand, const JoinPlan& plan) noexcept
{
    assert(std::ranges::equal(left_operand.columns(), plan.left_columns()) && std::ranges::equal(right_operand.columns(), plan.right_columns()));
    (void) left_operand, (void) right_operand, (void) plan;  // read only by the assert
}
}  // namespace detail

template<RelationViewConcept L, RelationViewConcept R>
void join(const L& left_operand, const R& right_operand, const JoinPlan& plan, urd::builder<ygg::Relation<ygg::Schema>>& out, Workspace& workspace)
{
    detail::assert_planned(left_operand, right_operand, plan);
    assert(std::ranges::equal(out.columns(), plan.output_columns()));
    detail::prepare_output(out, left_operand, right_operand);
    const bool build_left = left_operand.size() <= right_operand.size();
    if (!plan.left_keys().empty() && !left_operand.empty() && !right_operand.empty())
    {
        if (build_left)
            detail::build_join_index(left_operand, plan.left_keys(), workspace.join_index);
        else
            detail::build_join_index(right_operand, plan.right_keys(), workspace.join_index);
    }
    detail::join_rows(left_operand, right_operand, plan, build_left, workspace.join_index, out, workspace);
}

template<RelationViewConcept L, RelationViewConcept R>
void union_(const L& left_operand, const R& right_operand, [[maybe_unused]] const SetPlan& plan, urd::builder<ygg::Relation<ygg::Schema>>& out)
{
    assert(std::ranges::equal(left_operand.columns(), plan.output_columns()) && std::ranges::equal(right_operand.columns(), plan.output_columns()));
    detail::prepare_output(out, left_operand, right_operand);
    for (size_t i = 0; i < left_operand.size(); ++i)
        out.insert(left_operand.row(i).get_values());
    for (size_t i = 0; i < right_operand.size(); ++i)
        out.insert(right_operand.row(i).get_values());
}

template<RelationViewConcept L, RelationViewConcept R>
void difference(const L& left_operand, const R& right_operand, [[maybe_unused]] const SetPlan& plan, urd::builder<ygg::Relation<ygg::Schema>>& out)
{
    assert(std::ranges::equal(left_operand.columns(), plan.output_columns()) && std::ranges::equal(right_operand.columns(), plan.output_columns()));
    detail::prepare_output(out, left_operand, right_operand);
    for (size_t i = 0; i < left_operand.size(); ++i)
        if (const auto row = left_operand.row(i); !right_operand.contains(row))
            out.insert(row.get_values());
}

namespace detail
{
/// Keeps the left rows whose match in right on the plan's keys exists (`matching`) or does not.
template<RelationViewConcept L, RelationViewConcept R>
void filter_matches(const L& left_operand,
                    const R& right_operand,
                    const JoinPlan& plan,
                    bool matching,
                    urd::builder<ygg::Relation<ygg::Schema>>& out,
                    Workspace& workspace)
{
    assert_planned(left_operand, right_operand, plan);
    assert(std::ranges::equal(out.columns(), plan.left_columns()));
    prepare_output(out, left_operand, right_operand);
    const auto left_keys = plan.left_keys();
    const auto right_keys = plan.right_keys();
    if (!left_keys.empty() && !right_operand.empty())
        build_join_index(right_operand, right_keys, workspace.join_index);
    const auto matched = [&](const ygg::RowPool::row_type& row)
    {
        if (right_operand.empty() || left_keys.empty())
            return !right_operand.empty();
        for (const auto [first, last] = workspace.join_index.equal_range(join_key_hash(row, left_keys));
             const auto match : std::ranges::subrange(first, last) | std::views::values)
            if (join_keys_equal(right_operand.row(match).get_values(), right_keys, row, left_keys))
                return true;
        return false;
    };
    for (size_t i = 0; i < left_operand.size(); ++i)
        if (const auto row = left_operand.row(i).get_values(); matched(row) == matching)
            out.insert(row);
}
}  // namespace detail

template<RelationViewConcept L, RelationViewConcept R>
void semijoin(const L& left_operand, const R& right_operand, const JoinPlan& plan, urd::builder<ygg::Relation<ygg::Schema>>& out, Workspace& workspace)
{ detail::filter_matches(left_operand, right_operand, plan, true, out, workspace); }

template<RelationViewConcept L, RelationViewConcept R>
void antijoin(const L& left_operand, const R& right_operand, const JoinPlan& plan, urd::builder<ygg::Relation<ygg::Schema>>& out, Workspace& workspace)
{ detail::filter_matches(left_operand, right_operand, plan, false, out, workspace); }

}  // namespace tyr::datalog::database

#endif
