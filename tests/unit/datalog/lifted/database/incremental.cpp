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

ygg::FreeBuilderRelationView borrow(const urd::builder<ygg::Relation<ygg::Schema>>& relation) { return layer().view(relation); }

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
    for (const auto row : borrow(removed).rows())
        input.erase(input.find(row.get_values()).value());
    for (const auto row : borrow(added).rows())
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
    const auto replaced_index = evaluator.get_result().data().find(cells(7, 1)).value();
    const auto untouched_index = evaluator.get_result().data().find(cells(9, 3)).value();
    const auto replaced_row = evaluator.get_result().row(replaced_index);
    const auto untouched_row = evaluator.get_result().row(untouched_index);
    update(replacement.added, replacement.removed);
    // Consolidating witnesses must not erase/reinsert an unchanged output or
    // move the last output into its slot as a temporary consequence.
    EXPECT_EQ(evaluator.get_result().data().find(cells(7, 1)).value(), replaced_index);
    EXPECT_EQ(evaluator.get_result().data().find(cells(9, 3)).value(), untouched_index);
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
    urd::builder<ygg::Relation<ygg::Schema>> lhs { tyr::datalog::tests::ops::cols({ ColumnIndex(7), ColumnIndex(2), ColumnIndex(9) }) };
    urd::builder<ygg::Relation<ygg::Schema>> rhs { tyr::datalog::tests::ops::cols({ ColumnIndex(9), ColumnIndex(11), ColumnIndex(2) }) };
    lhs.insert(cells(1, 2, 3));
    lhs.insert(cells(4, 2, 3));
    lhs.insert(cells(5, 8, 3));
    rhs.insert(cells(3, 20, 2));
    rhs.insert(cells(3, 21, 2));
    rhs.insert(cells(3, 22, 8));
    rhs.insert(cells(9, 23, 2));
    const database::JoinPlan plan(lhs.columns(), rhs.columns());
    ygg::FreeRelationRepository repository;
    database::Workspace workspace;
    for (const bool change_left : { false, true })
    {
        auto& dynamic = change_left ? lhs : rhs;
        urd::builder<ygg::Relation<ygg::Schema>> unchanged(change_left ? rhs.columns() : lhs.columns());
        database::incremental::JoinEvaluator evaluator(layer(), plan);
        urd::builder<ygg::Relation<ygg::Schema>> reference(plan.output_columns());
        evaluator.initialize(urd::resolve(lhs, repository), urd::resolve(rhs, repository), workspace);
        database::join(borrow(lhs), borrow(rhs), plan, reference, workspace);
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
            database::join(borrow(lhs), borrow(rhs), plan, reference, workspace);
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
    urd::builder<ygg::Relation<ygg::Schema>> lhs { tyr::datalog::tests::ops::cols({ ColumnIndex(9), ColumnIndex(2), ColumnIndex(7) }) };
    urd::builder<ygg::Relation<ygg::Schema>> rhs { tyr::datalog::tests::ops::cols({ ColumnIndex(7), ColumnIndex(11), ColumnIndex(2) }) };
    lhs.insert(cells(1, 10, 100));
    lhs.insert(cells(2, 10, 100));
    lhs.insert(cells(3, 20, 200));
    lhs.insert(cells(4, 30, 300));
    rhs.insert(cells(100, 7, 10));
    rhs.insert(cells(100, 8, 10));
    rhs.insert(cells(200, 7, 20));
    rhs.insert(cells(300, 9, 30));
    const auto initial_lhs = rows<ygg::uint_t>(borrow(lhs));
    const auto initial_rhs = rows<ygg::uint_t>(borrow(rhs));
    const database::JoinPlan plan(lhs.columns(), rhs.columns());
    const database::ProjectionPlan projection_plan(plan.output_columns(), std::array { ColumnIndex(11) });
    database::incremental::JoinEvaluator evaluator(layer(), plan);
    database::incremental::ProjectionEvaluator projected(layer(), projection_plan);
    database::Workspace workspace;
    ygg::FreeRelationRepository repository;
    evaluator.initialize(urd::resolve(lhs, repository), urd::resolve(rhs, repository), workspace);
    projected.initialize(evaluator.get_result(), workspace);
    urd::builder<ygg::Relation<ygg::Schema>> reference(plan.output_columns());
    urd::builder<ygg::Relation<ygg::Schema>> reference_projection(projection_plan.output_columns());
    database::join(borrow(lhs), borrow(rhs), plan, reference, workspace);
    database::project(borrow(reference), projection_plan, reference_projection, workspace);
    EXPECT_EQ(rows<ygg::uint_t>(evaluator.get_result()), rows<ygg::uint_t>(borrow(reference)));
    EXPECT_TRUE(evaluator.get_delta().added.empty());
    EXPECT_TRUE(evaluator.get_delta().removed.empty());
    const auto update = [&](const auto& lhs_added, const auto& lhs_removed, const auto& rhs_added, const auto& rhs_removed)
    {
        const auto before = rows<ygg::uint_t>(borrow(reference));
        const auto projection_before = rows<ygg::uint_t>(borrow(reference_projection));
        apply_batch(lhs, lhs_added, lhs_removed);
        apply_batch(rhs, rhs_added, rhs_removed);
        evaluator.update(std::pair(urd::resolve(lhs_added, repository), urd::resolve(lhs_removed, repository)),
                         std::pair(urd::resolve(rhs_added, repository), urd::resolve(rhs_removed, repository)),
                         workspace);
        projected.update(evaluator.get_delta().change(), workspace);
        database::join(borrow(lhs), borrow(rhs), plan, reference, workspace);
        database::project(borrow(reference), projection_plan, reference_projection, workspace);
        expect_update(evaluator, before, reference);
        expect_update(projected, projection_before, reference_projection);
    };
    database::incremental::Delta lhs_a(layer(), lhs.columns());
    database::incremental::Delta rhs_a(layer(), rhs.columns());
    lhs_a.removed.insert(cells(1, 10, 100));
    lhs_a.removed.insert(cells(3, 20, 200));
    lhs_a.added.insert(cells(5, 40, 400));
    lhs_a.added.insert(cells(6, 10, 100));
    rhs_a.removed.insert(cells(100, 8, 10));
    rhs_a.removed.insert(cells(200, 7, 20));
    rhs_a.added.insert(cells(400, 10, 40));
    rhs_a.added.insert(cells(100, 11, 10));
    update(lhs_a.added, lhs_a.removed, rhs_a.added, rhs_a.removed);
    // New/new pairs must appear; old/new and new/old crossed pairs must not.
    EXPECT_TRUE(evaluator.get_delta().added.contains(cells(5, 40, 400, 10)));
    EXPECT_FALSE(evaluator.get_delta().added.contains(cells(1, 10, 100, 11)));
    EXPECT_FALSE(evaluator.get_delta().added.contains(cells(6, 10, 100, 8)));
    database::incremental::Delta lhs_b(layer(), lhs.columns());
    database::incremental::Delta rhs_b(layer(), rhs.columns());
    lhs_b.removed.insert(cells(4, 30, 300));
    lhs_b.added.insert(cells(7, 40, 400));
    rhs_b.removed.insert(cells(300, 9, 30));
    rhs_b.added.insert(cells(400, 12, 40));
    update(lhs_b.added, lhs_b.removed, rhs_b.added, rhs_b.removed);
    update(lhs_b.removed, lhs_b.added, rhs_b.removed, rhs_b.added);
    update(lhs_a.removed, lhs_a.added, rhs_a.removed, rhs_a.added);
    EXPECT_EQ(rows<ygg::uint_t>(borrow(lhs)), initial_lhs);
    EXPECT_EQ(rows<ygg::uint_t>(borrow(rhs)), initial_rhs);
    database::incremental::Delta sibling(layer(), lhs.columns());
    database::incremental::Delta unchanged(layer(), rhs.columns());
    sibling.removed.insert(cells(2, 10, 100));
    sibling.added.insert(cells(8, 30, 300));
    update(sibling.added, sibling.removed, unchanged.added, unchanged.removed);
    update(sibling.removed, sibling.added, unchanged.added, unchanged.removed);
    EXPECT_EQ(rows<ygg::uint_t>(borrow(lhs)), initial_lhs);
    EXPECT_EQ(rows<ygg::uint_t>(borrow(rhs)), initial_rhs);
    evaluator.initialize(borrow(lhs), borrow(rhs), workspace);
    EXPECT_EQ(rows<ygg::uint_t>(evaluator.get_result()), rows<ygg::uint_t>(borrow(reference)));
    EXPECT_TRUE(evaluator.get_delta().added.empty());
    EXPECT_TRUE(evaluator.get_delta().removed.empty());
}

