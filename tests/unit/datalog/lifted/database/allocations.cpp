/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "query_helpers.hpp"
#include "tyr/datalog/lifted/database/semantics/incremental/join.hpp"
#include "tyr/datalog/lifted/database/semantics/incremental/projection.hpp"
#include "tyr/datalog/lifted/database/semantics/operations.hpp"
#include "tyr/datalog/lifted/database/semantics/relation_view.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdlib>
#include <gtest/gtest.h>
#include <limits>
#include <memory>
#include <new>
#include <ranges>
#include <span>
#include <tuple>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <urd/testing/allocations.hpp>
#include <utility>
#include <vector>

// This executable links urd::count_allocations, which replaces the C++ allocation functions only. That covers the
// default std/GTL allocators and urd's containers, including aligned allocations. It does not intercept direct
// malloc/free calls, including Cista column/plan storage. Counting is restricted to the evaluating thread and excludes
// GTest, setup, warmup, and destruction of the retained evaluation objects.

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

using tyr::datalog::tests::ops::cols;

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

constexpr size_t key_count = 16;
constexpr std::array<std::pair<size_t, size_t>, 12> state_sizes = { {
    { 1025, 128 },
    { 128, 1025 },
    { 257, 257 },
    { 127, 128 },
    { 128, 127 },
    { 0, 128 },
    { 128, 0 },
    { 0, 0 },
    { 1, 7 },
    { 7, 1 },
    { 1024, 1 },
    { 1, 1024 },
} };

size_t frequency(size_t rows, size_t key) { return rows / key_count + static_cast<size_t>(key < rows % key_count); }
size_t distinct_keys(size_t rows) { return rows < key_count ? rows : key_count; }

size_t joined_size(size_t left_size, size_t right_size, size_t keys = key_count)
{
    size_t result = 0;
    for (size_t key = 0; key < keys; ++key)
        result += frequency(left_size, key) * frequency(right_size, key);
    return result;
}

size_t projected_right_size(size_t left_size, size_t right_size)
{
    size_t result = 0;
    for (size_t key = 0; key < key_count; ++key)
        if (frequency(left_size, key) != 0)
            result += frequency(right_size, key);
    return result;
}

void fill(urd::builder<ygg::Relation<ygg::Schema>>& relation, size_t rows, ygg::uint_t offset)
{
    relation.clear();
    for (size_t i = 0; i < rows; ++i)
        relation.insert(cells(static_cast<ygg::uint_t>(i % key_count), offset + static_cast<ygg::uint_t>(i)));
}

struct Evaluation
{
    urd::builder<ygg::Relation<ygg::Schema>> left { cols({ ColumnIndex(1), ColumnIndex(2) }) };
    urd::builder<ygg::Relation<ygg::Schema>> right { cols({ ColumnIndex(1), ColumnIndex(3) }) };
    std::vector<ygg::ColumnLayout> joined_columns = cols({ ColumnIndex(1), ColumnIndex(2), ColumnIndex(3) });
    std::vector<ygg::ColumnLayout> renamed_columns = cols({ ColumnIndex(4), ColumnIndex(5), ColumnIndex(6) });
    std::vector<ygg::ColumnLayout> key_columns = cols({ ColumnIndex(1) });
    database::JoinPlan joining { left.columns(), right.columns() };
    database::ProjectionPlan projecting { joined_columns, std::array { ColumnIndex(3) } };
    database::ProjectionPlan projecting_renamed { renamed_columns, std::array { ColumnIndex(6) } };
    database::ProjectionPlan projecting_left_keys { left.columns(), std::array { ColumnIndex(1) } };
    database::ProjectionPlan projecting_right_keys { right.columns(), std::array { ColumnIndex(1) } };
    database::ProjectionPlan testing_existence { joined_columns, std::array<ColumnIndex, 0> {} };
    database::SetPlan keys { key_columns, key_columns };
    urd::builder<ygg::Relation<ygg::Schema>> joined { joining.output_columns() };
    urd::builder<ygg::Relation<ygg::Schema>> selected { joining.output_columns() };
    urd::builder<ygg::Relation<ygg::Schema>> projected { projecting.output_columns() };
    urd::builder<ygg::Relation<ygg::Schema>> renamed_projection { projecting_renamed.output_columns() };
    urd::builder<ygg::Relation<ygg::Schema>> left_keys { key_columns };
    urd::builder<ygg::Relation<ygg::Schema>> right_keys { key_columns };
    urd::builder<ygg::Relation<ygg::Schema>> either_keys { key_columns };
    urd::builder<ygg::Relation<ygg::Schema>> only_left_keys { key_columns };
    urd::builder<ygg::Relation<ygg::Schema>> exists;
    database::Workspace workspace;

