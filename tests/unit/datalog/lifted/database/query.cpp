#include "query_helpers.hpp"

#include <algorithm>
#include <array>
#include <cista/serialization.h>
#include <gtest/gtest.h>
#include <memory>
#include <random>
#include <tyr/datalog/lifted/database/semantics/incremental/query.hpp>
#include <tyr/datalog/lifted/database/semantics/query_evaluation.hpp>
#include <tyr/datalog/lifted/database/semantics/relation_view.hpp>
#include <tyr/datalog/lifted/database/syntax/query_repository.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <vector>

namespace
{
using Column = urd::index<ygg::Column>;
using Relation = urd::builder<ygg::Relation<ygg::Schema>>;
using Delta = tyr::datalog::database::incremental::Delta;
using Change = std::pair<ygg::FreeBuilderRelationView, ygg::FreeBuilderRelationView>;

/// The layer the tests read their builders through.
const ygg::FreeRelationRepository& layer()
{
    static const ygg::FreeRelationRepository repository;
    return repository;
}

ygg::FreeBuilderRelationView borrow(const urd::builder<ygg::Relation<ygg::Schema>>& relation) { return layer().view(relation); }
const Column x(0), y(1), z(2), d(3);

auto rows(const auto& relation)
{
    urd::Store<urd::sequence<urd::Store<urd::sequence<ygg::Value>>>> result;
    result.reserve(relation.size());
    for (const auto row : relation.rows())
    {
        auto& values = result.emplace_back();
        for (size_t c = 0; c < row.size(); ++c)
            values.push_back(row.value(c));
    }
    std::ranges::sort(result);
    return result;
}
auto minus(const auto& lhs, const auto& rhs)
{
    std::remove_cvref_t<decltype(lhs)> result;
    std::ranges::set_difference(lhs, rhs, std::back_inserter(result));
    return result;
}
void apply(Relation& relation, const Delta& change)
{
    const auto [added, removed] = change.change();
    for (const auto row : removed.rows())
        relation.erase(relation.find(row).value());
    for (const auto row : added.rows())
        relation.insert(row);
}
void reverse(Delta& change) { std::swap(change.added, change.removed); }

template<typename T>
concept HasArgument = requires(T view) { view.get_arg(); };
template<typename T>
concept HasOperands = requires(T view) {
    view.get_lhs();
    view.get_rhs();
};
template<typename T>
concept HasSources = requires(T view) { view.get_sources(); };
using JoinView = tyr::datalog::database::QueryView<tyr::datalog::database::QueryJoinTag>;
using ProjectView = tyr::datalog::database::QueryView<tyr::datalog::database::QueryProjectTag>;
using DistanceView = tyr::datalog::database::QueryView<tyr::datalog::database::QueryDistanceTag>;
static_assert(HasOperands<JoinView> && !HasArgument<JoinView> && !HasSources<JoinView>);
static_assert(HasArgument<ProjectView> && !HasOperands<ProjectView>);
static_assert(HasSources<DistanceView> && !HasArgument<DistanceView> && !HasOperands<DistanceView>);
static_assert(!HasArgument<tyr::datalog::database::QueryView<>> && !HasOperands<tyr::datalog::database::QueryView<>>);
static_assert(!std::copy_constructible<tyr::datalog::database::QueryRepository>);
static_assert(!std::convertible_to<urd::index<tyr::datalog::database::Query<tyr::datalog::database::QueryInputTag>>,
                                   urd::index<tyr::datalog::database::Query<tyr::datalog::database::QueryJoinTag>>>);

TEST(DatabaseQuery, TypedLogicalAlternativesKeepOperandsAndRootProvenance)
{
    auto repository = tyr::datalog::tests::qb::repository();
    const auto a = tyr::datalog::tests::qb::input(repository, 0, { x, y });
    const auto b = tyr::datalog::tests::qb::input(repository, 1, { y, z });
    const auto empty = tyr::datalog::tests::qb::empty(repository, { x, y });
    const auto joined = tyr::datalog::tests::qb::join(repository, a, b);
    const auto projected = tyr::datalog::tests::qb::project(repository, a, { x });
    const auto renamed = tyr::datalog::tests::qb::rename(repository, a, { y, z });
    const auto equal = tyr::datalog::tests::qb::select_equal(repository, a, x, y);
    const auto selected = tyr::datalog::tests::qb::select_value(repository, a, x, static_cast<ygg::uint_t>(7));
    const auto united = tyr::datalog::tests::qb::union_(repository, a, empty);
    const auto difference = tyr::datalog::tests::qb::difference(repository, a, empty);
    const auto targets = tyr::datalog::tests::qb::project(repository, b, { z });
    const auto distance = tyr::datalog::tests::qb::distance(repository, projected, a, targets, d);
    const auto roots = std::array { a, empty, joined, projected, renamed, equal, selected, united, difference, distance };
    EXPECT_EQ(tyr::datalog::database::as<tyr::datalog::database::QueryInputTag>(a).get_input_slot(), 0);
    EXPECT_TRUE(std::ranges::equal(tyr::datalog::database::as<tyr::datalog::database::QueryEmptyTag>(empty).columns(), a.columns()));
    EXPECT_EQ(tyr::datalog::database::as<tyr::datalog::database::QueryJoinTag>(joined).get_lhs().index(), a.index());
    EXPECT_EQ(tyr::datalog::database::as<tyr::datalog::database::QueryJoinTag>(joined).get_rhs().index(), b.index());
    EXPECT_EQ(tyr::datalog::database::as<tyr::datalog::database::QueryProjectTag>(projected).get_arg().index(), a.index());
    EXPECT_EQ(tyr::datalog::database::as<tyr::datalog::database::QueryProjectTag>(projected).get_labels().front(), x);
    EXPECT_EQ(tyr::datalog::database::as<tyr::datalog::database::QueryRenameTag>(renamed).get_labels().front(), y);
    EXPECT_EQ(tyr::datalog::database::as<tyr::datalog::database::QuerySelectEqualTag>(equal).get_lhs_column(), x);
    EXPECT_EQ(tyr::datalog::database::as<tyr::datalog::database::QuerySelectEqualTag>(equal).get_rhs_column(), y);
    EXPECT_EQ(tyr::datalog::database::as<tyr::datalog::database::QuerySelectValueTag>(selected).get_column(), x);
    EXPECT_EQ(ygg::decode_value<ygg::uint_t>(tyr::datalog::database::as<tyr::datalog::database::QuerySelectValueTag>(selected).get_constant()), 7);
    EXPECT_EQ(tyr::datalog::database::as<tyr::datalog::database::QueryUnionTag>(united).get_rhs().index(), empty.index());
    EXPECT_EQ(tyr::datalog::database::as<tyr::datalog::database::QueryDifferenceTag>(difference).get_lhs().index(), a.index());
    EXPECT_EQ(tyr::datalog::database::as<tyr::datalog::database::QueryDistanceTag>(distance).get_sources().index(), projected.index());
    EXPECT_EQ(tyr::datalog::database::as<tyr::datalog::database::QueryDistanceTag>(distance).get_edges().index(), a.index());
    EXPECT_EQ(tyr::datalog::database::as<tyr::datalog::database::QueryDistanceTag>(distance).get_targets().index(), targets.index());
    EXPECT_EQ(tyr::datalog::database::as<tyr::datalog::database::QueryDistanceTag>(distance).get_distance_column(), d);
    const auto count = repository.size();
    EXPECT_EQ(count, 12);
    const auto duplicate = tyr::datalog::tests::qb::join(repository, a, b);
    EXPECT_EQ(duplicate.index(), joined.index());
    EXPECT_EQ(tyr::datalog::database::as<tyr::datalog::database::QueryJoinTag>(duplicate).index(),
              tyr::datalog::database::as<tyr::datalog::database::QueryJoinTag>(joined).index());
    EXPECT_EQ(repository.size(), count);
    EXPECT_EQ(tyr::datalog::database::as<tyr::datalog::database::QueryInputTag>(a).index().value,
              tyr::datalog::database::as<tyr::datalog::database::QueryJoinTag>(joined).index().value);
    EXPECT_NE(a.index(), joined.index());
    const auto plan = tyr::datalog::database::compile(roots);
    for (size_t i = 0; i < roots.size(); ++i)
    {
        std::visit([&](auto operation) { EXPECT_TRUE(std::ranges::equal(operation.columns(), roots[i].columns())); }, roots[i].get_variant());
        EXPECT_TRUE(std::ranges::equal(plan[plan.roots()[i]].columns(), roots[i].columns()));
    }
}

TEST(DatabaseQuery, RecordConstructorsMatchFieldsAndValidateConstantTypes)
{
    auto repository = tyr::datalog::tests::qb::repository();
    const auto a = tyr::datalog::tests::qb::input(repository, 0, { x, y });
    const auto b = tyr::datalog::tests::qb::input(repository, 1, { y, z });
    const auto insert = [&]<typename Tag>(urd::data<tyr::datalog::database::Query<Tag>> data)
    {
        const auto concrete = tyr::datalog::database::insert(repository, data).first;
        auto erased = urd::data<tyr::datalog::database::Query<>>(concrete.index());
        return tyr::datalog::database::insert(repository, erased).first;
    };

    const urd::data<tyr::datalog::database::Query<tyr::datalog::database::QueryJoinTag>> by_index(a.index(), b.index());
    EXPECT_EQ(by_index.lhs, a.index());
    EXPECT_EQ(by_index.rhs, b.index());
    EXPECT_EQ(insert(by_index).index(), tyr::datalog::tests::qb::join(repository, a, b).index());

    const std::array labels { x };
    EXPECT_EQ(insert(urd::data<tyr::datalog::database::Query<tyr::datalog::database::QueryProjectTag>>(a.index(), labels)).index(),
              tyr::datalog::tests::qb::project(repository, a, { x }).index());
    EXPECT_EQ(insert(urd::data<tyr::datalog::database::Query<tyr::datalog::database::QuerySelectValueTag>>(a.index(), x, ygg::uint_t(7))).index(),
              tyr::datalog::tests::qb::select_value(repository, a, x, ygg::uint_t(7)).index());
    const auto generic = insert(
        urd::data<tyr::datalog::database::Query<tyr::datalog::database::QueryGenericJoinTag>>(std::array { a.index(), b.index() }, std::array { y, x, z }));
    EXPECT_TRUE(std::ranges::equal(generic.columns() | std::views::transform(&ygg::ColumnLayout::label), std::array { x, y, z }));

    auto mismatched = urd::data<tyr::datalog::database::Query<tyr::datalog::database::QuerySelectValueTag>>(a.index(), x, static_cast<std::int32_t>(7));
    EXPECT_EQ(mismatched.constant_type, (ygg::type_id<std::int32_t>) );
    EXPECT_THROW(static_cast<void>(tyr::datalog::database::insert(repository, mismatched)), std::invalid_argument);
}

TEST(DatabaseQuery, InternsValidatedNodesAndCompilesIndependentReachablePlan)
{
    tyr::datalog::database::QueryPlan plan;
    {
        auto repository = tyr::datalog::tests::qb::repository();
        auto other = tyr::datalog::tests::qb::repository();
        const auto a = tyr::datalog::tests::qb::input(repository, 0, { x, y });
        EXPECT_EQ(a.index(), tyr::datalog::tests::qb::input(repository, 0, { x, y }).index());
        const auto same = tyr::datalog::tests::qb::input(repository, 0, { x, y });
        const auto foreign = tyr::datalog::tests::qb::input(other, 0, { x, y });
        EXPECT_NE(repository.layer(), other.layer());
        EXPECT_TRUE(urd::EqualTo<decltype(a)> {}(a, same));
        EXPECT_FALSE(urd::EqualTo<decltype(a)> {}(a, foreign));
        EXPECT_EQ(urd::Hash<decltype(a)> {}(a), urd::Hash<decltype(a)> {}(same));
        std::unordered_map<tyr::datalog::database::QueryView<>,
                           int,
                           urd::Hash<tyr::datalog::database::QueryView<>>,
                           urd::EqualTo<tyr::datalog::database::QueryView<>>>
            keys;
        keys.emplace(a, 1);
        keys.emplace(same, 2);
        keys.emplace(foreign, 3);
        EXPECT_EQ(keys.size(), 2);
        EXPECT_THROW(tyr::datalog::tests::qb::project(repository, a, { z }), std::out_of_range);
        EXPECT_THROW(tyr::datalog::tests::qb::rename(repository, a, { x, x }), std::invalid_argument);
        const auto p = tyr::datalog::tests::qb::project(repository, a, { x });
        EXPECT_EQ(p.index(), tyr::datalog::tests::qb::project(repository, a, { x }).index());
        tyr::datalog::tests::qb::input(repository, 9, { z });  // Unreachable inputs must not require bindings.
        const auto renamed = tyr::datalog::tests::qb::rename(repository, p, { z });
        plan = tyr::datalog::database::compile({ p, renamed, p });
        ASSERT_EQ(plan.node_count(), 3);
        EXPECT_EQ(plan.roots()[0], plan.roots()[2]);
        EXPECT_EQ(plan[plan.roots()[1]].columns()[0].label, z);
    }
    Relation input { tyr::datalog::tests::ops::cols({ x, y }) };
    input.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(2)), ygg::encode_value(static_cast<ygg::uint_t>(3)) });
    input.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(2)), ygg::encode_value(static_cast<ygg::uint_t>(4)) });
    tyr::datalog::database::QueryEvaluator evaluator(layer(), plan);
    EXPECT_THROW((void) evaluator.get_result(), std::logic_error);
    evaluator.evaluate(std::array { borrow(input) });
    EXPECT_EQ(evaluator.get_result(0).size(), 1);
    EXPECT_EQ(evaluator.get_result(1).columns()[0].label, z);
    input.clear();
    EXPECT_EQ(evaluator.get_result().size(), 1);
    EXPECT_THROW(evaluator.evaluate(std::array { evaluator.get_result() }), std::invalid_argument);
    EXPECT_EQ(evaluator.get_result().size(), 1);
}