TEST(YggdrasilTests, DatabaseIncrementalJoinMaintainsWideIndexesAcrossRowCompaction)
{
    urd::builder<ygg::Relation<ygg::Schema>> lhs(columns_of<ygg::Value>({ ColumnIndex(1), ColumnIndex(2) }));
    urd::builder<ygg::Relation<ygg::Schema>> rhs(columns_of<ygg::Value>({ ColumnIndex(2), ColumnIndex(3) }));
    for (ygg::uint_t i = 0; i < 5; ++i)
    {
        lhs.insert(std::array { ygg::encode_value(ygg::Value { i }), ygg::encode_value(ygg::Value { i % 3 }) });
        rhs.insert(std::array { ygg::encode_value(ygg::Value { i % 3 }), ygg::encode_value(ygg::Value { i }) });
    }
    const database::JoinPlan plan(lhs.columns(), rhs.columns());
    database::incremental::JoinEvaluator evaluator(layer(), plan);
    database::Workspace workspace;
    urd::builder<ygg::Relation<ygg::Schema>> reference(plan.output_columns());
    evaluator.initialize(borrow(lhs), borrow(rhs), workspace);
    database::join(borrow(lhs), borrow(rhs), plan, reference, workspace);
    EXPECT_EQ(rows<ygg::Value>(evaluator.get_result()), rows<ygg::Value>(borrow(reference)));
    database::incremental::Delta lhs_delta(layer(), lhs.columns());
    database::incremental::Delta rhs_delta(layer(), rhs.columns());
    for (ygg::uint_t generation = 0; generation < 24; ++generation)
    {
        lhs_delta.clear();
        rhs_delta.clear();
        if (generation % 3 != 1)
        {
            lhs_delta.removed.insert(borrow(lhs).row(generation % lhs.size()).get_values());
            lhs_delta.added.insert(std::array { ygg::encode_value(ygg::Value { 100 + generation }), ygg::encode_value(ygg::Value { generation % 3 }) });
        }
        if (generation % 3 != 2)
        {
            rhs_delta.removed.insert(borrow(rhs).row((generation + 1) % rhs.size()).get_values());
            rhs_delta.added.insert(std::array { ygg::encode_value(ygg::Value { generation % 3 }), ygg::encode_value(ygg::Value { 100 + generation }) });
        }
        const auto before = rows<ygg::Value>(borrow(reference));
        apply_batch(lhs, lhs_delta.added, lhs_delta.removed);
        apply_batch(rhs, rhs_delta.added, rhs_delta.removed);
        evaluator.update(lhs_delta.change(), rhs_delta.change(), workspace);
        database::join(borrow(lhs), borrow(rhs), plan, reference, workspace);
        expect_update(evaluator, before, reference);
    }
}

