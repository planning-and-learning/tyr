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
    std::vector<ygg::Value> row;  ///< An output row's values, built field by field.
    urd::Store<urd::map<ygg::hash_t, size_t>, urd::multi, urd::retains> join_index;

    Workspace() = default;
    ~Workspace() = default;
    Workspace(const Workspace&) = delete;  ///< Scratch storage is moved, never copied.
    Workspace& operator=(const Workspace&) = delete;
    Workspace(Workspace&&) = default;
    Workspace& operator=(Workspace&&) = default;

    /// Retained scratch storage.
    [[nodiscard]] size_t memory_usage() const { return urd::memory_usage(join_index); }
};

/// The operations take their inputs (relation views: a row is read as it is held, ygg::with_values, urd::visit_view), a
/// plan built from the inputs' columns, and an output scratch relation whose columns are the plan's output columns (draw
/// it with them); those building rows field by field take a Workspace. Each replaces the output's rows, which it keeps in
/// tuple order, and throws std::invalid_argument where the output is an input. An insertion failure may leave partial
/// output.

/// Keeps the plan's columns in its order and eliminates duplicate result rows.
template<ygg::RelationViewConcept<ygg::Schema> V>
void project(const V& input, const ProjectionPlan& plan, ygg::FreeScratchRelation& out, Workspace& workspace);

/// Replaces the destination's columns and rows by the source's; the destination's own view leaves it as it is.
template<ygg::RelationViewConcept<ygg::Schema> V>
ygg::FreeScratchRelation& assign(ygg::FreeScratchRelation& destination, const V& source);

/// The input rows the predicate accepts, into an output of the input's columns. The predicate receives a row as the
/// input holds it, a row view (Row<Schema>) of each form it may take; it must not mutate inputs or output. A throwing
/// predicate can leave a partial output result.
template<ygg::RelationViewConcept<ygg::Schema> V, typename Predicate>
void select(const V& input, Predicate predicate, ygg::FreeScratchRelation& out);

/// The rows whose two planned columns hold equal values.
template<ygg::RelationViewConcept<ygg::Schema> V>
void select_equal_columns(const V& input, const SelectEqualPlan& plan, ygg::FreeScratchRelation& out);

/// The rows whose planned column holds the plan's value.
template<ygg::RelationViewConcept<ygg::Schema> V>
void select_equal_value(const V& input, const SelectValuePlan& plan, ygg::FreeScratchRelation& out);

/// Natural join: left columns followed by right-only columns; disjoint schemas produce the Cartesian product. Hashes the
/// smaller input, retaining only hashes and row indices in the transient index; collisions are resolved by comparing
/// the actual values.
template<ygg::RelationViewConcept<ygg::Schema> L, ygg::RelationViewConcept<ygg::Schema> R>
void join(const L& left_operand, const R& right_operand, const JoinPlan& plan, ygg::FreeScratchRelation& out, Workspace& workspace);

/// The rows of either input. The trailing underscore avoids the C++ keyword union.
template<ygg::RelationViewConcept<ygg::Schema> L, ygg::RelationViewConcept<ygg::Schema> R>
void union_(const L& left_operand, const R& right_operand, const SetPlan& plan, ygg::FreeScratchRelation& out);

/// The left rows not in right.
template<ygg::RelationViewConcept<ygg::Schema> L, ygg::RelationViewConcept<ygg::Schema> R>
void difference(const L& left_operand, const R& right_operand, const SetPlan& plan, ygg::FreeScratchRelation& out);

/// Semijoin left ⋉ right: the left rows with a match in right on the plan's keys. Without shared columns, left if right is
/// nonempty. Hashes right. The output has the left columns.
template<ygg::RelationViewConcept<ygg::Schema> L, ygg::RelationViewConcept<ygg::Schema> R>
void semijoin(const L& left_operand, const R& right_operand, const JoinPlan& plan, ygg::FreeScratchRelation& out, Workspace& workspace);

