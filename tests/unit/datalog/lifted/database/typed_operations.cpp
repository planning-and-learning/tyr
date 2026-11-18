/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "query_helpers.hpp"
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
using RelationBuilder = Relation;

void expect_same_rows(const auto& actual, const auto& expected)
{
    ASSERT_EQ(actual.size(), expected.size());
    for (const auto& row : expected.get_rows())
        EXPECT_TRUE(actual.contains(values_of(row)));
}

template<typename C>
void apply_changes(RelationBuilder& input, const C& delta, bool undo = false)
{
    const auto [added, removed] = delta.change();
    for (const auto& row : (undo ? added : removed).get_rows())
        EXPECT_TRUE(input.erase(values_of(row)));
    for (const auto& row : (undo ? removed : added).get_rows())
        input.insert(values_of(row));
}

void expect_delta(const database::incremental::Delta& actual, const RelationBuilder& before, const RelationBuilder& after)
{
    RelationBuilder added(after.columns());
    RelationBuilder removed(before.columns());
    const database::SetPlan plan(after.columns(), before.columns());
    database::difference(borrow(after), borrow(before), plan, added);
    database::difference(borrow(before), borrow(after), plan, removed);
    const auto [actual_added, actual_removed] = actual.change();
    expect_same_rows(actual_added, borrow(added));
    expect_same_rows(actual_removed, borrow(removed));
}
}  // namespace

TEST(YggdrasilTests, DatabaseTypedProjectionConsolidatesWitnessesAndUndoesMixedChanges)
{
    const std::vector<ygg::ColumnLayout> columns { { ColumnIndex(1), ygg::type_id<ygg::uint_t> },
                                                   { ColumnIndex(2), ygg::type_id<double> },
                                                   { ColumnIndex(3), ygg::type_id<bool> } };
    RelationBuilder input(columns);
    input.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(1)), ygg::encode_value(1.5), ygg::encode_value(true) });
    input.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(2)), ygg::encode_value(1.5), ygg::encode_value(true) });
    input.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(3)), ygg::encode_value(2.5), ygg::encode_value(false) });

    const database::ProjectionPlan plan(columns, std::array { ColumnIndex(3), ColumnIndex(2) });
    const ygg::FreeRelationRepository repository;
    database::incremental::ProjectionEvaluator evaluator(scratch(), plan);
    database::Workspace workspace;
    evaluator.initialize(borrow(input), workspace);
    RelationBuilder initial(plan.output_columns());
    database::project(borrow(input), plan, initial, workspace);
    expect_same_rows(evaluator.get_result(), borrow(initial));
    ASSERT_EQ(initial.size(), 2);
    EXPECT_TRUE(initial.contains(std::array { ygg::encode_value(true), ygg::encode_value(1.5) }));

    Change changes(columns);
    changes.removed.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(1)), ygg::encode_value(1.5), ygg::encode_value(true) });
    changes.removed.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(2)), ygg::encode_value(1.5), ygg::encode_value(true) });
    changes.added.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(4)), ygg::encode_value(1.5), ygg::encode_value(true) });
    changes.added.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(5)), ygg::encode_value(3.5), ygg::encode_value(false) });
    evaluator.update(changes.change(), workspace);
    apply_changes(input, changes);
    RelationBuilder updated(plan.output_columns());
    database::project(borrow(input), plan, updated, workspace);
    expect_same_rows(evaluator.get_result(), borrow(updated));
    expect_delta(evaluator.get_delta(), initial, updated);
    EXPECT_TRUE(evaluator.get_delta().removed->view().empty());
    ASSERT_EQ(evaluator.get_delta().added->view().size(), 1);
    EXPECT_TRUE(evaluator.get_delta().added->view().contains(std::array { ygg::encode_value(false), ygg::encode_value(3.5) }));

    evaluator.update(std::pair { borrow(changes.removed), borrow(changes.added) }, workspace);
    apply_changes(input, changes, true);
    expect_same_rows(evaluator.get_result(), borrow(initial));
    expect_delta(evaluator.get_delta(), updated, initial);
}