TEST(DatabaseQuery, IncrementalDagMatchesFullAndExactDeltasForwardBackward)
{
    auto repository = tyr::datalog::tests::qb::repository();
    const auto a = tyr::datalog::tests::qb::input(repository, 0, { x, y });
    const auto b = tyr::datalog::tests::qb::input(repository, 1, { x, y });
    const auto c = tyr::datalog::tests::qb::input(repository, 2, { y, z });
    const auto joined = tyr::datalog::tests::qb::join(repository, a, c);
    const auto projection = tyr::datalog::tests::qb::project(repository, joined, { x });
    const auto roots = std::array { joined,
                                    projection,
                                    tyr::datalog::tests::qb::rename(repository, projection, { z }),
                                    tyr::datalog::tests::qb::select_equal(repository, a, x, y),
                                    tyr::datalog::tests::qb::select_value(repository, a, x, static_cast<ygg::uint_t>(1)),
                                    tyr::datalog::tests::qb::union_(repository, a, b),
                                    tyr::datalog::tests::qb::difference(repository, a, b),
                                    tyr::datalog::tests::qb::join(repository, a, a),
                                    tyr::datalog::tests::qb::project(repository, joined, {}),
                                    tyr::datalog::tests::qb::empty(repository, { x }),
                                    tyr::datalog::tests::qb::semijoin(repository, a, c),
                                    tyr::datalog::tests::qb::antijoin(repository, a, c) };
    const auto plan = tyr::datalog::database::compile(roots);
    tyr::datalog::database::QueryEvaluator full(layer(), plan);
    tyr::datalog::database::incremental::QueryEvaluator incremental(layer(), plan);
    Relation first(tyr::datalog::tests::ops::cols({ x, y })), second(tyr::datalog::tests::ops::cols({ x, y })), third(tyr::datalog::tests::ops::cols({ y, z }));
    first.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(1)), ygg::encode_value(static_cast<ygg::uint_t>(1)) });
    first.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(2)), ygg::encode_value(static_cast<ygg::uint_t>(2)) });
    second.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(1)), ygg::encode_value(static_cast<ygg::uint_t>(1)) });
    third.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(1)), ygg::encode_value(static_cast<ygg::uint_t>(5)) });
    third.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(2)), ygg::encode_value(static_cast<ygg::uint_t>(6)) });
    const auto inputs = std::array { borrow(first), borrow(second), borrow(third) };
    incremental.initialize(inputs);
    const auto compare = [&]
    {
        full.evaluate(inputs);
        for (size_t i = 0; i < roots.size(); ++i)
        {
            EXPECT_EQ(rows(incremental.get_result(i)), rows(full.get_result(i)));
            EXPECT_TRUE(std::ranges::equal(incremental.get_result(i).columns(), roots[i].columns()));
        }
    };
    compare();
    Delta da(layer(), first.columns()), dbb(layer(), second.columns()), dc(layer(), third.columns());
    da.removed.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(1)), ygg::encode_value(static_cast<ygg::uint_t>(1)) });
    da.added.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(1)), ygg::encode_value(static_cast<ygg::uint_t>(2)) });
    dbb.removed.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(1)), ygg::encode_value(static_cast<ygg::uint_t>(1)) });
    dbb.added.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(1)), ygg::encode_value(static_cast<ygg::uint_t>(2)) });
    dc.added.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(2)), ygg::encode_value(static_cast<ygg::uint_t>(7)) });
    const auto changes = std::array { std::pair { borrow(da.added), borrow(da.removed) },
                                      std::pair { borrow(dbb.added), borrow(dbb.removed) },
                                      std::pair { borrow(dc.added), borrow(dc.removed) } };
    for (int iteration = 0; iteration < 2; ++iteration)
    {
        urd::Store<urd::sequence<decltype(rows(borrow(first)))>> before;
        before.reserve(roots.size());
        for (size_t i = 0; i < roots.size(); ++i)
            before.push_back(rows(incremental.get_result(i)));
        incremental.update(changes);
        apply(first, da);
        apply(second, dbb);
        apply(third, dc);
        compare();
        for (size_t i = 0; i < roots.size(); ++i)
        {
            const auto after = rows(incremental.get_result(i));
            EXPECT_EQ(rows(incremental.get_delta(i).change().first), minus(after, before[i]));
            EXPECT_EQ(rows(incremental.get_delta(i).change().second), minus(before[i], after));
        }
        reverse(da);
        reverse(dbb);
        reverse(dc);
    }
    // A replaced projection witness keeps membership and must emit no change.
    EXPECT_TRUE(incremental.get_delta(1).added.empty());
    EXPECT_TRUE(incremental.get_delta(1).removed.empty());
    EXPECT_GT(incremental.memory_usage(), 0);
    EXPECT_GT(full.memory_usage(), 0);
}

