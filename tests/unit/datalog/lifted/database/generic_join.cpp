/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "query_helpers.hpp"
#include "tyr/datalog/lifted/database/semantics/incremental/generic_join.hpp"
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>

#include "tyr/datalog/lifted/database/semantics/incremental/projection.hpp"
#include "tyr/datalog/lifted/database/semantics/relation_view.hpp"

#include <array>
#include <bit>
#include <gtest/gtest.h>
#include <random>
#include <tuple>

namespace tyr::datalog::tests
{
namespace
{
using Values = boost::mp11::mp_list<ygg::uint_t>;
using RelationBuilder = urd::builder<ygg::Relation<ygg::Schema>>;
using Change = database::incremental::Delta;
using ColumnIndex = urd::index<ygg::Column>;

/// The layer the tests read their builders through.
const ygg::FreeRelationRepository& layer()
{
    static const ygg::FreeRelationRepository repository;
    return repository;
}

ygg::FreeBuilderRelationView borrow(const urd::builder<ygg::Relation<ygg::Schema>>& relation) { return layer().view(relation); }

static_assert(database::RelationViewConcept<ygg::FreeBuilderRelationView>);

struct ThrowingRelation : ygg::FreeBuilderRelationView
{
    size_t fail_on_row;
    mutable size_t row_calls = 0;

    ThrowingRelation(const RelationBuilder& input, size_t fail_on) : ygg::FreeBuilderRelationView(borrow(input)), fail_on_row(fail_on) {}

    auto row(size_t position) const
    {
        if (++row_calls == fail_on_row)
            throw std::runtime_error("Injected relation read failure.");
        return ygg::FreeBuilderRelationView::row(position);
    }
};

template<database::RelationViewRange R>
database::GenericJoinPlan plan_for(const R& inputs, std::initializer_list<ColumnIndex> variables, std::initializer_list<ColumnIndex> output)
{
    urd::Store<urd::sequence<urd::Store<urd::sequence<ygg::ColumnLayout>>>> schemas;
    schemas.reserve(inputs.size());
    for (const auto& input : inputs)
        schemas.emplace_back(input.columns().begin(), input.columns().end());
    return database::GenericJoinPlan(schemas, { variables.begin(), variables.size() }, { output.begin(), output.size() });
}

template<database::RelationViewConcept A, database::RelationViewConcept E>
void expect_same(const A& actual, const E& expected)
{
    ASSERT_TRUE(std::ranges::equal(actual.columns(), expected.columns()));
    ASSERT_EQ(actual.size(), expected.size());
    for (size_t row = 0; row < expected.size(); ++row)
        EXPECT_TRUE(actual.contains(expected.row(row)));
}

template<database::RelationViewRange R>
urd::builder<ygg::Relation<ygg::Schema>> binary_reference(const R& inputs, const database::GenericJoinPlan& plan)
{
    urd::builder<ygg::Relation<ygg::Schema>> result;
    result.insert(std::array<ygg::Value, 0> {});
    for (const auto input : inputs)
        result = tyr::datalog::tests::ops::joined(borrow(result), input);
    urd::Store<urd::sequence<ColumnIndex>> output;
    for (const auto column : plan.output_columns())
        output.push_back(column.label);
    return tyr::datalog::tests::ops::projected(borrow(result), std::span<const ColumnIndex>(output));
}

void apply(RelationBuilder& input, const Change& change, bool undo = false)
{
    const auto [change_added, change_removed] = change.change();
    const auto& removed = undo ? change_added : change_removed;
    const auto& added = undo ? change_removed : change_added;
    for (size_t i = 0; i < removed.size(); ++i)
    {
        const auto position = input.find(removed.row(i).get_values());
        ASSERT_TRUE(position);
        input.erase(position.value());
    }
    for (size_t i = 0; i < added.size(); ++i)
        input.insert(added.row(i).get_values());
}

void expect_update(const database::incremental::GenericJoinEvaluator& evaluator, const RelationBuilder& before, const RelationBuilder& after)
{
    expect_same(evaluator.get_result(), borrow(after));
    const auto added = tyr::datalog::tests::ops::subtracted(borrow(after), borrow(before));
    const auto removed = tyr::datalog::tests::ops::subtracted(borrow(before), borrow(after));
    const auto [delta_added, delta_removed] = evaluator.get_delta().change();
    expect_same(delta_added, borrow(added));
    expect_same(delta_removed, borrow(removed));
}
}  // namespace

TEST(YggdrasilTests, DatabaseGenericJoinTriangleMatchesBinaryForEveryVariableOrder)
{
    RelationBuilder r { tyr::datalog::tests::ops::cols({ ColumnIndex(1), ColumnIndex(2) }) };
    RelationBuilder s { tyr::datalog::tests::ops::cols({ ColumnIndex(2), ColumnIndex(3) }) };
    RelationBuilder t { tyr::datalog::tests::ops::cols({ ColumnIndex(3), ColumnIndex(1) }) };
    for (ygg::uint_t i = 0; i < 12; ++i)
    {
        r.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(0)), ygg::encode_value(i) });
        r.insert(std::array { ygg::encode_value(i), ygg::encode_value(static_cast<ygg::uint_t>(0)) });
        s.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(0)), ygg::encode_value(i) });
        s.insert(std::array { ygg::encode_value(i), ygg::encode_value(static_cast<ygg::uint_t>(0)) });
        t.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(0)), ygg::encode_value(i) });
        t.insert(std::array { ygg::encode_value(i), ygg::encode_value(static_cast<ygg::uint_t>(0)) });
    }
    const std::array inputs { borrow(r), borrow(s), borrow(t) };
    std::array variables { ColumnIndex(1), ColumnIndex(2), ColumnIndex(3) };
    const std::array output { ColumnIndex(3), ColumnIndex(1), ColumnIndex(2) };
    urd::Store<urd::sequence<urd::Store<urd::sequence<ygg::ColumnLayout>>>> schemas;
    schemas.reserve(inputs.size());
    for (const auto input : inputs)
        schemas.emplace_back(input.columns().begin(), input.columns().end());
    for (bool more = true; more; more = std::ranges::next_permutation(variables).found)
    {
        const database::GenericJoinPlan plan(schemas, variables, output);
        database::GenericJoinWorkspace workspace(plan);
        RelationBuilder result(plan.output_columns());
        const auto expected = binary_reference(inputs, plan);
        database::generic_join(inputs, plan, result, workspace);
        expect_same(borrow(result), borrow(expected));
        EXPECT_EQ(result.size(), 34);
        database::generic_join(inputs, plan, result, workspace);
        expect_same(borrow(result), borrow(expected));
        EXPECT_GT(workspace.memory_usage(), 0);
    }
}