    bool evaluate(size_t left_size, size_t right_size, ygg::uint_t offset)
    {
        bool valid = true;
        fill(left, left_size, offset);
        fill(right, right_size, offset + 10000);
        database::join(borrow(left), borrow(right), joining, joined, workspace);
        database::select(borrow(joined), [](const auto& row) { return ygg::decode_value<ygg::uint_t>(row.value(0)) < key_count / 2; }, selected);
        database::project(borrow(joined), projecting, projected, workspace);
        // Relabel and restore the reusable builder within the measured path.
        const auto* schema_storage = joined.columns().data();
        joined.rename(renamed_columns);
        database::project(borrow(joined), projecting_renamed, renamed_projection, workspace);
        valid &= joined.columns().data() == schema_storage;
        valid &= std::ranges::equal(joined.columns(), renamed_columns);
        joined.rename(joined_columns);
        database::project(borrow(left), projecting_left_keys, left_keys, workspace);
        database::project(borrow(right), projecting_right_keys, right_keys, workspace);
        database::union_(borrow(left_keys), borrow(right_keys), keys, either_keys);
        database::difference(borrow(left_keys), borrow(right_keys), keys, only_left_keys);
        database::project(borrow(joined), testing_existence, exists, workspace);
        const auto left_count = distinct_keys(left_size);
        const auto right_count = distinct_keys(right_size);
        valid &= joined.size() == joined_size(left_size, right_size);
        valid &= selected.size() == joined_size(left_size, right_size, key_count / 2);
        valid &= projected.size() == projected_right_size(left_size, right_size);
        valid &= renamed_projection.size() == projected.size();
        valid &= either_keys.size() == (left_count > right_count ? left_count : right_count);
        valid &= only_left_keys.size() == (left_count > right_count ? left_count - right_count : 0);
        valid &= exists.size() == static_cast<size_t>(left_size != 0 && right_size != 0);
        if (left_size != 0 && right_size != 0)
        {
            valid &= joined.contains(cells(0, offset, offset + 10000));
            valid &= projected.contains(cells(offset + 10000));
            valid &= renamed_projection.contains(cells(offset + 10000));
        }
        return valid;
    }
};

struct CachedJoinEvaluation
{
    ygg::FreeRelationRepository builders;
    std::shared_ptr<urd::builder<ygg::Relation<ygg::Schema>>> build = builders.get_relation_builder<ygg::Schema>(cols({ ColumnIndex(1), ColumnIndex(2) }));
    urd::builder<ygg::Relation<ygg::Schema>> probe { cols({ ColumnIndex(1), ColumnIndex(3) }) };
    database::JoinPlan forward { build->columns(), probe.columns() };
    database::JoinPlan reverse { probe.columns(), build->columns() };
    urd::builder<ygg::Relation<ygg::Schema>> forward_result { forward.output_columns() };
    urd::builder<ygg::Relation<ygg::Schema>> reverse_result { reverse.output_columns() };
    database::JoinIndexCache cache;
    database::Workspace workspace;

    CachedJoinEvaluation() { fill(*build, 128, 1000); }

    bool evaluate(size_t left_size, size_t right_size, ygg::uint_t offset)
    {
        bool valid = true;
        for (const auto probe_size : { left_size, right_size })
        {
            fill(probe, probe_size, offset);
            database::join(borrow(*build), borrow(probe), forward, cache, database::JoinReuse { true, false }, forward_result, workspace);
            database::join(borrow(probe), borrow(*build), reverse, cache, database::JoinReuse { false, true }, reverse_result, workspace);
            valid &= forward_result.size() == joined_size(build->size(), probe_size);
            valid &= reverse_result.size() == forward_result.size();
            valid &= cache.size() == 1;
            if (probe_size != 0)
            {
                valid &= forward_result.contains(cells(0, 1000, offset));
                valid &= reverse_result.contains(cells(0, offset, 1000));
            }
        }
        return valid;
    }
};

struct PooledEvaluation
{
    urd::builder<ygg::Relation<ygg::Schema>> left { cols({ ColumnIndex(1), ColumnIndex(2) }) };
    urd::builder<ygg::Relation<ygg::Schema>> right { cols({ ColumnIndex(1), ColumnIndex(3) }) };
    ygg::FreeRelationRepository builders;
    database::Workspace workspace;
    std::vector<ygg::ColumnLayout> key_columns = cols({ ColumnIndex(1) });
    database::JoinPlan joining { left.columns(), right.columns() };
    database::ProjectionPlan projecting_left_keys { left.columns(), std::array { ColumnIndex(1) } };
    database::ProjectionPlan projecting_right_keys { right.columns(), std::array { ColumnIndex(1) } };
    database::ProjectionPlan testing_existence { joining.output_columns(), std::array<ColumnIndex, 0> {} };
    database::SetPlan keys { key_columns, key_columns };