TEST(DatabaseQuery, MixedTypesConstantsAndDistanceComposition)
{
    using TypedRelation = urd::builder<ygg::Relation<ygg::Schema>>;
    const std::vector<ygg::ColumnLayout> schema { { x, ygg::type_id<std::uint32_t> }, { y, ygg::type_id<double> } };
    auto mixed = tyr::datalog::tests::qb::repository();
    auto input = tyr::datalog::tests::qb::input(mixed, 0, schema);
    EXPECT_THROW(tyr::datalog::tests::qb::select_equal(mixed, input, x, y), std::invalid_argument);
    EXPECT_THROW(tyr::datalog::tests::qb::select_value(mixed, input, y, std::uint32_t(1)), std::invalid_argument);
    auto selected = tyr::datalog::tests::qb::select_value(mixed, input, y, -0.0);
    const auto selected_view = std::get<tyr::datalog::database::QueryView<tyr::datalog::database::QuerySelectValueTag>>(selected.get_variant());
    EXPECT_EQ(selected_view.get_plan().position(), 1);
    EXPECT_EQ(selected.index(), tyr::datalog::tests::qb::select_value(mixed, input, y, 0.0).index());
    TypedRelation relation(schema);
    relation.insert(std::array { ygg::encode_value(static_cast<std::uint32_t>(4)), ygg::encode_value(0.0) });
    relation.insert(std::array { ygg::encode_value(static_cast<std::uint32_t>(5)), ygg::encode_value(2.0) });
    tyr::datalog::database::QueryEvaluator evaluator(layer(), tyr::datalog::database::compile({ tyr::datalog::tests::qb::project(mixed, selected, { y, x }) }));
    evaluator.evaluate(std::array { borrow(relation) });
    EXPECT_TRUE(evaluator.get_result().contains(std::array { ygg::encode_value(0.0), ygg::encode_value(std::uint32_t(4)) }));

    auto graph = tyr::datalog::tests::qb::repository();
    const auto sources = tyr::datalog::tests::qb::input(graph, 0, { x });
    const auto edges = tyr::datalog::tests::qb::input(graph, 1, { x, y });
    const auto targets = tyr::datalog::tests::qb::input(graph, 2, { z });
    const auto distance = tyr::datalog::tests::qb::distance(graph, sources, edges, targets, d);
    const auto plan = tyr::datalog::database::compile({ distance, tyr::datalog::tests::qb::select_value(graph, distance, d, static_cast<ygg::uint_t>(2)) });
    Relation s(tyr::datalog::tests::ops::cols({ x })), e(tyr::datalog::tests::ops::cols({ x, y })), t(tyr::datalog::tests::ops::cols({ z }));
    s.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(1)) });
    e.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(1)), ygg::encode_value(static_cast<ygg::uint_t>(2)) });
    e.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(2)), ygg::encode_value(static_cast<ygg::uint_t>(3)) });
    t.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(3)) });
    const auto inputs = std::array { borrow(s), borrow(e), borrow(t) };
    tyr::datalog::database::QueryEvaluator full(layer(), plan);
    tyr::datalog::database::incremental::QueryEvaluator incremental(layer(), plan);
    full.evaluate(inputs);
    incremental.initialize(inputs);
    EXPECT_EQ(rows(full.get_result()), rows(incremental.get_result()));
    ASSERT_TRUE(incremental.get_result(1).contains(
        std::array { ygg::encode_value(ygg::uint_t(1)), ygg::encode_value(ygg::uint_t(3)), ygg::encode_value(ygg::uint_t(2)) }));
    Delta ds(layer(), s.columns()), de(layer(), e.columns()), dt(layer(), t.columns());
    de.added.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(1)), ygg::encode_value(static_cast<ygg::uint_t>(3)) });
    incremental.update(std::array { std::pair { borrow(ds.added), borrow(ds.removed) },
                                    std::pair { borrow(de.added), borrow(de.removed) },
                                    std::pair { borrow(dt.added), borrow(dt.removed) } });
    apply(e, de);
    full.evaluate(inputs);
    EXPECT_EQ(rows(full.get_result()), rows(incremental.get_result()));
    EXPECT_TRUE(incremental.get_result(1).empty());
    EXPECT_EQ(incremental.get_delta().added.size(), 1);
    EXPECT_EQ(incremental.get_delta().removed.size(), 1);
}