TEST(YggdrasilTests, DatabaseGenericJoinAvoidsQuadraticTriangleIntermediate)
{
    for (const ygg::uint_t count : { static_cast<ygg::uint_t>(8), static_cast<ygg::uint_t>(32), static_cast<ygg::uint_t>(128) })
    {
        SCOPED_TRACE(count);
        RelationBuilder r { tyr::datalog::tests::ops::cols({ ColumnIndex(1), ColumnIndex(2) }) };
        RelationBuilder s { tyr::datalog::tests::ops::cols({ ColumnIndex(2), ColumnIndex(3) }) };
        RelationBuilder t { tyr::datalog::tests::ops::cols({ ColumnIndex(1), ColumnIndex(3) }) };
        for (ygg::uint_t i = 0; i < count; ++i)
        {
            r.insert(std::array { ygg::encode_value(i), ygg::encode_value(static_cast<ygg::uint_t>(0)) });
            s.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(0)), ygg::encode_value(i) });
            t.insert(std::array { ygg::encode_value(i), ygg::encode_value(i) });
        }
        const auto intermediate = tyr::datalog::tests::ops::joined(borrow(r), borrow(s));
        ASSERT_EQ(intermediate.size(), count * count);
        const auto expected = tyr::datalog::tests::ops::joined(borrow(intermediate), borrow(t));
        ASSERT_EQ(expected.size(), count);

        const std::array inputs { borrow(r), borrow(s), borrow(t) };
        urd::Store<urd::sequence<urd::Store<urd::sequence<ygg::ColumnLayout>>>> schemas;
        schemas.reserve(inputs.size());
        for (const auto input : inputs)
            schemas.emplace_back(input.columns().begin(), input.columns().end());
        std::array variables { ColumnIndex(1), ColumnIndex(2), ColumnIndex(3) };
        const auto output = variables;
        for (bool more = true; more; more = std::ranges::next_permutation(variables).found)
        {
            const database::GenericJoinPlan plan(schemas, variables, output);
            database::GenericJoinWorkspace workspace(plan);
            RelationBuilder result(plan.output_columns());
            database::detail::generic_join_work() = {};
            database::generic_join(inputs, plan, result, workspace);
            const auto work = database::detail::generic_join_work();
            expect_same(borrow(result), borrow(expected));
            // This family has linear extension intersections in every order.
            // Counts cover traversal only; building the three tries reads 3n rows.
            EXPECT_GE(work.candidate_keys, 2 * count);
            EXPECT_LE(work.candidate_keys, 3 * count);
            EXPECT_LE(work.probes, 3 * count);
            EXPECT_LE(work.prefixes, 3 * count + 1);
        }
    }
}

