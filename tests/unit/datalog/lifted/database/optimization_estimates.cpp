#include "query_helpers.hpp"

#include <array>
#include <gtest/gtest.h>
#include <tyr/datalog/lifted/database/optimization/optimization.hpp>
#include <urd/repr/maps/unordered_flat_map.hpp>
#include <urd/store.hpp>

namespace
{
using Column = urd::index<ygg::Column>;
using PlanId = urd::index<tyr::datalog::database::Query<>>;
const Column x(0), y(1), z(2);

// Exercises System R estimation independently of planning: the same conjunction
// must have one estimate whatever its join and filter association.
auto estimate(tyr::datalog::database::QueryView<> root, const tyr::datalog::database::Statistics& statistics)
{
    urd::Store<urd::map<PlanId, tyr::datalog::database::detail::Estimate>> estimates;
    for (const auto query : tyr::datalog::database::reachable(std::span<const tyr::datalog::database::QueryView<>>(&root, 1)))
        estimates[query.index()] = tyr::datalog::database::detail::estimate(
            query,
            query.index(),
            [&](tyr::datalog::database::QueryView<> child) { return estimates.at(child.index()); },
            statistics);
    return estimates.at(root.index());
}

void same_estimate(const auto& lhs, const auto& rhs)
{
    EXPECT_NEAR(lhs.rows, rhs.rows, 1e-9 * std::max(1.0, lhs.rows));
    ASSERT_EQ(lhs.distinct.size(), rhs.distinct.size());
    for (const auto& [column, count] : lhs.distinct)
        EXPECT_NEAR(count, rhs.distinct.at(column), 1e-9 * std::max(1.0, count));
}

TEST(DatabaseOptimizationEstimates, JoinAndFilterAssociationHaveOneNormalizedEstimate)
{
    auto queries = tyr::datalog::tests::qb::repository();
    const auto a = tyr::datalog::tests::qb::input(queries, 0, { x, y });
    const auto b = tyr::datalog::tests::qb::input(queries, 1, { y, z });
    const auto c = tyr::datalog::tests::qb::input(queries, 2, { x, z });
    tyr::datalog::database::Statistics statistics;
    statistics.inputs[0] = { 100, { { x, 10 }, { y, 20 } } };
    statistics.inputs[1] = { 200, { { y, 20 }, { z, 10 } } };
    statistics.inputs[2] = { 50, { { x, 10 }, { z, 10 } } };

    const auto left = tyr::datalog::tests::qb::join(queries, tyr::datalog::tests::qb::join(queries, a, b), c);
    const auto right = tyr::datalog::tests::qb::join(queries, a, tyr::datalog::tests::qb::join(queries, b, c));
    const auto reordered = tyr::datalog::tests::qb::join(queries, c, tyr::datalog::tests::qb::join(queries, b, a));
    const auto expected = estimate(left, statistics);
    EXPECT_NEAR(expected.rows, 500, 1e-9);
    same_estimate(expected, estimate(right, statistics));
    same_estimate(expected, estimate(reordered, statistics));

    const auto above = tyr::datalog::tests::qb::select_value(queries, left, x, static_cast<ygg::uint_t>(7));
    const auto below = tyr::datalog::tests::qb::join(
        queries,
        tyr::datalog::tests::qb::join(queries, tyr::datalog::tests::qb::select_value(queries, a, x, static_cast<ygg::uint_t>(7)), b),
        c);
    const auto repeated = tyr::datalog::tests::qb::select_value(queries, below, x, static_cast<ygg::uint_t>(7));
    EXPECT_NEAR(estimate(above, statistics).rows, 50, 1e-9);
    same_estimate(estimate(above, statistics), estimate(below, statistics));
    same_estimate(estimate(above, statistics), estimate(repeated, statistics));

    const auto equality_above = tyr::datalog::tests::qb::select_equal(queries, left, x, z);
    const auto equality_below =
        tyr::datalog::tests::qb::join(queries, tyr::datalog::tests::qb::join(queries, a, b), tyr::datalog::tests::qb::select_equal(queries, c, z, x));
    same_estimate(estimate(equality_above, statistics), estimate(equality_below, statistics));
}

TEST(DatabaseOptimizationEstimates, EqualityAndConstantPredicatesAreIdempotentAndDetectConflicts)
{
    auto queries = tyr::datalog::tests::qb::repository();
    const auto input = tyr::datalog::tests::qb::input(queries, 0, { x, y, z });
    tyr::datalog::database::Statistics statistics;
    statistics.inputs[0] = { 1000, { { x, 10 }, { y, 20 }, { z, 25 } } };
    const auto equal = tyr::datalog::tests::qb::select_equal(queries, input, x, y);
    EXPECT_NEAR(estimate(equal, statistics).rows, 50, 1e-9);
    same_estimate(estimate(equal, statistics), estimate(tyr::datalog::tests::qb::select_equal(queries, equal, y, x), statistics));
    same_estimate(estimate(input, statistics), estimate(tyr::datalog::tests::qb::select_equal(queries, input, x, x), statistics));

    const auto transitive = tyr::datalog::tests::qb::select_equal(queries, equal, y, z);
    EXPECT_NEAR(estimate(transitive, statistics).rows, 2, 1e-9);
    same_estimate(estimate(transitive, statistics), estimate(tyr::datalog::tests::qb::select_equal(queries, transitive, z, x), statistics));

    const auto selected = tyr::datalog::tests::qb::select_value(queries, equal, x, static_cast<ygg::uint_t>(7));
    same_estimate(estimate(selected, statistics),
                  estimate(tyr::datalog::tests::qb::select_value(queries, selected, y, static_cast<ygg::uint_t>(7)), statistics));
    const auto conflict = estimate(tyr::datalog::tests::qb::select_value(queries, selected, y, static_cast<ygg::uint_t>(8)), statistics);
    EXPECT_EQ(conflict.rows, 0);
    for (const auto& [column, count] : conflict.distinct)
        EXPECT_EQ(count, 0);
    EXPECT_EQ(
        estimate(tyr::datalog::tests::qb::select_value(queries, tyr::datalog::tests::qb::select_value(queries, input, x, ygg::uint_t(7)), x, ygg::uint_t(8)),
                 statistics)
            .rows,
        0);

    const auto self_join = tyr::datalog::tests::qb::join(queries, input, input);
    same_estimate(estimate(input, statistics), estimate(self_join, statistics));
    const auto alias = tyr::datalog::tests::qb::rename(queries, input, { z, y, x });
    const auto aliased = tyr::datalog::tests::qb::join(queries, input, alias);
    EXPECT_LT(estimate(aliased, statistics).rows, estimate(input, statistics).rows);
}

TEST(DatabaseOptimizationEstimates, ProjectionCreatesAnIndependentScopeForHiddenColumns)
{
    auto queries = tyr::datalog::tests::qb::repository();
    const auto a = tyr::datalog::tests::qb::input(queries, 0, { x, y });
    const auto b = tyr::datalog::tests::qb::input(queries, 1, { y, z });
    tyr::datalog::database::Statistics statistics;
    statistics.inputs[0] = { 100, { { x, 10 }, { y, 20 } } };
    statistics.inputs[1] = { 200, { { y, 20 }, { z, 10 } } };

    const auto x_values = tyr::datalog::tests::qb::project(queries, a, { x });
    const auto hidden_y = tyr::datalog::tests::qb::join(queries, x_values, b);
    EXPECT_NEAR(estimate(hidden_y, statistics).rows, 2000, 1e-9);
    EXPECT_NEAR(estimate(tyr::datalog::tests::qb::join(queries, a, b), statistics).rows, 1000, 1e-9);

    const auto y_values = tyr::datalog::tests::qb::project(queries, a, { y });
    EXPECT_NEAR(estimate(tyr::datalog::tests::qb::join(queries, x_values, y_values), statistics).rows, 200, 1e-9);
    const auto selected_scope = tyr::datalog::tests::qb::project(queries, tyr::datalog::tests::qb::select_equal(queries, a, x, y), { x });
    // Duplicate elimination: min(T/2, V(x)) = min(5/2, 5) rows survive the projection.
    EXPECT_NEAR(estimate(tyr::datalog::tests::qb::join(queries, selected_scope, b), statistics).rows, 500, 1e-9);
    const auto renamed = tyr::datalog::tests::qb::rename(queries, x_values, { y });
    EXPECT_NEAR(estimate(tyr::datalog::tests::qb::join(queries, renamed, b), statistics).rows, 100, 1e-9);
    same_estimate(estimate(a, statistics), estimate(tyr::datalog::tests::qb::project(queries, a, { y, x }), statistics));
}

TEST(DatabaseOptimizationEstimates, ObservationsOverrideDerivedEstimates)
{
    auto queries = tyr::datalog::tests::qb::repository();
    const auto a = tyr::datalog::tests::qb::input(queries, 3, { x, y });
    const auto b = tyr::datalog::tests::qb::input(queries, 8, { y, z });
    const auto join = tyr::datalog::tests::qb::join(queries, a, b);
    tyr::datalog::database::Statistics statistics;
    statistics.inputs[3] = { 100, {} };
    statistics.inputs[8] = { 200, {} };
    EXPECT_NEAR(estimate(join, statistics).rows, 100, 1e-9);
    statistics.expressions[join.index()] = { 7, { { x, 2 }, { y, 3 } } };
    const auto observed = estimate(join, statistics);
    EXPECT_EQ(observed.rows, 7);
    EXPECT_EQ(observed.distinct.at(x), 2);
    EXPECT_EQ(observed.distinct.at(z), 7);
    // A known empty input stays empty.
    statistics.expressions.clear();
    statistics.inputs[3].rows = 0;
    EXPECT_EQ(estimate(join, statistics).rows, 0);
}

TEST(DatabaseOptimizationEstimates, BoundedDomainsCapDistinctCounts)
{
    auto queries = tyr::datalog::tests::qb::repository();
    const auto a = tyr::datalog::tests::qb::input(queries, 0, { x, y });
    const auto b = tyr::datalog::tests::qb::input(queries, 1, { y, z });
    tyr::datalog::database::Statistics statistics;
    statistics.inputs[0] = { 1000, {} };
    statistics.inputs[1] = { 1000, {} };
    EXPECT_NEAR(estimate(tyr::datalog::tests::qb::join(queries, a, b), statistics).rows, 1000, 1e-9);
    statistics.objects = 10;
    EXPECT_NEAR(estimate(a, statistics).distinct.at(y), 10, 1e-9);
    EXPECT_NEAR(estimate(tyr::datalog::tests::qb::join(queries, a, b), statistics).rows, 100000, 1e-9);
    EXPECT_LE(estimate(tyr::datalog::tests::qb::union_(queries, a, a), statistics).distinct.at(x), 10);
}

TEST(DatabaseOptimizationEstimates, FactorIdentitySeparatesInputsAndQueries)
{
    const std::vector<ygg::ColumnLayout> columns { { x, ygg::type_id<ygg::uint_t> } };
    const tyr::datalog::database::RelationStatistics statistics { 10, { { x, 2 } } };
    const auto input = tyr::datalog::database::detail::factor(statistics, columns, tyr::datalog::database::detail::InputFactorIdentity { 7 });
    const auto query = tyr::datalog::database::detail::factor(statistics, columns, tyr::datalog::database::detail::QueryFactorIdentity { 7 });
    const std::array labels { x };
    EXPECT_NEAR(tyr::datalog::database::detail::conjunction(tyr::datalog::database::detail::combine(input, input), labels).rows, 10, 1e-9);
    const auto combined = tyr::datalog::database::detail::conjunction(tyr::datalog::database::detail::combine(input, query), labels);
    EXPECT_EQ(combined.factors.size(), 2);
    EXPECT_NEAR(combined.rows, 50, 1e-9);
}
}  // namespace

namespace
{
TEST(DatabaseOptimizationEstimates, SemijoinAndAntijoinPartitionTheirLhs)
{
    auto queries = tyr::datalog::tests::qb::repository();
    const auto a = tyr::datalog::tests::qb::input(queries, 0, { x, y });
    const auto b = tyr::datalog::tests::qb::input(queries, 1, { y, z });
    tyr::datalog::database::Statistics statistics;
    statistics.inputs[0] = { 100, { { x, 10 }, { y, 20 } } };
    statistics.inputs[1] = { 200, { { y, 20 }, { z, 10 } } };
    const auto semijoin = estimate(tyr::datalog::tests::qb::semijoin(queries, a, b), statistics);
    const auto antijoin = estimate(tyr::datalog::tests::qb::antijoin(queries, a, b), statistics);
    EXPECT_LE(semijoin.rows, 100);
    EXPECT_NEAR(semijoin.rows + antijoin.rows, 100, 1e-9);
    EXPECT_NEAR(semijoin.distinct.at(x), std::min(10.0, semijoin.rows), 1e-9);  // V(R, a), capped by the rows.
}
}  // namespace
