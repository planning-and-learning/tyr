/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "tyr/datalog/lifted/database/semantics/operations.hpp"

#include "query_helpers.hpp"
#include "tyr/datalog/lifted/database/semantics/relation_view.hpp"

#include <array>
#include <cista/serialization.h>
#include <gtest/gtest.h>
#include <initializer_list>
#include <limits>
#include <optional>
#include <random>
#include <ranges>
#include <set>
#include <span>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <utility>
#include <vector>

namespace tyr::datalog::tests
{
using Values = boost::mp11::mp_list<ygg::uint_t>;
using ColumnIndex = urd::index<ygg::Column>;

static_assert(!std::is_copy_constructible_v<Relation>);
static_assert(std::is_move_constructible_v<Relation>);
static_assert(!ygg::RelationViewConcept<Relation, ygg::Schema>);
static_assert(ygg::RelationViewConcept<ygg::FreeRelationBuilderView, ygg::Schema>);

static_assert(!std::same_as<ColumnIndex, ygg::uint_t>);
static_assert(!std::is_convertible_v<ygg::uint_t, ColumnIndex> && !std::is_convertible_v<ColumnIndex, ygg::uint_t>);
static_assert(sizeof(ColumnIndex) == sizeof(ygg::uint_t));
static_assert(std::is_trivially_copyable_v<ColumnIndex>);

static_assert(std::same_as<decltype(std::declval<const Relation&>().columns()), std::span<const urd::data<ygg::ColumnLayout>>>);
static_assert(std::same_as<decltype(std::declval<const database::ProjectionPlan&>().output_columns()), std::span<const urd::data<ygg::ColumnLayout>>>);
static_assert(std::same_as<decltype(std::declval<const database::JoinPlan&>().output_columns()), std::span<const urd::data<ygg::ColumnLayout>>>);

namespace
{

using Layouts = std::vector<urd::data<ygg::ColumnLayout>>;

/// Columns of type T labeled `labels`.
template<ygg::ColumnValueType T = ygg::uint_t>
Layouts columns_of(std::span<const ColumnIndex> labels)
{
    Layouts columns;
    for (const auto label : labels)
        columns.push_back({ label, ygg::type_id<T> });
    return columns;
}

template<ygg::ColumnValueType T = ygg::uint_t>
Layouts columns_of(std::initializer_list<ColumnIndex> labels)
{ return columns_of<T>(std::span<const ColumnIndex>(labels)); }

/// uint_t columns labeled `labels`.
Layouts cols(std::initializer_list<ColumnIndex> labels) { return columns_of(labels); }

// Each operation with a plan built from its inputs' columns, into a new output of the plan's columns.
template<typename V>
Relation projected(const V& input, std::initializer_list<ColumnIndex> labels)
{
    const database::ProjectionPlan plan(input.columns(), std::span<const ColumnIndex>(labels));
    Relation out(plan.output_columns());
    database::Workspace workspace;
    database::project(input, plan, out, workspace);
    return out;
}

template<typename L, typename R>
Relation joined(const L& left_operand, const R& right_operand)
{
    const database::JoinPlan plan(left_operand.columns(), right_operand.columns());
    Relation out(plan.output_columns());
    database::Workspace workspace;
    database::join(left_operand, right_operand, plan, out, workspace);
    return out;
}

template<typename L, typename R>
Relation unioned(const L& left_operand, const R& right_operand)
{
    const database::SetPlan plan(left_operand.columns(), right_operand.columns());
    Relation out(plan.output_columns());
    database::union_(left_operand, right_operand, plan, out);
    return out;
}

template<typename L, typename R>
Relation subtracted(const L& left_operand, const R& right_operand)
{
    const database::SetPlan plan(left_operand.columns(), right_operand.columns());
    Relation out(plan.output_columns());
    database::difference(left_operand, right_operand, plan, out);
    return out;
}

template<typename V, typename Predicate>
Relation selected(const V& input, Predicate predicate)
{
    Relation out(input.columns());
    database::select(input, predicate, out);
    return out;
}

template<typename T = ygg::uint_t, typename... Args>
auto cells(Args... values)
{ return std::array<ygg::Value, sizeof...(Args)> { ygg::encode_value(T(values))... }; }

template<typename T>
auto packed(std::span<const T> values)
{
    urd::Store<urd::sequence<ygg::Value>> result(values.size());
    for (size_t i = 0; i < values.size(); ++i)
        result[i] = ygg::encode_value(values[i]);
    return result;
}

template<typename T>
auto relocated_serialization(T value)
{
    const auto original = cista::serialize(value);
    auto relocated = original;
    EXPECT_NE(relocated.data(), original.data());
    return relocated;
}

template<typename T = ygg::uint_t>
void expect_relation(const Relation& relation,
                     std::initializer_list<ColumnIndex> columns,
                     std::initializer_list<std::initializer_list<std::type_identity_t<T>>> rows)
{
    urd::Store<urd::sequence<ColumnIndex>> labels;
    for (const auto column : relation.columns())
        labels.push_back(column.label);
    EXPECT_EQ(labels, urd::Store<urd::sequence<ColumnIndex>>(columns));
    ASSERT_EQ(relation.size(), rows.size());
    for (const auto row : rows)
        EXPECT_TRUE(relation.contains(packed(std::span<const T>(row.begin(), row.size()))));
}

std::set<urd::Store<urd::sequence<ygg::uint_t>>> reference_join(const Relation& left_builder, const Relation& right_builder)
{
    const auto left_relation = borrow(left_builder);
    const auto right_relation = borrow(right_builder);
    urd::Store<urd::sequence<std::pair<size_t, size_t>>> keys;
    urd::Store<urd::sequence<size_t>> extra;
    for (size_t right = 0; right < right_relation.columns().size(); ++right)
    {
        bool shared = false;
        for (size_t left = 0; left < left_relation.columns().size(); ++left)
        {
            if (left_relation.columns()[left].label == right_relation.columns()[right].label)
            {
                keys.emplace_back(left, right);
                shared = true;
                break;
            }
        }
        if (!shared)
            extra.push_back(right);
    }
    std::set<urd::Store<urd::sequence<ygg::uint_t>>> result;
    for (size_t left = 0; left < left_relation.get_rows().size(); ++left)
    {
        for (size_t right = 0; right < right_relation.get_rows().size(); ++right)
        {
            bool matches = true;
            for (const auto& [l, r] : keys)
                matches = matches
                          && ygg::decode_value<ygg::uint_t>(value_of(left_relation.row(left), l))
                                 == ygg::decode_value<ygg::uint_t>(value_of(right_relation.row(right), r));
            if (!matches)
                continue;
            auto row = urd::Store<urd::sequence<ygg::uint_t>>();
            for (size_t column = 0; column < left_relation.columns().size(); ++column)
                row.push_back(ygg::decode_value<ygg::uint_t>(value_of(left_relation.row(left), column)));
            for (const auto column : extra)
                row.push_back(ygg::decode_value<ygg::uint_t>(value_of(right_relation.row(right), column)));
            result.insert(std::move(row));
        }
    }
    return result;
}

}  // namespace

TEST(YggdrasilTests, DatabaseRelationColumnsAreValidatedAndOwned)
{
    auto labels = cols({ ColumnIndex(4), ColumnIndex(9) });
    Relation relation(labels);
    labels[0].label = ColumnIndex(8);
    EXPECT_EQ(relation.columns()[0].label, ColumnIndex(4));
    EXPECT_EQ(relation.columns().size(), 2);
    EXPECT_TRUE(Relation().columns().empty());
    EXPECT_NE(relation.columns().data(), labels.data());
    const auto duplicates = cols({ ColumnIndex(4), ColumnIndex(4) });
    EXPECT_THROW((Relation(duplicates)), std::invalid_argument);
    const auto* storage = relation.columns().data();
    EXPECT_THROW(relation.initialize(duplicates), std::invalid_argument);
    EXPECT_THROW(relation.rename(duplicates), std::invalid_argument);
    EXPECT_EQ(relation.columns().data(), storage);
    EXPECT_EQ(relation.columns()[1].label, ColumnIndex(9));
    const auto invalid = Layouts { { ColumnIndex(1), static_cast<ygg::TypeId>(std::variant_size_v<ygg::ColumnValue>) } };
    EXPECT_THROW((Relation(invalid)), std::invalid_argument);
}

/// A relation's columns are an interned record of its rows' repository: the same columns, the same record.
TEST(YggdrasilTests, DatabaseRelationInitializationRetainsColumnStorageForSelfSubviews)
{
    Relation relation(cols({ ColumnIndex(1), ColumnIndex(2), ColumnIndex(3), ColumnIndex(4) }));
    const auto* storage = relation.columns().data();
    relation.initialize(relation.columns());
    EXPECT_EQ(relation.columns().data(), storage);
    EXPECT_EQ(relation.columns().size(), 4);
    relation.initialize(relation.columns().first(2));
    EXPECT_EQ(relation.columns()[0].label, ColumnIndex(1));
    EXPECT_EQ(relation.columns()[1].label, ColumnIndex(2));
    EXPECT_EQ(relation.columns().size(), 2);
    relation.initialize({});
    EXPECT_TRUE(relation.columns().empty());
    relation.initialize(cols({ ColumnIndex(5), ColumnIndex(6), ColumnIndex(7), ColumnIndex(8) }));
    EXPECT_EQ(relation.columns()[3].label, ColumnIndex(8));
}

TEST(YggdrasilTests, DatabaseRelationRenamePreservesRowsAndStorage)
{
    Relation relation { cols({ ColumnIndex(1), ColumnIndex(2) }) };
    relation.insert(cells(10, 20));
    relation.insert(cells(30, 40));
    const auto first_row = record_of(borrow(relation).row(0));
    const auto second_row = record_of(borrow(relation).row(1));
    const auto memory = relation.memory_usage();
    {
        const auto labels = cols({ ColumnIndex(4), ColumnIndex(9) });
        relation.rename(labels);
        relation.rename(labels);
    }
    relation.rename(relation.columns());
    expect_relation(relation, { ColumnIndex(4), ColumnIndex(9) }, { { 10, 20 }, { 30, 40 } });
    EXPECT_EQ(record_of(borrow(relation).row(0)), first_row);
    EXPECT_EQ(record_of(borrow(relation).row(1)), second_row);
    EXPECT_EQ(relation.memory_usage(), memory);
    EXPECT_EQ(relation.insert(cells(10, 20)), 0);

    const auto wrong_arity = cols({ ColumnIndex(4) });
    EXPECT_THROW(relation.rename(wrong_arity), std::invalid_argument);
    expect_relation(relation, { ColumnIndex(4), ColumnIndex(9) }, { { 10, 20 }, { 30, 40 } });
    EXPECT_EQ(record_of(borrow(relation).row(0)), first_row);
    EXPECT_EQ(record_of(borrow(relation).row(1)), second_row);

    Relation nullary;
    nullary.rename({});
    EXPECT_TRUE(nullary.empty());
    nullary.insert(cells());
    nullary.rename(nullary.columns());
    expect_relation(nullary, {}, { {} });
}

TEST(YggdrasilTests, DatabaseRelationMaintainsSetAndSchemaInvariants)
{
    Relation relation(cols({ ColumnIndex(4), ColumnIndex(9) }));
    EXPECT_EQ(relation.columns().size(), 2);
    EXPECT_TRUE(relation.empty());
    EXPECT_EQ(relation.insert(cells(1, 2)), 0);
    EXPECT_EQ(relation.insert(cells(1, 2)), 0);
    EXPECT_EQ(relation.insert(cells(3, 4)), 1);
    EXPECT_EQ(relation.size(), 2);
    EXPECT_EQ(ygg::decode_value<ygg::uint_t>(value_of(borrow(relation).row(1), 0)), 3);
    EXPECT_FALSE(relation.contains(cells(1, 4)));
    EXPECT_THROW(relation.insert(cells(1)), std::invalid_argument);
    EXPECT_FALSE(relation.contains(cells(1)));  // not one value per column: in no row
    EXPECT_THROW((Relation(cols({ ColumnIndex(4), ColumnIndex(4) }))), std::invalid_argument);
    const auto view = borrow(relation);
    EXPECT_EQ(view.columns()[1].label, ColumnIndex(9));
    EXPECT_TRUE(view.contains(cells(3, 4)));
    relation.clear();
    expect_relation(relation, { ColumnIndex(4), ColumnIndex(9) }, {});
    EXPECT_EQ(relation.insert(cells(5, 6)), 0);
}

TEST(YggdrasilTests, DatabaseBuilderViewsShareSchemasAndObserveRefills)
{
    ygg::FreeRelationRepository repository;
    Relation relation { cols({ ColumnIndex(1), ColumnIndex(2) }) };
    relation.insert(cells(7, 8));
    const auto view = relation.view();
    const auto copy = view;
    EXPECT_EQ(view.columns().data(), relation.columns().data());
    EXPECT_EQ(copy.columns().data(), relation.columns().data());
    const auto first_row = record_of(view.row(0));
    const auto labels = cols({ ColumnIndex(3), ColumnIndex(4) });
    relation.rename(labels);
    EXPECT_EQ(view.columns()[1].label, ColumnIndex(4));
    EXPECT_EQ(copy.columns()[0].label, ColumnIndex(3));
    EXPECT_EQ(record_of(view.row(0)), first_row);
    relation.clear();
    EXPECT_TRUE(view.get_rows().empty());
    relation.insert(cells(9, 10));
    EXPECT_TRUE(copy.contains(cells(9, 10)));
    expect_relation(projected(view, { ColumnIndex(4), ColumnIndex(3) }), { ColumnIndex(4), ColumnIndex(3) }, { { 10, 9 } });
}

TEST(YggdrasilTests, DatabaseRelationReinitializationRetainsCompatibleStorage)
{
    Relation relation { cols({ ColumnIndex(1), ColumnIndex(2) }) };
    relation.insert(cells(3, 4));
    const auto memory = relation.memory_usage();
    EXPECT_THROW(relation.initialize(cols({ ColumnIndex(5), ColumnIndex(5) })), std::invalid_argument);
    expect_relation(relation, { ColumnIndex(1), ColumnIndex(2) }, { { 3, 4 } });
    relation.initialize(relation.columns());
    EXPECT_EQ(relation.memory_usage(), memory);
    expect_relation(relation, { ColumnIndex(1), ColumnIndex(2) }, {});
    relation.insert(cells(3, 4));
    const auto validated = cols({ ColumnIndex(5), ColumnIndex(6) });
    relation.initialize(validated);
    EXPECT_EQ(relation.memory_usage(), memory);
    expect_relation(relation, { ColumnIndex(5), ColumnIndex(6) }, {});
    relation.initialize(cols({ ColumnIndex(7) }));
    relation.insert(cells(9));
    expect_relation(relation, { ColumnIndex(7) }, { { 9 } });
    relation.initialize({});
    relation.insert(cells());
    expect_relation(relation, {}, { {} });
}

TEST(YggdrasilTests, DatabaseProjectionReordersDeduplicatesAndOwnsResults)
{
    Relation relation { cols({ ColumnIndex(1), ColumnIndex(2), ColumnIndex(3) }) };
    relation.insert(cells(7, 8, 1));
    relation.insert(cells(7, 8, 2));
    relation.insert(cells(9, 8, 3));
    auto result = projected(borrow(relation), { ColumnIndex(2), ColumnIndex(1) });
    expect_relation(result, { ColumnIndex(2), ColumnIndex(1) }, { { 8, 7 }, { 8, 9 } });
    const std::array<ColumnIndex, 2> duplicates { ColumnIndex(1), ColumnIndex(1) };
    EXPECT_THROW((database::ProjectionPlan(relation.columns(), duplicates)), std::invalid_argument);
    EXPECT_THROW((database::ProjectionPlan(relation.columns(), std::array { ColumnIndex(4) })), std::out_of_range);
    const database::ProjectionPlan plan(relation.columns(), std::array { ColumnIndex(2), ColumnIndex(1) });
    Relation output(plan.output_columns());
    output.insert(cells(99, 99));
    database::Workspace workspace;
    database::project(borrow(relation), plan, output, workspace);
    expect_relation(output, { ColumnIndex(2), ColumnIndex(1) }, { { 8, 7 }, { 8, 9 } });
    relation.clear();
    expect_relation(result, { ColumnIndex(2), ColumnIndex(1) }, { { 8, 7 }, { 8, 9 } });
    database::project(borrow(relation), plan, output, workspace);
    expect_relation(output, { ColumnIndex(2), ColumnIndex(1) }, {});
}

TEST(YggdrasilTests, DatabasePlansOwnSchemasAndReuseResolvedPositions)
{
    constexpr auto large = ColumnIndex();
    auto plans = [large]
    {
        auto left = cols({ large, ColumnIndex(1), ColumnIndex(2) });
        auto right = cols({ ColumnIndex(2), ColumnIndex(3), large });
        auto projection = database::ProjectionPlan(left, std::array { ColumnIndex(2), large });
        auto join = database::JoinPlan(left, right);
        std::ranges::fill(left, urd::data<ygg::ColumnLayout> {});
        std::ranges::fill(right, urd::data<ygg::ColumnLayout> {});
        return std::pair(std::move(projection), std::move(join));
    }();
    auto copied = plans;
    auto [projection, joining] = std::move(copied);
    Relation left_operand { cols({ large, ColumnIndex(1), ColumnIndex(2) }) };
    Relation right_operand { cols({ ColumnIndex(2), ColumnIndex(3), large }) };
    Relation projected { cols({ ColumnIndex(2), large }) };
    Relation joined { cols({ large, ColumnIndex(1), ColumnIndex(2), ColumnIndex(3) }) };
    database::Workspace workspace;
    for (ygg::uint_t generation = 0; generation < 3; ++generation)
    {
        left_operand.clear();
        right_operand.clear();
        left_operand.insert(cells(7 + generation, 8, 9));
        left_operand.insert(cells(7 + generation, 10, 9));
        right_operand.insert(cells(9, 11, 7 + generation));
        database::project(borrow(left_operand), projection, projected, workspace);
        expect_relation(projected, { ColumnIndex(2), large }, { { 9, 7 + generation } });
        database::join(borrow(left_operand), borrow(right_operand), joining, joined, workspace);
        expect_relation(joined, { large, ColumnIndex(1), ColumnIndex(2), ColumnIndex(3) }, { { 7 + generation, 8, 9, 11 }, { 7 + generation, 10, 9, 11 } });
    }
    const database::ProjectionPlan guard(left_operand.columns(), std::span<const ColumnIndex>());
    Relation exists;
    database::project(borrow(left_operand), guard, exists, workspace);
    expect_relation(exists, {}, { {} });
    left_operand.clear();
    database::project(borrow(left_operand), guard, exists, workspace);
    expect_relation(exists, {}, {});
}

TEST(YggdrasilTests, DatabasePlansSurviveCistaRelocation)
{
    // The owning objects and original byte buffers are gone before decoding.
    auto projection_bytes = relocated_serialization(
        database::ProjectionPlan(cols({ ColumnIndex(3), ColumnIndex(1), ColumnIndex(2), ColumnIndex(4) }), std::array { ColumnIndex(4), ColumnIndex(3) }));
    auto join_bytes = relocated_serialization(
        database::JoinPlan(cols({ ColumnIndex(3), ColumnIndex(1), ColumnIndex(2) }), cols({ ColumnIndex(2), ColumnIndex(4), ColumnIndex(1) })));
    const auto* projection = cista::deserialize<database::ProjectionPlan>(projection_bytes);
    const auto* joining = cista::deserialize<database::JoinPlan>(join_bytes);
    const auto values = [](auto range)
    {
        urd::Store<urd::sequence<ColumnIndex>> labels;
        for (const auto column : range)
            labels.push_back(column.label);
        return labels;
    };
    const auto offsets = [](auto range)
    {
        urd::Store<urd::sequence<size_t>> result;
        for (const auto column : range)
            result.push_back(column.position);
        return result;
    };
    EXPECT_EQ(values(projection->input_columns()), (urd::Store<urd::sequence<ColumnIndex>> { ColumnIndex(3), ColumnIndex(1), ColumnIndex(2), ColumnIndex(4) }));
    EXPECT_EQ(values(projection->output_columns()), (urd::Store<urd::sequence<ColumnIndex>> { ColumnIndex(4), ColumnIndex(3) }));
    EXPECT_EQ(offsets(projection->positions()), (urd::Store<urd::sequence<size_t>> { 3, 0 }));
    EXPECT_EQ(values(joining->left_columns()), (urd::Store<urd::sequence<ColumnIndex>> { ColumnIndex(3), ColumnIndex(1), ColumnIndex(2) }));
    EXPECT_EQ(values(joining->right_columns()), (urd::Store<urd::sequence<ColumnIndex>> { ColumnIndex(2), ColumnIndex(4), ColumnIndex(1) }));
    EXPECT_EQ(values(joining->output_columns()), (urd::Store<urd::sequence<ColumnIndex>> { ColumnIndex(3), ColumnIndex(1), ColumnIndex(2), ColumnIndex(4) }));
    EXPECT_EQ(offsets(joining->left_keys()), (urd::Store<urd::sequence<size_t>> { 2, 1 }));
    EXPECT_EQ(offsets(joining->right_keys()), (urd::Store<urd::sequence<size_t>> { 0, 2 }));
    EXPECT_EQ(offsets(joining->right_payload()), (urd::Store<urd::sequence<size_t>> { 1 }));

    // Keep the relocated buffers unchanged and alive while using decoded plans.
    Relation left_operand(joining->left_columns());
    Relation right_operand(joining->right_columns());
    left_operand.insert(cells(10, 1, 2));
    left_operand.insert(cells(11, 1, 3));
    right_operand.insert(cells(2, 20, 1));
    right_operand.insert(cells(3, 30, 1));
    right_operand.insert(cells(2, 99, 9));
    Relation joined(joining->output_columns());
    Relation projected(projection->output_columns());
    database::Workspace workspace;
    database::join(borrow(left_operand), borrow(right_operand), *joining, joined, workspace);
    expect_relation(joined, { ColumnIndex(3), ColumnIndex(1), ColumnIndex(2), ColumnIndex(4) }, { { 10, 1, 2, 20 }, { 11, 1, 3, 30 } });
    database::project(borrow(joined), *projection, projected, workspace);
    expect_relation(projected, { ColumnIndex(4), ColumnIndex(3) }, { { 20, 10 }, { 30, 11 } });
}

TEST(YggdrasilTests, DatabaseDefaultPlansRoundTripAndEvaluateNullaryRelations)
{
    auto projection_bytes = relocated_serialization(database::ProjectionPlan());
    auto join_bytes = relocated_serialization(database::JoinPlan());
    const auto* projection = cista::deserialize<database::ProjectionPlan>(projection_bytes);
    const auto* joining = cista::deserialize<database::JoinPlan>(join_bytes);
    EXPECT_TRUE(projection->input_columns().empty());
    EXPECT_TRUE(projection->output_columns().empty());
    EXPECT_TRUE(projection->positions().empty());
    EXPECT_TRUE(joining->left_columns().empty());
    EXPECT_TRUE(joining->right_columns().empty());
    EXPECT_TRUE(joining->output_columns().empty());
    EXPECT_TRUE(joining->left_keys().empty());
    EXPECT_TRUE(joining->right_keys().empty());
    EXPECT_TRUE(joining->right_payload().empty());

    Relation left_operand, right_operand, projected, joined;
    database::Workspace workspace;
    for (const bool left_nonempty : { false, true })
        for (const bool right_nonempty : { false, true })
        {
            left_operand.clear();
            right_operand.clear();
            if (left_nonempty)
                left_operand.insert(cells());
            if (right_nonempty)
                right_operand.insert(cells());
            database::project(borrow(left_operand), *projection, projected, workspace);
            EXPECT_EQ(projected.columns().size(), 0);
            EXPECT_EQ(projected.size(), left_nonempty ? 1 : 0);
            database::join(borrow(left_operand), borrow(right_operand), *joining, joined, workspace);
            EXPECT_EQ(joined.columns().size(), 0);
            EXPECT_EQ(joined.size(), left_nonempty && right_nonempty ? 1 : 0);
        }
}

TEST(YggdrasilTests, DatabasePlansValidateSchemasAndOperationsRejectAliasedOutputs)
{
    Relation left { cols({ ColumnIndex(1), ColumnIndex(2) }) };
    left.insert(cells(7, 8));
    database::Workspace workspace;

    // An output that is an input is rejected before it is cleared.
    const database::ProjectionPlan identity(left.columns(), std::array { ColumnIndex(1), ColumnIndex(2) });
    const database::JoinPlan same(left.columns(), left.columns());
    EXPECT_THROW(database::project(borrow(left), identity, left, workspace), std::invalid_argument);
    EXPECT_THROW(database::join(borrow(left), borrow(left), same, left, workspace), std::invalid_argument);
    EXPECT_THROW(database::semijoin(borrow(left), borrow(left), same, left, workspace), std::invalid_argument);
    EXPECT_THROW(database::antijoin(borrow(left), borrow(left), same, left, workspace), std::invalid_argument);
    expect_relation(left, { ColumnIndex(1), ColumnIndex(2) }, { { 7, 8 } });

    // Plans validate the columns they are built from.
    const auto duplicates = cols({ ColumnIndex(1), ColumnIndex(1) });
    const std::array<ColumnIndex, 2> duplicate_labels { ColumnIndex(1), ColumnIndex(1) };
    EXPECT_THROW((database::ProjectionPlan(duplicates, std::array { ColumnIndex(1) })), std::invalid_argument);
    EXPECT_THROW((database::ProjectionPlan(left.columns(), duplicate_labels)), std::invalid_argument);
    EXPECT_THROW((database::ProjectionPlan(cols({ ColumnIndex(1) }), std::array { ColumnIndex(2) })), std::out_of_range);
    EXPECT_THROW((database::JoinPlan(duplicates, cols({ ColumnIndex(2), ColumnIndex(3) }))), std::invalid_argument);
    EXPECT_THROW((database::JoinPlan(cols({ ColumnIndex(1), ColumnIndex(2) }), duplicates)), std::invalid_argument);
    EXPECT_THROW((database::JoinPlan(cols({ ColumnIndex(1) }), columns_of<double>({ ColumnIndex(1) }))), std::invalid_argument);
    EXPECT_THROW((database::SetPlan(cols({ ColumnIndex(1), ColumnIndex(2) }), cols({ ColumnIndex(2), ColumnIndex(1) }))), std::invalid_argument);
    EXPECT_THROW((database::SetPlan(duplicates, duplicates)), std::invalid_argument);
    EXPECT_THROW((database::SelectEqualPlan(left.columns(), ColumnIndex(1), ColumnIndex(9))), std::out_of_range);
    EXPECT_THROW((database::SelectEqualPlan(Layouts { { ColumnIndex(1), ygg::type_id<ygg::uint_t> }, { ColumnIndex(2), ygg::type_id<double> } },
                                            ColumnIndex(1),
                                            ColumnIndex(2))),
                 std::invalid_argument);
    EXPECT_THROW((database::SelectValuePlan(left.columns(), ColumnIndex(9), ygg::type_id<ygg::uint_t>, 3)), std::out_of_range);
    EXPECT_THROW((database::SelectValuePlan(left.columns(), ColumnIndex(1), ygg::type_id<double>, ygg::encode_value(3.0))), std::invalid_argument);
}

TEST(YggdrasilTests, DatabaseSelectionsSupportColumnsConstantsAndPredicates)
{
    Relation relation { cols({ ColumnIndex(1), ColumnIndex(2) }) };
    relation.insert(cells(1, 1));
    relation.insert(cells(1, 2));
    relation.insert(cells(2, 2));
    const database::SelectEqualPlan equal(relation.columns(), ColumnIndex(1), ColumnIndex(2));
    const database::SelectValuePlan first_one(relation.columns(), ColumnIndex(1), ygg::type_id<ygg::uint_t>, ygg::encode_value(ygg::uint_t { 1 }));
    const database::SelectValuePlan second_two(relation.columns(), ColumnIndex(2), ygg::type_id<ygg::uint_t>, ygg::encode_value(ygg::uint_t { 2 }));
    Relation output { cols({ ColumnIndex(1), ColumnIndex(2) }) };
    output.insert(cells(99, 99));
    database::select_equal_columns(borrow(relation), equal, output);
    expect_relation(output, { ColumnIndex(1), ColumnIndex(2) }, { { 1, 1 }, { 2, 2 } });
    database::select_equal_value(borrow(relation), first_one, output);
    expect_relation(output, { ColumnIndex(1), ColumnIndex(2) }, { { 1, 1 }, { 1, 2 } });
    database::select_equal_value(borrow(relation), second_two, output);
    expect_relation(output, { ColumnIndex(1), ColumnIndex(2) }, { { 1, 2 }, { 2, 2 } });
    const auto less = [](const auto& row) { return ygg::decode_value<ygg::uint_t>(value_of(row, 0)) < ygg::decode_value<ygg::uint_t>(value_of(row, 1)); };
    database::select(borrow(relation), less, output);
    expect_relation(output, { ColumnIndex(1), ColumnIndex(2) }, { { 1, 2 } });
}

TEST(YggdrasilTests, DatabaseJoinPreservesAllMatchingRowsAndLogicalColumnOrder)
{
    Relation left_operand { cols({ ColumnIndex(3), ColumnIndex(1), ColumnIndex(2) }) };
    Relation right_operand { cols({ ColumnIndex(2), ColumnIndex(4), ColumnIndex(1) }) };
    left_operand.insert(cells(10, 1, 2));
    left_operand.insert(cells(11, 1, 2));
    left_operand.insert(cells(12, 1, 3));
    right_operand.insert(cells(2, 20, 1));
    right_operand.insert(cells(2, 21, 1));
    right_operand.insert(cells(2, 22, 9));
    right_operand.insert(cells(3, 23, 1));
    right_operand.insert(cells(8, 24, 8));
    expect_relation(joined(borrow(left_operand), borrow(right_operand)),
                    { ColumnIndex(3), ColumnIndex(1), ColumnIndex(2), ColumnIndex(4) },
                    { { 10, 1, 2, 20 }, { 10, 1, 2, 21 }, { 11, 1, 2, 20 }, { 11, 1, 2, 21 }, { 12, 1, 3, 23 } });
    expect_relation(joined(borrow(right_operand), borrow(left_operand)),
                    { ColumnIndex(2), ColumnIndex(4), ColumnIndex(1), ColumnIndex(3) },
                    { { 2, 20, 1, 10 }, { 2, 20, 1, 11 }, { 2, 21, 1, 10 }, { 2, 21, 1, 11 }, { 3, 23, 1, 12 } });
    Relation output { cols({ ColumnIndex(3), ColumnIndex(1), ColumnIndex(2), ColumnIndex(4) }) };
    output.insert(cells(99, 99, 99, 99));
    const database::JoinPlan plan(left_operand.columns(), right_operand.columns());
    database::Workspace workspace;
    database::join(borrow(left_operand), borrow(right_operand), plan, output, workspace);
    EXPECT_EQ(output.size(), 5);
    right_operand.clear();
    database::join(borrow(left_operand), borrow(right_operand), plan, output, workspace);
    expect_relation(output, { ColumnIndex(3), ColumnIndex(1), ColumnIndex(2), ColumnIndex(4) }, {});
}

TEST(YggdrasilTests, DatabaseJoinHandlesCartesianProductsAndAllSharedColumns)
{
    Relation left_operand { cols({ ColumnIndex(1) }) };
    Relation right_operand { cols({ ColumnIndex(2) }) };
    left_operand.insert(cells(3));
    left_operand.insert(cells(4));
    right_operand.insert(cells(5));
    right_operand.insert(cells(6));
    expect_relation(joined(borrow(left_operand), borrow(right_operand)), { ColumnIndex(1), ColumnIndex(2) }, { { 3, 5 }, { 3, 6 }, { 4, 5 }, { 4, 6 } });
    auto pairs = joined(borrow(left_operand), borrow(right_operand));
    Relation reversed { cols({ ColumnIndex(2), ColumnIndex(1) }) };
    reversed.insert(cells(5, 3));
    reversed.insert(cells(8, 3));
    expect_relation(joined(borrow(pairs), borrow(reversed)), { ColumnIndex(1), ColumnIndex(2) }, { { 3, 5 } });
}

TEST(YggdrasilTests, DatabaseSetOperationsRequireAlignedSignatures)
{
    Relation left_operand { cols({ ColumnIndex(1), ColumnIndex(2) }) };
    Relation right_operand { cols({ ColumnIndex(1), ColumnIndex(2) }) };
    left_operand.insert(cells(1, 2));
    left_operand.insert(cells(3, 4));
    right_operand.insert(cells(3, 4));
    right_operand.insert(cells(5, 6));
    expect_relation(unioned(borrow(left_operand), borrow(right_operand)), { ColumnIndex(1), ColumnIndex(2) }, { { 1, 2 }, { 3, 4 }, { 5, 6 } });
    expect_relation(subtracted(borrow(left_operand), borrow(right_operand)), { ColumnIndex(1), ColumnIndex(2) }, { { 1, 2 } });
    expect_relation(subtracted(borrow(left_operand), borrow(left_operand)), { ColumnIndex(1), ColumnIndex(2) }, {});
    Relation output { cols({ ColumnIndex(1), ColumnIndex(2) }) };
    output.insert(cells(99, 99));
    const database::SetPlan plan(left_operand.columns(), right_operand.columns());
    database::union_(borrow(left_operand), borrow(right_operand), plan, output);
    EXPECT_EQ(output.size(), 3);
    database::difference(borrow(left_operand), borrow(right_operand), plan, output);
    expect_relation(output, { ColumnIndex(1), ColumnIndex(2) }, { { 1, 2 } });
    auto reversed = projected(borrow(right_operand), { ColumnIndex(2), ColumnIndex(1) });
    EXPECT_THROW(unioned(borrow(left_operand), borrow(reversed)), std::invalid_argument);
    EXPECT_THROW(subtracted(borrow(left_operand), borrow(reversed)), std::invalid_argument);
    auto aligned = projected(borrow(reversed), { ColumnIndex(1), ColumnIndex(2) });
    expect_relation(subtracted(borrow(left_operand), borrow(aligned)), { ColumnIndex(1), ColumnIndex(2) }, { { 1, 2 } });
}

TEST(YggdrasilTests, DatabaseNullaryRelationsObeyBooleanLaws)
{
    Relation false_;
    Relation true_;
    EXPECT_EQ(true_.insert(cells()), 0);
    EXPECT_EQ(true_.insert(cells()), 0);
    EXPECT_EQ(true_.size(), 1);
    EXPECT_EQ(true_.columns().size(), 0);
    EXPECT_TRUE(true_.contains(cells()));
    EXPECT_FALSE(false_.contains(cells()));
    expect_relation(joined(borrow(true_), borrow(true_)), {}, { {} });
    expect_relation(joined(borrow(true_), borrow(false_)), {}, {});
    expect_relation(unioned(borrow(true_), borrow(false_)), {}, { {} });
    expect_relation(subtracted(borrow(true_), borrow(false_)), {}, { {} });
    expect_relation(subtracted(borrow(true_), borrow(true_)), {}, {});
    Relation objects { cols({ ColumnIndex(1) }) };
    objects.insert(cells(7));
    expect_relation(joined(borrow(objects), borrow(true_)), { ColumnIndex(1) }, { { 7 } });
    expect_relation(joined(borrow(true_), borrow(objects)), { ColumnIndex(1) }, { { 7 } });
    expect_relation(joined(borrow(false_), borrow(objects)), { ColumnIndex(1) }, {});
    expect_relation(projected(borrow(objects), std::initializer_list<ColumnIndex> {}), {}, { {} });
    const database::ProjectionPlan nothing(objects.columns(), std::span<const ColumnIndex>());
    Relation output;
    database::Workspace workspace;
    database::project(borrow(objects), nothing, output, workspace);
    expect_relation(output, {}, { {} });
    objects.clear();
    database::project(borrow(objects), nothing, output, workspace);
    expect_relation(output, {}, {});
    expect_relation(selected(borrow(true_), [](const auto& row) { return row.get_values().size() == 0; }), {}, { {} });
    expect_relation(selected(borrow(true_), [](const auto&) { return false; }), {}, {});
}

TEST(YggdrasilTests, DatabaseOutputGuardsPreserveExistingResults)
{
    Relation input { cols({ ColumnIndex(1), ColumnIndex(2) }) };
    input.insert(cells(3, 4));
    const auto keep = [](const auto&) { return true; };
    const database::SelectEqualPlan equal(input.columns(), ColumnIndex(1), ColumnIndex(2));
    const database::SelectValuePlan value(input.columns(), ColumnIndex(1), ygg::type_id<ygg::uint_t>, ygg::encode_value(ygg::uint_t { 3 }));
    const database::ProjectionPlan projection(input.columns(), std::array { ColumnIndex(1), ColumnIndex(2) });
    const database::JoinPlan joining(input.columns(), input.columns());
    const database::SetPlan sets(input.columns(), input.columns());
    database::Workspace workspace;
    EXPECT_THROW(database::select(borrow(input), keep, input), std::invalid_argument);
    EXPECT_THROW(database::select_equal_columns(borrow(input), equal, input), std::invalid_argument);
    EXPECT_THROW(database::select_equal_value(borrow(input), value, input), std::invalid_argument);
    EXPECT_THROW(database::project(borrow(input), projection, input, workspace), std::invalid_argument);
    EXPECT_THROW(database::join(borrow(input), borrow(input), joining, input, workspace), std::invalid_argument);
    EXPECT_THROW(database::union_(borrow(input), borrow(input), sets, input), std::invalid_argument);
    EXPECT_THROW(database::difference(borrow(input), borrow(input), sets, input), std::invalid_argument);
    Relation right_only { cols({ ColumnIndex(1), ColumnIndex(2) }) };
    right_only.insert(cells(9, 10));
    EXPECT_THROW(database::join(borrow(input), borrow(right_only), joining, right_only, workspace), std::invalid_argument);
    EXPECT_THROW(database::union_(borrow(input), borrow(right_only), sets, right_only), std::invalid_argument);
    EXPECT_THROW(database::difference(borrow(input), borrow(right_only), sets, right_only), std::invalid_argument);
    expect_relation(right_only, { ColumnIndex(1), ColumnIndex(2) }, { { 9, 10 } });
    ygg::FreeRelationRepository repository;
    const auto alias = input.view();
    EXPECT_THROW(database::select(alias, keep, input), std::invalid_argument);
    expect_relation(input, { ColumnIndex(1), ColumnIndex(2) }, { { 3, 4 } });
}

TEST(YggdrasilTests, DatabaseOperatorsRespectCanonicalEquality)
{
    // Signed zeros are equal values with one canonical encoding.
    const ygg::FreeRelationRepository builders;
    const auto left_columns = columns_of<double>({ ColumnIndex(1), ColumnIndex(2) });
    const auto right_columns = columns_of<double>({ ColumnIndex(1), ColumnIndex(3) });
    auto left_operand = std::optional<Relation>(std::in_place, left_columns);
    auto right_operand = std::optional<Relation>(std::in_place, right_columns);
    left_operand->insert(std::array { ygg::encode_value(0.0), ygg::encode_value(2.0) });
    left_operand->insert(std::array { ygg::encode_value(3.0), ygg::encode_value(4.0) });
    EXPECT_EQ(left_operand->insert(std::array { ygg::encode_value(-0.0), ygg::encode_value(2.0) }), 0);
    right_operand->insert(std::array { ygg::encode_value(-0.0), ygg::encode_value(5.0) });
    right_operand->insert(std::array { ygg::encode_value(6.0), ygg::encode_value(7.0) });
    auto result = joined(borrow(*left_operand), borrow(*right_operand));
    EXPECT_EQ(result.size(), 1);
    EXPECT_TRUE(result.contains(std::array { ygg::encode_value(0.0), ygg::encode_value(2.0), ygg::encode_value(5.0) }));
    const database::JoinPlan plan(left_operand->columns(), right_operand->columns());
    database::Workspace workspace;
    database::join(borrow(*left_operand), borrow(*right_operand), plan, result, workspace);
    EXPECT_EQ(result.size(), 1);
    EXPECT_TRUE(result.contains(std::array { ygg::encode_value(-0.0), ygg::encode_value(2.0), ygg::encode_value(5.0) }));
    const database::SelectValuePlan zero(left_operand->columns(), ColumnIndex(1), ygg::type_id<double>, ygg::encode_value(-0.0));
    Relation selected(left_operand->columns());
    database::select_equal_value(borrow(*left_operand), zero, selected);
    EXPECT_EQ(selected.size(), 1);
    EXPECT_TRUE(selected.contains(std::array { ygg::encode_value(0.0), ygg::encode_value(2.0) }));
    Relation pairs(columns_of<double>({ ColumnIndex(1), ColumnIndex(2) }));
    pairs.insert(std::array { ygg::encode_value(0.0), ygg::encode_value(-0.0) });
    pairs.insert(std::array { ygg::encode_value(2.0), ygg::encode_value(3.0) });
    Relation equal(pairs.columns());
    database::select_equal_columns(borrow(pairs), database::SelectEqualPlan(pairs.columns(), ColumnIndex(1), ColumnIndex(2)), equal);
    EXPECT_EQ(equal.size(), 1);
    EXPECT_TRUE(equal.contains(std::array { ygg::encode_value(0.0), ygg::encode_value(0.0) }));
}

TEST(YggdrasilTests, DatabaseNaturalJoinMatchesSmallReferenceAcrossSchemas)
{
    const ygg::FreeRelationRepository builders;
    using SchemaPair = std::pair<urd::Store<urd::sequence<ColumnIndex>>, urd::Store<urd::sequence<ColumnIndex>>>;
    const std::array<SchemaPair, 6> schemas = { {
        { { ColumnIndex(1), ColumnIndex(2), ColumnIndex(3) }, { ColumnIndex(3), ColumnIndex(4), ColumnIndex(1) } },
        { { ColumnIndex(1), ColumnIndex(2) }, { ColumnIndex(2), ColumnIndex(1) } },
        { { ColumnIndex(1) }, { ColumnIndex(2), ColumnIndex(3) } },
        { {}, { ColumnIndex(1) } },
        { { ColumnIndex(1) }, {} },
        { {}, {} },
    } };
    // NOLINTNEXTLINE(bugprone-random-generator-seed): a fixed seed keeps the test reproducible
    std::mt19937 random(1701);
    database::Workspace workspace;
    for (const auto& [left_columns, right_columns] : schemas)
    {
        const database::JoinPlan join_plan(columns_of(left_columns), columns_of(right_columns));
        const database::ProjectionPlan project_plan(join_plan.output_columns(), left_columns);
        for (size_t trial = 0; trial < 25; ++trial)
        {
            SCOPED_TRACE(trial);
            auto left_operand = std::optional<Relation>(std::in_place, columns_of(left_columns));
            auto right_operand = std::optional<Relation>(std::in_place, columns_of(right_columns));
            for (auto* relation : { &*left_operand, &*right_operand })
            {
                const auto count = random() % 9;
                urd::Store<urd::sequence<ygg::uint_t>> row(relation->columns().size());
                for (size_t i = 0; i < count; ++i)
                {
                    for (auto& value : row)
                        value = random() % 4;
                    relation->insert(packed(std::span<const ygg::uint_t>(row)));
                }
            }
            const auto expected = reference_join(*left_operand, *right_operand);
            Relation result(join_plan.output_columns());
            database::join(borrow(*left_operand), borrow(*right_operand), join_plan, result, workspace);
            ASSERT_EQ(result.size(), expected.size());
            for (const auto& row : expected)
                EXPECT_TRUE(result.contains(packed(std::span<const ygg::uint_t>(row))));
            Relation projected(columns_of(left_columns));
            database::project(borrow(result), project_plan, projected, workspace);
            std::set<urd::Store<urd::sequence<ygg::uint_t>>> expected_projection;
            for (const auto& row : expected)
                expected_projection.emplace(row.begin(), row.begin() + static_cast<std::ptrdiff_t>(left_columns.size()));
            ASSERT_EQ(projected.size(), expected_projection.size());
            for (const auto& row : expected_projection)
                EXPECT_TRUE(projected.contains(packed(std::span<const ygg::uint_t>(row))));
            urd::Store<urd::sequence<ColumnIndex>> expected_columns = left_columns;
            for (const auto column : right_columns)
            {
                bool shared = false;
                for (const auto left_column : left_columns)
                    shared = shared || column == left_column;
                if (!shared)
                    expected_columns.push_back(column);
            }
            urd::Store<urd::sequence<ColumnIndex>> actual_columns;
            for (const auto column : result.columns())
                actual_columns.push_back(column.label);
            EXPECT_EQ(actual_columns, expected_columns);
        }
    }
}

template<typename R>
concept CanInsertTypedRow = requires(Relation& relation, const R& row) { relation.insert(row); };

static_assert(CanInsertTypedRow<std::span<const ygg::Value>>);
static_assert(!CanInsertTypedRow<std::span<const ygg::uint_t>>);

template<typename V, typename R>
concept CanQueryTypedRow = requires(const V& view, const R& row) {
    { view.contains(row) } -> std::same_as<bool>;
};

using TypedRelationRepository = ygg::FreeRelationRepository;
using TypedBuilderView = urd::borrowed_view<ygg::Relation<ygg::Schema>, TypedRelationRepository, ygg::FreeRelationBuilder>;
using TypedDataView = urd::borrowed_view<ygg::Relation<ygg::Schema>, TypedRelationRepository>;
using TypedIndexView = urd::index_view<ygg::Relation<ygg::Schema>, TypedRelationRepository>;
static_assert(CanQueryTypedRow<TypedBuilderView, std::span<const ygg::Value>>);
static_assert(CanQueryTypedRow<TypedDataView, std::span<const ygg::Value>>);
static_assert(CanQueryTypedRow<TypedIndexView, std::span<const ygg::Value>>);
static_assert(!CanQueryTypedRow<TypedBuilderView, std::span<const ygg::uint_t>>);
static_assert(!CanQueryTypedRow<TypedDataView, std::span<const ygg::uint_t>>);
static_assert(!CanQueryTypedRow<TypedIndexView, std::span<const ygg::uint_t>>);

TEST(YggdrasilTests, DatabaseRelationInsertsTypedTuplesAndCanonicalBytesWithoutStaging)
{
    const auto values = std::array { ygg::encode_value(ygg::uint_t { 2 }), ygg::encode_value(ygg::uint_t { 5 }), ygg::encode_value(ygg::uint_t { 7 }) };
    auto relation = Relation(columns_of<ygg::uint_t>({ ColumnIndex(0), ColumnIndex(1), ColumnIndex(2) }));
    EXPECT_EQ(relation.insert(values), 0);
    const auto encoded = borrow(relation).row(0);
    EXPECT_EQ(relation.insert(values_of(encoded)), 0);
    EXPECT_EQ(relation.size(), 1);
    EXPECT_TRUE(relation.contains(values));
    EXPECT_TRUE(relation.contains(values_of(encoded)));
    for (size_t c = 0; c < values.size(); ++c)
        EXPECT_EQ(value_of(borrow(relation).row(0), c), values[c]);
}

TEST(YggdrasilTests, DatabaseRelationViewsQueryTypedRowsWithinTheirOwnRows)
{
    ygg::FreeRelationRepository repository;
    auto relation = Relation(columns_of<ygg::uint_t>({ ColumnIndex(0), ColumnIndex(1), ColumnIndex(2) }));
    const auto values = std::array { ygg::encode_value(ygg::uint_t { 2 }), ygg::encode_value(ygg::uint_t { 5 }), ygg::encode_value(ygg::uint_t { 7 }) };
    relation.insert(values);
    const auto encoded = borrow(relation).row(0);
    const auto indexed = tyr::datalog::tests::ops::intern(repository, relation).first;
    const auto data_view = urd::resolve(repository[indexed.index()], repository);
    const auto builder_view = relation.view();
    auto other = Relation(columns_of<ygg::uint_t>({ ColumnIndex(0), ColumnIndex(1), ColumnIndex(2) }));
    const auto other_row = std::array { ygg::encode_value(ygg::uint_t { 2 }), ygg::encode_value(ygg::uint_t { 5 }), ygg::encode_value(ygg::uint_t { 6 }) };
    other.insert(other_row);
    const auto other_indexed = tyr::datalog::tests::ops::intern(repository, other).first;
    ASSERT_TRUE(other_indexed.contains(other_row));
    const auto missing = std::array { ygg::encode_value(ygg::uint_t { 2 }), ygg::encode_value(ygg::uint_t { 5 }), ygg::encode_value(ygg::uint_t { 4 }) };
    const auto verify = [&](const auto& view)
    {
        EXPECT_TRUE(view.contains(values));
        EXPECT_TRUE(view.contains(values_of(encoded)));
        EXPECT_FALSE(view.contains(other_row));
        EXPECT_FALSE(view.contains(missing));
        EXPECT_FALSE(view.contains(std::array { ygg::encode_value(ygg::uint_t { 2 }), ygg::encode_value(ygg::uint_t { 5 }) }));  // not one value per column
    };
    verify(builder_view);
    verify(data_view);
    verify(indexed);
}

TEST(YggdrasilTests, DatabaseRelationViewsQueryEmptyTypedRows)
{
    ygg::FreeRelationRepository repository;
    auto relation = Relation();
    const auto row = std::array<ygg::Value, 0> {};
    const auto empty = tyr::datalog::tests::ops::intern(repository, relation).first;
    EXPECT_FALSE(relation.view().contains(row));
    EXPECT_FALSE(urd::resolve(repository[empty.index()], repository).contains(row));
    EXPECT_FALSE(empty.contains(row));
    relation.insert(row);
    const auto unit = tyr::datalog::tests::ops::intern(repository, relation).first;
    EXPECT_TRUE(relation.view().contains(row));
    EXPECT_TRUE(urd::resolve(repository[unit.index()], repository).contains(row));
    EXPECT_TRUE(unit.contains(row));
    EXPECT_FALSE(empty.contains(row));
}

TEST(YggdrasilTests, DatabaseRelationRejectedInsertStoresNothing)
{
    auto relation = Relation(columns_of<ygg::uint_t>({ ColumnIndex(0), ColumnIndex(1) }));
    EXPECT_THROW(relation.insert(std::array { ygg::encode_value(ygg::uint_t { 2 }) }), std::invalid_argument);
    EXPECT_THROW(relation.insert(std::array { ygg::encode_value(2.0), ygg::encode_value(5.0) }), std::out_of_range);  // too wide for the columns
    EXPECT_TRUE(relation.empty());
    const auto values = std::array { ygg::encode_value(ygg::uint_t { 2 }), ygg::encode_value(ygg::uint_t { 5 }) };
    EXPECT_EQ(relation.insert(values), 0);
    EXPECT_TRUE(relation.contains(values));
}

}  // namespace tyr::datalog::tests