TEST(DatabaseQuery, RejectsInvalidBatchesBeforeMutatingAndSupportsNonDistanceTypeFamilies)
{
    const auto schema = tyr::datalog::tests::qb::columns_of<double>({ x });
    auto repository = tyr::datalog::tests::qb::repository();
    const auto input = tyr::datalog::tests::qb::input(repository, 0, schema);
    urd::builder<ygg::Relation<ygg::Schema>> relation(schema);
    relation.insert(std::array { ygg::encode_value(1.0) });
    const auto plan = tyr::datalog::database::compile({ tyr::datalog::tests::qb::select_value(repository, input, x, 1.0) });
    tyr::datalog::database::QueryEvaluator full(layer(), plan);
    tyr::datalog::database::incremental::QueryEvaluator incremental(layer(), plan);
    const auto inputs = std::array { borrow(relation) };
    full.evaluate(inputs);
    incremental.initialize(inputs);
    tyr::datalog::database::incremental::Delta invalid(layer(), relation.columns());
    invalid.added.insert(std::array { ygg::encode_value(1.0) });
    const auto changes = std::array { std::pair { borrow(invalid.added), borrow(invalid.removed) } };
    EXPECT_THROW(incremental.update(changes), std::invalid_argument);
    EXPECT_EQ(rows(incremental.get_result()), rows(full.get_result()));
    EXPECT_THROW(incremental.update(std::span<const Change> {}), std::invalid_argument);
    EXPECT_EQ(incremental.get_result().size(), 1);
}