TEST(YggdrasilTests, DatabaseGenericJoinHandlesDisconnectedAndNullaryInputs)
{
    RelationBuilder a { tyr::datalog::tests::ops::cols({ ColumnIndex(1) }) };
    RelationBuilder b { tyr::datalog::tests::ops::cols({ ColumnIndex(2) }) };
    RelationBuilder truth;
    a.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(1)) });
    a.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(2)) });
    b.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(3)) });
    b.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(4)) });
    truth.insert(std::array<ygg::Value, 0> {});
    const std::array inputs { borrow(a), borrow(b), borrow(truth) };
    const auto plan = plan_for(inputs, { ColumnIndex(2), ColumnIndex(1) }, { ColumnIndex(1), ColumnIndex(2) });
    database::GenericJoinWorkspace workspace(plan);
    RelationBuilder result(plan.output_columns());
    database::generic_join(inputs, plan, result, workspace);
    EXPECT_EQ(result.size(), 4);
    truth.clear();
    database::generic_join(inputs, plan, result, workspace);
    EXPECT_TRUE(result.empty());

    const auto empty_plan = plan_for(std::array<ygg::FreeBuilderRelationView, 0> {}, {}, {});
    database::GenericJoinWorkspace empty_workspace(empty_plan);
    RelationBuilder empty_result;
    database::generic_join(std::array<ygg::FreeBuilderRelationView, 0> {}, empty_plan, empty_result, empty_workspace);
    EXPECT_EQ(empty_result.size(), 1);
    EXPECT_TRUE(empty_result.contains(std::array<ygg::Value, 0> {}));
    const std::array nullary_inputs { borrow(truth), borrow(truth) };
    const auto nullary_plan = plan_for(nullary_inputs, {}, {});
    database::incremental::GenericJoinEvaluator evaluator(layer(), nullary_plan);
    evaluator.initialize(nullary_inputs);
    Change change(layer(), {});
    change.added.insert(std::array<ygg::Value, 0> {});
    const std::array changes { std::pair { borrow(change.added), borrow(change.removed) }, std::pair { borrow(change.added), borrow(change.removed) } };
    evaluator.update(changes);
    EXPECT_EQ(evaluator.get_delta().added.size(), 1);
    EXPECT_EQ(evaluator.get_result().size(), 1);
    const std::array undo { std::pair { borrow(change.removed), borrow(change.added) }, std::pair { borrow(change.removed), borrow(change.added) } };
    evaluator.update(undo);
    EXPECT_TRUE(evaluator.get_result().empty());
    EXPECT_EQ(evaluator.get_delta().removed.size(), 1);
}

