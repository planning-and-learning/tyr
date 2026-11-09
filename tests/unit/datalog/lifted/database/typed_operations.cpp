/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "tyr/datalog/lifted/database/semantics/incremental/join.hpp"
#include "tyr/datalog/lifted/database/semantics/incremental/projection.hpp"
#include "tyr/datalog/lifted/database/semantics/relation_view.hpp"

#include <array>
#include <bit>
#include <cstdint>
#include <gtest/gtest.h>
#include <tuple>
#include <vector>

namespace tyr::datalog::tests
{
namespace
{
using ColumnIndex = urd::index<ygg::Column>;
using RelationBuilder = urd::builder<ygg::Relation<ygg::Schema>>;

void expect_same_rows(const auto& actual, const auto& expected)
{
    ASSERT_EQ(actual.size(), expected.size());
    for (size_t i = 0; i < expected.size(); ++i)
        EXPECT_TRUE(actual.contains(expected.row(i)));
}

void apply_changes(RelationBuilder& input, const database::incremental::Delta& delta, bool undo = false)
{
    const auto [added, removed] = delta.change();
    for (const auto row : (undo ? added : removed).rows())
        input.erase(input.find(row).value());
    for (const auto row : (undo ? removed : added).rows())
        input.insert(row);
}

void expect_delta(const database::incremental::Delta& actual, const RelationBuilder& before, const RelationBuilder& after)
{
    const auto& layer = *actual.layer;
    RelationBuilder added(after.columns());
    RelationBuilder removed(before.columns());
    const database::SetPlan plan(after.columns(), before.columns());
    database::difference(layer.view(after), layer.view(before), plan, added);
    database::difference(layer.view(before), layer.view(after), plan, removed);
    const auto [actual_added, actual_removed] = actual.change();
    expect_same_rows(actual_added, layer.view(added));
    expect_same_rows(actual_removed, layer.view(removed));
}
}  // namespace

TEST(YggdrasilTests, DatabaseTypedProjectionConsolidatesWitnessesAndUndoesMixedChanges)
{
    const std::vector<ygg::ColumnLayout> columns { { ColumnIndex(1), ygg::type_id<ygg::uint_t> }, { ColumnIndex(2), ygg::type_id<double> }, { ColumnIndex(3), ygg::type_id<bool> } };
    RelationBuilder input(columns);
    input.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(1)), ygg::encode_value(1.5), ygg::encode_value(true) });
    input.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(2)), ygg::encode_value(1.5), ygg::encode_value(true) });
    input.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(3)), ygg::encode_value(2.5), ygg::encode_value(false) });

    const database::ProjectionPlan plan(columns, std::array { ColumnIndex(3), ColumnIndex(2) });
    const ygg::FreeRelationRepository repository;
    database::incremental::ProjectionEvaluator evaluator(repository, plan);
    database::Workspace workspace;
    evaluator.initialize(repository.view(input), workspace);
    RelationBuilder initial(plan.output_columns());
    database::project(repository.view(input), plan, initial, workspace);
    expect_same_rows(evaluator.get_result(), repository.view(initial));
    ASSERT_EQ(initial.size(), 2);
    EXPECT_EQ(initial.widths().at(1).at, 1U);  // after a bool of one bit
    EXPECT_TRUE(initial.contains(std::array { ygg::encode_value(true), ygg::encode_value(1.5) }));

    database::incremental::Delta changes(repository, columns);
    changes.removed.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(1)), ygg::encode_value(1.5), ygg::encode_value(true) });
    changes.removed.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(2)), ygg::encode_value(1.5), ygg::encode_value(true) });
    changes.added.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(4)), ygg::encode_value(1.5), ygg::encode_value(true) });
    changes.added.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(5)), ygg::encode_value(3.5), ygg::encode_value(false) });
    evaluator.update(changes.change(), workspace);
    apply_changes(input, changes);
    RelationBuilder updated(plan.output_columns());
    database::project(repository.view(input), plan, updated, workspace);
    expect_same_rows(evaluator.get_result(), repository.view(updated));
    expect_delta(evaluator.get_delta(), initial, updated);
    EXPECT_TRUE(evaluator.get_delta().removed.empty());
    ASSERT_EQ(evaluator.get_delta().added.size(), 1);
    EXPECT_TRUE(evaluator.get_delta().added.contains(std::array { ygg::encode_value(false), ygg::encode_value(3.5) }));

    evaluator.update(std::pair { repository.view(changes.removed), repository.view(changes.added) }, workspace);
    apply_changes(input, changes, true);
    expect_same_rows(evaluator.get_result(), repository.view(initial));
    expect_delta(evaluator.get_delta(), updated, initial);
}