TEST(DatabaseQuery, RandomizedSharedSetDagMaintainsExactNetChanges)
{
    auto repository = tyr::datalog::tests::qb::repository();
    const auto a = tyr::datalog::tests::qb::input(repository, 0, { x, y });
    const auto b = tyr::datalog::tests::qb::input(repository, 1, { x, y });
    const auto edges = tyr::datalog::tests::qb::input(repository, 2, { y, z });
    const auto ready = tyr::datalog::tests::qb::input(repository, 3, {});
    const auto u = tyr::datalog::tests::qb::union_(repository, a, b);
    const auto difference = tyr::datalog::tests::qb::difference(repository, a, b);
    const auto project_union = tyr::datalog::tests::qb::project(repository, u, { x });
    const auto project_a = tyr::datalog::tests::qb::project(repository, a, { x });
    const auto project_b = tyr::datalog::tests::qb::project(repository, b, { x });
    const auto roots = std::array { tyr::datalog::tests::qb::join(repository, u, edges),
                                    tyr::datalog::tests::qb::join(repository, difference, edges),
                                    tyr::datalog::tests::qb::project(repository, difference, { x }),
                                    project_union,
                                    tyr::datalog::tests::qb::union_(repository, project_a, project_b),
                                    tyr::datalog::tests::qb::difference(repository, project_a, project_b),
                                    tyr::datalog::tests::qb::join(repository, project_a, project_b),
                                    tyr::datalog::tests::qb::select_equal(repository, u, x, y),
                                    tyr::datalog::tests::qb::select_value(repository, difference, x, static_cast<ygg::uint_t>(1)),
                                    tyr::datalog::tests::qb::project(repository, tyr::datalog::tests::qb::join(repository, a, edges), {}),
                                    tyr::datalog::tests::qb::join(repository, ready, project_union),
                                    tyr::datalog::tests::qb::rename(repository, project_union, { z }),
                                    tyr::datalog::tests::qb::semijoin(repository, u, edges),
                                    tyr::datalog::tests::qb::antijoin(repository, u, edges),
                                    tyr::datalog::tests::qb::semijoin(repository, a, ready),
                                    tyr::datalog::tests::qb::antijoin(repository, difference, ready),
                                    tyr::datalog::tests::qb::antijoin(repository, project_union, project_b),
                                    // References: u ⋉ edges = π_{x,y}(u ⋈ edges) and u ▷ edges = u \ (u ⋉ edges).
                                    tyr::datalog::tests::qb::project(repository, tyr::datalog::tests::qb::join(repository, u, edges), { x, y }),
                                    u };
    constexpr size_t semijoin_root = 12, antijoin_root = 13, reference_root = 17, union_root = 18;
    const auto plan = tyr::datalog::database::compile(roots);
    std::array<Relation, 4> inputs { Relation(tyr::datalog::tests::ops::cols({ x, y })),
                                     Relation(tyr::datalog::tests::ops::cols({ x, y })),
                                     Relation(tyr::datalog::tests::ops::cols({ y, z })),
                                     Relation() };
    const auto refs = std::array { borrow(inputs[0]), borrow(inputs[1]), borrow(inputs[2]), borrow(inputs[3]) };
    tyr::datalog::database::QueryEvaluator full(layer(), plan);
    tyr::datalog::database::incremental::QueryEvaluator incremental(layer(), plan);
    incremental.initialize(refs);
    std::mt19937 random(741);
    for (size_t iteration = 0; iteration < 120; ++iteration)
    {
        urd::Store<urd::sequence<decltype(rows(borrow(inputs[0])))>> before;
        before.reserve(roots.size());
        for (size_t i = 0; i < roots.size(); ++i)
            before.push_back(rows(incremental.get_result(i)));
        urd::Store<urd::sequence<Delta>> changes;
        for (size_t input = 0; input < inputs.size(); ++input)
        {
            changes.emplace_back(layer(), inputs[input].columns());
            auto& relation = inputs[input];
            auto& change = changes.back();
            const auto toggle = [&](const auto& tuple)
            {
                if (random() % 3 != 0)
                    return;
                if (const auto position = relation.find(tuple))
                {
                    change.removed.insert(tuple);
                    relation.erase(*position);
                }
                else
                {
                    change.added.insert(tuple);
                    relation.insert(tuple);
                }
            };
            if (input == 3)
                toggle(std::array<ygg::Value, 0> {});
            else
                for (ygg::uint_t i = 0; i < 3; ++i)
                    for (ygg::uint_t j = 0; j < 3; ++j)
                        toggle(std::array { ygg::encode_value(i), ygg::encode_value(j) });
        }
        urd::Store<urd::sequence<Change>> delta_refs;
        delta_refs.reserve(changes.size());
        for (const auto& change : changes)
            delta_refs.push_back(change.change());
        incremental.update(delta_refs);
        full.evaluate(refs);
        for (size_t root = 0; root < roots.size(); ++root)
        {
            SCOPED_TRACE(iteration);
            SCOPED_TRACE(root);
            const auto after = rows(full.get_result(root));
            EXPECT_EQ(rows(incremental.get_result(root)), after);
            EXPECT_EQ(rows(incremental.get_delta(root).change().first), minus(after, before[root]));
            EXPECT_EQ(rows(incremental.get_delta(root).change().second), minus(before[root], after));
        }
        const auto reference = rows(full.get_result(reference_root));
        EXPECT_EQ(rows(full.get_result(semijoin_root)), reference);
        EXPECT_EQ(rows(full.get_result(antijoin_root)), minus(rows(full.get_result(union_root)), reference));
    }
}