TEST(YggdrasilTests, DatabaseGenericJoinUsesCanonicalTypedBytesWithoutValueComparators)
{
    const std::vector<ygg::ColumnLayout> left_schema { { ColumnIndex(1), ygg::type_id<std::uint64_t> }, { ColumnIndex(2), ygg::type_id<double> } };
    const std::vector<ygg::ColumnLayout> right_schema { { ColumnIndex(2), ygg::type_id<double> }, { ColumnIndex(3), ygg::type_id<bool> } };
    urd::builder<ygg::Relation<ygg::Schema>> left(left_schema);
    urd::builder<ygg::Relation<ygg::Schema>> right(right_schema);
    const auto nan = std::bit_cast<double>(std::uint64_t { 0x7ff8000000000001 });
    const auto other_nan = std::bit_cast<double>(std::uint64_t { 0x7ff8000000000002 });
    left.insert(std::array { ygg::encode_value(std::uint64_t { 4 }), ygg::encode_value(-0.0) });
    left.insert(std::array { ygg::encode_value(std::uint64_t { 5 }), ygg::encode_value(nan) });
    right.insert(std::array { ygg::encode_value(0.0), ygg::encode_value(false) });
    right.insert(std::array { ygg::encode_value(other_nan), ygg::encode_value(true) });
    const std::array inputs { borrow(left), borrow(right) };
    const auto plan = plan_for(inputs, { ColumnIndex(2), ColumnIndex(3), ColumnIndex(1) }, { ColumnIndex(3), ColumnIndex(1), ColumnIndex(2) });
    database::GenericJoinWorkspace workspace(plan);
    urd::builder<ygg::Relation<ygg::Schema>> result(plan.output_columns());
    database::generic_join(inputs, plan, result, workspace);
    const auto expected = binary_reference(inputs, plan);
    expect_same(borrow(result), borrow(expected));
    ASSERT_EQ(result.size(), 2);
    EXPECT_TRUE(result.contains(std::array { ygg::encode_value(false), ygg::encode_value(std::uint64_t { 4 }), ygg::encode_value(0.0) }));
    database::incremental::GenericJoinEvaluator evaluator(layer(), plan);
    evaluator.initialize(inputs);
    database::incremental::Delta left_change(layer(), left_schema);
    database::incremental::Delta right_change(layer(), right_schema);
    left_change.removed.insert(std::array { ygg::encode_value(std::uint64_t { 4 }), ygg::encode_value(0.0) });
    left_change.added.insert(std::array { ygg::encode_value(std::uint64_t { 6 }), ygg::encode_value(-0.0) });
    right_change.removed.insert(std::array { ygg::encode_value(nan), ygg::encode_value(true) });
    const std::array changes { std::pair { borrow(left_change.added), borrow(left_change.removed) },
                               std::pair { borrow(right_change.added), borrow(right_change.removed) } };
    evaluator.update(changes);
    ASSERT_EQ(evaluator.get_result().size(), 1);
    EXPECT_TRUE(evaluator.get_result().contains(std::array { ygg::encode_value(false), ygg::encode_value(std::uint64_t { 6 }), ygg::encode_value(0.0) }));
    const std::array undo { std::pair { borrow(left_change.removed), borrow(left_change.added) },
                            std::pair { borrow(right_change.removed), borrow(right_change.added) } };
    evaluator.update(undo);
    expect_same(evaluator.get_result(), borrow(expected));
}

TEST(YggdrasilTests, DatabaseGenericJoinIncrementalConsolidatesMixedAliasChangesAndUndo)
{
    ygg::FreeRelationRepository repository;
    RelationBuilder edge { tyr::datalog::tests::ops::cols({ ColumnIndex(1), ColumnIndex(2) }) };
    edge.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(1)), ygg::encode_value(static_cast<ygg::uint_t>(2)) });
    edge.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(2)), ygg::encode_value(static_cast<ygg::uint_t>(3)) });
    edge.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(3)), ygg::encode_value(static_cast<ygg::uint_t>(1)) });
    const auto first = tyr::datalog::tests::ops::intern(repository, edge).first;
    const auto second = tyr::datalog::tests::ops::rename(repository, first, tyr::datalog::tests::ops::cols({ ColumnIndex(2), ColumnIndex(3) }));
    ASSERT_TRUE(std::ranges::equal(first.data().rows, second.data().rows));  // a rename keeps the rows
    const std::array inputs { first, second, first };
    const auto plan = plan_for(inputs, { ColumnIndex(2), ColumnIndex(1), ColumnIndex(3) }, { ColumnIndex(1), ColumnIndex(2), ColumnIndex(3) });
    database::incremental::GenericJoinEvaluator evaluator(layer(), plan);
    evaluator.initialize(inputs);
    const auto before = binary_reference(inputs, plan);
    Change change(layer(), edge.columns());
    change.removed.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(1)), ygg::encode_value(static_cast<ygg::uint_t>(2)) });
    change.added.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(4)), ygg::encode_value(static_cast<ygg::uint_t>(2)) });
    change.added.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(2)), ygg::encode_value(static_cast<ygg::uint_t>(4)) });
    Change renamed_change(layer(), second.columns());
    for (size_t i = 0; i < change.added.size(); ++i)
        renamed_change.added.insert(borrow(change.added).row(i).get_values());
    for (size_t i = 0; i < change.removed.size(); ++i)
        renamed_change.removed.insert(borrow(change.removed).row(i).get_values());
    const std::array changes { std::pair { borrow(change.added), borrow(change.removed) },
                               std::pair { borrow(renamed_change.added), borrow(renamed_change.removed) },
                               std::pair { borrow(change.added), borrow(change.removed) } };
    evaluator.update(changes);
    apply(edge, change);
    RelationBuilder renamed(second.columns());
    for (size_t i = 0; i < edge.size(); ++i)
        renamed.insert(borrow(edge).row(i).get_values());
    const std::array updated_inputs { borrow(edge), borrow(renamed), borrow(edge) };
    const auto after = binary_reference(updated_inputs, plan);
    expect_update(evaluator, before, after);
    const std::array undo { std::pair { borrow(change.removed), borrow(change.added) },
                            std::pair { borrow(renamed_change.removed), borrow(renamed_change.added) },
                            std::pair { borrow(change.removed), borrow(change.added) } };
    evaluator.update(undo);
    expect_update(evaluator, after, before);
    EXPECT_GT(evaluator.memory_usage(), 0);
}