    bool evaluate(size_t left_size, size_t right_size, ygg::uint_t offset)
    {
        fill(left, left_size, offset);
        fill(right, right_size, offset + 10000);
        auto joined = builders.get_relation_builder<ygg::Schema>(joining.output_columns());
        auto left_keys = builders.get_relation_builder<ygg::Schema>(key_columns);
        auto right_keys = builders.get_relation_builder<ygg::Schema>(key_columns);
        auto either = builders.get_relation_builder<ygg::Schema>(key_columns);
        auto exists = builders.get_relation_builder<ygg::Schema>({});
        database::join(borrow(left), borrow(right), joining, *joined, workspace);
        database::project(borrow(left), projecting_left_keys, *left_keys, workspace);
        database::project(borrow(right), projecting_right_keys, *right_keys, workspace);
        database::union_(borrow(*left_keys), borrow(*right_keys), keys, *either);
        database::project(borrow(*joined), testing_existence, *exists, workspace);
        bool valid = joined->size() == joined_size(left_size, right_size);
        const auto left_count = distinct_keys(left_size);
        const auto right_count = distinct_keys(right_size);
        valid &= either->size() == (left_count > right_count ? left_count : right_count);
        valid &= exists->size() == static_cast<size_t>(left_size != 0 && right_size != 0);
        if (left_size != 0 && right_size != 0)
            valid &= joined->contains(cells(0, offset, offset + 10000));
        // Returning these handles is also inside the measurement. In particular,
        // all three unary intermediates must coexist without new pool entries.
        return valid;
    }
};

struct PreparedPooledEvaluation
{
    // Sparse labels must not be interpreted as positions or dense-map sizes.
    static constexpr ColumnIndex key_column = ColumnIndex(900000001);
    static constexpr ColumnIndex left_column = ColumnIndex(3);
    static constexpr ColumnIndex right_column = ColumnIndex(800000001);
    urd::builder<ygg::Relation<ygg::Schema>> left { cols({ left_column, key_column }) };
    urd::builder<ygg::Relation<ygg::Schema>> right { cols({ key_column, right_column }) };
    database::JoinPlan joining { left.columns(), right.columns() };
    database::ProjectionPlan projecting { joining.output_columns(), std::array { right_column, left_column } };
    database::ProjectionPlan projecting_left_keys { left.columns(), std::array { key_column } };
    database::ProjectionPlan projecting_right_keys { right.columns(), std::array { key_column } };
    database::ProjectionPlan testing_existence { joining.output_columns(), std::array<ColumnIndex, 0> {} };
    database::SetPlan keys { projecting_left_keys.output_columns(), projecting_right_keys.output_columns() };
    std::vector<ygg::ColumnLayout> alternate_join_columns = cols({ ColumnIndex(7), ColumnIndex(8), ColumnIndex(9) });
    ygg::FreeRelationRepository builders;
    database::Workspace workspace;

    bool evaluate(size_t left_size, size_t right_size, ygg::uint_t offset)
    {
        bool valid = true;
        // Alternate matching and nonempty disjoint key sets to detect stale
        // indexes, with changed keys and tuple values on every measured state.
        for (const bool disjoint : { false, true })
        {
            left.clear();
            right.clear();
            for (size_t i = 0; i < left_size; ++i)
                left.insert(cells(offset + static_cast<ygg::uint_t>(i), offset + static_cast<ygg::uint_t>(i % key_count)));
            const auto right_key_offset = offset + static_cast<ygg::uint_t>(disjoint ? key_count : 0);
            for (size_t i = 0; i < right_size; ++i)
                right.insert(cells(right_key_offset + static_cast<ygg::uint_t>(i % key_count), offset + 10000 + static_cast<ygg::uint_t>(i)));

            // Relabel the same pooled relation before each prepared checkout.
            // Schema changes and read-only access stay in the measurement.
            {
                auto relabeled = builders.get_relation_builder<ygg::Schema>(alternate_join_columns);
                valid &= relabeled->empty();
                relabeled->insert(cells(offset, offset + 1, offset + 2));
                const auto borrowed = borrow(*relabeled);
                valid &= borrowed.size() == 1;
                valid &= std::ranges::equal(borrowed.columns(), alternate_join_columns);
            }
            auto joined = builders.get_relation_builder<ygg::Schema>(joining.output_columns());
            valid &= joined->empty();
            valid &= std::ranges::equal(joined->columns(), joining.output_columns());
            auto projected = builders.get_relation_builder<ygg::Schema>(projecting.output_columns());
            auto left_keys = builders.get_relation_builder<ygg::Schema>(projecting_left_keys.output_columns());
            auto right_keys = builders.get_relation_builder<ygg::Schema>(projecting_right_keys.output_columns());
            auto either = builders.get_relation_builder<ygg::Schema>(projecting_left_keys.output_columns());
            auto exists = builders.get_relation_builder<ygg::Schema>(testing_existence.output_columns());
            database::join(borrow(left), borrow(right), joining, *joined, workspace);
            database::project(borrow(*joined), projecting, *projected, workspace);
            database::project(borrow(left), projecting_left_keys, *left_keys, workspace);
            database::project(borrow(right), projecting_right_keys, *right_keys, workspace);
            database::union_(borrow(*left_keys), borrow(*right_keys), keys, *either);
            database::project(borrow(*joined), testing_existence, *exists, workspace);

            const auto expected_size = disjoint ? 0 : joined_size(left_size, right_size);
            const auto left_count = distinct_keys(left_size);
            const auto right_count = distinct_keys(right_size);
            valid &= joined->size() == expected_size;
            valid &= projected->size() == expected_size;
            valid &= left_keys->size() == left_count;
            valid &= right_keys->size() == right_count;
            valid &= either->size() == (disjoint ? left_count + right_count : (left_count > right_count ? left_count : right_count));
            valid &= exists->size() == static_cast<size_t>(expected_size != 0);
            if (expected_size != 0)
            {
                valid &= joined->contains(cells(offset, offset, offset + 10000));
                valid &= projected->contains(cells(offset + 10000, offset));
            }
            // Pool checkout and return both stay inside the measurement.
        }
        return valid;
    }
};