TEST(YggdrasilTests, DatabaseIncrementalJoinHandlesCartesianAndNullaryChangesOnBothSides)
{
    database::Workspace workspace;
    for (unsigned nullary_sides = 0; nullary_sides < 4; ++nullary_sides)
    {
        urd::builder<ygg::Relation<ygg::Schema>> lhs;
        urd::builder<ygg::Relation<ygg::Schema>> rhs;
        if (!(nullary_sides & 1))
            lhs.initialize(tyr::datalog::tests::ops::cols({ ColumnIndex(1) }));
        if (!(nullary_sides & 2))
            rhs.initialize(tyr::datalog::tests::ops::cols({ ColumnIndex(2) }));
        const database::JoinPlan plan(lhs.columns(), rhs.columns());
        database::incremental::JoinEvaluator evaluator(layer(), plan);
        urd::builder<ygg::Relation<ygg::Schema>> reference(plan.output_columns());
        evaluator.initialize(borrow(lhs), borrow(rhs), workspace);
        EXPECT_TRUE(evaluator.get_result().empty());
        database::incremental::Delta lhs_delta(layer(), lhs.columns());
        database::incremental::Delta rhs_delta(layer(), rhs.columns());
        if (lhs.columns().size() == 0)
            lhs_delta.added.insert(cells());
        else
        {
            lhs_delta.added.insert(cells(1));
            lhs_delta.added.insert(cells(2));
        }
        if (rhs.columns().size() == 0)
            rhs_delta.added.insert(cells());
        else
        {
            rhs_delta.added.insert(cells(10));
            rhs_delta.added.insert(cells(20));
        }
        const auto update = [&](const auto& lhs_added, const auto& lhs_removed, const auto& rhs_added, const auto& rhs_removed)
        {
            const auto before = rows<ygg::uint_t>(borrow(reference));
            apply_batch(lhs, lhs_added, lhs_removed);
            apply_batch(rhs, rhs_added, rhs_removed);
            evaluator.update(std::pair { borrow(lhs_added), borrow(lhs_removed) }, std::pair { borrow(rhs_added), borrow(rhs_removed) }, workspace);
            database::join(borrow(lhs), borrow(rhs), plan, reference, workspace);
            expect_update(evaluator, before, reference);
        };
        update(lhs_delta.added, lhs_delta.removed, rhs_delta.added, rhs_delta.removed);
        database::incremental::Delta lhs_remove(layer(), lhs.columns());
        database::incremental::Delta rhs_remove(layer(), rhs.columns());
        lhs_remove.removed.insert(borrow(lhs).row(0).get_values());
        rhs_remove.removed.insert(borrow(rhs).row(0).get_values());
        update(lhs_remove.added, lhs_remove.removed, rhs_remove.added, rhs_remove.removed);
        update(lhs_remove.removed, lhs_remove.added, rhs_remove.removed, rhs_remove.added);
        update(lhs_delta.removed, lhs_delta.added, rhs_delta.removed, rhs_delta.added);
        EXPECT_TRUE(lhs.empty());
        EXPECT_TRUE(rhs.empty());
        update(lhs_remove.added, lhs_remove.added, rhs_remove.added, rhs_remove.added);
        EXPECT_TRUE(evaluator.get_delta().added.empty());
        EXPECT_TRUE(evaluator.get_delta().removed.empty());
        // Changing one input while the other remains empty emits no rows.
        update(lhs_delta.added, lhs_delta.removed, rhs_delta.removed, rhs_delta.removed);
        EXPECT_TRUE(evaluator.get_result().empty());
        EXPECT_TRUE(evaluator.get_delta().added.empty());
        update(lhs_delta.removed, lhs_delta.removed, rhs_delta.added, rhs_delta.removed);
        update(lhs_delta.removed, lhs_delta.added, rhs_delta.removed, rhs_delta.removed);
        EXPECT_TRUE(evaluator.get_result().empty());
        update(lhs_delta.removed, lhs_delta.removed, rhs_delta.removed, rhs_delta.added);
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
    urd::builder<ygg::Relation<ygg::Schema>> lhs { tyr::datalog::tests::ops::cols({ ColumnIndex(0), ColumnIndex(1) }) };
    urd::builder<ygg::Relation<ygg::Schema>> rhs { tyr::datalog::tests::ops::cols({ ColumnIndex(0), ColumnIndex(1) }) };
    const urd::builder<ygg::Relation<ygg::Schema>> none { tyr::datalog::tests::ops::cols({ ColumnIndex(0), ColumnIndex(1) }) };
    lhs.insert(cells(1, 2));
    rhs.insert(cells(1, 2));
    database::Workspace workspace;

    database::incremental::JoinEvaluator join_evaluator(layer(), database::JoinPlan(lhs.columns(), rhs.columns()));
    EXPECT_THROW(static_cast<void>(join_evaluator.get_result()), std::logic_error);
    EXPECT_THROW(static_cast<void>(join_evaluator.get_delta()), std::logic_error);
    join_evaluator.initialize(borrow(lhs), borrow(rhs), workspace);
    EXPECT_THROW(
        join_evaluator.update(std::pair { borrow(join_evaluator.get_delta().added), borrow(none) }, std::pair { borrow(none), borrow(none) }, workspace),
        std::invalid_argument);
    join_evaluator.update(std::pair { borrow(none), borrow(none) }, std::pair { borrow(none), borrow(none) }, workspace);
    EXPECT_EQ(rows<ygg::uint_t>(join_evaluator.get_result()), (std::set<urd::Store<urd::sequence<ygg::uint_t>>> { { 1, 2 } }));

    database::incremental::ProjectionEvaluator projection(layer(), database::ProjectionPlan(lhs.columns(), std::array { ColumnIndex(0), ColumnIndex(1) }));
    EXPECT_THROW(static_cast<void>(projection.get_result()), std::logic_error);
    projection.initialize(borrow(lhs), workspace);
    EXPECT_THROW(projection.update(std::pair { borrow(projection.get_delta().added), borrow(none) }, workspace), std::invalid_argument);
    projection.update(std::pair { borrow(none), borrow(none) }, workspace);
    EXPECT_EQ(rows<ygg::uint_t>(projection.get_result()), (std::set<urd::Store<urd::sequence<ygg::uint_t>>> { { 1, 2 } }));
}

}  // namespace tyr::datalog::tests