TEST(YggdrasilTests, DatabaseTypedJoinUpdatesBothInputsAndRepairsCompactedIndexes)
{
    const std::vector<ygg::ColumnLayout> left_columns { { ColumnIndex(1), ygg::type_id<ygg::uint_t> }, { ColumnIndex(2), ygg::type_id<double> }, { ColumnIndex(3), ygg::type_id<bool> } };
    const std::vector<ygg::ColumnLayout> right_columns { { ColumnIndex(3), ygg::type_id<bool> }, { ColumnIndex(1), ygg::type_id<ygg::uint_t> }, { ColumnIndex(4), ygg::type_id<double> } };
    RelationBuilder left(left_columns);
    RelationBuilder right(right_columns);
    left.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(1)), ygg::encode_value(1.5), ygg::encode_value(true) });
    left.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(2)), ygg::encode_value(2.5), ygg::encode_value(false) });
    left.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(3)), ygg::encode_value(3.5), ygg::encode_value(true) });
    right.insert(std::array { ygg::encode_value(true), ygg::encode_value(static_cast<ygg::uint_t>(1)), ygg::encode_value(10.0) });
    right.insert(std::array { ygg::encode_value(false), ygg::encode_value(static_cast<ygg::uint_t>(2)), ygg::encode_value(20.0) });
    right.insert(std::array { ygg::encode_value(true), ygg::encode_value(static_cast<ygg::uint_t>(3)), ygg::encode_value(30.0) });
    const database::JoinPlan plan(left_columns, right_columns);
    const ygg::FreeRelationRepository repository;
    database::incremental::JoinEvaluator evaluator(repository, plan);
    database::Workspace workspace;
    evaluator.initialize(repository.view(left), repository.view(right), workspace);
    RelationBuilder initial(plan.output_columns());
    database::join(repository.view(left), repository.view(right), plan, initial, workspace);
    expect_same_rows(evaluator.get_result(), repository.view(initial));
    ASSERT_EQ(initial.size(), 3);

    database::incremental::Delta left_changes(repository, left_columns);
    database::incremental::Delta right_changes(repository, right_columns);
    left_changes.removed.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(1)), ygg::encode_value(1.5), ygg::encode_value(true) });
    left_changes.added.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(1)), ygg::encode_value(1.75), ygg::encode_value(true) });
    left_changes.added.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(4)), ygg::encode_value(4.5), ygg::encode_value(false) });
    right_changes.removed.insert(std::array { ygg::encode_value(false), ygg::encode_value(static_cast<ygg::uint_t>(2)), ygg::encode_value(20.0) });
    right_changes.added.insert(std::array { ygg::encode_value(false), ygg::encode_value(static_cast<ygg::uint_t>(2)), ygg::encode_value(22.0) });
    right_changes.added.insert(std::array { ygg::encode_value(false), ygg::encode_value(static_cast<ygg::uint_t>(4)), ygg::encode_value(40.0) });
    evaluator.update(left_changes.change(), right_changes.change(), workspace);
    apply_changes(left, left_changes);
    apply_changes(right, right_changes);
    RelationBuilder updated(plan.output_columns());
    database::join(repository.view(left), repository.view(right), plan, updated, workspace);
    expect_same_rows(evaluator.get_result(), repository.view(updated));
    expect_delta(evaluator.get_delta(), initial, updated);
    ASSERT_EQ(updated.size(), 4);
    EXPECT_TRUE(updated.contains(std::array { ygg::encode_value(ygg::uint_t(1)), ygg::encode_value(1.75), ygg::encode_value(true), ygg::encode_value(10.0) }));
    EXPECT_TRUE(updated.contains(std::array { ygg::encode_value(ygg::uint_t(2)), ygg::encode_value(2.5), ygg::encode_value(false), ygg::encode_value(22.0) }));
    EXPECT_TRUE(updated.contains(std::array { ygg::encode_value(ygg::uint_t(4)), ygg::encode_value(4.5), ygg::encode_value(false), ygg::encode_value(40.0) }));
    EXPECT_EQ(evaluator.get_delta().added.size(), 3);
    EXPECT_EQ(evaluator.get_delta().removed.size(), 2);

    evaluator.update(std::pair { repository.view(left_changes.removed), repository.view(left_changes.added) },
                     std::pair { repository.view(right_changes.removed), repository.view(right_changes.added) },
                     workspace);
    apply_changes(left, left_changes, true);
    apply_changes(right, right_changes, true);
    expect_same_rows(evaluator.get_result(), repository.view(initial));
    expect_delta(evaluator.get_delta(), updated, initial);
}

