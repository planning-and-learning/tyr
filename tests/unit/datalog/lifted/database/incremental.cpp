/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "query_helpers.hpp"
#include "tyr/datalog/lifted/database/semantics/incremental/join.hpp"
#include "tyr/datalog/lifted/database/semantics/incremental/projection.hpp"
#include "tyr/datalog/lifted/database/semantics/relation_view.hpp"

#include <algorithm>
#include <array>
#include <compare>
#include <gtest/gtest.h>
#include <set>
#include <span>
#include <stdexcept>
#include <tuple>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <vector>

namespace tyr::datalog::tests
{
using Values = boost::mp11::mp_list<ygg::uint_t>;
using ColumnIndex = urd::index<ygg::Column>;

namespace
{

/// The layer the tests read their builders through.
const ygg::FreeRelationRepository& layer()
{
    static const ygg::FreeRelationRepository repository;
    return repository;
}

ygg::FreeRelationBuilderView borrow(const urd::builder<ygg::Relation<ygg::Schema>>& relation) { return layer().view(relation); }

using tyr::datalog::tests::qb::columns_of;
template<typename T = ygg::uint_t, typename... Args>
auto cells(Args... values)
{ return std::array<ygg::Value, sizeof...(Args)> { ygg::encode_value(T(values))... }; }

template<typename T, size_t N>
auto packed(const std::array<T, N>& values)
{
    std::array<ygg::Value, N> result {};
    for (size_t i = 0; i < N; ++i)
        result[i] = ygg::encode_value(values[i]);
    return result;
}

template<typename T, database::RelationViewConcept V>
std::set<urd::Store<urd::sequence<T>>> rows(const V& relation)
{
    std::set<urd::Store<urd::sequence<T>>> result;
    for (size_t i = 0; i < relation.size(); ++i)
    {
        const auto row = relation.row(i);
        urd::Store<urd::sequence<T>> values;
        values.reserve(row.size());
        for (size_t column = 0; column < row.size(); ++column)
            values.push_back(row.get_values().template get<T>(column));
        result.insert(std::move(values));
    }
    return result;
}

template<typename T, typename Evaluator>
void expect_update(const Evaluator& evaluator, const std::set<urd::Store<urd::sequence<T>>>& before, const urd::builder<ygg::Relation<ygg::Schema>>& reference)
{
    const auto after = rows<T>(borrow(reference));
    EXPECT_TRUE(std::ranges::equal(evaluator.get_result().columns(), reference.columns()));
    EXPECT_TRUE(std::ranges::equal(evaluator.get_delta().added.columns(), reference.columns()));
    EXPECT_TRUE(std::ranges::equal(evaluator.get_delta().removed.columns(), reference.columns()));
    EXPECT_EQ(rows<T>(evaluator.get_result()), after);
    std::set<urd::Store<urd::sequence<T>>> added;
    std::set<urd::Store<urd::sequence<T>>> removed;
    for (const auto& row : after)
        if (!before.contains(row))
            added.insert(row);
    for (const auto& row : before)
        if (!after.contains(row))
            removed.insert(row);
    EXPECT_EQ(rows<T>(evaluator.get_delta().change().first), added);
    EXPECT_EQ(rows<T>(evaluator.get_delta().change().second), removed);
}

void apply_batch(urd::builder<ygg::Relation<ygg::Schema>>& input,
                 const urd::builder<ygg::Relation<ygg::Schema>>& added,
                 const urd::builder<ygg::Relation<ygg::Schema>>& removed)
{
    for (const auto row : borrow(removed).get_rows())
        input.erase(input.find(row.get_values()).value());
    for (const auto row : borrow(added).get_rows())
        input.insert(row.get_values());
}
}  // namespace

TEST(YggdrasilTests, DatabaseRelationErasureMovesTheLastRow)
{
    urd::builder<ygg::Relation<ygg::Schema>> relation { tyr::datalog::tests::ops::cols({ ColumnIndex(7), ColumnIndex(3) }) };
    const auto first = relation.insert(cells(1, 2));
    relation.insert(cells(3, 4));
    relation.insert(cells(5, 6));
    const std::array<ygg::uint_t, 2> sought { 1, 2 };
    EXPECT_EQ(relation.find(packed(sought)), first);
    EXPECT_EQ(relation.find(packed(sought)), first);
    EXPECT_EQ(relation.find(cells(1, 2)), first);
    EXPECT_FALSE(relation.find(cells(9, 9)));
    EXPECT_FALSE(relation.find(cells(1)));  // not one value per column
    EXPECT_THROW(relation.erase(relation.size()), std::out_of_range);
    relation.erase(first);
    EXPECT_FALSE(relation.contains(cells(1, 2)));
    EXPECT_TRUE(relation.contains(cells(3, 4)));
    EXPECT_TRUE(relation.contains(cells(5, 6)));
    EXPECT_EQ(relation.find(cells(5, 6)), first);
}

TEST(YggdrasilTests, DatabaseIncrementalProjectionTracksSupportAcrossCompactionAndUndo)
{
    const auto large = ColumnIndex();
    urd::builder<ygg::Relation<ygg::Schema>> input { tyr::datalog::tests::ops::cols({ large, ColumnIndex(4), ColumnIndex(11) }) };
    input.insert(cells(1, 10, 7));
    input.insert(cells(1, 11, 7));
    input.insert(cells(2, 10, 8));
    input.insert(cells(3, 10, 9));
    const database::ProjectionPlan plan(input.columns(), std::array { ColumnIndex(11), large });
    database::incremental::ProjectionEvaluator evaluator(layer(), plan);
    database::Workspace workspace;
    urd::builder<ygg::Relation<ygg::Schema>> reference(plan.output_columns());
    evaluator.initialize(borrow(input), workspace);
    database::project(borrow(input), plan, reference, workspace);
    EXPECT_EQ(rows<ygg::uint_t>(evaluator.get_result()), rows<ygg::uint_t>(borrow(reference)));
    EXPECT_TRUE(evaluator.get_delta().added.empty());
    EXPECT_TRUE(evaluator.get_delta().removed.empty());

    database::incremental::Delta replacement(layer(), input.columns());
    replacement.removed.insert(cells(1, 10, 7));
    replacement.removed.insert(cells(1, 11, 7));
    replacement.added.insert(cells(1, 12, 7));
    database::incremental::Delta erase_first(layer(), input.columns());
    erase_first.removed.insert(cells(1, 12, 7));
    erase_first.added.insert(cells(4, 1, 10));
    database::incremental::Delta erase_moved(layer(), input.columns());
    erase_moved.removed.insert(cells(3, 10, 9));
    erase_moved.added.insert(cells(2, 11, 8));
    const auto update = [&](const auto& added, const auto& removed)
    {
        const auto before = rows<ygg::uint_t>(borrow(reference));
        apply_batch(input, added, removed);
        evaluator.update(std::pair { borrow(added), borrow(removed) }, workspace);
        database::project(borrow(input), plan, reference, workspace);
        expect_update(evaluator, before, reference);
    };
    // The position of the result's row of these values.
    const auto position = [&](const auto& values)
    {
        const auto rows = evaluator.get_result().get_rows();
        const auto found = std::ranges::find_if(rows, [&](const auto& row) { return std::ranges::equal(row.get_values(), values); });
        EXPECT_NE(found, rows.end());
        return static_cast<size_t>(std::ranges::distance(rows.begin(), found));
    };
    const auto replaced_index = position(cells(7, 1));
    const auto untouched_index = position(cells(9, 3));
    const auto replaced_row = evaluator.get_result().row(replaced_index);
    const auto untouched_row = evaluator.get_result().row(untouched_index);
    update(replacement.added, replacement.removed);
    // Consolidating witnesses must not erase/reinsert an unchanged output or
    // move the last output into its slot as a temporary consequence.
    EXPECT_EQ(position(cells(7, 1)), replaced_index);
    EXPECT_EQ(position(cells(9, 3)), untouched_index);
    const auto same_storage = [](const auto& lhs, const auto& rhs)
    { return lhs.get_values().data() == rhs.get_values().data() && lhs.get_values().first_bit() == rhs.get_values().first_bit(); };
    EXPECT_TRUE(same_storage(evaluator.get_result().row(replaced_index), replaced_row));
    EXPECT_TRUE(same_storage(evaluator.get_result().row(untouched_index), untouched_row));
    EXPECT_EQ(ygg::decode_value<ygg::uint_t>(replaced_row.value(0)), 7);
    EXPECT_EQ(ygg::decode_value<ygg::uint_t>(replaced_row.value(1)), 1);
    EXPECT_EQ(ygg::decode_value<ygg::uint_t>(untouched_row.value(0)), 9);
    EXPECT_EQ(ygg::decode_value<ygg::uint_t>(untouched_row.value(1)), 3);
    EXPECT_TRUE(evaluator.get_delta().added.empty());
    EXPECT_TRUE(evaluator.get_delta().removed.empty());
    update(erase_first.added, erase_first.removed);
    update(erase_moved.added, erase_moved.removed);
    update(erase_moved.removed, erase_moved.added);
    update(erase_first.removed, erase_first.added);
    update(replacement.removed, replacement.added);
    const auto retained = evaluator.memory_usage();
    evaluator.initialize(borrow(input), workspace);
    EXPECT_EQ(evaluator.memory_usage(), retained);
    EXPECT_TRUE(evaluator.get_delta().added.empty());
    EXPECT_TRUE(evaluator.get_delta().removed.empty());
}

TEST(YggdrasilTests, DatabaseIncrementalProjectionConsolidatesMixedChangesAndUndo)
{
    urd::builder<ygg::Relation<ygg::Schema>> input { tyr::datalog::tests::ops::cols({ ColumnIndex(1), ColumnIndex(2) }) };
    input.insert(cells(1, 10));
    input.insert(cells(1, 11));
    input.insert(cells(1, 12));
    input.insert(cells(2, 20));
    input.insert(cells(2, 21));
    input.insert(cells(3, 30));
    input.insert(cells(4, 40));
    input.insert(cells(5, 50));
    const auto initial = rows<ygg::uint_t>(borrow(input));
    const database::ProjectionPlan plan(input.columns(), std::array { ColumnIndex(1) });
    database::incremental::ProjectionEvaluator evaluator(layer(), plan);
    database::Workspace workspace;
    urd::builder<ygg::Relation<ygg::Schema>> reference(plan.output_columns());
    evaluator.initialize(borrow(input), workspace);
    database::project(borrow(input), plan, reference, workspace);
    database::incremental::Delta delta(layer(), input.columns());
    delta.removed.insert(cells(1, 10));
    delta.removed.insert(cells(1, 11));
    delta.added.insert(cells(1, 13));  // Fewer witnesses, same output.
    delta.removed.insert(cells(2, 20));
    delta.removed.insert(cells(2, 21));
    delta.added.insert(cells(2, 22));  // Replace every witness, same output.
    delta.added.insert(cells(2, 23));
    delta.removed.insert(cells(3, 30));  // Last witness removes the output.
    delta.added.insert(cells(5, 51));    // More witnesses, same output.
    delta.added.insert(cells(6, 60));    // New output has multiple witnesses.
    delta.added.insert(cells(6, 61));
    const auto update = [&](const auto& added, const auto& removed)
    {
        const auto before = rows<ygg::uint_t>(borrow(reference));
        apply_batch(input, added, removed);
        evaluator.update(std::pair { borrow(added), borrow(removed) }, workspace);
        database::project(borrow(input), plan, reference, workspace);
        expect_update(evaluator, before, reference);
    };
    update(delta.added, delta.removed);
    EXPECT_EQ(evaluator.get_delta().added.size(), 1);
    EXPECT_TRUE(evaluator.get_delta().added.contains(cells(6)));
    EXPECT_EQ(evaluator.get_delta().removed.size(), 1);
    EXPECT_TRUE(evaluator.get_delta().removed.contains(cells(3)));
    update(delta.removed, delta.added);
    EXPECT_EQ(rows<ygg::uint_t>(borrow(input)), initial);
    // Removing the restored input checks every consolidated support count,
    // including unchanged membership and rows moved by compact erasure.
    urd::builder<ygg::Relation<ygg::Schema>> empty(input.columns());
    urd::builder<ygg::Relation<ygg::Schema>> all(input.columns());
    for (size_t i = 0; i < input.size(); ++i)
        all.insert(borrow(input).row(i).get_values());
    update(empty, all);
    EXPECT_TRUE(evaluator.get_result().empty());
    update(all, empty);
    EXPECT_EQ(rows<ygg::uint_t>(borrow(input)), initial);
}

TEST(YggdrasilTests, DatabaseIncrementalProjectionDoesNotHideRemovalUnderflowWithAdditions)
{
    urd::builder<ygg::Relation<ygg::Schema>> input { tyr::datalog::tests::ops::cols({ ColumnIndex(1), ColumnIndex(2) }) };
    input.insert(cells(7, 10));
    database::incremental::ProjectionEvaluator evaluator(layer(), database::ProjectionPlan(input.columns(), std::array { ColumnIndex(1) }));
    database::Workspace workspace;
    evaluator.initialize(borrow(input), workspace);
    database::incremental::Delta invalid(layer(), input.columns());
    invalid.removed.insert(cells(7, 10));
    invalid.removed.insert(cells(7, 11));  // Only one existing witness is available.
    invalid.added.insert(cells(7, 12));
    invalid.added.insert(cells(7, 13));  // Net support change is zero, but removal is invalid.
    EXPECT_THROW(evaluator.update(invalid.change(), workspace), std::invalid_argument);
    urd::builder<ygg::Relation<ygg::Schema>> empty(input.columns());
    EXPECT_THROW(evaluator.update(std::pair { borrow(empty), borrow(empty) }, workspace), std::logic_error);
    evaluator.initialize(borrow(input), workspace);
    database::incremental::Delta valid(layer(), input.columns());
    valid.removed.insert(cells(7, 10));
    valid.added.insert(cells(7, 12));
    evaluator.update(valid.change(), workspace);
    EXPECT_TRUE(evaluator.get_result().contains(cells(7)));
    EXPECT_TRUE(evaluator.get_delta().added.empty());
    EXPECT_TRUE(evaluator.get_delta().removed.empty());
    evaluator.update(std::pair { borrow(empty), borrow(valid.added) }, workspace);
    EXPECT_TRUE(evaluator.get_result().empty());
    EXPECT_TRUE(evaluator.get_delta().removed.contains(cells(7)));
}

TEST(YggdrasilTests, DatabaseIncrementalProjectionHandlesEmptyAndNullaryRelations)
{
    urd::builder<ygg::Relation<ygg::Schema>> input { tyr::datalog::tests::ops::cols({ ColumnIndex(4) }) };
    const database::ProjectionPlan plan(input.columns(), {});
    database::incremental::ProjectionEvaluator evaluator(layer(), plan);
    database::Workspace workspace;
    urd::builder<ygg::Relation<ygg::Schema>> reference;
    evaluator.initialize(borrow(input), workspace);
    EXPECT_TRUE(evaluator.get_result().empty());
    database::incremental::Delta delta(layer(), input.columns());
    delta.added.insert(cells(1));
    delta.added.insert(cells(2));
    const auto update = [&]
    {
        const auto before = rows<ygg::uint_t>(borrow(reference));
        apply_batch(input, delta.added, delta.removed);
        evaluator.update(delta.change(), workspace);
        database::project(borrow(input), plan, reference, workspace);
        expect_update(evaluator, before, reference);
    };
    update();
    EXPECT_TRUE(evaluator.get_delta().added.contains(cells()));
    delta.clear();
    delta.removed.insert(cells(1));
    update();
    EXPECT_TRUE(evaluator.get_delta().removed.empty());
    delta.clear();
    delta.removed.insert(cells(2));
    update();
    EXPECT_TRUE(evaluator.get_delta().removed.contains(cells()));
    delta.clear();
    update();
    EXPECT_TRUE(evaluator.get_delta().added.empty());
    EXPECT_TRUE(evaluator.get_delta().removed.empty());

    urd::builder<ygg::Relation<ygg::Schema>> nullary;
    database::incremental::Delta nullary_delta(layer(), {});
    database::incremental::ProjectionEvaluator identity(layer(), database::ProjectionPlan({}, {}));
    identity.initialize(borrow(nullary), workspace);
    nullary_delta.added.insert(cells());
    identity.update(nullary_delta.change(), workspace);
    EXPECT_TRUE(identity.get_result().contains(cells()));
    EXPECT_TRUE(identity.get_delta().added.contains(cells()));
    identity.update(std::pair { borrow(nullary_delta.removed), borrow(nullary_delta.added) }, workspace);
    EXPECT_TRUE(identity.get_result().empty());
    EXPECT_TRUE(identity.get_delta().removed.contains(cells()));
}

TEST(YggdrasilTests, DatabaseIncrementalJoinUpdatesEitherSideIndependently)
{
    urd::builder<ygg::Relation<ygg::Schema>> left_operand { tyr::datalog::tests::ops::cols({ ColumnIndex(7), ColumnIndex(2), ColumnIndex(9) }) };
    urd::builder<ygg::Relation<ygg::Schema>> right_operand { tyr::datalog::tests::ops::cols({ ColumnIndex(9), ColumnIndex(11), ColumnIndex(2) }) };
    left_operand.insert(cells(1, 2, 3));
    left_operand.insert(cells(4, 2, 3));
    left_operand.insert(cells(5, 8, 3));
    right_operand.insert(cells(3, 20, 2));
    right_operand.insert(cells(3, 21, 2));
    right_operand.insert(cells(3, 22, 8));
    right_operand.insert(cells(9, 23, 2));
    const database::JoinPlan plan(left_operand.columns(), right_operand.columns());
    ygg::FreeRelationRepository repository;
    database::Workspace workspace;
    for (const bool change_left : { false, true })
    {
        auto& dynamic = change_left ? left_operand : right_operand;
        urd::builder<ygg::Relation<ygg::Schema>> unchanged(change_left ? right_operand.columns() : left_operand.columns());
        database::incremental::JoinEvaluator evaluator(layer(), plan);
        urd::builder<ygg::Relation<ygg::Schema>> reference(plan.output_columns());
        evaluator.initialize(urd::resolve(left_operand, repository), urd::resolve(right_operand, repository), workspace);
        database::join(borrow(left_operand), borrow(right_operand), plan, reference, workspace);
        EXPECT_EQ(rows<ygg::uint_t>(evaluator.get_result()), rows<ygg::uint_t>(borrow(reference)));
        EXPECT_TRUE(evaluator.get_delta().added.empty());
        EXPECT_TRUE(evaluator.get_delta().removed.empty());
        database::incremental::Delta delta(layer(), dynamic.columns());
        delta.removed.insert(borrow(dynamic).row(0).get_values());
        if (!change_left)
            delta.added.insert(cells(3, 30, 2));
        else
            delta.added.insert(cells(6, 2, 3));
        const auto update = [&](const auto& added, const auto& removed)
        {
            const auto before = rows<ygg::uint_t>(borrow(reference));
            apply_batch(dynamic, added, removed);
            if (change_left)
                evaluator.update(std::pair { borrow(added), borrow(removed) }, std::pair { borrow(unchanged), borrow(unchanged) }, workspace);
            else
                evaluator.update(std::pair { borrow(unchanged), borrow(unchanged) }, std::pair { borrow(added), borrow(removed) }, workspace);
            database::join(borrow(left_operand), borrow(right_operand), plan, reference, workspace);
            expect_update(evaluator, before, reference);
        };
        update(delta.added, delta.removed);
        update(delta.removed, delta.added);
        delta.clear();
        update(delta.added, delta.removed);
    }
}

TEST(YggdrasilTests, DatabaseIncrementalJoinAcceptsInternedWideInputs)
{
    urd::builder<ygg::Relation<ygg::Schema>> dynamic(columns_of<ygg::Value>({ ColumnIndex(1), ColumnIndex(2) }));
    urd::builder<ygg::Relation<ygg::Schema>> immutable(columns_of<ygg::Value>({ ColumnIndex(2), ColumnIndex(3) }));
    dynamic.insert(std::array { ygg::encode_value(ygg::Value { 1 }), ygg::encode_value(ygg::Value { 10 }) });
    immutable.insert(std::array { ygg::encode_value(ygg::Value { 10 }), ygg::encode_value(ygg::Value { 100 }) });
    immutable.insert(std::array { ygg::encode_value(ygg::Value { 20 }), ygg::encode_value(ygg::Value { 200 }) });
    immutable.insert(std::array { ygg::encode_value(ygg::Value { 30 }), ygg::encode_value(ygg::Value { 300 }) });
    ygg::FreeRelationRepository repository;
    const auto view = tyr::datalog::tests::ops::intern(repository, immutable).first;
    const database::JoinPlan plan(dynamic.columns(), immutable.columns());
    database::incremental::JoinEvaluator evaluator(layer(), plan);
    urd::builder<ygg::Relation<ygg::Schema>> unchanged(immutable.columns());
    database::Workspace workspace;
    urd::builder<ygg::Relation<ygg::Schema>> reference(plan.output_columns());
    evaluator.initialize(borrow(dynamic), view, workspace);
    database::join(borrow(dynamic), borrow(immutable), plan, reference, workspace);
    EXPECT_EQ(rows<ygg::Value>(evaluator.get_result()), rows<ygg::Value>(borrow(reference)));
    database::incremental::Delta delta(layer(), dynamic.columns());
    delta.removed.insert(std::array { ygg::encode_value(ygg::Value { 1 }), ygg::encode_value(ygg::Value { 10 }) });
    delta.added.insert(std::array { ygg::encode_value(ygg::Value { 2 }), ygg::encode_value(ygg::Value { 20 }) });
    delta.added.insert(std::array { ygg::encode_value(ygg::Value { 3 }), ygg::encode_value(ygg::Value { 40 }) });
    const auto before = rows<ygg::Value>(borrow(reference));
    apply_batch(dynamic, delta.added, delta.removed);
    evaluator.update(delta.change(), std::pair { borrow(unchanged), borrow(unchanged) }, workspace);
    database::join(borrow(dynamic), borrow(immutable), plan, reference, workspace);
    expect_update(evaluator, before, reference);
}

TEST(YggdrasilTests, DatabaseIncrementalProjectionValidationPrecedesMutation)
{
    urd::builder<ygg::Relation<ygg::Schema>> input { tyr::datalog::tests::ops::cols({ ColumnIndex(1) }) };
    input.insert(cells(1));
    urd::builder<ygg::Relation<ygg::Schema>> empty(input.columns());
    urd::builder<ygg::Relation<ygg::Schema>> overlap(input.columns());
    overlap.insert(cells(1));
    database::incremental::ProjectionEvaluator evaluator(layer(), database::ProjectionPlan(input.columns(), std::array { ColumnIndex(1) }));
    database::Workspace workspace;
    EXPECT_THROW(evaluator.update(std::pair { borrow(empty), borrow(empty) }, workspace), std::logic_error);
    evaluator.initialize(borrow(input), workspace);
    const auto before = rows<ygg::uint_t>(evaluator.get_result());
    EXPECT_THROW(evaluator.update(std::pair { borrow(overlap), borrow(overlap) }, workspace), std::invalid_argument);
    EXPECT_EQ(rows<ygg::uint_t>(evaluator.get_result()), before);
    evaluator.update(std::pair { borrow(empty), borrow(empty) }, workspace);
    EXPECT_EQ(rows<ygg::uint_t>(evaluator.get_result()), before);
    EXPECT_TRUE(evaluator.get_delta().added.empty());
    EXPECT_TRUE(evaluator.get_delta().removed.empty());
    database::incremental::Delta delta(layer(), input.columns());
    delta.added.insert(cells(1));
    delta.removed.insert(cells(2));
    const auto memory = delta.added.memory_usage() + delta.removed.memory_usage();
    delta.clear();
    EXPECT_TRUE(delta.added.empty());
    EXPECT_TRUE(delta.removed.empty());
    EXPECT_EQ(delta.added.memory_usage() + delta.removed.memory_usage(), memory);
}

TEST(YggdrasilTests, DatabaseIncrementalJoinUpdatesBothInputsAndRestoresBranches)
{
    urd::builder<ygg::Relation<ygg::Schema>> left_operand { tyr::datalog::tests::ops::cols({ ColumnIndex(9), ColumnIndex(2), ColumnIndex(7) }) };
    urd::builder<ygg::Relation<ygg::Schema>> right_operand { tyr::datalog::tests::ops::cols({ ColumnIndex(7), ColumnIndex(11), ColumnIndex(2) }) };
    left_operand.insert(cells(1, 10, 100));
    left_operand.insert(cells(2, 10, 100));
    left_operand.insert(cells(3, 20, 200));
    left_operand.insert(cells(4, 30, 300));
    right_operand.insert(cells(100, 7, 10));
    right_operand.insert(cells(100, 8, 10));
    right_operand.insert(cells(200, 7, 20));
    right_operand.insert(cells(300, 9, 30));
    const auto initial_left = rows<ygg::uint_t>(borrow(left_operand));
    const auto initial_right = rows<ygg::uint_t>(borrow(right_operand));
    const database::JoinPlan plan(left_operand.columns(), right_operand.columns());
    const database::ProjectionPlan projection_plan(plan.output_columns(), std::array { ColumnIndex(11) });
    database::incremental::JoinEvaluator evaluator(layer(), plan);
    database::incremental::ProjectionEvaluator projected(layer(), projection_plan);
    database::Workspace workspace;
    ygg::FreeRelationRepository repository;
    evaluator.initialize(urd::resolve(left_operand, repository), urd::resolve(right_operand, repository), workspace);
    projected.initialize(evaluator.get_result(), workspace);
    urd::builder<ygg::Relation<ygg::Schema>> reference(plan.output_columns());
    urd::builder<ygg::Relation<ygg::Schema>> reference_projection(projection_plan.output_columns());
    database::join(borrow(left_operand), borrow(right_operand), plan, reference, workspace);
    database::project(borrow(reference), projection_plan, reference_projection, workspace);
    EXPECT_EQ(rows<ygg::uint_t>(evaluator.get_result()), rows<ygg::uint_t>(borrow(reference)));
    EXPECT_TRUE(evaluator.get_delta().added.empty());
    EXPECT_TRUE(evaluator.get_delta().removed.empty());
    const auto update = [&](const auto& left_added, const auto& left_removed, const auto& right_added, const auto& right_removed)
    {
        const auto before = rows<ygg::uint_t>(borrow(reference));
        const auto projection_before = rows<ygg::uint_t>(borrow(reference_projection));
        apply_batch(left_operand, left_added, left_removed);
        apply_batch(right_operand, right_added, right_removed);
        evaluator.update(std::pair(urd::resolve(left_added, repository), urd::resolve(left_removed, repository)),
                         std::pair(urd::resolve(right_added, repository), urd::resolve(right_removed, repository)),
                         workspace);
        projected.update(evaluator.get_delta().change(), workspace);
        database::join(borrow(left_operand), borrow(right_operand), plan, reference, workspace);
        database::project(borrow(reference), projection_plan, reference_projection, workspace);
        expect_update(evaluator, before, reference);
        expect_update(projected, projection_before, reference_projection);
    };
    database::incremental::Delta left_a(layer(), left_operand.columns());
    database::incremental::Delta right_a(layer(), right_operand.columns());
    left_a.removed.insert(cells(1, 10, 100));
    left_a.removed.insert(cells(3, 20, 200));
    left_a.added.insert(cells(5, 40, 400));
    left_a.added.insert(cells(6, 10, 100));
    right_a.removed.insert(cells(100, 8, 10));
    right_a.removed.insert(cells(200, 7, 20));
    right_a.added.insert(cells(400, 10, 40));
    right_a.added.insert(cells(100, 11, 10));
    update(left_a.added, left_a.removed, right_a.added, right_a.removed);
    // New/new pairs must appear; old/new and new/old crossed pairs must not.
    EXPECT_TRUE(evaluator.get_delta().added.contains(cells(5, 40, 400, 10)));
    EXPECT_FALSE(evaluator.get_delta().added.contains(cells(1, 10, 100, 11)));
    EXPECT_FALSE(evaluator.get_delta().added.contains(cells(6, 10, 100, 8)));
    database::incremental::Delta left_b(layer(), left_operand.columns());
    database::incremental::Delta right_b(layer(), right_operand.columns());
    left_b.removed.insert(cells(4, 30, 300));
    left_b.added.insert(cells(7, 40, 400));
    right_b.removed.insert(cells(300, 9, 30));
    right_b.added.insert(cells(400, 12, 40));
    update(left_b.added, left_b.removed, right_b.added, right_b.removed);
    update(left_b.removed, left_b.added, right_b.removed, right_b.added);
    update(left_a.removed, left_a.added, right_a.removed, right_a.added);
    EXPECT_EQ(rows<ygg::uint_t>(borrow(left_operand)), initial_left);
    EXPECT_EQ(rows<ygg::uint_t>(borrow(right_operand)), initial_right);
    database::incremental::Delta sibling(layer(), left_operand.columns());
    database::incremental::Delta unchanged(layer(), right_operand.columns());
    sibling.removed.insert(cells(2, 10, 100));
    sibling.added.insert(cells(8, 30, 300));
    update(sibling.added, sibling.removed, unchanged.added, unchanged.removed);
    update(sibling.removed, sibling.added, unchanged.added, unchanged.removed);
    EXPECT_EQ(rows<ygg::uint_t>(borrow(left_operand)), initial_left);
    EXPECT_EQ(rows<ygg::uint_t>(borrow(right_operand)), initial_right);
    evaluator.initialize(borrow(left_operand), borrow(right_operand), workspace);
    EXPECT_EQ(rows<ygg::uint_t>(evaluator.get_result()), rows<ygg::uint_t>(borrow(reference)));
    EXPECT_TRUE(evaluator.get_delta().added.empty());
    EXPECT_TRUE(evaluator.get_delta().removed.empty());
}

TEST(YggdrasilTests, DatabaseIncrementalJoinMaintainsWideIndexesAcrossRowCompaction)
{
    urd::builder<ygg::Relation<ygg::Schema>> left_operand(columns_of<ygg::Value>({ ColumnIndex(1), ColumnIndex(2) }));
    urd::builder<ygg::Relation<ygg::Schema>> right_operand(columns_of<ygg::Value>({ ColumnIndex(2), ColumnIndex(3) }));
    for (ygg::uint_t i = 0; i < 5; ++i)
    {
        left_operand.insert(std::array { ygg::encode_value(ygg::Value { i }), ygg::encode_value(ygg::Value { i % 3 }) });
        right_operand.insert(std::array { ygg::encode_value(ygg::Value { i % 3 }), ygg::encode_value(ygg::Value { i }) });
    }
    const database::JoinPlan plan(left_operand.columns(), right_operand.columns());
    database::incremental::JoinEvaluator evaluator(layer(), plan);
    database::Workspace workspace;
    urd::builder<ygg::Relation<ygg::Schema>> reference(plan.output_columns());
    evaluator.initialize(borrow(left_operand), borrow(right_operand), workspace);
    database::join(borrow(left_operand), borrow(right_operand), plan, reference, workspace);
    EXPECT_EQ(rows<ygg::Value>(evaluator.get_result()), rows<ygg::Value>(borrow(reference)));
    database::incremental::Delta left_delta(layer(), left_operand.columns());
    database::incremental::Delta right_delta(layer(), right_operand.columns());
    for (ygg::uint_t generation = 0; generation < 24; ++generation)
    {
        left_delta.clear();
        right_delta.clear();
        if (generation % 3 != 1)
        {
            left_delta.removed.insert(borrow(left_operand).row(generation % left_operand.size()).get_values());
            left_delta.added.insert(std::array { ygg::encode_value(ygg::Value { 100 + generation }), ygg::encode_value(ygg::Value { generation % 3 }) });
        }
        if (generation % 3 != 2)
        {
            right_delta.removed.insert(borrow(right_operand).row((generation + 1) % right_operand.size()).get_values());
            right_delta.added.insert(std::array { ygg::encode_value(ygg::Value { generation % 3 }), ygg::encode_value(ygg::Value { 100 + generation }) });
        }
        const auto before = rows<ygg::Value>(borrow(reference));
        apply_batch(left_operand, left_delta.added, left_delta.removed);
        apply_batch(right_operand, right_delta.added, right_delta.removed);
        evaluator.update(left_delta.change(), right_delta.change(), workspace);
        database::join(borrow(left_operand), borrow(right_operand), plan, reference, workspace);
        expect_update(evaluator, before, reference);
    }
}

TEST(YggdrasilTests, DatabaseIncrementalJoinHandlesCartesianAndNullaryChangesOnBothSides)
{
    database::Workspace workspace;
    for (unsigned nullary_sides = 0; nullary_sides < 4; ++nullary_sides)
    {
        urd::builder<ygg::Relation<ygg::Schema>> left_operand;
        urd::builder<ygg::Relation<ygg::Schema>> right_operand;
        if (!(nullary_sides & 1))
            left_operand.initialize(tyr::datalog::tests::ops::cols({ ColumnIndex(1) }));
        if (!(nullary_sides & 2))
            right_operand.initialize(tyr::datalog::tests::ops::cols({ ColumnIndex(2) }));
        const database::JoinPlan plan(left_operand.columns(), right_operand.columns());
        database::incremental::JoinEvaluator evaluator(layer(), plan);
        urd::builder<ygg::Relation<ygg::Schema>> reference(plan.output_columns());
        evaluator.initialize(borrow(left_operand), borrow(right_operand), workspace);
        EXPECT_TRUE(evaluator.get_result().empty());
        database::incremental::Delta left_delta(layer(), left_operand.columns());
        database::incremental::Delta right_delta(layer(), right_operand.columns());
        if (left_operand.columns().size() == 0)
            left_delta.added.insert(cells());
        else
        {
            left_delta.added.insert(cells(1));
            left_delta.added.insert(cells(2));
        }
        if (right_operand.columns().size() == 0)
            right_delta.added.insert(cells());
        else
        {
            right_delta.added.insert(cells(10));
            right_delta.added.insert(cells(20));
        }
        const auto update = [&](const auto& left_added, const auto& left_removed, const auto& right_added, const auto& right_removed)
        {
            const auto before = rows<ygg::uint_t>(borrow(reference));
            apply_batch(left_operand, left_added, left_removed);
            apply_batch(right_operand, right_added, right_removed);
            evaluator.update(std::pair { borrow(left_added), borrow(left_removed) }, std::pair { borrow(right_added), borrow(right_removed) }, workspace);
            database::join(borrow(left_operand), borrow(right_operand), plan, reference, workspace);
            expect_update(evaluator, before, reference);
        };
        update(left_delta.added, left_delta.removed, right_delta.added, right_delta.removed);
        database::incremental::Delta left_remove(layer(), left_operand.columns());
        database::incremental::Delta right_remove(layer(), right_operand.columns());
        left_remove.removed.insert(borrow(left_operand).row(0).get_values());
        right_remove.removed.insert(borrow(right_operand).row(0).get_values());
        update(left_remove.added, left_remove.removed, right_remove.added, right_remove.removed);
        update(left_remove.removed, left_remove.added, right_remove.removed, right_remove.added);
        update(left_delta.removed, left_delta.added, right_delta.removed, right_delta.added);
        EXPECT_TRUE(left_operand.empty());
        EXPECT_TRUE(right_operand.empty());
        update(left_remove.added, left_remove.added, right_remove.added, right_remove.added);
        EXPECT_TRUE(evaluator.get_delta().added.empty());
        EXPECT_TRUE(evaluator.get_delta().removed.empty());
        // Changing one input while the other remains empty emits no rows.
        update(left_delta.added, left_delta.removed, right_delta.removed, right_delta.removed);
        EXPECT_TRUE(evaluator.get_result().empty());
        EXPECT_TRUE(evaluator.get_delta().added.empty());
        update(left_delta.removed, left_delta.removed, right_delta.added, right_delta.removed);
        update(left_delta.removed, left_delta.added, right_delta.removed, right_delta.removed);
        EXPECT_TRUE(evaluator.get_result().empty());
        update(left_delta.removed, left_delta.removed, right_delta.removed, right_delta.added);
    }
}

TEST(YggdrasilTests, DatabaseIncrementalJoinValidatesAllInputsBeforeMutation)
{
    urd::builder<ygg::Relation<ygg::Schema>> input { tyr::datalog::tests::ops::cols({ ColumnIndex(1) }) };
    input.insert(cells(1));
    urd::builder<ygg::Relation<ygg::Schema>> empty(input.columns());
    urd::builder<ygg::Relation<ygg::Schema>> addition(input.columns());
    addition.insert(cells(2));
    database::incremental::JoinEvaluator evaluator(layer(), database::JoinPlan(input.columns(), input.columns()));
    database::Workspace workspace;
    EXPECT_THROW(evaluator.update(std::pair { borrow(empty), borrow(empty) }, std::pair { borrow(empty), borrow(empty) }, workspace), std::logic_error);
    evaluator.initialize(borrow(input), borrow(input), workspace);
    evaluator.update(std::pair { borrow(addition), borrow(empty) }, std::pair { borrow(addition), borrow(empty) }, workspace);
    EXPECT_TRUE(evaluator.get_delta().added.contains(cells(2)));
    const auto before = rows<ygg::uint_t>(evaluator.get_result());
    const auto added_before = rows<ygg::uint_t>(evaluator.get_delta().change().first);
    EXPECT_THROW(evaluator.update(std::pair { borrow(addition), borrow(addition) }, std::pair { borrow(empty), borrow(empty) }, workspace),
                 std::invalid_argument);
    EXPECT_THROW(evaluator.update(std::pair { borrow(empty), borrow(empty) }, std::pair { borrow(addition), borrow(addition) }, workspace),
                 std::invalid_argument);
    EXPECT_EQ(rows<ygg::uint_t>(evaluator.get_result()), before);
    EXPECT_EQ(rows<ygg::uint_t>(evaluator.get_delta().change().first), added_before);
    evaluator.update(std::pair { borrow(empty), borrow(empty) }, std::pair { borrow(empty), borrow(empty) }, workspace);
    EXPECT_EQ(rows<ygg::uint_t>(evaluator.get_result()), before);
    EXPECT_TRUE(evaluator.get_delta().added.empty());
    EXPECT_TRUE(evaluator.get_delta().removed.empty());
}

TEST(YggdrasilTests, DatabaseIncrementalJoinRequiresReinitializationAfterMutationFailure)
{
    urd::builder<ygg::Relation<ygg::Schema>> input { tyr::datalog::tests::ops::cols({ ColumnIndex(1) }) };
    input.insert(cells(1));
    urd::builder<ygg::Relation<ygg::Schema>> empty(input.columns());
    urd::builder<ygg::Relation<ygg::Schema>> addition(input.columns());
    addition.insert(cells(2));
    urd::builder<ygg::Relation<ygg::Schema>> absent(input.columns());
    absent.insert(cells(99));
    database::incremental::JoinEvaluator evaluator(layer(), database::JoinPlan(input.columns(), input.columns()));
    database::Workspace workspace;
    for (const bool invalid_removal : { true, false })
    {
        evaluator.initialize(borrow(input), borrow(input), workspace);
        // Mutate the left input successfully before the right input fails.
        if (invalid_removal)
            EXPECT_THROW(evaluator.update(std::pair { borrow(empty), borrow(input) }, std::pair { borrow(empty), borrow(absent) }, workspace),
                         std::invalid_argument);
        else
            EXPECT_THROW(evaluator.update(std::pair { borrow(addition), borrow(empty) }, std::pair { borrow(input), borrow(empty) }, workspace),
                         std::invalid_argument);
        EXPECT_THROW(evaluator.update(std::pair { borrow(empty), borrow(empty) }, std::pair { borrow(empty), borrow(empty) }, workspace), std::logic_error);
        evaluator.initialize(borrow(input), borrow(input), workspace);
        EXPECT_EQ(rows<ygg::uint_t>(evaluator.get_result()), rows<ygg::uint_t>(borrow(input)));
        EXPECT_TRUE(evaluator.get_delta().added.empty());
        EXPECT_TRUE(evaluator.get_delta().removed.empty());
        evaluator.update(std::pair { borrow(addition), borrow(empty) }, std::pair { borrow(addition), borrow(empty) }, workspace);
        EXPECT_EQ(evaluator.get_result().size(), 2);
        EXPECT_TRUE(evaluator.get_result().contains(cells(1)));
        EXPECT_TRUE(evaluator.get_result().contains(cells(2)));
        EXPECT_EQ(rows<ygg::uint_t>(evaluator.get_delta().change().first), rows<ygg::uint_t>(borrow(addition)));
        EXPECT_TRUE(evaluator.get_delta().removed.empty());
    }
}

TEST(YggdrasilTests, DatabaseIncrementalEvaluatorsShareAccessAndAliasingContracts)
{
    // Equal schemas make the evaluators' own (empty) deltas valid changes, so only aliasing is rejected.
    urd::builder<ygg::Relation<ygg::Schema>> left_operand { tyr::datalog::tests::ops::cols({ ColumnIndex(0), ColumnIndex(1) }) };
    urd::builder<ygg::Relation<ygg::Schema>> right_operand { tyr::datalog::tests::ops::cols({ ColumnIndex(0), ColumnIndex(1) }) };
    const urd::builder<ygg::Relation<ygg::Schema>> none { tyr::datalog::tests::ops::cols({ ColumnIndex(0), ColumnIndex(1) }) };
    left_operand.insert(cells(1, 2));
    right_operand.insert(cells(1, 2));
    database::Workspace workspace;

    database::incremental::JoinEvaluator join_evaluator(layer(), database::JoinPlan(left_operand.columns(), right_operand.columns()));
    EXPECT_THROW(static_cast<void>(join_evaluator.get_result()), std::logic_error);
    EXPECT_THROW(static_cast<void>(join_evaluator.get_delta()), std::logic_error);
    join_evaluator.initialize(borrow(left_operand), borrow(right_operand), workspace);
    EXPECT_THROW(
        join_evaluator.update(std::pair { borrow(join_evaluator.get_delta().added), borrow(none) }, std::pair { borrow(none), borrow(none) }, workspace),
        std::invalid_argument);
    join_evaluator.update(std::pair { borrow(none), borrow(none) }, std::pair { borrow(none), borrow(none) }, workspace);
    EXPECT_EQ(rows<ygg::uint_t>(join_evaluator.get_result()), (std::set<urd::Store<urd::sequence<ygg::uint_t>>> { { 1, 2 } }));

    database::incremental::ProjectionEvaluator projection(layer(),
                                                          database::ProjectionPlan(left_operand.columns(), std::array { ColumnIndex(0), ColumnIndex(1) }));
    EXPECT_THROW(static_cast<void>(projection.get_result()), std::logic_error);
    projection.initialize(borrow(left_operand), workspace);
    EXPECT_THROW(projection.update(std::pair { borrow(projection.get_delta().added), borrow(none) }, workspace), std::invalid_argument);
    projection.update(std::pair { borrow(none), borrow(none) }, workspace);
    EXPECT_EQ(rows<ygg::uint_t>(projection.get_result()), (std::set<urd::Store<urd::sequence<ygg::uint_t>>> { { 1, 2 } }));
}

}  // namespace tyr::datalog::tests