struct InterningEvaluation
{
    ygg::FreeRelationRepository builders;
    ygg::FreeRelationRepository repository;
    ygg::FreeRelationRepository copied_repository;
    urd::data<ygg::Columns> schema_data;
    const std::vector<ygg::ColumnLayout> columns = cols({ ColumnIndex(11), ColumnIndex(12), ColumnIndex(13) });
    const std::vector<ygg::ColumnLayout> renamed_columns = cols({ ColumnIndex(21), ColumnIndex(22), ColumnIndex(23) });

    bool evaluate(bool reverse, ygg::uint_t generation)
    {
        repository.clear();
        copied_repository.clear();
        bool valid = true;
        constexpr size_t count = 512;
        for (size_t i = 0; i < count; ++i)
        {
            // Vary row lengths and registration order across repository clears.
            const auto logical_index = reverse ? count - i - 1 : i;
            const auto arity = 1 + logical_index % 3;
            const auto schema = std::span<const ygg::ColumnLayout>(columns).first(arity);
            const auto renamed_schema = std::span<const ygg::ColumnLayout>(renamed_columns).first(arity);
            schema_data.values.set(schema.begin(), schema.end());
            const auto interned_schema = repository.insert(schema_data).first;
            const auto [duplicate_schema, schema_created] = repository.insert(schema_data);
            valid &= !schema_created && interned_schema.index() == duplicate_schema.index();
            valid &= std::ranges::equal(interned_schema.data().values, schema);
            auto builder = builders.get_relation_builder<ygg::Schema>(schema);
            const auto seed = generation * 8192 + static_cast<ygg::uint_t>(logical_index) * 4;
            const std::array<ygg::uint_t, 3> first_row { seed, seed + 1, seed + 2 };
            const std::array<ygg::uint_t, 3> second_row { seed + 1, seed + 2, seed + 3 };
            builder->insert(std::span<const ygg::Value>(packed(first_row)).first(arity));
            builder->insert(std::span<const ygg::Value>(packed(second_row)).first(arity));
            const auto [original, created] = tyr::datalog::tests::ops::intern(repository, *builder);
            const auto [copied, copied_created] = tyr::datalog::tests::ops::intern(copied_repository, original);
            valid &= copied_created;
            valid &= !tyr::datalog::tests::ops::intern(copied_repository, original).second;
            auto extracted = builders.get_relation_builder<ygg::Schema>(renamed_schema);
            const auto identity = ygg::relation_identity(*extracted);
            const auto* schema_storage = extracted->columns().data();
            database::assign(*extracted, copied);
            database::assign(*extracted, borrow(*extracted));
            database::assign(*extracted, urd::resolve(*extracted, repository));
            valid &= extracted->size() == original.size();
            valid &= ygg::relation_identity(*extracted) == identity && extracted->columns().data() == schema_storage;
            valid &= std::ranges::equal(extracted->columns(), schema);
            valid &= extracted->contains(std::span<const ygg::Value>(packed(first_row)).first(arity));
            builder->clear();
            builder->insert(std::span<const ygg::Value>(packed(second_row)).first(arity));
            builder->insert(std::span<const ygg::Value>(packed(first_row)).first(arity));
            const auto [duplicate, duplicate_created] = tyr::datalog::tests::ops::intern(repository, *builder);
            const auto renamed = tyr::datalog::tests::ops::rename(repository, original, renamed_schema);
            valid &= created && !duplicate_created;
            valid &= original.index() == duplicate.index();
            valid &= original.index() != renamed.index();
            valid &= std::ranges::equal(original.data().rows, renamed.data().rows);
            valid &= original.row(0).get_values().data() == renamed.row(0).get_values().data()
                     && original.row(0).get_values().first_bit() == renamed.row(0).get_values().first_bit();
            valid &= original.size() == 2 && renamed.columns().size() == arity;
            valid &= renamed.contains(std::span<const ygg::Value>(packed(first_row)).first(arity));
        }
        auto nullary = builders.get_relation_builder<ygg::Schema>({});
        const auto false_view = tyr::datalog::tests::ops::intern(repository, *nullary).first;
        nullary->insert(cells());
        const auto true_view = tyr::datalog::tests::ops::intern(repository, *nullary).first;
        valid &= false_view.empty() && true_view.size() == 1;
        valid &= repository.symbols().size<ygg::Relation<ygg::Schema>>() == 2 * count + 2;
        return valid;
    }
};

