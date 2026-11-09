/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "query_helpers.hpp"
#include "tyr/datalog/lifted/database/semantics/relation_view.hpp"
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>

#include "tyr/datalog/lifted/database/semantics/operations.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cista/serialization.h>
#include <cmath>
#include <concepts>
#include <cstring>
#include <gtest/gtest.h>
#include <iterator>
#include <limits>
#include <ranges>
#include <span>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <urd/elements/equal_to.hpp>
#include <urd/elements/hash.hpp>
#include <utility>
#include <vector>

namespace tyr::datalog::tests
{
using Values = boost::mp11::mp_list<ygg::uint_t>;
using ColumnIndex = urd::index<ygg::Column>;

namespace
{
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

/// Whether two row views read the same stored row (the same storage), or have the same values.
template<typename L, typename R>
bool same_storage(const L& lhs, const R& rhs)
{ return lhs.get_values().data() == rhs.get_values().data() && lhs.get_values().first_bit() == rhs.get_values().first_bit(); }
template<typename L, typename R>
bool same_values(const L& lhs, const R& rhs) { return lhs.get_values().equal(rhs.get_values()); }

using RelationTag = ygg::Relation<ygg::Schema>;
using RelationBuilder = urd::builder<RelationTag>;
using Repository = ygg::FreeRelationRepository;
using BuilderView = ygg::FreeBuilderRelationView;
using DataView = urd::borrowed_view<RelationTag, Repository>;
using IndexView = urd::index_view<RelationTag, Repository>;
using ColumnsIndex = urd::index<ygg::Columns>;
using RowIndex = urd::index<ygg::Row<ygg::Schema>>;
using BuilderRowView = ygg::RowView<ygg::Schema, RelationBuilder>;
using LayerRowView = ygg::RowView<ygg::Schema, Repository>;
using ColumnsIndexView = urd::index_view<ygg::Columns, Repository>;

struct AliasFreeRelationView
{
    [[nodiscard]] std::span<const ygg::ColumnLayout> columns() const;
    [[nodiscard]] size_t size() const;
    [[nodiscard]] bool empty() const;
    [[nodiscard]] ygg::RelationIdentity relation_identity() const;
    [[nodiscard]] BuilderRowView row(size_t) const;
    [[nodiscard]] bool contains(std::span<const ygg::Value>) const;
    [[nodiscard]] bool contains(BuilderRowView) const;
};

struct MutableRelationView : AliasFreeRelationView
{
    BuilderRowView row(size_t);
};

static_assert(database::RelationViewConcept<AliasFreeRelationView>);
static_assert(database::RelationViewConcept<AliasFreeRelationView&>);
static_assert(database::RelationViewConcept<const AliasFreeRelationView&>);
static_assert(database::RelationViewConcept<AliasFreeRelationView&&>);
static_assert(!database::RelationViewConcept<MutableRelationView&>);
static_assert(!database::RelationViewConcept<int>);
static_assert(std::same_as<IndexView, ygg::FreeRelationView>);
static_assert(database::RelationViewConcept<BuilderView>);
static_assert(!database::RelationViewConcept<RelationBuilder>);
static_assert(database::RelationViewConcept<DataView>);
static_assert(database::RelationViewConcept<IndexView>);

static_assert(std::ranges::random_access_range<decltype(std::declval<const IndexView&>().rows())>);
static_assert(std::ranges::borrowed_range<decltype(std::declval<const BuilderView&>().rows())>);
static_assert(std::ranges::borrowed_range<decltype(std::declval<const IndexView&>().rows())>);
static_assert(std::same_as<Repository::SymbolTypes, boost::mp11::mp_list<ygg::Columns, RelationTag>>);
static_assert(::ygg::formalism::SupportsSymbol<Repository, ygg::Columns>);
static_assert(::ygg::formalism::SupportsSymbol<Repository, RelationTag>);
static_assert(!::ygg::formalism::SupportsSymbol<Repository, ygg::Column>);

static_assert(std::same_as<decltype(std::declval<const BuilderView&>().row(0)), BuilderRowView>);
static_assert(std::same_as<decltype(std::declval<const DataView&>().row(0)), LayerRowView>);
static_assert(std::same_as<decltype(std::declval<const IndexView&>().row(0)), LayerRowView>);
static_assert(std::same_as<decltype(std::declval<const IndexView&>().row(0).get_values()), ygg::RowPool::row_type>);
static_assert(ygg::RowViewConcept<BuilderRowView, ygg::Schema> && ygg::RowViewConcept<LayerRowView, ygg::Schema>);

static_assert(::ygg::formalism::HasRows<Repository, ygg::Schema> && ::ygg::formalism::HasRelations<Repository, ygg::Schema>);
static_assert(std::same_as<decltype(std::declval<const BuilderView&>().data()), const RelationBuilder&>);
static_assert(std::same_as<decltype(std::declval<const DataView&>().data()), const urd::data<RelationTag>&>);
static_assert(std::same_as<decltype(std::declval<const IndexView&>().index()), urd::index<RelationTag>>);
static_assert(std::same_as<decltype(std::declval<const IndexView&>().index()), urd::index<RelationTag>>);
static_assert(std::same_as<decltype(std::declval<const IndexView&>().data()), const urd::data<RelationTag>&>);
static_assert(!std::is_trivially_copyable_v<urd::data<RelationTag>>);
static_assert(!std::is_copy_constructible_v<RelationBuilder>);
static_assert(std::is_copy_constructible_v<IndexView>);
static_assert(std::same_as<decltype(urd::data<RelationTag>::columns), ColumnsIndex>);
static_assert(std::same_as<std::ranges::range_value_t<decltype(urd::data<RelationTag>::rows)>, RowIndex>);
static_assert(!std::is_constructible_v<ColumnsIndex, RowIndex> && !std::is_constructible_v<RowIndex, ColumnsIndex>);
static_assert(!std::is_convertible_v<ygg::uint_t, ColumnsIndex> && !std::is_convertible_v<ygg::uint_t, RowIndex>);
static_assert(!std::is_convertible_v<ColumnsIndex, ygg::uint_t> && !std::is_convertible_v<RowIndex, ygg::uint_t>);
static_assert(sizeof(ColumnsIndex) == sizeof(ygg::uint_t) && sizeof(RowIndex) == sizeof(ygg::uint_t));
static_assert(std::is_trivially_copyable_v<ColumnsIndex> && std::is_trivially_copyable_v<RowIndex>);
static_assert(std::same_as<decltype(std::declval<const ColumnsIndexView&>().index()), ColumnsIndex>);
static_assert(std::same_as<decltype(std::declval<const ColumnsIndexView&>().data()), const urd::data<ygg::Columns>&>);

template<typename T, typename L, typename R>
concept CanJoinRelations = requires(const L& lhs, const R& rhs, const database::JoinPlan& plan, urd::builder<ygg::Relation<ygg::Schema>>& out, database::Workspace& workspace) {
    database::join(lhs, rhs, plan, out, workspace);
};

static_assert(CanJoinRelations<ygg::uint_t, BuilderView, DataView>);
static_assert(CanJoinRelations<ygg::uint_t, IndexView, BuilderView>);
static_assert(!CanJoinRelations<ygg::uint_t, IndexView, RelationBuilder>);

/// The layer the tests read their builders through.
const Repository& layer()
{
    static const Repository repository;
    return repository;
}

ygg::FreeBuilderRelationView borrow(const RelationBuilder& relation) { return layer().view(relation); }

template<typename ViewType>
void expect_rows(const ViewType& view, std::initializer_list<std::initializer_list<ygg::uint_t>> rows)
{
    ASSERT_EQ(view.size(), rows.size());
    for (const auto& row : rows)
        EXPECT_TRUE(view.contains(packed(std::span<const ygg::uint_t>(row.begin(), row.size()))) && view.columns().size() == row.size());
}

/// Interns columns of these labels (each uint_t).
auto insert_columns(Repository& repository, std::initializer_list<ColumnIndex> labels)
{
    auto columns = urd::data<ygg::Columns>(tyr::datalog::tests::ops::cols(labels));
    return repository.insert(columns);
}

/// The columns relabeled.
std::vector<ygg::ColumnLayout> relabeled(std::span<const ygg::ColumnLayout> columns, std::initializer_list<ColumnIndex> labels)
{
    auto result = std::vector<ygg::ColumnLayout>(columns.begin(), columns.end());
    for (size_t i = 0; i < result.size(); ++i)
        result[i].label = labels.begin()[i];
    return result;
}

/// The relations a repository holds.
size_t relation_count(const Repository& repository) { return repository.symbols().template size<RelationTag>(); }
}  // namespace

TEST(YggdrasilTests, DatabaseColumnsInternOrderedSchemasThroughDataAndIndexViews)
{
    Repository repository;
    const auto expected = tyr::datalog::tests::ops::cols({ ColumnIndex(8), ColumnIndex(3) });
    auto record = urd::data<ygg::Columns>(expected);
    const auto [original, created] = repository.insert(record);
    ASSERT_TRUE(created);
    EXPECT_EQ(repository.view(original.index()).index(), original.index());

    urd::data<ygg::Columns> pending;
    pending.values.set(expected.begin(), expected.end());
    const auto index_view = repository.view(original.index());
    EXPECT_TRUE(std::ranges::equal(index_view.data().values, expected));
    EXPECT_EQ(&index_view.data(), &repository[original.index()]);

    pending.index = urd::index<ygg::Columns>(99);
    const auto [duplicate, duplicate_created] = repository.insert(pending);
    EXPECT_FALSE(duplicate_created);
    EXPECT_EQ(pending.index, original.index());
    EXPECT_TRUE(urd::EqualTo<ColumnsIndexView> {}(original, duplicate));
    EXPECT_EQ(urd::Hash<ColumnsIndexView> {}(original), urd::Hash<ColumnsIndexView> {}(duplicate));
    ASSERT_TRUE(repository.find(pending));
    EXPECT_EQ(repository.find(pending).value().index(), original.index());

    const auto reversed = tyr::datalog::tests::ops::cols({ ColumnIndex(3), ColumnIndex(8) });
    auto reversed_record = urd::data<ygg::Columns>(reversed);
    const auto [other_order, order_created] = repository.insert(reversed_record);
    EXPECT_TRUE(order_created);
    EXPECT_NE(other_order.index(), original.index());

    RelationBuilder rows(expected);
    rows.insert(cells(10, 20));
    const auto relation = tyr::datalog::tests::ops::intern(repository, rows).first;
    const database::JoinPlan joining(relation.columns(), relation.columns());
    const database::ProjectionPlan projecting(relation.columns(), std::array { ColumnIndex(3), ColumnIndex(8) });
    auto output = repository.get_relation_builder<ygg::Schema>(relation.columns());
    database::Workspace workspace;
    database::join(relation, relation, joining, *output, workspace);
    expect_rows(*output, { { 10, 20 } });
    output->initialize(reversed);
    database::project(relation, projecting, *output, workspace);
    expect_rows(*output, { { 20, 10 } });
    const auto projected = tyr::datalog::tests::ops::projected(relation, { ColumnIndex(8), ColumnIndex(3) });
    expect_rows(projected, { { 10, 20 } });
    EXPECT_TRUE(std::ranges::equal(original.data().values, expected));
    EXPECT_TRUE(std::ranges::equal(other_order.data().values, reversed));
}

TEST(YggdrasilTests, DatabaseRelationRepositoryInternsRowsWithOrderedSchemas)
{
    Repository repository;
    RelationBuilder builder { tyr::datalog::tests::ops::cols({ ColumnIndex(3), ColumnIndex(8) }) };
    builder.insert(cells(10, 11));
    builder.insert(cells(20, 21));
    const auto [first, created] = tyr::datalog::tests::ops::intern(repository, builder);
    ASSERT_TRUE(created);
    EXPECT_EQ(first.data().index, first.index());

    const auto [duplicate, duplicate_created] = tyr::datalog::tests::ops::intern(repository, builder);
    EXPECT_FALSE(duplicate_created);
    EXPECT_TRUE(urd::EqualTo<IndexView> {}(first, duplicate));
    EXPECT_EQ(urd::Hash<IndexView> {}(first), urd::Hash<IndexView> {}(duplicate));

    const auto reordered_columns = tyr::datalog::tests::ops::cols({ ColumnIndex(8), ColumnIndex(3) });
    builder.rename(reordered_columns);
    const auto [other_schema, schema_created] = tyr::datalog::tests::ops::intern(repository, builder);
    EXPECT_TRUE(schema_created);
    EXPECT_NE(first.index(), other_schema.index());
    EXPECT_TRUE(same_storage(first.row(0), other_schema.row(0)));
    EXPECT_EQ(first.columns()[0].label, ColumnIndex(3));
    EXPECT_EQ(first.columns()[1].label, ColumnIndex(8));

    const auto original_columns = tyr::datalog::tests::ops::cols({ ColumnIndex(3), ColumnIndex(8) });
    builder.initialize(original_columns);
    builder.insert(cells(20, 21));
    builder.insert(cells(10, 11));
    const auto [other_order, order_created] = tyr::datalog::tests::ops::intern(repository, builder);
    EXPECT_FALSE(order_created);
    EXPECT_EQ(first.index(), other_order.index());
    expect_rows(first, { { 10, 11 }, { 20, 21 } });
    expect_rows(other_order, { { 10, 11 }, { 20, 21 } });
    EXPECT_EQ(relation_count(repository), 2);
}

TEST(YggdrasilTests, DatabaseRelationRepositoryDistinguishesNullaryTruthAndEmptySchemas)
{
    Repository repository;
    RelationBuilder nullary;
    const auto false_view = tyr::datalog::tests::ops::intern(repository, nullary).first;
    nullary.insert(cells());
    const auto true_view = tyr::datalog::tests::ops::intern(repository, nullary).first;
    EXPECT_NE(false_view.index(), true_view.index());
    EXPECT_EQ(false_view.columns().size(), 0);
    EXPECT_EQ(true_view.columns().size(), 0);
    EXPECT_TRUE(false_view.empty());
    EXPECT_EQ(true_view.size(), 1);
    EXPECT_TRUE(true_view.contains(cells()));

    RelationBuilder unary { tyr::datalog::tests::ops::cols({ ColumnIndex(5) }) };
    const auto empty_unary = tyr::datalog::tests::ops::intern(repository, unary).first;
    RelationBuilder binary { tyr::datalog::tests::ops::cols({ ColumnIndex(5), ColumnIndex(6) }) };
    const auto empty_binary = tyr::datalog::tests::ops::intern(repository, binary).first;
    EXPECT_NE(empty_unary.index(), false_view.index());
    EXPECT_NE(empty_binary.index(), empty_unary.index());
    EXPECT_EQ(empty_unary.columns().size(), 1);
    EXPECT_EQ(empty_binary.columns().size(), 2);

    // Rows of other column types lie in other pools (schemas): their (empty) row sets are separate storage.
    EXPECT_NE(false_view.relation_identity(), empty_unary.relation_identity());
    database::JoinIndexCache cache;
    const std::array<database::ColumnSlice, 1> key_positions { database::ColumnSlice { 0 } };
    EXPECT_NO_THROW(cache.get_or_create(empty_unary, key_positions));
    EXPECT_EQ(cache.size(), 1);
}

TEST(YggdrasilTests, DatabaseRelationRepositorySharesRowsAcrossOverlappingRelations)
{
    Repository repository;
    RelationBuilder builder { tyr::datalog::tests::ops::cols({ ColumnIndex(1), ColumnIndex(2) }) };
    builder.insert(cells(10, 11));
    builder.insert(cells(20, 21));
    const auto first = tyr::datalog::tests::ops::intern(repository, builder).first;
    builder.clear();
    builder.insert(cells(30, 31));
    builder.insert(cells(20, 21));
    const auto second = tyr::datalog::tests::ops::intern(repository, builder).first;

    EXPECT_NE(first.index(), second.index());
    EXPECT_EQ(repository.relations().get<ygg::Schema>().size(), 1);
    EXPECT_EQ(repository.relations().get<ygg::Schema>().at(first.data().pool).size(), 3);
    EXPECT_EQ(repository.symbols().size<RelationTag>(), 2);
    EXPECT_TRUE(first.contains(cells(20, 21)));
    EXPECT_TRUE(second.contains(cells(20, 21)));
    EXPECT_TRUE(same_storage(first.row(1), second.row(0)));
    EXPECT_FALSE(first.contains(cells(30, 31)));
    EXPECT_FALSE(second.contains(cells(10, 11)));

    RelationBuilder difference_builder { tyr::datalog::tests::ops::cols({ ColumnIndex(1), ColumnIndex(2) }) };
    database::difference(second, first, database::SetPlan(second.columns(), first.columns()), difference_builder);
    expect_rows(difference_builder, { { 30, 31 } });
}

TEST(YggdrasilTests, DatabaseRelationViewsMixBuildersDataAndInternedIndicesInOperations)
{
    Repository repository;
    RelationBuilder left { tyr::datalog::tests::ops::cols({ ColumnIndex(1), ColumnIndex(2) }) };
    left.insert(cells(4, 40));
    left.insert(cells(5, 50));
    RelationBuilder right { tyr::datalog::tests::ops::cols({ ColumnIndex(1), ColumnIndex(3) }) };
    right.insert(cells(4, 400));
    right.insert(cells(6, 600));
    const auto right_view = tyr::datalog::tests::ops::intern(repository, right).first;
    auto right_data = right_view.data();
    const auto left_builder_view = urd::resolve(left, repository);
    const auto right_data_view = urd::resolve(right_data, repository);
    const auto right_index_view = repository.view(right_view.index());
    // The record holds its row ids, so it is not trivially copyable: its view is borrowed.
    static_assert(!std::is_trivially_copyable_v<urd::data<RelationTag>>);
    EXPECT_TRUE(urd::EqualTo<std::remove_cvref_t<decltype(right_data)>> {}(right_data_view.data(), right_data));
    EXPECT_EQ(&left_builder_view.data(), &left);
    EXPECT_EQ(&right_index_view.context(), &repository);
    const auto check_access = [](const auto& view)
    {
        EXPECT_TRUE(view.contains(cells(4, 400)));
        EXPECT_FALSE(view.contains(cells(4, 401)));
        EXPECT_FALSE(view.contains(cells(4)));  // not one value per column
    };
    check_access(right_data_view);
    check_access(right_index_view);

    database::Workspace workspace;
    RelationBuilder joined { tyr::datalog::tests::ops::cols({ ColumnIndex(1), ColumnIndex(2), ColumnIndex(3) }) };
    const database::JoinPlan joining(left.columns(), right.columns());
    database::join(left_builder_view, right_data_view, joining, joined, workspace);
    expect_rows(joined, { { 4, 40, 400 } });
    database::join(left_builder_view, right_index_view, joining, joined, workspace);
    expect_rows(joined, { { 4, 40, 400 } });

    RelationBuilder projected { tyr::datalog::tests::ops::cols({ ColumnIndex(3) }) };
    database::project(right_data_view, database::ProjectionPlan(right.columns(), std::array { ColumnIndex(3) }), projected, workspace);
    expect_rows(projected, { { 400 }, { 600 } });
    RelationBuilder selected { tyr::datalog::tests::ops::cols({ ColumnIndex(1), ColumnIndex(3) }) };
    database::select(right_index_view, [](const auto& row) { return ygg::decode_value<ygg::uint_t>(row.value(0)) == 4; }, selected);
    expect_rows(selected, { { 4, 400 } });
    const database::SetPlan sets(right.columns(), right.columns());
    database::union_(right_data_view, right_index_view, sets, selected);
    expect_rows(selected, { { 4, 400 }, { 6, 600 } });
    database::difference(right_data_view, right_index_view, sets, selected);
    EXPECT_TRUE(selected.empty());

    const auto labels = tyr::datalog::tests::ops::cols({ ColumnIndex(1), ColumnIndex(30) });
    const auto renamed = tyr::datalog::tests::ops::rename(repository, right_index_view, labels);
    EXPECT_NE(renamed.relation_identity(), right_index_view.relation_identity());  // another relation over the same rows
    EXPECT_TRUE(std::ranges::equal(renamed.data().rows, right_index_view.data().rows));
    RelationBuilder renamed_projection { tyr::datalog::tests::ops::cols({ ColumnIndex(30) }) };
    const database::ProjectionPlan to_30(labels, std::array { ColumnIndex(30) });
    database::project(renamed, to_30, renamed_projection, workspace);
    expect_rows(renamed_projection, { { 400 }, { 600 } });

    right.rename(labels);
    const auto borrowed = urd::resolve(right, repository);
    static_assert(database::RelationViewConcept<decltype(borrowed)>);
    EXPECT_EQ(borrowed.relation_identity(), ygg::relation_identity(right));
    database::project(borrowed, to_30, renamed_projection, workspace);
    expect_rows(renamed_projection, { { 400 }, { 600 } });
}

TEST(YggdrasilTests, DatabaseInternedRowsOutliveBuilderReuseAndRenamesShareStorage)
{
    Repository repository;
    const auto pair = tyr::datalog::tests::ops::cols({ ColumnIndex(1), ColumnIndex(2) });
    auto builder = repository.get_relation_builder<ygg::Schema>(pair);
    builder->insert(cells(7, 70));
    builder->insert(cells(8, 80));
    const auto original = tyr::datalog::tests::ops::intern(repository, *builder).first;
    const auto* builder_address = &*builder;
    builder = {};
    builder = repository.get_relation_builder<ygg::Schema>(tyr::datalog::tests::ops::cols({ ColumnIndex(4), ColumnIndex(5) }));
    EXPECT_EQ(&*builder, builder_address);
    builder->insert(cells(99, 100));
    expect_rows(original, { { 7, 70 }, { 8, 80 } });
    EXPECT_FALSE(same_storage(original.row(0), repository.view(*builder).row(0)));

    const auto renamed = [&]()
    {
        const auto temporary_columns = tyr::datalog::tests::ops::cols({ ColumnIndex(4), ColumnIndex(5) });
        return tyr::datalog::tests::ops::rename(repository, original, temporary_columns);
    }();
    const auto renamed_columns = tyr::datalog::tests::ops::cols({ ColumnIndex(4), ColumnIndex(5) });
    EXPECT_NE(original.index(), renamed.index());
    EXPECT_TRUE(same_storage(original.row(0), renamed.row(0)));
    EXPECT_NE(original.relation_identity(), renamed.relation_identity());  // another relation over the same rows
    EXPECT_NE(original.relation_identity(), ygg::relation_identity(*builder));
    EXPECT_TRUE(std::ranges::equal(renamed.columns(), renamed_columns));
    EXPECT_TRUE(urd::EqualTo<IndexView> {}(tyr::datalog::tests::ops::rename(repository, original, renamed_columns), renamed));
    expect_rows(renamed, { { 7, 70 }, { 8, 80 } });

    RelationBuilder probe { tyr::datalog::tests::ops::cols({ ColumnIndex(4), ColumnIndex(6) }) };
    probe.insert(cells(7, 700));
    RelationBuilder joined { tyr::datalog::tests::ops::cols({ ColumnIndex(4), ColumnIndex(5), ColumnIndex(6) }) };
    database::JoinPlan plan(renamed.columns(), probe.columns());
    database::JoinIndexCache cache;
    database::Workspace workspace;
    database::join(renamed, urd::resolve(probe, repository), plan, cache, database::JoinReuse { true, false }, joined, workspace);
    expect_rows(joined, { { 7, 70, 700 } });
    EXPECT_EQ(cache.size(), 1);
}

TEST(YggdrasilTests, DatabaseRelationRepositoryViewsKeepRepositoryIdentity)
{
    Repository first_repository;
    Repository second_repository;
    RelationBuilder builder { tyr::datalog::tests::ops::cols({ ColumnIndex(1) }) };
    builder.insert(cells(12));
    const auto first = tyr::datalog::tests::ops::intern(first_repository, builder).first;
    const auto second = tyr::datalog::tests::ops::intern(second_repository, builder).first;
    EXPECT_EQ(first.index(), second.index());
    EXPECT_NE(first_repository.layer(), second_repository.layer());
    EXPECT_FALSE(urd::EqualTo<IndexView> {}(first, second));
    EXPECT_NE(first.relation_identity(), second.relation_identity());

    auto data = first.data();
    const auto original_hash = urd::Hash<urd::data<RelationTag>> {}(data);
    data.index = urd::index<RelationTag>(123);
    EXPECT_EQ(urd::Hash<urd::data<RelationTag>> {}(data), original_hash);
    EXPECT_TRUE(urd::EqualTo<urd::data<RelationTag>> {}(data, first.data()));
    const auto bytes = cista::serialize(data);
    auto relocated = bytes;
    const auto* round_trip = cista::deserialize<urd::data<RelationTag>>(relocated);
    EXPECT_EQ(round_trip->index, data.index);
    EXPECT_TRUE(urd::EqualTo<urd::data<RelationTag>> {}(*round_trip, data));
}

TEST(YggdrasilTests, DatabaseRelationRepositoryInternsCanonicalEncodings)
{
    Repository repository;
    const auto columns = tyr::datalog::tests::qb::columns_of<double>({ ColumnIndex(1), ColumnIndex(2) });
    const double first_nan = std::bit_cast<double>(uint64_t { 0x7ff8000000000001 });
    const double second_nan = std::bit_cast<double>(uint64_t { 0xfff8000000000042 });
    urd::builder<ygg::Relation<ygg::Schema>> first { columns };
    first.insert(std::array { ygg::encode_value(-0.0), ygg::encode_value(1.0) });
    first.insert(std::array { ygg::encode_value(first_nan), ygg::encode_value(2.0) });
    urd::builder<ygg::Relation<ygg::Schema>> equivalent { columns };
    equivalent.insert(std::array { ygg::encode_value(second_nan), ygg::encode_value(2.0) });
    equivalent.insert(std::array { ygg::encode_value(0.0), ygg::encode_value(1.0) });
    const auto [original, created] = tyr::datalog::tests::ops::intern(repository, first);
    const auto [duplicate, duplicate_created] = tyr::datalog::tests::ops::intern(repository, equivalent);
    EXPECT_TRUE(created);
    EXPECT_FALSE(duplicate_created);
    EXPECT_EQ(original.index(), duplicate.index());
    EXPECT_TRUE(same_storage(original.row(0), duplicate.row(0)));
    EXPECT_FALSE(std::signbit(ygg::decode_value<double>(original.row(0).value(0))));
    EXPECT_EQ(relation_count(repository), 1);
}

TEST(YggdrasilTests, DatabaseRelationRepositoryRejectsInvalidRelabelingAndForeignRename)
{
    Repository repository;
    Repository foreign_repository;
    RelationBuilder builder { tyr::datalog::tests::ops::cols({ ColumnIndex(1), ColumnIndex(2) }) };
    builder.insert(cells(10, 20));
    builder.insert(cells(30, 40));
    const auto original = tyr::datalog::tests::ops::intern(repository, builder).first;
    const std::array duplicate_schema { ygg::ColumnLayout { ColumnIndex(1), ygg::type_id<std::uint32_t> }, ygg::ColumnLayout { ColumnIndex(1), ygg::type_id<std::uint32_t> } };
    EXPECT_THROW(builder.rename(duplicate_schema), std::invalid_argument);
    EXPECT_THROW(builder.initialize(duplicate_schema), std::invalid_argument);
    EXPECT_TRUE(std::ranges::equal(builder.columns(), original.columns()));
    expect_rows(builder, { { 10, 20 }, { 30, 40 } });

    const auto foreign = tyr::datalog::tests::ops::intern(foreign_repository, builder).first;
    ASSERT_EQ(foreign.index(), original.index());
    const auto unary = tyr::datalog::tests::ops::cols({ ColumnIndex(1) });
    EXPECT_THROW(tyr::datalog::tests::ops::rename(repository, original, unary), std::invalid_argument);
    const std::array duplicate_columns { ygg::ColumnLayout { ColumnIndex(1), ygg::type_id<ygg::uint_t> },
                                         ygg::ColumnLayout { ColumnIndex(1), ygg::type_id<ygg::uint_t> } };
    EXPECT_THROW(tyr::datalog::tests::ops::rename(repository, original, std::span<const ygg::ColumnLayout>(duplicate_columns)), std::invalid_argument);
    const std::vector<ygg::ColumnLayout> retyped { { ColumnIndex(1), ygg::type_id<double> }, { ColumnIndex(2), ygg::type_id<ygg::uint_t> } };
    EXPECT_THROW(tyr::datalog::tests::ops::rename(repository, original, retyped), std::invalid_argument);
    EXPECT_EQ(relation_count(repository), 1);
    EXPECT_EQ(original.columns()[0].label, ColumnIndex(1));
    EXPECT_EQ(original.columns()[1].label, ColumnIndex(2));
    expect_rows(original, { { 10, 20 }, { 30, 40 } });
}

TEST(YggdrasilTests, DatabaseConversionsRemapStoredIdentityAndReuseMutableStorage)
{
    Repository source_repository;
    Repository target_repository;
    RelationBuilder source_builder { tyr::datalog::tests::ops::cols({ ColumnIndex(7), ColumnIndex(3) }) };
    source_builder.insert(cells(4, 5));
    source_builder.insert(cells(1, 2));
    const auto source = tyr::datalog::tests::ops::intern(source_repository, source_builder).first;
    RelationBuilder seed { tyr::datalog::tests::ops::cols({ ColumnIndex(99) }) };
    seed.insert(cells(77));
    (void) tyr::datalog::tests::ops::intern(target_repository, seed);

    const auto [target, created] = tyr::datalog::tests::ops::intern(target_repository, source);
    EXPECT_TRUE(created);
    EXPECT_EQ(&target.context(), &target_repository);
    EXPECT_NE(target.data().columns, source.data().columns);
    EXPECT_NE(target.data().pool, source.data().pool);  // the target's second schema: the seed's came first
    EXPECT_TRUE(std::ranges::equal(target.columns(), source.columns()));
    expect_rows(target, { { 4, 5 }, { 1, 2 } });
    EXPECT_FALSE(same_storage(target.row(0), source.row(0)));
    EXPECT_FALSE(tyr::datalog::tests::ops::intern(target_repository, source).second);
    const auto [same, same_created] = tyr::datalog::tests::ops::intern(target_repository, target);
    EXPECT_FALSE(same_created);
    EXPECT_EQ(same.index(), target.index());

    const auto pending = source.data();

    auto output_handle = source_repository.get_relation_builder<ygg::Schema>(tyr::datalog::tests::ops::cols({ ColumnIndex(3), ColumnIndex(7) }));
    auto& output = *output_handle;
    const auto identity = ygg::relation_identity(output);
    output.insert(cells(99, 88));
    database::assign(output, source);
    EXPECT_TRUE(std::ranges::equal(output.columns(), source.columns()));
    expect_rows(output, { { 4, 5 }, { 1, 2 } });
    const auto capacity = output.memory_usage();
    const auto* columns_storage = output.columns().data();
    const auto canonical = tyr::datalog::tests::ops::intern(source_repository, output).first;
    EXPECT_EQ(&database::assign(output, target), &output);
    EXPECT_EQ(output.memory_usage(), capacity);
    EXPECT_EQ(output.columns().data(), columns_storage);
    EXPECT_EQ(ygg::relation_identity(output), identity);
    expect_rows(output, { { 4, 5 }, { 1, 2 } });

    const auto* first_row = source_repository.view(output).row(0).get_values().data();
    (void) tyr::datalog::tests::ops::intern(source_repository, output);
    EXPECT_EQ(&database::assign(output, borrow(output)), &output);
    EXPECT_EQ(source_repository.view(output).row(0).get_values().data(), first_row);
    (void) tyr::datalog::tests::ops::intern(source_repository, output);
    EXPECT_EQ(&database::assign(output, urd::resolve(output, source_repository)), &output);
    EXPECT_EQ(source_repository.view(output).row(0).get_values().data(), first_row);
    EXPECT_EQ(output.columns().data(), columns_storage);
    EXPECT_EQ(output.memory_usage(), capacity);
    EXPECT_EQ(ygg::relation_identity(output), identity);
    expect_rows(output, { { 4, 5 }, { 1, 2 } });

    // A changed arity replaces storage; row values follow the source's column order.
    database::assign(seed, borrow(source_builder));
    EXPECT_TRUE(std::ranges::equal(seed.columns(), source.columns()));
    expect_rows(seed, { { 4, 5 }, { 1, 2 } });
    database::assign(seed, urd::resolve(pending, source_repository));
    expect_rows(seed, { { 4, 5 }, { 1, 2 } });

    RelationBuilder empty { tyr::datalog::tests::ops::cols({ ColumnIndex(9), ColumnIndex(8) }) };
    database::assign(output, borrow(empty));
    EXPECT_TRUE(output.empty());
    EXPECT_TRUE(std::ranges::equal(output.columns(), empty.columns()));
    EXPECT_EQ(output.memory_usage(), capacity);
    EXPECT_EQ(ygg::relation_identity(output), identity);
    expect_rows(canonical, { { 4, 5 }, { 1, 2 } });

    RelationBuilder nullary;
    database::assign(seed, borrow(nullary));
    EXPECT_EQ(seed.columns().size(), 0);
    EXPECT_TRUE(seed.empty());
    nullary.insert(cells());
    database::assign(seed, borrow(nullary));
    EXPECT_EQ(seed.columns().size(), 0);
    expect_rows(seed, { {} });

    source_repository.clear();
    source_builder.clear();
    expect_rows(target, { { 4, 5 }, { 1, 2 } });
}

TEST(YggdrasilTests, DatabaseRelationIteratorsSupportRandomAccessThroughTemporaryViews)
{
    Repository repository;
    auto builder = RelationBuilder(tyr::datalog::tests::ops::cols({ ColumnIndex(0) }));
    builder.insert(cells(10));
    builder.insert(cells(20));
    builder.insert(cells(30));
    const auto stored = tyr::datalog::tests::ops::intern(repository, builder).first;
    const auto check = [](auto view)
    {
        using V = decltype(view);
        using Iterator = std::ranges::iterator_t<decltype(view.rows())>;
        static_assert(std::random_access_iterator<Iterator>);
        static_assert(std::same_as<typename std::iterator_traits<Iterator>::pointer, void>);
        EXPECT_EQ(Iterator {}, Iterator {});
        const auto begin = V(view).rows().begin();
        const auto end = V(view).rows().end();
        EXPECT_EQ(end - begin, 3);
        EXPECT_EQ(begin - end, -3);
        EXPECT_EQ(std::ranges::distance(begin, end), 3);
        EXPECT_LT(begin, end);
        EXPECT_LE(begin, begin);
        EXPECT_GT(end, begin);
        EXPECT_GE(end, end);
        EXPECT_NE(begin, end);
        auto it = begin;
        const auto copy = it;
        EXPECT_EQ(it++, copy);
        EXPECT_TRUE(same_values((*copy), view.row(0)));
        EXPECT_TRUE(same_values((*it), view.row(1)));
        EXPECT_EQ(++it, begin + 2);
        EXPECT_EQ(it--, begin + 2);
        EXPECT_EQ(--it, begin);
        it += 3;
        EXPECT_EQ(it, end);
        it += -2;
        EXPECT_EQ(it, 1 + begin);
        it -= -1;
        EXPECT_EQ(it, end - 1);
        it -= 2;
        EXPECT_EQ(it, begin);
        EXPECT_TRUE(same_values((end[-1]), view.row(2)));
        EXPECT_TRUE(same_values(((begin + 1)[-1]), view.row(0)));
    };
    check(urd::resolve(builder, repository));
    check(urd::resolve(stored.data(), repository));
    check(repository.view(stored.index()));

    auto nullary = RelationBuilder {};
    const auto check_empty = [](auto view)
    {
        EXPECT_EQ(view.rows().begin(), view.rows().end());
        EXPECT_EQ(view.rows().end() - view.rows().begin(), 0);
    };
    const auto empty = tyr::datalog::tests::ops::intern(repository, nullary).first;
    check_empty(urd::resolve(nullary, repository));
    check_empty(urd::resolve(empty.data(), repository));
    check_empty(repository.view(empty.index()));
    nullary.insert(cells());
    const auto unit = tyr::datalog::tests::ops::intern(repository, nullary).first;
    const auto check_unit = [](auto view)
    {
        using V = decltype(view);
        const auto it = V(view).rows().end() - 1;
        EXPECT_EQ((*it).size(), 0);
        EXPECT_EQ(it + 1, view.rows().end());
    };
    check_unit(urd::resolve(nullary, repository));
    check_unit(urd::resolve(unit.data(), repository));
    check_unit(repository.view(unit.index()));
}

namespace
{
/// Columns store indices as uint_t; their owner resolves them.
template<typename Repository>
auto resolve_in(ygg::uint_t value, const Repository& repository)
{ return urd::resolve(ColumnsIndex(value), repository); }
}

TEST(YggdrasilTests, TypedRelationRowsLeaveIndicesOfOtherRepositoriesRaw)
{
    Repository entities;
    const auto first = insert_columns(entities, { ColumnIndex(7) }).first;
    const auto second = insert_columns(entities, { ColumnIndex(9) }).first;
    const auto first_value = first.index().value;
    const auto second_value = second.index().value;
    Repository rows;
    const std::vector<ygg::ColumnLayout> columns { { ColumnIndex(1), ygg::type_id<ygg::uint_t> }, { ColumnIndex(2), ygg::type_id<ygg::uint_t> } };
    auto builder = urd::builder<ygg::Relation<ygg::Schema>>(columns);
    builder.insert(std::array { ygg::encode_value(first_value), ygg::encode_value(second_value) });
    const auto published = tyr::datalog::tests::ops::intern(rows, builder).first;
    const auto index_view = rows.view(published.index());
    const auto data_view = urd::resolve(published.data(), rows);
    // Even in the repository that owns the values, a row leaves them raw.
    const auto builder_view = urd::resolve(builder, entities);
    static_assert(database::RelationViewConcept<decltype(index_view)>);
    static_assert(std::same_as<decltype(index_view.row(0).value(0)), ygg::Value>);
    static_assert(std::same_as<decltype(builder_view.row(0).value(0)), ygg::Value>);
    const auto check = [&](const auto& view)
    {
        ASSERT_EQ(view.size(), 1);
        EXPECT_EQ(ygg::decode_value<ygg::uint_t>(view.row(0).value(0)), first_value);
        // Rows of interned relations name values owned elsewhere; callers resolve them explicitly.
        EXPECT_EQ(resolve_in(ygg::decode_value<ygg::uint_t>(view.row(0).value(0)), entities), first);
        EXPECT_EQ(resolve_in(ygg::decode_value<ygg::uint_t>(view.row(0).value(1)), entities), second);
        size_t count = 0;
        for (auto objects : view.rows())
        {
            EXPECT_EQ(resolve_in(ygg::decode_value<ygg::uint_t>(objects.value(0)), entities), first);
            EXPECT_EQ(resolve_in(ygg::decode_value<ygg::uint_t>(objects.value(1)), entities), second);
            ++count;
        }
        EXPECT_EQ(count, 1);
    };
    check(index_view);
    check(data_view);
    check(builder_view);
    const auto iterator = index_view.rows().begin();
    EXPECT_EQ(ygg::decode_value<ygg::uint_t>((*iterator).value(1)), second_value);

    const auto labels = relabeled(index_view.columns(), { ColumnIndex(3), ColumnIndex(4) });
    const auto renamed = tyr::datalog::tests::ops::rename(rows, index_view, labels);
    static_assert(std::same_as<decltype(renamed), decltype(index_view)>);
    EXPECT_EQ(&renamed.context(), &rows);
    EXPECT_TRUE(std::ranges::equal(renamed.data().rows, index_view.data().rows));
    EXPECT_EQ(ygg::decode_value<ygg::uint_t>(renamed.row(0).value(0)), first_value);
    Repository other_rows;
    EXPECT_THROW(tyr::datalog::tests::ops::rename(other_rows, index_view, labels), std::out_of_range);  // its pool is not declared there

    const std::vector<ygg::ColumnLayout> probe_columns { { ColumnIndex(2), ygg::type_id<ygg::uint_t> } };
    auto probe = urd::builder<ygg::Relation<ygg::Schema>>(probe_columns);
    probe.insert(std::array { ygg::encode_value(second_value) });
    auto joined = tyr::datalog::tests::ops::joined(data_view, urd::resolve(probe, entities));
    EXPECT_TRUE(joined.contains(std::array { ygg::encode_value(first_value), ygg::encode_value(second_value) }));
    auto projected = tyr::datalog::tests::ops::projected(index_view, { ColumnIndex(2) });
    EXPECT_TRUE(projected.contains(std::array { ygg::encode_value(second_value) }));
    auto assigned = urd::builder<ygg::Relation<ygg::Schema>>();
    database::assign(assigned, index_view);
    EXPECT_TRUE(assigned.contains(std::array { ygg::encode_value(first_value), ygg::encode_value(second_value) }));
    const auto copied = tyr::datalog::tests::ops::intern(other_rows, index_view).first;
    EXPECT_TRUE(copied.contains(std::array { ygg::encode_value(first_value), ygg::encode_value(second_value) }));

    auto truth = urd::builder<ygg::Relation<ygg::Schema>>();
    truth.insert(cells<ygg::uint_t>());
    const auto truth_view = urd::resolve(truth, entities);
    EXPECT_EQ((*truth_view.rows().begin()).size(), 0);
    EXPECT_EQ(std::ranges::distance(truth_view.rows()), 1);
    truth.clear();
    EXPECT_EQ(truth_view.rows().begin(), truth_view.rows().end());
}

namespace
{
template<typename Destination, typename Source>
concept CanAssignColumns = requires(Destination& destination, const Source& source) { database::assign(destination, source); };

struct UnsupportedColumn
{
    void* pointer;
};

static_assert(ygg::ColumnValueType<ygg::uint_t> && ygg::ColumnValueType<double> && ygg::ColumnValueType<bool> && ygg::ColumnValueType<urd::index<ygg::formalism::Object<void>>>);
static_assert(!ygg::ColumnValueType<ColumnsIndex> && !ygg::ColumnValueType<const ygg::uint_t> && !ygg::ColumnValueType<UnsupportedColumn>);
static_assert(ygg::type_id<std::uint32_t> == ygg::TypeId { 0 } && ygg::type_id<bool> == ygg::TypeId { 6 } && ygg::type_id<urd::index<ygg::formalism::Object<void>>> == ygg::TypeId { 7 });
static_assert(ygg::column_bits(ygg::type_id<bool>, 5) == 1 && ygg::column_bits(ygg::type_id<double>, 5) == 64 && ygg::column_bits(ygg::type_id<urd::index<ygg::formalism::Object<void>>>, 5) == 5);
}

TEST(YggdrasilTests, ColumnValuesDecodeFromTheirBits)
{
    for (const auto value : { ygg::ColumnValue(std::int32_t { -3 }), ygg::ColumnValue(-0.0f), ygg::ColumnValue(true), ygg::ColumnValue(urd::index<ygg::formalism::Object<void>>(9)) })
        EXPECT_EQ(ygg::decode(ygg::encode(value), static_cast<ygg::TypeId>(value.index())), value);
    EXPECT_EQ(ygg::encode(ygg::ColumnValue(std::int32_t { -1 })), 0xffffffffu);
    EXPECT_THROW((void) ygg::decode(0, static_cast<ygg::TypeId>(std::variant_size_v<ygg::ColumnValue>)), std::invalid_argument);
}

TEST(YggdrasilTests, DatabaseMixedWidthRowsPreserveTypedValuesAndResolveIndices)
{
    Repository entities;
    const auto entity = insert_columns(entities, { ColumnIndex(11) }).first;
    const std::vector<ygg::ColumnLayout> columns { { ColumnIndex(4), ygg::type_id<ygg::uint_t> }, { ColumnIndex(9), ygg::type_id<double> }, { ColumnIndex(2), ygg::type_id<bool> }, { ColumnIndex(7), ygg::type_id<ygg::uint_t> } };
    ASSERT_EQ(columns.size(), 4);
    // Rows by the schema: each column whole, a bool in one bit.
    const auto schema = urd::builder<ygg::Relation<ygg::Schema>>(columns).widths();
    EXPECT_EQ(schema.bits(), 32U + 64 + 1 + 32);
    EXPECT_EQ(schema.at(0).at, 0U);
    EXPECT_EQ(schema.at(1).at, 32U);
    EXPECT_EQ(schema.at(2).at, 96U);
    EXPECT_EQ(schema.at(3).at, 97U);

    Repository rows;
    urd::builder<ygg::Relation<ygg::Schema>> builder(columns);
    const auto tuple = std::array { ygg::encode_value(ygg::uint_t { 42 }), ygg::encode_value(1.25), ygg::encode_value(true), ygg::encode_value(entity.index().value) };
    builder.insert(tuple);
    const auto stored = tyr::datalog::tests::ops::intern(rows, builder).first;
    const auto verify = [&](const auto& view)
    {
        const auto row = view.row(0);
        const auto schema = view.columns();
        const auto at = [&](ColumnIndex label) { return static_cast<size_t>(std::ranges::find(schema, label, &ygg::ColumnLayout::label) - schema.begin()); };
        EXPECT_EQ(ygg::decode_value<ygg::uint_t>(row.value(at(ColumnIndex(4)))), 42);
        EXPECT_DOUBLE_EQ(ygg::decode_value<double>(row.value(at(ColumnIndex(9)))), 1.25);
        EXPECT_EQ(row.type(at(ColumnIndex(9))), ygg::type_id<double>);
        EXPECT_DOUBLE_EQ(std::get<double>(row.get(at(ColumnIndex(9)))), 1.25);
        EXPECT_EQ(std::get<ygg::uint_t>(row.get(at(ColumnIndex(7)))), entity.index().value);
        EXPECT_DOUBLE_EQ(row.template get<double>(at(ColumnIndex(9))), 1.25);
        EXPECT_THROW((void) row.template get<float>(at(ColumnIndex(9))), std::invalid_argument);
        EXPECT_THROW((void) row.template get<urd::index<ygg::formalism::Object<void>>>(), std::invalid_argument);  // mixed columns
        EXPECT_THROW((void) row.template get<double>(0, 2), std::invalid_argument);
        EXPECT_DOUBLE_EQ(row.template get<double>(at(ColumnIndex(9)), at(ColumnIndex(9)) + 1)[0], 1.25);
        EXPECT_TRUE(ygg::decode_value<bool>(row.value(at(ColumnIndex(2)))));
        EXPECT_EQ(resolve_in(ygg::decode_value<ygg::uint_t>(row.value(at(ColumnIndex(7)))), entities), entity);
        EXPECT_EQ(at(ColumnIndex(99)), schema.size());
    };
    verify(urd::resolve(builder, entities));
    verify(urd::resolve(stored.data(), rows));
    verify(rows.view(stored.index()));
    EXPECT_EQ(ColumnsIndex(ygg::decode_value<ygg::uint_t>(borrow(builder).row(0).value(3))), entity.index());
    EXPECT_TRUE(stored.contains(tuple));

    urd::data<ygg::Columns> data;
    data.values.set(columns.begin(), columns.end());
    auto data_bytes = cista::serialize(data);
    const auto* restored_data = cista::deserialize<urd::data<ygg::Columns>>(data_bytes);
    EXPECT_TRUE(std::ranges::equal(restored_data->values, columns));
}

TEST(YggdrasilTests, DatabaseFloatingPointRowsInternCanonicalNaNsAndSignedZero)
{
    const std::vector<ygg::ColumnLayout> columns { { ColumnIndex(1), ygg::type_id<double> }, { ColumnIndex(2), ygg::type_id<bool> } };
    urd::builder<ygg::Relation<ygg::Schema>> builder(columns);
    const double first_nan = std::bit_cast<double>(uint64_t { 0x7ff8000000000001 });
    const double second_nan = std::bit_cast<double>(uint64_t { 0xfff8000000000042 });
    EXPECT_EQ(builder.insert(std::array { ygg::encode_value(-0.0), ygg::encode_value(false) }), 0);
    EXPECT_EQ(builder.insert(std::array { ygg::encode_value(0.0), ygg::encode_value(false) }), 0);
    EXPECT_EQ(builder.insert(std::array { ygg::encode_value(first_nan), ygg::encode_value(true) }), 1);
    EXPECT_EQ(builder.insert(std::array { ygg::encode_value(second_nan), ygg::encode_value(true) }), 1);
    EXPECT_EQ(builder.size(), 2);
    EXPECT_FALSE(std::signbit(ygg::decode_value<double>(borrow(builder).row(0).value(0))));
    EXPECT_TRUE(std::isnan(ygg::decode_value<double>(borrow(builder).row(1).value(0))));

    Repository repository;
    const auto first = tyr::datalog::tests::ops::intern(repository, builder).first;
    builder.clear();
    builder.insert(std::array { ygg::encode_value(second_nan), ygg::encode_value(true) });
    builder.insert(std::array { ygg::encode_value(+0.0), ygg::encode_value(false) });
    const auto [second, created] = tyr::datalog::tests::ops::intern(repository, builder);
    EXPECT_FALSE(created);
    EXPECT_EQ(first.index(), second.index());
    EXPECT_TRUE(second.contains(std::array { ygg::encode_value(first_nan), ygg::encode_value(true) }));

    const auto invalid = std::array { ygg::encode_value(+0.0), ygg::encode_value(false) };
    const auto truncated = std::span<const ygg::Value>(invalid).first(invalid.size() - 1);
    EXPECT_THROW(builder.insert(truncated), std::invalid_argument);
    EXPECT_FALSE(builder.contains(truncated));
    EXPECT_EQ(builder.size(), 2);
}

TEST(YggdrasilTests, DatabaseTypedSchemaRejectsMalformedMetadataBeforeMutation)
{
    const std::vector<ygg::ColumnLayout> columns { { ColumnIndex(1), ygg::type_id<ygg::uint_t> }, { ColumnIndex(2), ygg::type_id<double> } };
    urd::builder<ygg::Relation<ygg::Schema>> relation(columns);
    relation.insert(std::array { ygg::encode_value(ygg::uint_t { 7 }), ygg::encode_value(3.25) });
    auto invalid = urd::Store<urd::sequence<ygg::ColumnLayout>>(columns.begin(), columns.end());
    invalid[1].label = invalid[0].label;
    EXPECT_THROW(relation.initialize(std::span(invalid)), std::invalid_argument);
    invalid.assign(columns.begin(), columns.end());
    invalid[1].type = ygg::TypeId { 99 };
    EXPECT_THROW(relation.initialize(std::span(invalid)), std::invalid_argument);
    EXPECT_THROW(relation.insert(std::array { ygg::encode_value(3.25), ygg::encode_value(ygg::uint_t { 7 }) }), std::out_of_range);  // too wide
    ASSERT_EQ(relation.size(), 1);
    EXPECT_TRUE(relation.contains(std::array { ygg::encode_value(ygg::uint_t { 7 }), ygg::encode_value(3.25) }));
}

TEST(YggdrasilTests, DatabaseBuiltinCodecsPreserveSignedAndWideValues)
{
    const std::vector<ygg::ColumnLayout> columns { { ColumnIndex(0), ygg::type_id<uint32_t> }, { ColumnIndex(1), ygg::type_id<int32_t> }, { ColumnIndex(2), ygg::type_id<uint64_t> }, { ColumnIndex(3), ygg::type_id<int64_t> }, { ColumnIndex(4), ygg::type_id<float> }, { ColumnIndex(5), ygg::type_id<double> }, { ColumnIndex(6), ygg::type_id<bool> } };
    urd::builder<ygg::Relation<ygg::Schema>> relation(columns);
    const auto values = std::array { ygg::encode_value(std::numeric_limits<uint32_t>::max()), ygg::encode_value(std::numeric_limits<int32_t>::min()), ygg::encode_value(std::numeric_limits<uint64_t>::max()), ygg::encode_value(std::numeric_limits<int64_t>::min()), ygg::encode_value(1.25F), ygg::encode_value(std::numeric_limits<double>::infinity()), ygg::encode_value(true) };
    relation.insert(values);
    const auto row = borrow(relation).row(0);
    EXPECT_EQ(relation.widths().bits(), 32U + 32 + 64 + 64 + 32 + 64 + 1);
    EXPECT_EQ(row.value(0), std::get<0>(values));
    EXPECT_EQ(row.value(1), std::get<1>(values));
    EXPECT_EQ(row.value(2), std::get<2>(values));
    EXPECT_EQ(row.value(3), std::get<3>(values));
    EXPECT_FLOAT_EQ(ygg::decode_value<float>(row.value(4)), 1.25F);
    EXPECT_EQ(ygg::decode_value<double>(row.value(5)), std::numeric_limits<double>::infinity());
    EXPECT_TRUE(ygg::decode_value<bool>(row.value(6)));
    EXPECT_EQ(relation.insert(row.get_values()), 0);
    EXPECT_TRUE(relation.contains(values));
}

TEST(YggdrasilTests, HomogeneousRowsRoundTripThroughReusableBuffers)
{
    const std::vector<ygg::ColumnLayout> columns { { ColumnIndex(1), ygg::type_id<std::uint64_t> }, { ColumnIndex(2), ygg::type_id<std::uint64_t> } };
    const auto values = urd::Store<urd::sequence<std::uint64_t>> { 4, 9 };
    auto encoded = urd::Store<urd::sequence<ygg::Value>>();
    for (const auto value : values)
        encoded.push_back(ygg::encode_value(value));
    EXPECT_EQ(encoded.size(), columns.size());

    auto stored = urd::builder<ygg::Relation<ygg::Schema>>(columns);
    stored.insert(encoded);
    const auto row = borrow(stored).row(0);
    for (size_t c = 0; c < values.size(); ++c)
        EXPECT_EQ(ygg::decode_value<std::uint64_t>(row.value(c)), values[c]);

    auto relation = urd::builder<ygg::Relation<ygg::Schema>>(columns);
    relation.insert(std::array { ygg::encode_value(std::uint64_t { 4 }), ygg::encode_value(std::uint64_t { 9 }) });
    EXPECT_TRUE(relation.contains(encoded));
}

TEST(YggdrasilTests, DatabasePoolsKeepTheirSchema)
{
    Repository repository;
    auto unsigned_types = urd::data<ygg::Schema>();
    unsigned_types.types.push_back(ygg::type_id<ygg::uint_t>);
    unsigned_types.types.push_back(ygg::type_id<ygg::uint_t>);
    auto double_types = urd::data<ygg::Schema>();
    double_types.types.push_back(ygg::type_id<double>);
    double_types.types.push_back(ygg::type_id<double>);
    const auto key = repository.relations().schema(unsigned_types);  // a free relation's pool is keyed by its Schema
    const auto other = repository.relations().schema(double_types);
    EXPECT_EQ(repository.relations().schema(unsigned_types), key);  // interned once
    auto values = urd::flat_vector<ygg::Value>();
    values.push_back(1);
    values.push_back(2);
    EXPECT_TRUE(repository.relations().schema(key).is_none());  // not declared yet
    EXPECT_THROW((void) repository.relations().insert(key, values), std::out_of_range);  // rows need a declared pool
    repository.relations().declare(key, key);
    repository.relations().declare(key, key);  // again with the same Schema: nothing changes
    EXPECT_THROW(repository.relations().declare(key, other), std::invalid_argument);
    EXPECT_EQ(repository.relations().schema(key), key);
    EXPECT_TRUE(repository.relations().insert(key, values).second);
    EXPECT_TRUE(std::ranges::equal(repository.relations().types(key), unsigned_types.types));
    values.push_back(3);
    EXPECT_THROW((void) repository.relations().insert(key, values), std::invalid_argument);  // checked against the Schema
    values.pop_back();
    Repository child(&repository);
    EXPECT_THROW(child.relations().declare(key, other), std::invalid_argument);  // the parent's declaration
    EXPECT_EQ(child.relations().schema(key), key);
    repository.clear();
    EXPECT_EQ(repository.relations().schema(key), key);  // the declaration and its Schema stay, the rows go
    EXPECT_TRUE(std::ranges::equal(repository.relations().types(key), unsigned_types.types));
    EXPECT_EQ(repository.relations().size(key), 0U);
    EXPECT_TRUE(repository.relations().insert(key, values).second);
}

TEST(YggdrasilTests, DatabaseBuildersAreNumberedLikeLayers)
{
    RelationBuilder first(tyr::datalog::tests::ops::cols({ ColumnIndex(1) }));
    RelationBuilder second;
    const Repository layer;
    EXPECT_NE(first.layer(), second.layer());
    EXPECT_NE(first.layer(), layer.layer());
    const auto number = first.layer();
    first.insert(cells(1));
    first.clear();
    first.initialize(tyr::datalog::tests::ops::cols({ ColumnIndex(1), ColumnIndex(2) }));
    EXPECT_EQ(first.layer(), number);
    EXPECT_EQ(ygg::relation_identity(first).context, number);
    EXPECT_EQ(ygg::relation_identity(first).relation, urd::index<void>::none_value);
    first.insert(cells(3, 4));
    EXPECT_EQ(layer.view(first).row(0).context().layer(), number);
}

}  // namespace tyr::datalog::tests