TEST(DatabaseQuery, GenericJoinPlanExecutesInBothModesAndPrunesAlternatives)
{
    auto repository = tyr::datalog::tests::qb::repository();
    const auto ai = tyr::datalog::tests::qb::input(repository, 0, { x, y });
    const auto bi = tyr::datalog::tests::qb::input(repository, 1, { y, z });
    const auto ci = tyr::datalog::tests::qb::input(repository, 2, { z, x });
    tyr::datalog::tests::qb::input(repository, 8, { x, y });
    const auto root = tyr::datalog::tests::qb::generic_join(repository, { ai, bi, ci }, { z, x, y });
    const auto plan = tyr::datalog::database::compile({ root });
    ASSERT_EQ(plan.node_count(), 4);
    tyr::datalog::database::QueryEvaluator full(layer(), plan);
    tyr::datalog::database::incremental::QueryEvaluator incremental(layer(), plan);
    Relation ar(tyr::datalog::tests::ops::cols({ x, y })), br(tyr::datalog::tests::ops::cols({ y, z })), cr(tyr::datalog::tests::ops::cols({ z, x }));
    ar.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(1)), ygg::encode_value(static_cast<ygg::uint_t>(2)) });
    br.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(2)), ygg::encode_value(static_cast<ygg::uint_t>(3)) });
    cr.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(3)), ygg::encode_value(static_cast<ygg::uint_t>(1)) });
    const auto inputs = std::array { borrow(ar), borrow(br), borrow(cr) };
    full.evaluate(inputs);
    incremental.initialize(inputs);
    ASSERT_EQ(incremental.get_result().size(), 1);
    EXPECT_EQ(rows(full.get_result()), rows(incremental.get_result()));
    Delta da(layer(), ar.columns()), dbb(layer(), br.columns()), dc(layer(), cr.columns());
    dc.removed.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(3)), ygg::encode_value(static_cast<ygg::uint_t>(1)) });
    incremental.update(std::array { std::pair { borrow(da.added), borrow(da.removed) },
                                    std::pair { borrow(dbb.added), borrow(dbb.removed) },
                                    std::pair { borrow(dc.added), borrow(dc.removed) } });
    EXPECT_TRUE(incremental.get_result().empty());
    EXPECT_EQ(incremental.get_delta().removed.size(), 1);
    apply(cr, dc);
    full.evaluate(inputs);
    EXPECT_EQ(rows(full.get_result()), rows(incremental.get_result()));
}