template<typename EvaluationType>
void expect_no_allocations_after_warmup(EvaluationType& evaluation)
{
    bool valid = true;
    for (const auto& [left_size, right_size] : state_sizes)
        valid &= evaluation.evaluate(left_size, right_size, 1000);
    ASSERT_TRUE(valid);

    const urd::testing::Counted measured;
    ygg::uint_t generation = 1;
    for (size_t repeat = 0; repeat < 4; ++repeat)
        for (const auto& [left_size, right_size] : state_sizes)
            valid &= evaluation.evaluate(left_size, right_size, 1000 + 20000 * generation++);
    const auto counts = urd::testing::counts();

    EXPECT_TRUE(valid);
    EXPECT_EQ(counts.allocations, 0);
    EXPECT_EQ(counts.deallocations, 0);
}
}  // namespace

TEST(YggdrasilTests, DatabaseAllocationTrackingCountsOrdinaryAndAlignedStorage)
{
    const urd::testing::Counted measured;
    // Explicit allocation-function calls avoid new-expression allocation elision.
    auto* scalar = ::operator new(32);
    auto* array = ::operator new[](32);
    auto* aligned = ::operator new(64, std::align_val_t(64));
    auto* aligned_array = ::operator new[](64, std::align_val_t(64));
    ::operator delete(scalar);
    ::operator delete[](array);
    ::operator delete(aligned, std::align_val_t(64));
    ::operator delete[](aligned_array, std::align_val_t(64));
    const auto counts = urd::testing::counts();
    EXPECT_EQ(counts.allocations, 4);
    EXPECT_EQ(counts.deallocations, 4);
}

TEST(YggdrasilTests, UnorderedMultiMapWarmedRebuildsAllocateAndFreeNothing)
{
    urd::Store<urd::map<size_t, size_t>, urd::multi, urd::retains> map;
    map.reserve(1024);
    bool valid = true;
    const urd::testing::Counted measured;
    for (size_t repeat = 0; repeat < 4; ++repeat)
        for (const auto count : { static_cast<size_t>(0), static_cast<size_t>(1), static_cast<size_t>(7), static_cast<size_t>(511), static_cast<size_t>(1024) })
            for (const bool duplicates : { false, true })
            {
                map.clear();
                map.reserve(count);  // Includes reserve(0) on retained storage.
                const auto distinct = duplicates ? std::min(count, static_cast<size_t>(17)) : count;
                const auto offset = repeat * 2000;
                for (size_t i = 0; i < count; ++i)
                    map.insert({ offset + i % distinct, i });
                size_t seen = 0;
                size_t sum = 0;
                for (size_t key = 0; key < distinct; ++key)
                    for (const auto [first, last] = map.equal_range(offset + key); const auto value : std::ranges::subrange(first, last) | std::views::values)
                    {
                        valid &= value % distinct == key;
                        ++seen;
                        sum += value;
                    }
                valid &= seen == count && map.size() == count;
                valid &= sum == (count == 0 ? 0 : count * (count - 1) / 2);
            }
    map.clear();
    const auto counts = urd::testing::counts();
    EXPECT_TRUE(valid);
    EXPECT_EQ(counts.allocations, 0);
    EXPECT_EQ(counts.deallocations, 0);
}

TEST(YggdrasilTests, UnorderedMultiMapWarmedErasureReusesSlotsAcrossNovelKeys)
{
    urd::Store<urd::map<size_t, size_t>, urd::multi, urd::retains> map;
    constexpr size_t count = 1024;
    map.reserve(count);
    for (size_t value = 0; value < count; ++value)
        map.insert({ value, value });
    bool valid = true;
    const auto advance = [&](size_t generation)
    {
        for (size_t i = 0; i < count; ++i)
        {
            const auto old_value = generation * count + i;
            const auto found = map.find(old_value);  // the key's only value
            valid &= found != map.end() && found->second == old_value;
            map.erase(found);
            map.insert({ old_value + count, old_value + count });
        }
        valid &= map.size() == count;
    };
    for (size_t generation = 0; generation < 16; ++generation)
        advance(generation);
    const auto retained = urd::memory_usage(map);

    const urd::testing::Counted measured;
    for (size_t generation = 16; generation < 272; ++generation)
        advance(generation);
    const auto counts = urd::testing::counts();
    EXPECT_TRUE(valid);
    EXPECT_EQ(counts.allocations, 0);
    EXPECT_EQ(counts.deallocations, 0);
    EXPECT_EQ(urd::memory_usage(map), retained);
}

TEST(YggdrasilTests, DatabaseWarmedEvaluationReusesAllStorageAcrossChangingStates)
{
    Evaluation evaluation;
    expect_no_allocations_after_warmup(evaluation);
}

TEST(YggdrasilTests, DatabaseWarmedPooledIntermediatesAllocateAndFreeNothing)
{
    PooledEvaluation evaluation;
    expect_no_allocations_after_warmup(evaluation);
}

TEST(YggdrasilTests, DatabaseWarmedCachedJoinsAllocateAndFreeNothing)
{
    CachedJoinEvaluation evaluation;
    expect_no_allocations_after_warmup(evaluation);
}

TEST(YggdrasilTests, DatabaseWarmedPreparedPooledEvaluationAllocatesAndFreesNothing)
{
    PreparedPooledEvaluation evaluation;
    expect_no_allocations_after_warmup(evaluation);
}