TEST(YggdrasilTests, DatabaseTypedConstantSelectionUsesCanonicalFloatEquality)
{
    const std::vector<ygg::ColumnLayout> columns { { ColumnIndex(1), ygg::type_id<double> }, { ColumnIndex(2), ygg::type_id<ygg::uint_t> } };
    RelationBuilder input(columns);
    const auto first_nan = std::bit_cast<double>(std::uint64_t { 0x7ff8000000000001 });
    const auto second_nan = std::bit_cast<double>(std::uint64_t { 0x7ff8000000000002 });
    input.insert(std::array { ygg::encode_value(0.0), ygg::encode_value(static_cast<ygg::uint_t>(1)) });
    input.insert(std::array { ygg::encode_value(-0.0), ygg::encode_value(static_cast<ygg::uint_t>(1)) });
    input.insert(std::array { ygg::encode_value(first_nan), ygg::encode_value(static_cast<ygg::uint_t>(2)) });
    input.insert(std::array { ygg::encode_value(second_nan), ygg::encode_value(static_cast<ygg::uint_t>(2)) });
    ASSERT_EQ(input.size(), 2);
    RelationBuilder result(columns);
    const ygg::FreeRelationRepository repository;
    database::select_equal_value(repository.view(input), database::SelectValuePlan(columns, ColumnIndex(1), ygg::type_id<double>, ygg::encode_value(-0.0)), result);
    ASSERT_EQ(result.size(), 1);
    EXPECT_TRUE(result.contains(std::array { ygg::encode_value(0.0), ygg::encode_value(ygg::uint_t(1)) }));
    database::select_equal_value(repository.view(input), database::SelectValuePlan(columns, ColumnIndex(1), ygg::type_id<double>, ygg::encode_value(second_nan)), result);
    ASSERT_EQ(result.size(), 1);
    EXPECT_TRUE(result.contains(std::array { ygg::encode_value(first_nan), ygg::encode_value(ygg::uint_t(2)) }));
}

TEST(YggdrasilTests, DatabaseTypedSchemaErrorsPreserveOperatorAndIncrementalResults)
{
    const std::vector<ygg::ColumnLayout> columns { { ColumnIndex(1), ygg::type_id<ygg::uint_t> }, { ColumnIndex(2), ygg::type_id<double> } };
    const std::vector<ygg::ColumnLayout> incompatible { { ColumnIndex(1), ygg::type_id<double> }, { ColumnIndex(2), ygg::type_id<double> } };
    RelationBuilder input(columns);
    RelationBuilder wrong(incompatible);
    RelationBuilder output(columns);
    input.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(1)), ygg::encode_value(1.5) });
    wrong.insert(std::array { ygg::encode_value(1.0), ygg::encode_value(1.5) });
    output.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(99)), ygg::encode_value(9.5) });
    database::Workspace workspace;
    const ygg::FreeRelationRepository repository;
    const auto input_view = repository.view(input);
    const auto wrong_view = repository.view(wrong);
    // Plans of incompatible column types are refused when built; the output is untouched.
    EXPECT_THROW((database::JoinPlan(input_view.columns(), wrong_view.columns())), std::invalid_argument);
    EXPECT_THROW((database::SetPlan(input_view.columns(), wrong_view.columns())), std::invalid_argument);
    EXPECT_THROW((database::SelectEqualPlan(input_view.columns(), ColumnIndex(1), ColumnIndex(2))), std::invalid_argument);
    EXPECT_THROW((database::SelectValuePlan(input_view.columns(), ColumnIndex(1), ygg::type_id<double>, ygg::encode_value(1.0))), std::invalid_argument);
    ASSERT_EQ(output.size(), 1);
    EXPECT_TRUE(output.contains(std::array { ygg::encode_value(ygg::uint_t(99)), ygg::encode_value(9.5) }));

    const database::ProjectionPlan plan(columns, std::array { ColumnIndex(1) });
    database::incremental::ProjectionEvaluator evaluator(repository, plan);
    evaluator.initialize(input_view, workspace);
    EXPECT_TRUE(evaluator.get_result().contains(std::array { ygg::encode_value(ygg::uint_t(1)) }));
    EXPECT_TRUE(evaluator.get_delta().added.empty());
    EXPECT_TRUE(evaluator.get_delta().removed.empty());
    RelationBuilder empty(columns);
    EXPECT_NO_THROW(evaluator.update(std::pair { repository.view(empty), repository.view(empty) }, workspace));
}