TEST(YggdrasilTests, DatabaseGenericJoinIncrementalAvoidsTransientCrossedMatches)
{
    RelationBuilder left { tyr::datalog::tests::ops::cols({ ColumnIndex(1) }) };
    RelationBuilder right { tyr::datalog::tests::ops::cols({ ColumnIndex(1) }) };
    left.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(1)) });
    right.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(2)) });
    const std::array inputs { borrow(left), borrow(right) };
    const auto plan = plan_for(inputs, { ColumnIndex(1) }, { ColumnIndex(1) });
    database::incremental::GenericJoinEvaluator evaluator(layer(), plan);
    evaluator.initialize(inputs);
    Change a(layer(), left.columns());
    Change b(layer(), right.columns());
    a.removed.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(1)) });
    a.added.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(2)) });
    b.removed.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(2)) });
    b.added.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(3)) });
    const std::array changes { std::pair { borrow(a.added), borrow(a.removed) }, std::pair { borrow(b.added), borrow(b.removed) } };
    evaluator.update(changes);
    EXPECT_TRUE(evaluator.get_result().empty());
    EXPECT_TRUE(evaluator.get_delta().added.empty());
    EXPECT_TRUE(evaluator.get_delta().removed.empty());
}

TEST(YggdrasilTests, DatabaseGenericJoinIncrementalProjectionPreservesReplacedWitnesses)
{
    RelationBuilder left { tyr::datalog::tests::ops::cols({ ColumnIndex(1), ColumnIndex(2) }) };
    RelationBuilder right { tyr::datalog::tests::ops::cols({ ColumnIndex(2), ColumnIndex(3) }) };
    left.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(1)), ygg::encode_value(static_cast<ygg::uint_t>(10)) });
    right.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(10)), ygg::encode_value(static_cast<ygg::uint_t>(9)) });
    const std::array inputs { borrow(left), borrow(right) };
    const auto plan = plan_for(inputs, { ColumnIndex(2), ColumnIndex(1), ColumnIndex(3) }, { ColumnIndex(1), ColumnIndex(2), ColumnIndex(3) });
    database::incremental::GenericJoinEvaluator evaluator(layer(), plan);
    evaluator.initialize(inputs);
    database::incremental::ProjectionEvaluator projected(layer(), database::ProjectionPlan(plan.output_columns(), std::array { ColumnIndex(3) }));
    database::Workspace workspace;
    projected.initialize(evaluator.get_result(), workspace);
    Change a(layer(), left.columns());
    Change b(layer(), right.columns());
    a.removed.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(1)), ygg::encode_value(static_cast<ygg::uint_t>(10)) });
    a.added.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(2)), ygg::encode_value(static_cast<ygg::uint_t>(10)) });
    const std::array changes { std::pair { borrow(a.added), borrow(a.removed) }, std::pair { borrow(b.added), borrow(b.removed) } };
    evaluator.update(changes);
    projected.update(evaluator.get_delta().change(), workspace);
    EXPECT_EQ(projected.get_result().size(), 1);
    EXPECT_TRUE(projected.get_delta().added.empty());
    EXPECT_TRUE(projected.get_delta().removed.empty());
}