/// Antijoin left ▷ right: the left rows without a match in right on the plan's keys. Without shared columns, left if right is
/// empty. Hashes right. The output has the left columns.
template<ygg::RelationViewConcept<ygg::Schema> L, ygg::RelationViewConcept<ygg::Schema> R>
void antijoin(const L& left_operand, const R& right_operand, const JoinPlan& plan, ygg::FreeScratchRelation& out, Workspace& workspace);

}  // namespace tyr::datalog::database

namespace tyr::datalog::database
{

namespace detail
{
/// Appends the input's fields to the output row's values.
template<ygg::RowValues R>
void copy_fields(std::vector<ygg::Value>& output, const R& input, std::span<const ColumnSlice> fields)
{
    for (const auto field : fields)
        output.push_back(field_value(input, field.position));
}

/// Inserts a row of values into the output, in its place in tuple order.
template<ygg::RowValues R>
void insert(ygg::FreeScratchRelation& out, const R& values)
{ out.insert(values); }

/// Clears the output, which must be none of the inputs.
template<ygg::RelationViewConcept<ygg::Schema>... Vs>
void prepare_output(ygg::FreeScratchRelation& out, const Vs&... inputs)
{
    if ((reads(inputs, out) || ...))
        throw std::invalid_argument("Relational operation: output aliases an input.");
    out.clear();
}
}  // namespace detail

template<ygg::RelationViewConcept<ygg::Schema> V>
void project(const V& input, const ProjectionPlan& plan, ygg::FreeScratchRelation& out, Workspace& workspace)
{
    assert(std::ranges::equal(input.columns(), plan.input_columns()) && std::ranges::equal(out.view().columns(), plan.output_columns()));
    detail::prepare_output(out, input);
    const auto positions = plan.positions();
    auto& row = workspace.row;
    for (const auto& held : input.get_rows())
        ygg::with_values(held,
                         [&](const auto& values)
                         {
                             row.clear();
                             detail::copy_fields(row, values, positions);
                             detail::insert(out, row);
                         });
}

template<ygg::RelationViewConcept<ygg::Schema> V>
ygg::FreeScratchRelation& assign(ygg::FreeScratchRelation& destination, const V& source)
{
    if (reads(source, destination))
        return destination;
    destination.relation.initialize(destination.rows, destination.rows.schema(source.columns()), source.columns());
    for (const auto& held : source.get_rows())
        ygg::with_values(held, [&](const auto& values) { detail::insert(destination, values); });
    return destination;
}

template<ygg::RelationViewConcept<ygg::Schema> V, typename Predicate>
void select(const V& input, Predicate predicate, ygg::FreeScratchRelation& out)
{
    assert(std::ranges::equal(input.columns(), out.view().columns()));
    detail::prepare_output(out, input);
    for (const auto& held : input.get_rows())
        urd::visit_view(held,
                        [&](const auto& row)
                        {
                            if (std::invoke(predicate, row))
                                detail::insert(out, row.get_values());
                        });
}

template<ygg::RelationViewConcept<ygg::Schema> V>
void select_equal_columns(const V& input, const SelectEqualPlan& plan, ygg::FreeScratchRelation& out)
{
    assert(std::ranges::equal(input.columns(), plan.output_columns()));
    select(input, [left = plan.left_position(), right = plan.right_position()](const auto& row) { return row.value(left) == row.value(right); }, out);
}

template<ygg::RelationViewConcept<ygg::Schema> V>
void select_equal_value(const V& input, const SelectValuePlan& plan, ygg::FreeScratchRelation& out)
{
    assert(std::ranges::equal(input.columns(), plan.output_columns()));
    select(input, [position = plan.position(), value = plan.value()](const auto& row) { return row.value(position) == value; }, out);
}

namespace detail
{
/// The row of a batch join index's entry: its position in the build input (an index built for one operation).
inline constexpr auto row_at_position = [](const auto& build, size_t position) { return build.row(position); };

/// Appends joined rows. The index names the build input's rows by the hash of their keys, each entry read by
/// row_at(build, entry): a position (row_at_position), or what an index kept across changes names a row by; actual key
/// equality resolves hash collisions.
template<ygg::RelationViewConcept<ygg::Schema> L, ygg::RelationViewConcept<ygg::Schema> R, typename Index, typename RowAt>
void join_rows(const L& left_operand,
               const R& right_operand,
               const JoinPlan& plan,
               bool build_left,
               const Index& index,  ///< urd::Store<urd::map<hash_t, entry>, urd::multi, urd::retains>
               const RowAt& row_at,
               ygg::FreeScratchRelation& out,
               Workspace& workspace)
{
    if (left_operand.get_rows().empty() || right_operand.get_rows().empty())
        return;

    auto& row = workspace.row;
    const auto emit = [&](const auto& left_row, const auto& right_row)
    {
        row.assign(std::ranges::begin(left_row), std::ranges::end(left_row));
        copy_fields(row, right_row, plan.right_payload());
        insert(out, row);
    };

    if (plan.left_keys().empty())
    {
        for (const auto& left_held : left_operand.get_rows())
            ygg::with_values(left_held,
                             [&](const auto& left_row)
                             {
                                 for (const auto& right_held : right_operand.get_rows())
                                     ygg::with_values(right_held, [&](const auto& right_row) { emit(left_row, right_row); });
                             });
        return;
    }

    const auto probe_rows = [&](const ygg::RelationViewConcept<ygg::Schema> auto& build,
                                const ygg::RelationViewConcept<ygg::Schema> auto& probe,
                                std::span<const ColumnSlice> build_keys,
                                std::span<const ColumnSlice> probe_keys)
    {
        for (const auto& probe_held : probe.get_rows())
            ygg::with_values(probe_held,
                             [&](const auto& probe_row)
                             {
                                 for (const auto [first, last] = index.equal_range(join_key_hash(probe_row, probe_keys));
                                      const auto match : std::ranges::subrange(first, last) | std::views::values)
                                     ygg::with_values(row_at(build, match),
                                                      [&](const auto& build_row)
                                                      {
                                                          if (!join_keys_equal(build_row, build_keys, probe_row, probe_keys))
                                                              return;
                                                          if (build_left)
                                                              emit(build_row, probe_row);
                                                          else
                                                              emit(probe_row, build_row);
                                                      });
                             });
    };
    if (build_left)
        probe_rows(left_operand, right_operand, plan.left_keys(), plan.right_keys());
    else
        probe_rows(right_operand, left_operand, plan.right_keys(), plan.left_keys());
}

template<ygg::RelationViewConcept<ygg::Schema> L, ygg::RelationViewConcept<ygg::Schema> R>
// Reads the operands' columns in place (a builder's, built or interned in its layer): nothing is allocated or checked by
// throwing.
// NOLINTNEXTLINE(bugprone-exception-escape)
void assert_planned(const L& left_operand, const R& right_operand, const JoinPlan& plan) noexcept
{
    assert(std::ranges::equal(left_operand.columns(), plan.left_columns()) && std::ranges::equal(right_operand.columns(), plan.right_columns()));
    (void) left_operand, (void) right_operand, (void) plan;  // read only by the assert
}
}  // namespace detail

template<ygg::RelationViewConcept<ygg::Schema> L, ygg::RelationViewConcept<ygg::Schema> R>
void join(const L& left_operand, const R& right_operand, const JoinPlan& plan, ygg::FreeScratchRelation& out, Workspace& workspace)
{
    detail::assert_planned(left_operand, right_operand, plan);
    assert(std::ranges::equal(out.view().columns(), plan.output_columns()));
    detail::prepare_output(out, left_operand, right_operand);
    const bool build_left = left_operand.get_rows().size() <= right_operand.get_rows().size();
    if (!plan.left_keys().empty() && !left_operand.get_rows().empty() && !right_operand.get_rows().empty())
    {
        if (build_left)
            detail::build_join_index(left_operand, plan.left_keys(), workspace.join_index);
        else
            detail::build_join_index(right_operand, plan.right_keys(), workspace.join_index);
    }
    detail::join_rows(left_operand, right_operand, plan, build_left, workspace.join_index, detail::row_at_position, out, workspace);
}

template<ygg::RelationViewConcept<ygg::Schema> L, ygg::RelationViewConcept<ygg::Schema> R>
void union_(const L& left_operand, const R& right_operand, [[maybe_unused]] const SetPlan& plan, ygg::FreeScratchRelation& out)
{
    assert(std::ranges::equal(left_operand.columns(), plan.output_columns()) && std::ranges::equal(right_operand.columns(), plan.output_columns()));
    detail::prepare_output(out, left_operand, right_operand);
    for (const auto& held : left_operand.get_rows())
        ygg::with_values(held, [&](const auto& values) { detail::insert(out, values); });
    for (const auto& held : right_operand.get_rows())
        ygg::with_values(held, [&](const auto& values) { detail::insert(out, values); });
}

template<ygg::RelationViewConcept<ygg::Schema> L, ygg::RelationViewConcept<ygg::Schema> R>
void difference(const L& left_operand, const R& right_operand, [[maybe_unused]] const SetPlan& plan, ygg::FreeScratchRelation& out)
{
    assert(std::ranges::equal(left_operand.columns(), plan.output_columns()) && std::ranges::equal(right_operand.columns(), plan.output_columns()));
    detail::prepare_output(out, left_operand, right_operand);
    for (const auto& held : left_operand.get_rows())
        ygg::with_values(held,
                         [&](const auto& values)
                         {
                             if (!right_operand.contains(values))
                                 detail::insert(out, values);
                         });
}

namespace detail
{
/// Keeps the left rows whose match in right on the plan's keys exists (`matching`) or does not.
template<ygg::RelationViewConcept<ygg::Schema> L, ygg::RelationViewConcept<ygg::Schema> R>
void filter_matches(const L& left_operand, const R& right_operand, const JoinPlan& plan, bool matching, ygg::FreeScratchRelation& out, Workspace& workspace)
{
    assert_planned(left_operand, right_operand, plan);
    assert(std::ranges::equal(out.view().columns(), plan.left_columns()));
    prepare_output(out, left_operand, right_operand);
    const auto left_keys = plan.left_keys();
    const auto right_keys = plan.right_keys();
    if (!left_keys.empty() && !right_operand.get_rows().empty())
        build_join_index(right_operand, right_keys, workspace.join_index);
    const auto matched = [&](const auto& row)
    {
        if (right_operand.get_rows().empty() || left_keys.empty())
            return !right_operand.get_rows().empty();
        for (const auto [first, last] = workspace.join_index.equal_range(join_key_hash(row, left_keys));
             const auto match : std::ranges::subrange(first, last) | std::views::values)
            if (ygg::with_values(right_operand.row(match), [&](const auto& right_row) { return join_keys_equal(right_row, right_keys, row, left_keys); }))
                return true;
        return false;
    };
    for (const auto& held : left_operand.get_rows())
        ygg::with_values(held,
                         [&](const auto& row)
                         {
                             if (matched(row) == matching)
                                 insert(out, row);
                         });
}
}  // namespace detail

template<ygg::RelationViewConcept<ygg::Schema> L, ygg::RelationViewConcept<ygg::Schema> R>
void semijoin(const L& left_operand, const R& right_operand, const JoinPlan& plan, ygg::FreeScratchRelation& out, Workspace& workspace)
{ detail::filter_matches(left_operand, right_operand, plan, true, out, workspace); }

template<ygg::RelationViewConcept<ygg::Schema> L, ygg::RelationViewConcept<ygg::Schema> R>
void antijoin(const L& left_operand, const R& right_operand, const JoinPlan& plan, ygg::FreeScratchRelation& out, Workspace& workspace)
{ detail::filter_matches(left_operand, right_operand, plan, false, out, workspace); }

}  // namespace tyr::datalog::database

#endif