TEST(YggdrasilTests, DatabaseWarmedRelationInterningRetainsStorageAcrossReorderedArities)
{
    InterningEvaluation evaluation;
    bool valid = true;
    for (ygg::uint_t generation = 0; generation < 8; ++generation)
        valid &= evaluation.evaluate(generation % 2, generation);
    ASSERT_TRUE(valid);

    const auto* schema_builder_storage = evaluation.schema_data.values.data();

    const urd::testing::Counted measured;
    for (ygg::uint_t generation = 8; generation < 108; ++generation)
        valid &= evaluation.evaluate(generation % 2, generation);
    const auto counts = urd::testing::counts();
    EXPECT_TRUE(valid);
    EXPECT_EQ(counts.allocations, 0);
    EXPECT_EQ(counts.deallocations, 0);
    EXPECT_EQ(evaluation.schema_data.values.data(), schema_builder_storage);
}

TEST(YggdrasilTests, DatabaseWarmedTypedTupleInsertionAllocatesAndFreesNothing)
{
    auto relation = urd::builder<ygg::Relation<ygg::Schema>>(cols({ ColumnIndex(0), ColumnIndex(1) }));
    const auto row = std::array { ygg::encode_value(ygg::uint_t { 2 }), ygg::encode_value(ygg::uint_t { 5 }) };
    for (size_t i = 0; i < 8; ++i)
    {
        relation.clear();
        ASSERT_EQ(relation.insert(row), 0);
    }
    auto valid = true;
    const urd::testing::Counted measured;
    for (size_t i = 0; i < 1000; ++i)
    {
        relation.clear();
        valid &= relation.insert(row) == 0;
        valid &= relation.insert(row) == 0;
        valid &= relation.contains(row);
        valid &= relation.size() == 1;
    }
    const auto counts = urd::testing::counts();
    EXPECT_TRUE(valid);
    EXPECT_EQ(counts.allocations, 0);
    EXPECT_EQ(counts.deallocations, 0);
}

TEST(YggdrasilTests, DatabaseTypedTupleMembershipAcrossViewsAllocatesAndFreesNothing)
{
    ygg::FreeRelationRepository repository;
    auto relation = urd::builder<ygg::Relation<ygg::Schema>>(cols({ ColumnIndex(0), ColumnIndex(1) }));
    const auto row = std::array { ygg::encode_value(ygg::uint_t { 2 }), ygg::encode_value(ygg::uint_t { 5 }) };
    relation.insert(row);
    const auto indexed = tyr::datalog::tests::ops::intern(repository, relation).first;
    const auto data_view = urd::resolve(indexed.data(), repository);
    const auto builder_view = urd::resolve(relation, repository);
    auto other = urd::builder<ygg::Relation<ygg::Schema>>(cols({ ColumnIndex(0), ColumnIndex(1) }));
    const auto other_values = std::array { ygg::encode_value(ygg::uint_t { 2 }), ygg::encode_value(ygg::uint_t { 6 }) };
    other.insert(other_values);
    const auto other_indexed = tyr::datalog::tests::ops::intern(repository, other).first;
    ASSERT_TRUE(other_indexed.contains(other_values));
    const auto missing = other_values;
    auto valid = true;
    const urd::testing::Counted measured;
    for (size_t i = 0; i < 1000; ++i)
    {
        valid &= relation.contains(row) && builder_view.contains(row) && data_view.contains(row) && indexed.contains(row);
        valid &= !relation.contains(missing) && !builder_view.contains(missing) && !data_view.contains(missing) && !indexed.contains(missing);
    }
    const auto counts = urd::testing::counts();
    EXPECT_TRUE(valid);
    EXPECT_EQ(counts.allocations, 0);
    EXPECT_EQ(counts.deallocations, 0);
}

namespace
{
struct IncrementalEvaluation
{
    static constexpr size_t rows = 128;
    static constexpr size_t changed_rows = 32;
    static constexpr size_t fanout = 8;
    database::incremental::JoinEvaluator joining { layer(),
                                                   database::JoinPlan(cols({ ColumnIndex(1), ColumnIndex(2) }), cols({ ColumnIndex(1), ColumnIndex(3) })) };
    const urd::builder<ygg::Relation<ygg::Schema>> unchanged { cols({ ColumnIndex(1), ColumnIndex(3) }) };
    // Every output has eight join witnesses. Replacing a source row exercises
    // both support counts and mutable result storage without retaining history.
    database::incremental::ProjectionEvaluator projecting { layer(),
                                                            database::ProjectionPlan(cols({ ColumnIndex(1), ColumnIndex(2), ColumnIndex(3) }),
                                                                                     std::array { ColumnIndex(2) }) };
    database::incremental::Delta delta { layer(), cols({ ColumnIndex(1), ColumnIndex(2) }) };
    std::array<ygg::uint_t, rows> values {};
    database::Workspace workspace;