TEST(YggdrasilTests, DatabaseGenericJoinIncrementalRandomBatchesMatchFullReevaluation)
{
    std::array<RelationBuilder, 3> relations { RelationBuilder(tyr::datalog::tests::ops::cols({ ColumnIndex(1), ColumnIndex(2) })),
                                               RelationBuilder(tyr::datalog::tests::ops::cols({ ColumnIndex(2), ColumnIndex(3) })),
                                               RelationBuilder(tyr::datalog::tests::ops::cols({ ColumnIndex(3), ColumnIndex(1) })) };
    const std::array inputs { borrow(relations[0]), borrow(relations[1]), borrow(relations[2]) };
    const auto plan = plan_for(inputs, { ColumnIndex(2), ColumnIndex(3), ColumnIndex(1) }, { ColumnIndex(3), ColumnIndex(2), ColumnIndex(1) });
    database::incremental::GenericJoinEvaluator evaluator(layer(), plan);
    evaluator.initialize(inputs);
    std::mt19937 random(42);
    for (size_t batch = 0; batch < 50; ++batch)
    {
        const auto before = binary_reference(inputs, plan);
        std::array<Change, 3> changes { Change(layer(), relations[0].columns()), Change(layer(), relations[1].columns()), Change(layer(), relations[2].columns()) };
        for (size_t input = 0; input < relations.size(); ++input)
            for (ygg::uint_t a = 0; a < 4; ++a)
                for (ygg::uint_t b = 0; b < 4; ++b)
                    if (random() % 5 == 0)
                    {
                        const auto row = std::array { ygg::encode_value(a), ygg::encode_value(b) };
                        if (relations[input].contains(row))
                            changes[input].removed.insert(row);
                        else
                            changes[input].added.insert(row);
                    }
        const std::array refs { std::pair { borrow(changes[0].added), borrow(changes[0].removed) },
                                std::pair { borrow(changes[1].added), borrow(changes[1].removed) },
                                std::pair { borrow(changes[2].added), borrow(changes[2].removed) } };
        evaluator.update(refs);
        for (size_t input = 0; input < relations.size(); ++input)
            apply(relations[input], changes[input]);
        const auto after = binary_reference(inputs, plan);
        expect_update(evaluator, before, after);
        if (batch % 3 == 0)
        {
            const std::array undo { std::pair { borrow(changes[0].removed), borrow(changes[0].added) },
                                    std::pair { borrow(changes[1].removed), borrow(changes[1].added) },
                                    std::pair { borrow(changes[2].removed), borrow(changes[2].added) } };
            evaluator.update(undo);
            for (size_t input = 0; input < relations.size(); ++input)
                apply(relations[input], changes[input], true);
            expect_update(evaluator, after, before);
        }
    }
}