TEST(DatabaseQuery, NullaryGenericJoinIsIdentityInBothModes)
{
    auto repository = tyr::datalog::tests::qb::repository();
    const auto root = tyr::datalog::tests::qb::generic_join(repository, {}, {});
    const auto plan = tyr::datalog::database::compile({ root, root });
    ASSERT_EQ(plan.node_count(), 1);
    EXPECT_EQ(plan.roots().front(), plan.roots().back());
    tyr::datalog::database::QueryEvaluator full(layer(), plan);
    tyr::datalog::database::incremental::QueryEvaluator incremental(layer(), plan);
    const std::array<ygg::FreeBuilderRelationView, 0> no_inputs;
    full.evaluate(no_inputs);
    incremental.initialize(no_inputs);
    ASSERT_EQ(full.get_result().size(), 1);
    EXPECT_TRUE(full.get_result().contains(std::array<ygg::Value, 0> {}));
    EXPECT_EQ(rows(full.get_result()), rows(incremental.get_result()));
    incremental.update(std::array<Change, 0> {});
    EXPECT_EQ(rows(full.get_result()), rows(incremental.get_result(1)));
    EXPECT_TRUE(incremental.get_delta().added.empty());
    EXPECT_TRUE(incremental.get_delta().removed.empty());
}

TEST(DatabaseQuery, InterningUsesArgumentsAndCanonicalOrders)
{
    auto repository = tyr::datalog::tests::qb::repository();
    const auto input = tyr::datalog::tests::qb::input(repository, 0, { x, y });
    EXPECT_EQ(tyr::datalog::tests::qb::input(repository, 0, { x, y }).index(), input.index());
    const auto project = tyr::datalog::tests::qb::project(repository, input, { x, y });
    EXPECT_EQ(tyr::datalog::tests::qb::project(repository, input, { x, y }).index(), project.index());
    EXPECT_NE(tyr::datalog::tests::qb::project(repository, input, { y, x }).index(), project.index());
    const auto renamed = tyr::datalog::tests::qb::rename(repository, input, { x, y });
    EXPECT_NE(tyr::datalog::tests::qb::rename(repository, input, { y, x }).index(), renamed.index());
    const auto joined = tyr::datalog::tests::qb::generic_join(repository, { input }, { x, y });
    EXPECT_EQ(tyr::datalog::tests::qb::generic_join(repository, { input }, { x, y }, { x, y }).index(), joined.index());
    EXPECT_NE(tyr::datalog::tests::qb::generic_join(repository, { input }, { y, x }, { x, y }).index(), joined.index());
    EXPECT_NE(tyr::datalog::tests::qb::generic_join(repository, { input }, { x, y }, { y, x }).index(), joined.index());
}