    IncrementalEvaluation()
    {
        urd::builder<ygg::Relation<ygg::Schema>> initial { cols({ ColumnIndex(1), ColumnIndex(2) }) };
        for (size_t i = 0; i < rows; ++i)
        {
            values[i] = static_cast<ygg::uint_t>(i);
            initial.insert(cells(static_cast<ygg::uint_t>(i % key_count), values[i]));
        }
        urd::builder<ygg::Relation<ygg::Schema>> fixed { cols({ ColumnIndex(1), ColumnIndex(3) }) };
        for (ygg::uint_t key = 0; key < key_count; ++key)
            for (ygg::uint_t match = 0; match < fanout; ++match)
                fixed.insert(cells(key, match));
        joining.initialize(borrow(initial), borrow(fixed), workspace);
        projecting.initialize(joining.get_result(), workspace);
    }

    bool evaluate(ygg::uint_t generation)
    {
        auto valid = true;
        const auto replace = [&](size_t begin, ygg::uint_t phase)
        {
            delta.clear();
            for (size_t i = begin; i < begin + changed_rows; ++i)
            {
                const auto key = static_cast<ygg::uint_t>(i % key_count);
                delta.removed.insert(cells(key, values[i]));
                values[i] = static_cast<ygg::uint_t>(i + (phase == 0 ? 0 : (generation * 3 + phase) * rows));
                delta.added.insert(cells(key, values[i]));
            }
            joining.update(delta.change(), std::pair { borrow(unchanged), borrow(unchanged) }, workspace);
            const auto& joined_delta = joining.get_delta();
            projecting.update(joined_delta.change(), workspace);
            valid &= joining.get_result().size() == rows * fanout;
            valid &= projecting.get_result().size() == rows;
            valid &= joined_delta.added.size() == changed_rows * fanout;
            valid &= joined_delta.removed.size() == changed_rows * fanout;
            valid &= projecting.get_delta().added.size() == changed_rows;
            valid &= projecting.get_delta().removed.size() == changed_rows;
            for (size_t i = begin; i < begin + changed_rows; ++i)
                valid &= projecting.get_result().contains(cells(values[i]));
        };
        replace(0, 1);
        replace(changed_rows, 2);
        replace(changed_rows, 0);
        replace(0, 0);
        replace(2 * changed_rows, 3);
        replace(2 * changed_rows, 0);
        for (size_t i = 0; i < rows; ++i)
            valid &= values[i] == i && projecting.get_result().contains(cells(static_cast<ygg::uint_t>(i)));
        return valid;
    }

    [[nodiscard]] size_t memory_usage() const { return joining.memory_usage() + projecting.memory_usage() + delta.memory_usage() + unchanged.memory_usage(); }
};
}  // namespace

TEST(YggdrasilTests, DatabaseWarmedIncrementalForwardUndoAndSiblingUpdatesRetainStorage)
{
    IncrementalEvaluation evaluation;
    auto valid = true;
    for (ygg::uint_t generation = 0; generation < 16; ++generation)
        valid &= evaluation.evaluate(generation);
    ASSERT_TRUE(valid);
    const auto retained_bytes = evaluation.memory_usage();

    const urd::testing::Counted measured;
    // Novel tuples on every cycle detect storage growing with historical values,
    // even though the live input/result cardinalities remain bounded throughout.
    for (ygg::uint_t generation = 16; generation < 272; ++generation)
    {
        valid &= evaluation.evaluate(generation);
        valid &= evaluation.memory_usage() == retained_bytes;
    }
    const auto counts = urd::testing::counts();
    EXPECT_TRUE(valid);
    EXPECT_EQ(counts.allocations, 0);
    EXPECT_EQ(counts.deallocations, 0);
    EXPECT_EQ(evaluation.memory_usage(), retained_bytes);
}

namespace
{
struct ChangingJoinEvaluation
{
    static constexpr size_t rows = 128;
    static constexpr size_t witnesses = 8;
    // Partial key groups force mutable-index bucket updates as well as complete
    // key disappearance; moved last rows must remain reachable after erasure.
    static constexpr size_t changed_rows = 17;

    database::incremental::JoinEvaluator joining { layer(),
                                                   database::JoinPlan(cols({ ColumnIndex(1), ColumnIndex(2) }), cols({ ColumnIndex(1), ColumnIndex(3) })) };
    database::incremental::Delta lhs_delta { layer(), cols({ ColumnIndex(1), ColumnIndex(2) }) };
    database::incremental::Delta rhs_delta { layer(), cols({ ColumnIndex(1), ColumnIndex(3) }) };
    std::array<std::array<ygg::uint_t, 2>, rows> values {};
    database::Workspace workspace;

    ChangingJoinEvaluation()
    {
        urd::builder<ygg::Relation<ygg::Schema>> lhs { cols({ ColumnIndex(1), ColumnIndex(2) }) };
        urd::builder<ygg::Relation<ygg::Schema>> rhs { cols({ ColumnIndex(1), ColumnIndex(3) }) };
        for (size_t i = 0; i < rows; ++i)
        {
            values[i] = { static_cast<ygg::uint_t>(i / witnesses), static_cast<ygg::uint_t>(i) };
            lhs.insert(packed(values[i]));
            rhs.insert(packed(values[i]));
        }
        joining.initialize(borrow(lhs), borrow(rhs), workspace);
    }