TEST(YggdrasilTests, DatabaseGenericJoinValidatesPlansInputsAndFailureRecovery)
{
    RelationBuilder input { tyr::datalog::tests::ops::cols({ ColumnIndex(1) }) };
    input.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(1)) });
    const std::array inputs { borrow(input) };
    EXPECT_THROW(plan_for(inputs, {}, {}), std::invalid_argument);
    EXPECT_THROW(plan_for(inputs, { ColumnIndex(1) }, {}), std::invalid_argument);
    EXPECT_THROW(plan_for(inputs, { ColumnIndex(2) }, { ColumnIndex(1) }), std::invalid_argument);
    RelationBuilder second { tyr::datalog::tests::ops::cols({ ColumnIndex(2) }) };
    const std::array two_inputs { borrow(input), borrow(second) };
    EXPECT_THROW((plan_for(two_inputs, { ColumnIndex(1), ColumnIndex(1) }, { ColumnIndex(1), ColumnIndex(2) })), std::invalid_argument);
    const auto plan = plan_for(inputs, { ColumnIndex(1) }, { ColumnIndex(1) });
    database::GenericJoinWorkspace workspace(plan);
    RelationBuilder output(plan.output_columns());
    output.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(99)) });
    EXPECT_THROW(database::generic_join(inputs, plan, input, workspace), std::invalid_argument);
    EXPECT_EQ(input.size(), 1);
    database::incremental::GenericJoinEvaluator evaluator(layer(), plan);
    Change delta(layer(), input.columns());
    const std::array change { std::pair { borrow(delta.added), borrow(delta.removed) } };
    EXPECT_THROW(evaluator.update(change), std::logic_error);
    EXPECT_THROW((void) evaluator.get_result(), std::logic_error);
    EXPECT_THROW((void) evaluator.get_delta(), std::logic_error);
    evaluator.initialize(inputs);
    delta.added.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(2)) });
    delta.removed.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(2)) });
    EXPECT_THROW(evaluator.update(change), std::invalid_argument);
    expect_same(evaluator.get_result(), borrow(input));
    delta.added.clear();
    EXPECT_THROW(evaluator.update(change), std::invalid_argument);
    expect_same(evaluator.get_result(), borrow(input));
    EXPECT_THROW(evaluator.update(change), std::invalid_argument);
    delta.clear();
    EXPECT_NO_THROW(evaluator.update(change));
    expect_same(evaluator.get_result(), borrow(input));
    const std::array aliased { std::pair { evaluator.get_result(), borrow(delta.removed) } };
    EXPECT_THROW(evaluator.update(aliased), std::invalid_argument);
    expect_same(evaluator.get_result(), borrow(input));
    auto moved = std::move(evaluator);
    EXPECT_NO_THROW(moved.update(change));
    expect_same(moved.get_result(), borrow(input));
}

TEST(YggdrasilTests, DatabaseGenericJoinRejectsInvalidBatchesBeforeMutationAndGuardsFailedUpdates)
{
    RelationBuilder input { tyr::datalog::tests::ops::cols({ ColumnIndex(1) }) };
    input.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(1)) });
    const std::array inputs { borrow(input), borrow(input) };
    const auto plan = plan_for(inputs, { ColumnIndex(1) }, { ColumnIndex(1) });
    database::incremental::GenericJoinEvaluator evaluator(layer(), plan);
    evaluator.initialize(inputs);
    Change first(layer(), input.columns()), second(layer(), input.columns());
    first.removed.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(1)) });
    second.removed.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(99)) });
    const std::array invalid { std::pair { borrow(first.added), borrow(first.removed) }, std::pair { borrow(second.added), borrow(second.removed) } };
    EXPECT_THROW(evaluator.update(invalid), std::invalid_argument);
    expect_same(evaluator.get_result(), borrow(input));
    EXPECT_TRUE(evaluator.get_delta().added.empty());
    EXPECT_TRUE(evaluator.get_delta().removed.empty());

    // The late invalid slot must not have removed the first slot's row.
    second.clear();
    EXPECT_NO_THROW(evaluator.update(invalid));
    EXPECT_TRUE(evaluator.get_result().empty());
    expect_same(evaluator.get_delta().change().second, borrow(input));

    const std::array single_input { borrow(input) };
    const auto single_plan = plan_for(single_input, { ColumnIndex(1) }, { ColumnIndex(1) });
    database::incremental::GenericJoinEvaluator failing(layer(), single_plan);
    failing.initialize(single_input);
    Change insertion(layer(), input.columns());
    insertion.added.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(2)) });
    // Validation and the delta trie read successfully; mutation then fails after
    // the new output has already been appended to the internal delta relation.
    const ThrowingRelation unstable(insertion.added, 4);
    const std::array broken { std::pair<ThrowingRelation, ygg::FreeBuilderRelationView> { unstable, borrow(insertion.removed) } };
    EXPECT_THROW(failing.update(broken), std::runtime_error);
    EXPECT_THROW((void) failing.get_result(), std::logic_error);
    EXPECT_THROW((void) failing.get_delta(), std::logic_error);
    EXPECT_THROW(failing.update(broken), std::logic_error);
    failing.initialize(single_input);
    const std::array valid { std::pair { borrow(insertion.added), borrow(insertion.removed) } };
    failing.update(valid);
    EXPECT_EQ(failing.get_result().size(), 2);
    expect_same(failing.get_delta().change().first, borrow(insertion.added));
}
}  // namespace tyr::datalog::tests