TEST(YggdrasilTests, DatabaseTypedJoinCacheSeparatesRowsByTypeAndLayout)
{
    ygg::FreeRelationRepository repository;
    database::JoinIndexCache cache;
    const std::vector<ygg::ColumnLayout> unsigned_columns { { ColumnIndex(1), ygg::type_id<std::uint64_t> } };
    const std::vector<ygg::ColumnLayout> float_columns { { ColumnIndex(1), ygg::type_id<double> } };
    RelationBuilder unsigned_input(unsigned_columns);
    RelationBuilder float_input(float_columns);
    unsigned_input.insert(std::array { ygg::encode_value(static_cast<std::uint64_t>(0)) });
    float_input.insert(std::array { ygg::encode_value(0.0) });
    const auto unsigned_view = repository.insert(unsigned_input).first;
    const auto float_view = repository.insert(float_input).first;
    // Rows of other column types lie in other pools (schemas), even of equal widths: separate relations.
    ASSERT_NE(unsigned_view.relation_identity(), float_view.relation_identity());
    const database::ProjectionPlan unsigned_key(unsigned_columns, std::array { ColumnIndex(1) });
    const database::ProjectionPlan float_key(float_columns, std::array { ColumnIndex(1) });
    cache.get_or_create(unsigned_view, unsigned_key.positions());
    cache.get_or_create(float_view, float_key.positions());
    EXPECT_EQ(cache.size(), 2);
    const std::vector<ygg::ColumnLayout> renamed_columns { { ColumnIndex(9), ygg::type_id<std::uint64_t> } };
    const auto renamed = repository.rename(unsigned_view, renamed_columns);
    cache.get_or_create(renamed, unsigned_key.positions());
    EXPECT_EQ(cache.size(), 3);  // a renamed relation is another relation

    const std::vector<ygg::ColumnLayout> before_columns { { ColumnIndex(2), ygg::type_id<bool> }, { ColumnIndex(1), ygg::type_id<ygg::uint_t> } };
    const std::vector<ygg::ColumnLayout> after_columns { { ColumnIndex(1), ygg::type_id<ygg::uint_t> }, { ColumnIndex(2), ygg::type_id<bool> } };
    RelationBuilder before(before_columns);
    RelationBuilder after(after_columns);
    before.insert(std::array { ygg::encode_value(false), ygg::encode_value(static_cast<ygg::uint_t>(0)) });
    after.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(0)), ygg::encode_value(false) });
    const auto before_view = repository.insert(before).first;
    const auto after_view = repository.insert(after).first;
    ASSERT_NE(before_view.relation_identity(), after_view.relation_identity());
    const database::ProjectionPlan before_key(before_columns, std::array { ColumnIndex(1) });
    const database::ProjectionPlan after_key(after_columns, std::array { ColumnIndex(1) });
    cache.get_or_create(before_view, before_key.positions());
    cache.get_or_create(after_view, after_key.positions());
    EXPECT_EQ(cache.size(), 5);
}
}  // namespace tyr::datalog::tests