TEST(DatabaseQuery, CachedPlansSurviveRepositoryGrowthAndCistaRelocation)
{
    auto repository = tyr::datalog::tests::qb::repository();
    const auto a = tyr::datalog::tests::qb::input(repository, 0, { x, y });
    const auto b = tyr::datalog::tests::qb::input(repository, 1, { y, z });
    const auto joined = tyr::datalog::tests::qb::join(repository, a, b);
    const auto projected = tyr::datalog::tests::qb::project(repository, joined, { z, x });
    const auto generic = tyr::datalog::tests::qb::generic_join(repository, { a, b }, { z, y, x }, { x, z, y });
    const auto distance = tyr::datalog::tests::qb::distance(repository,
                                                            tyr::datalog::tests::qb::project(repository, a, { x }),
                                                            a,
                                                            tyr::datalog::tests::qb::project(repository, b, { z }),
                                                            d);
    const auto view = tyr::datalog::database::as<tyr::datalog::database::QueryJoinTag>(joined);
    const auto* original = &view.data();
    for (size_t slot = 2; slot < 260; ++slot)
        tyr::datalog::tests::qb::join(repository, tyr::datalog::tests::qb::input(repository, slot, { x, y }), b);
    EXPECT_EQ(&view.data(), original);
    EXPECT_TRUE(std::ranges::equal(view.data().plan.output_columns(), joined.columns()));

    const auto round_trip = []<typename Concrete>(Concrete query)
    {
        using Record = std::remove_cvref_t<decltype(query.data())>;
        auto original_bytes = cista::serialize(query.data());
        auto relocated = original_bytes;
        original_bytes.clear();
        original_bytes.shrink_to_fit();
        const auto* restored = cista::deserialize<Record>(relocated);
        EXPECT_TRUE(std::ranges::equal(restored->plan.output_columns(), query.columns()));
        if constexpr (std::same_as<Concrete, tyr::datalog::database::QueryView<tyr::datalog::database::QueryGenericJoinTag>>)
        {
            const auto& expected = query.data().plan;
            EXPECT_TRUE(std::ranges::equal(restored->plan.variable_order(), expected.variable_order()));
            for (size_t input = 0; input < expected.input_count(); ++input)
            {
                EXPECT_TRUE(std::ranges::equal(restored->plan.input_columns(input), expected.input_columns(input)));
                EXPECT_EQ(restored->plan.input_keys(input).size(), expected.input_keys(input).size());
            }
            for (size_t depth = 0; depth < expected.variable_order().size(); ++depth)
                EXPECT_TRUE(std::ranges::equal(restored->plan.variable_inputs(depth), expected.variable_inputs(depth)));
        }
    };
    round_trip(view);
    round_trip(tyr::datalog::database::as<tyr::datalog::database::QueryProjectTag>(projected));
    round_trip(tyr::datalog::database::as<tyr::datalog::database::QueryGenericJoinTag>(generic));
    round_trip(tyr::datalog::database::as<tyr::datalog::database::QueryDistanceTag>(distance));
}

TEST(DatabaseQuery, FactoriesRejectMalformedQueriesBeforePublishingPlans)
{
    auto repository = tyr::datalog::tests::qb::repository();
    auto other = tyr::datalog::tests::qb::repository();
    const auto input = tyr::datalog::tests::qb::input(repository, 0, { x });
    const auto foreign = tyr::datalog::tests::qb::input(other, 0, { x });
    EXPECT_THROW(tyr::datalog::database::compile({ input, foreign }), std::invalid_argument);
    EXPECT_THROW(tyr::datalog::tests::qb::generic_join(repository, { input }, { y }), std::invalid_argument);
    EXPECT_THROW(tyr::datalog::tests::qb::generic_join(repository, { input }, { x, x }), std::invalid_argument);
    EXPECT_THROW(tyr::datalog::tests::qb::generic_join(repository, { input }, { x }, { y }), std::invalid_argument);
    EXPECT_THROW(tyr::datalog::tests::qb::select_encoded(repository, input, x, 0, ygg::type_id<double>), std::invalid_argument);
    const auto schema = tyr::datalog::tests::qb::columns_of({ x });
    urd::Store<urd::sequence<ygg::ColumnLayout>> malformed(schema.begin(), schema.end());
    malformed[0].type = static_cast<ygg::TypeId>(std::variant_size_v<ygg::ColumnValue>);
    EXPECT_THROW(tyr::datalog::tests::qb::input(repository, 1, malformed), std::invalid_argument);
    EXPECT_THROW(tyr::datalog::tests::qb::input(repository, 0, malformed), std::invalid_argument);
    const auto plan = tyr::datalog::database::compile({ input });
    EXPECT_THROW(plan[urd::index<tyr::datalog::database::Query<>>(99)], std::out_of_range);
    using Typed = decltype(tyr::datalog::database::as<tyr::datalog::database::QueryInputTag>(plan[plan.roots().front()]));
    static_assert(std::is_const_v<std::remove_reference_t<decltype(std::declval<const Typed&>().data())>>);
    EXPECT_EQ(repository.size(), 1);
}
}  // namespace