    bool evaluate(ygg::uint_t generation)
    {
        auto valid = true;
        const auto replace = [&](size_t begin, ygg::uint_t phase)
        {
            lhs_delta.clear();
            rhs_delta.clear();
            const auto offset = phase == 0 ? ygg::uint_t { 0 } : static_cast<ygg::uint_t>((generation * 3 + phase) * rows);
            for (size_t i = begin; i < begin + changed_rows; ++i)
            {
                lhs_delta.removed.insert(packed(values[i]));
                rhs_delta.removed.insert(packed(values[i]));
                values[i] = { static_cast<ygg::uint_t>(i / witnesses + offset), static_cast<ygg::uint_t>(i + offset) };
                lhs_delta.added.insert(packed(values[i]));
                rhs_delta.added.insert(packed(values[i]));
            }
            // Overlapping changes exercise old/old, old/new, and new/new join
            // matches without double-reporting the tuples affected on both sides.
            joining.update(lhs_delta.change(), rhs_delta.change(), workspace);
            size_t expected = 0;
            for (const auto& lhs : values)
                for (const auto& rhs : values)
                    if (lhs[0] == rhs[0])
                    {
                        ++expected;
                        valid &= joining.get_result().contains(cells(lhs[0], lhs[1], rhs[1]));
                    }
            valid &= joining.get_result().size() == expected;
        };
        replace(0, 1);
        replace(changed_rows, 2);
        replace(changed_rows, 0);
        replace(0, 0);
        replace(2 * changed_rows, 3);
        replace(2 * changed_rows, 0);
        for (size_t i = 0; i < rows; ++i)
            valid &= values[i][0] == i / witnesses && values[i][1] == i;
        return valid;
    }

    [[nodiscard]] size_t memory_usage() const
    { return joining.memory_usage() + lhs_delta.memory_usage() + rhs_delta.memory_usage() + urd::memory_usage(workspace.join_index); }
};
}  // namespace

TEST(YggdrasilTests, DatabaseWarmedChangingJoinReusesStorageAcrossNovelKeysAndCompaction)
{
    ChangingJoinEvaluation evaluation;
    auto valid = true;
    for (ygg::uint_t generation = 0; generation < 16; ++generation)
        valid &= evaluation.evaluate(generation);
    ASSERT_TRUE(valid);
    const auto retained_bytes = evaluation.memory_usage();

    const urd::testing::Counted measured;
    for (ygg::uint_t generation = 16; generation < 272; ++generation)
    {
        valid &= evaluation.evaluate(generation);
        valid &= evaluation.memory_usage() == retained_bytes;
    }
    const auto counts = urd::testing::counts();
    EXPECT_TRUE(valid);
    EXPECT_EQ(counts.allocations, 0);
    EXPECT_EQ(counts.deallocations, 0);
    EXPECT_EQ(evaluation.memory_usage(), retained_bytes);
}

TEST(YggdrasilTests, DatabaseWarmedMixedWidthTuplesAndOperationsRetainStorage)
{
    const std::vector<ygg::ColumnLayout> left_columns { { ColumnIndex(1), ygg::type_id<ygg::uint_t> },
                                                        { ColumnIndex(2), ygg::type_id<double> },
                                                        { ColumnIndex(3), ygg::type_id<bool> } };
    const std::vector<ygg::ColumnLayout> right_columns { { ColumnIndex(2), ygg::type_id<double> }, { ColumnIndex(4), ygg::type_id<ygg::uint_t> } };
    urd::builder<ygg::Relation<ygg::Schema>> left(left_columns), right(right_columns);
    const database::JoinPlan joining(left.columns(), right.columns());
    const database::ProjectionPlan projecting(joining.output_columns(), std::array { ColumnIndex(2), ColumnIndex(3) });
    urd::builder<ygg::Relation<ygg::Schema>> joined(joining.output_columns()), projected(projecting.output_columns());
    database::Workspace workspace;
    const auto cycle = [&](ygg::uint_t generation)
    {
        left.clear();
        right.clear();
        for (ygg::uint_t i = 0; i < 12; ++i)
        {
            left.insert(std::array { ygg::encode_value(generation + i), ygg::encode_value(static_cast<double>(i % 4)), ygg::encode_value(i % 2 != 0) });
            right.insert(std::array { ygg::encode_value(static_cast<double>(i % 4)), ygg::encode_value(generation + i) });
        }
        database::join(borrow(left), borrow(right), joining, joined, workspace);
        database::project(borrow(joined), projecting, projected, workspace);
        return joined.size() == 36 && projected.size() == 4 && projected.contains(std::array { ygg::encode_value(1.0), ygg::encode_value(true) })
               && projected.contains(std::array { ygg::encode_value(2.0), ygg::encode_value(false) });
    };
    for (ygg::uint_t generation = 0; generation < 8; ++generation)
        ASSERT_TRUE(cycle(generation));
    bool valid = true;
    const urd::testing::Counted measured;
    for (ygg::uint_t generation = 0; generation < 100; ++generation)
        valid &= cycle(generation);
    const auto counts = urd::testing::counts();
    EXPECT_TRUE(valid);
    EXPECT_EQ(counts.allocations, 0);
    EXPECT_EQ(counts.deallocations, 0);
}

}  // namespace tyr::datalog::tests