TEST(YggdrasilTests, DatabaseTypedJoinUpdatesBothInputsAndRepairsCompactedIndexes)
{
    const std::vector<ygg::ColumnLayout> left_columns { { ColumnIndex(1), ygg::type_id<ygg::uint_t> },
                                                        { ColumnIndex(2), ygg::type_id<double> },
                                                        { ColumnIndex(3), ygg::type_id<bool> } };
    const std::vector<ygg::ColumnLayout> right_columns { { ColumnIndex(3), ygg::type_id<bool> },
                                                         { ColumnIndex(1), ygg::type_id<ygg::uint_t> },
                                                         { ColumnIndex(4), ygg::type_id<double> } };
    RelationBuilder left_operand(left_columns);
    RelationBuilder right_operand(right_columns);
    left_operand.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(1)), ygg::encode_value(1.5), ygg::encode_value(true) });
    left_operand.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(2)), ygg::encode_value(2.5), ygg::encode_value(false) });
    left_operand.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(3)), ygg::encode_value(3.5), ygg::encode_value(true) });
    right_operand.insert(std::array { ygg::encode_value(true), ygg::encode_value(static_cast<ygg::uint_t>(1)), ygg::encode_value(10.0) });
    right_operand.insert(std::array { ygg::encode_value(false), ygg::encode_value(static_cast<ygg::uint_t>(2)), ygg::encode_value(20.0) });
    right_operand.insert(std::array { ygg::encode_value(true), ygg::encode_value(static_cast<ygg::uint_t>(3)), ygg::encode_value(30.0) });
    const database::JoinPlan plan(left_columns, right_columns);
    const ygg::FreeRelationRepository repository;
    database::incremental::JoinEvaluator evaluator(scratch(), plan);
    database::Workspace workspace;
    evaluator.initialize(borrow(left_operand), borrow(right_operand), workspace);
    RelationBuilder initial(plan.output_columns());
    database::join(borrow(left_operand), borrow(right_operand), plan, initial, workspace);
    expect_same_rows(evaluator.get_result(), borrow(initial));
    ASSERT_EQ(initial.size(), 3);

    Change left_changes(left_columns);
    Change right_changes(right_columns);
    left_changes.removed.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(1)), ygg::encode_value(1.5), ygg::encode_value(true) });
    left_changes.added.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(1)), ygg::encode_value(1.75), ygg::encode_value(true) });
    left_changes.added.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(4)), ygg::encode_value(4.5), ygg::encode_value(false) });
    right_changes.removed.insert(std::array { ygg::encode_value(false), ygg::encode_value(static_cast<ygg::uint_t>(2)), ygg::encode_value(20.0) });
    right_changes.added.insert(std::array { ygg::encode_value(false), ygg::encode_value(static_cast<ygg::uint_t>(2)), ygg::encode_value(22.0) });
    right_changes.added.insert(std::array { ygg::encode_value(false), ygg::encode_value(static_cast<ygg::uint_t>(4)), ygg::encode_value(40.0) });
    evaluator.update(left_changes.change(), right_changes.change(), workspace);
    apply_changes(left_operand, left_changes);
    apply_changes(right_operand, right_changes);
    RelationBuilder updated(plan.output_columns());
    database::join(borrow(left_operand), borrow(right_operand), plan, updated, workspace);
    expect_same_rows(evaluator.get_result(), borrow(updated));
    expect_delta(evaluator.get_delta(), initial, updated);
    ASSERT_EQ(updated.size(), 4);
    EXPECT_TRUE(updated.contains(std::array { ygg::encode_value(ygg::uint_t(1)), ygg::encode_value(1.75), ygg::encode_value(true), ygg::encode_value(10.0) }));
    EXPECT_TRUE(updated.contains(std::array { ygg::encode_value(ygg::uint_t(2)), ygg::encode_value(2.5), ygg::encode_value(false), ygg::encode_value(22.0) }));
    EXPECT_TRUE(updated.contains(std::array { ygg::encode_value(ygg::uint_t(4)), ygg::encode_value(4.5), ygg::encode_value(false), ygg::encode_value(40.0) }));
    EXPECT_EQ(evaluator.get_delta().added->view().size(), 3);
    EXPECT_EQ(evaluator.get_delta().removed->view().size(), 2);

    evaluator.update(std::pair { borrow(left_changes.removed), borrow(left_changes.added) },
                     std::pair { borrow(right_changes.removed), borrow(right_changes.added) },
                     workspace);
    apply_changes(left_operand, left_changes, true);
    apply_changes(right_operand, right_changes, true);
    expect_same_rows(evaluator.get_result(), borrow(initial));
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
    database::select_equal_value(borrow(input),
                                 database::SelectValuePlan(columns, ColumnIndex(1), ygg::type_id<double>, ygg::encode_value(-0.0)),
                                 result);
    ASSERT_EQ(result.size(), 1);
    EXPECT_TRUE(result.contains(std::array { ygg::encode_value(0.0), ygg::encode_value(ygg::uint_t(1)) }));
    database::select_equal_value(borrow(input),
                                 database::SelectValuePlan(columns, ColumnIndex(1), ygg::type_id<double>, ygg::encode_value(second_nan)),
                                 result);
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
    const auto input_view = borrow(input);
    const auto wrong_view = borrow(wrong);
    // Plans of incompatible column types are refused when built; the output is untouched.
    EXPECT_THROW((database::JoinPlan(input_view.columns(), wrong_view.columns())), std::invalid_argument);
    EXPECT_THROW((database::SetPlan(input_view.columns(), wrong_view.columns())), std::invalid_argument);
    EXPECT_THROW((database::SelectEqualPlan(input_view.columns(), ColumnIndex(1), ColumnIndex(2))), std::invalid_argument);
    EXPECT_THROW((database::SelectValuePlan(input_view.columns(), ColumnIndex(1), ygg::type_id<double>, ygg::encode_value(1.0))), std::invalid_argument);
    ASSERT_EQ(output.size(), 1);
    EXPECT_TRUE(output.contains(std::array { ygg::encode_value(ygg::uint_t(99)), ygg::encode_value(9.5) }));

    const database::ProjectionPlan plan(columns, std::array { ColumnIndex(1) });
    database::incremental::ProjectionEvaluator evaluator(scratch(), plan);
    evaluator.initialize(input_view, workspace);
    EXPECT_TRUE(evaluator.get_result().contains(std::array { ygg::encode_value(ygg::uint_t(1)) }));
    EXPECT_TRUE(evaluator.get_delta().added->view().empty());
    EXPECT_TRUE(evaluator.get_delta().removed->view().empty());
    RelationBuilder empty(columns);
    EXPECT_NO_THROW(evaluator.update(std::pair { borrow(empty), borrow(empty) }, workspace));
}
}  // namespace tyr::datalog::tests
