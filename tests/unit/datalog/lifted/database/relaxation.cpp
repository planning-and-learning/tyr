#include "query_helpers.hpp"

#include <algorithm>
#include <array>
#include <gtest/gtest.h>
#include <limits>
#include <random>
#include <tyr/datalog/lifted/database/semantics/bound/query_evaluation.hpp>
#include <tyr/datalog/lifted/database/semantics/operations.hpp>
#include <tyr/datalog/lifted/database/semantics/query_evaluation.hpp>
#include <tyr/datalog/lifted/database/semantics/relation_view.hpp>
#include <tyr/datalog/lifted/database/syntax/formatter.hpp>
#include <tyr/datalog/lifted/database/syntax/translations/relaxation.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <vector>
#include <yggdrasil/formalism/relation_view.hpp>

namespace
{
using Column = urd::index<ygg::Column>;
using Relation = urd::builder<ygg::Relation<ygg::Schema>>;
using View = tyr::datalog::database::QueryView<>;
namespace qb = tyr::datalog::tests::qb;
const Column x(0), y(1), z(2);

/// The layer the tests read their builders through.
const ygg::FreeRelationRepository& layer()
{
    static const ygg::FreeRelationRepository repository;
    return repository;
}

ygg::FreeBuilderRelationView borrow(const Relation& relation) { return layer().view(relation); }

/// Evaluates a query over the given inputs.
Relation evaluate(View query, const urd::Store<urd::sequence<const Relation*>>& inputs)
{
    auto bindings = urd::Store<urd::sequence<ygg::FreeBuilderRelationView>>();
    for (const auto* input : inputs)
        bindings.push_back(borrow(*input));
    auto evaluator = tyr::datalog::database::QueryEvaluator(layer(), tyr::datalog::database::compile({ query }));
    evaluator.evaluate(bindings);
    auto result = Relation(evaluator.get_result().columns());
    tyr::datalog::database::assign(result, evaluator.get_result());
    return result;
}

struct Relaxed
{
    tyr::datalog::database::QueryRepository repository = qb::repository();
    tyr::datalog::database::Relaxation relaxation;
};

std::unique_ptr<Relaxed> relax(std::span<const View> roots, const std::array<bool, 4>& known)
{
    auto result = std::make_unique<Relaxed>();
    result->relaxation = tyr::datalog::database::relax(roots, [&](size_t slot) { return known[slot]; }, result->repository, qb::builder());
    return result;
}
std::unique_ptr<Relaxed> relax(View query, const std::array<bool, 4>& known) { return relax(std::span<const View>(&query, 1), known); }

/// Q(I) ⊆ Q̂(I) × U^(free labels): Q's rows projected onto Q̂'s labels are rows of Q̂.
void expect_superset(View query, const tyr::datalog::database::Relaxation& relaxation, size_t root, const std::array<Relation, 4>& inputs)
{
    const auto& relaxed_query = relaxation.queries[root];
    if (!relaxed_query)
        return;
    const auto full = evaluate(query, { &inputs[0], &inputs[1], &inputs[2], &inputs[3] });
    auto known = urd::Store<urd::sequence<const Relation*>>();
    for (const auto slot : relaxation.slots)
        known.push_back(&inputs[slot]);
    const auto relaxed = evaluate(*relaxed_query, known);
    auto labels = urd::Store<urd::sequence<Column>>();
    const auto schema = query.columns();
    for (const auto& column : relaxed_query->columns())
    {
        labels.push_back(column.label);
        EXPECT_TRUE(std::ranges::find(schema, column.label, &ygg::ColumnLayout::label) != schema.end());
    }
    const auto projected = tyr::datalog::tests::ops::projected(borrow(full), std::span<const Column>(labels));
    for (size_t i = 0; i < projected.size(); ++i)
        EXPECT_TRUE(relaxed.contains(borrow(projected).row(i)));
}

TEST(DatabaseRelaxation, RandomExpressionsAreOverApproximatedAndExactWhenKnown)
{
    auto queries = qb::repository();
    const auto a = qb::input(queries, 0, { x, y });
    const auto b = qb::input(queries, 1, { x, y });
    const auto c = qb::input(queries, 2, { y, z });
    const auto e = qb::input(queries, 3, { x, z });
    const auto leaves = std::array { a, b, c, e };
    auto inputs = std::array<Relation, 4> { Relation(tyr::datalog::tests::ops::cols({ x, y })),
                                            Relation(tyr::datalog::tests::ops::cols({ x, y })),
                                            Relation(tyr::datalog::tests::ops::cols({ y, z })),
                                            Relation(tyr::datalog::tests::ops::cols({ x, z })) };
    // NOLINTNEXTLINE(bugprone-random-generator-seed): a fixed seed keeps the test reproducible
    auto random = std::mt19937(4711);
    const auto labels_of = [](View query)
    {
        auto result = urd::Store<urd::sequence<Column>>();
        for (const auto& column : query.columns())
            result.push_back(column.label);
        return result;
    };
    const auto peer = [&](View query)
    {
        const auto target = labels_of(query);
        auto result = target.size() == 3 ? qb::join(queries, a, c) : leaves[random() % leaves.size()];
        auto columns = labels_of(result);
        columns.resize(target.size());
        result = qb::project(queries, result, std::span<const Column>(columns));
        return qb::rename(queries, result, std::span<const Column>(target));
    };
    for (size_t expression = 0; expression < 128; ++expression)
    {
        auto root = leaves[random() % leaves.size()];
        // Exactness is lost once a projection eliminates labels a difference or antijoin below it shares.
        auto negated = false;
        auto exact = true;
        for (size_t depth = 0; depth < 4; ++depth)
        {
            auto columns = labels_of(root);
            const auto operation = random() % 9;
            negated = negated || operation == 6 || operation == 8;
            exact = exact && !(operation == 1 && negated);
            switch (operation)
            {
                case 0:
                    root = qb::join(queries, root, leaves[random() % leaves.size()]);
                    break;
                case 1:
                    std::shuffle(columns.begin(), columns.end(), random);
                    columns.resize(random() % (columns.size() + 1));
                    root = qb::project(queries, root, std::span<const Column>(columns));
                    break;
                case 2:
                {
                    auto names = std::array { x, y, z };
                    std::shuffle(names.begin(), names.end(), random);
                    root = qb::rename(queries, root, std::span<const Column>(names.data(), columns.size()));
                    break;
                }
                case 3:
                    if (!columns.empty())
                        root = qb::select_equal(queries, root, columns[random() % columns.size()], columns[random() % columns.size()]);
                    break;
                case 4:
                    if (!columns.empty())
                        root = qb::select_value(queries, root, columns[random() % columns.size()], static_cast<ygg::uint_t>(random() % 4));
                    break;
                case 5:
                    root = qb::union_(queries, root, peer(root));
                    break;
                case 6:
                    root = qb::difference(queries, root, peer(root));
                    break;
                case 7:
                    root = qb::semijoin(queries, root, leaves[random() % leaves.size()]);
                    break;
                case 8:
                    root = qb::antijoin(queries, root, leaves[random() % leaves.size()]);
                    break;
                default:
                    ADD_FAILURE() << "no operation " << operation;
                    break;
            }
        }
        SCOPED_TRACE(expression);
        SCOPED_TRACE(tyr::datalog::database::explain(tyr::datalog::database::compile({ root })));
        for (auto& input : inputs)
        {
            input.clear();
            for (ygg::uint_t left = 0; left < 4; ++left)
                for (ygg::uint_t right = 0; right < 4; ++right)
                    if (random() % 3 != 0)
                        input.insert(std::array { ygg::encode_value(left), ygg::encode_value(right) });
        }
        const auto known =
            std::array { static_cast<bool>(random() % 2), static_cast<bool>(random() % 2), static_cast<bool>(random() % 2), static_cast<bool>(random() % 2) };
        // The root and a projection of it onto random labels, relaxed together.
        auto kept = labels_of(root);
        std::shuffle(kept.begin(), kept.end(), random);
        kept.resize(random() % (kept.size() + 1));
        const auto roots = std::array { root, qb::project(queries, root, std::span<const Column>(kept)) };
        const auto partial = relax(roots, known);
        expect_superset(roots[0], partial->relaxation, 0, inputs);
        expect_superset(roots[1], partial->relaxation, 1, inputs);

        if (!exact)
            continue;
        const auto all_known = relax(root, { true, true, true, true });
        ASSERT_TRUE(all_known->relaxation.queries[0].has_value());
        EXPECT_TRUE(std::ranges::equal(all_known->relaxation.queries[0].value().columns() | std::views::transform(&ygg::ColumnLayout::label), labels_of(root)));
        auto all = urd::Store<urd::sequence<const Relation*>>();
        for (const auto slot : all_known->relaxation.slots)
            all.push_back(&inputs[slot]);
        const auto expected = evaluate(root, { &inputs[0], &inputs[1], &inputs[2], &inputs[3] });
        const auto actual = evaluate(all_known->relaxation.queries[0].value(), all);
        EXPECT_EQ(actual.size(), expected.size());
        for (size_t i = 0; i < expected.size(); ++i)
            EXPECT_TRUE(actual.contains(borrow(expected).row(i)));
    }
}

TEST(DatabaseRelaxation, UnknownInputsDropConstraints)
{
    auto queries = qb::repository();
    const auto known_edges = qb::input(queries, 0, { x, y });
    const auto unknown = qb::input(queries, 1, { y, z });
    const auto blocked = qb::input(queries, 2, { x, y });
    const auto both_known = std::array { true, false, true, true };

    // An unknown conjunct drops out of a join; its label z is unconstrained.
    const auto joined = relax(qb::join(queries, known_edges, unknown), both_known);
    ASSERT_TRUE(joined->relaxation.queries[0].has_value());
    EXPECT_TRUE(std::ranges::equal(joined->relaxation.queries[0].value().columns() | std::views::transform(&ygg::ColumnLayout::label),
                                   (urd::Store<urd::sequence<Column>> { x, y })));
    EXPECT_EQ(joined->relaxation.slots, (urd::Store<urd::sequence<size_t>> { 0 }));

    // A union with an unknown side constrains nothing.
    const auto renamed = qb::rename(queries, unknown, { x, y });
    EXPECT_FALSE(relax(qb::union_(queries, known_edges, renamed), both_known)->relaxation.queries[0].has_value());

    // A semijoin with an unknown lhs still constrains the shared label y by its known rhs: ⊤ ⋉ B̂ = B̂ on y.
    const auto semi = relax(qb::semijoin(queries, unknown, known_edges), both_known);
    ASSERT_TRUE(semi->relaxation.queries[0].has_value());
    EXPECT_TRUE(std::ranges::equal(semi->relaxation.queries[0].value().columns() | std::views::transform(&ygg::ColumnLayout::label),
                                   (urd::Store<urd::sequence<Column>> { y })));

    // A known antijoin rhs stays: the result is exact.
    const auto anti = relax(qb::antijoin(queries, known_edges, blocked), both_known);
    ASSERT_TRUE(anti->relaxation.queries[0].has_value());
    EXPECT_TRUE(tyr::datalog::database::is<tyr::datalog::database::QueryAntijoinTag>(anti->relaxation.queries[0].value()));

    // A selection on an unconstrained column is dropped.
    const auto selected = relax(qb::select_value(queries, qb::join(queries, known_edges, unknown), z, static_cast<ygg::uint_t>(1)), both_known);
    ASSERT_TRUE(selected->relaxation.queries[0].has_value());
    EXPECT_TRUE(tyr::datalog::database::is<tyr::datalog::database::QueryJoinTag>(selected->relaxation.queries[0].value())
                || tyr::datalog::database::is<tyr::datalog::database::QueryInputTag>(selected->relaxation.queries[0].value()));
}

TEST(DatabaseRelaxation, ProjectedJoinEliminatesUnneededConjunctsAndAntijoins)
{
    // π_{x,y}((E(x,y) ⋈ I(u,v)) ▷ I(y,u)): I(u,v) shares nothing needed, and the antijoin filters on u, which is
    // eliminated, so the relaxation is E with I(u,v) reduced to an existence test, without a product.
    auto queries = qb::repository();
    const Column u(3), v(4);
    const auto edges = qb::input(queries, 0, { x, y });
    const auto identity = qb::input(queries, 1, { u, v });
    const auto blocked = qb::rename(queries, identity, { y, u });
    const auto walk = qb::antijoin(queries, qb::join(queries, edges, identity), blocked);
    const auto pattern = qb::project(queries, walk, { x, y });
    const auto relaxed = relax(pattern, { true, true, true, true });
    ASSERT_TRUE(relaxed->relaxation.queries[0].has_value());
    const auto plan = tyr::datalog::database::compile({ relaxed->relaxation.queries[0].value() });
    for (size_t i = 0; i < plan.node_count(); ++i)
    {
        const auto node = plan[urd::index<tyr::datalog::database::Query<>>(ygg::to_uint_t(i))];
        EXPECT_FALSE(tyr::datalog::database::is<tyr::datalog::database::QueryAntijoinTag>(node));
        // Every join has a nullary side: the existence test of I.
        if (tyr::datalog::database::is<tyr::datalog::database::QueryJoinTag>(node))
        {
            const auto join = tyr::datalog::database::as<tyr::datalog::database::QueryJoinTag>(node);
            EXPECT_TRUE(join.get_lhs().columns().empty() || join.get_rhs().columns().empty());
        }
    }

    // An antijoin whose shared labels are all needed stays exact.
    const auto kept = relax(qb::antijoin(queries, edges, qb::rename(queries, identity, { x, y })), { true, true, true, true });
    ASSERT_TRUE(kept->relaxation.queries[0].has_value());
    EXPECT_TRUE(tyr::datalog::database::is<tyr::datalog::database::QueryAntijoinTag>(kept->relaxation.queries[0].value()));
}

TEST(DatabaseRelaxation, RootsShareOneInputNumbering)
{
    auto queries = qb::repository();
    const auto a = qb::input(queries, 0, { x, y });
    const auto c = qb::input(queries, 2, { y, z });
    const auto joined = qb::join(queries, a, c);
    const auto roots = std::array { qb::project(queries, joined, { x }), qb::project(queries, c, { z }) };
    const auto relaxed = relax(roots, { true, false, true, false });
    ASSERT_TRUE(relaxed->relaxation.queries[0].has_value() && relaxed->relaxation.queries[1].has_value());
    EXPECT_EQ(relaxed->relaxation.slots, (urd::Store<urd::sequence<size_t>> { 0, 2 }));
    auto inputs = std::array<Relation, 4> { Relation(tyr::datalog::tests::ops::cols({ x, y })),
                                            Relation(tyr::datalog::tests::ops::cols({ x, y })),
                                            Relation(tyr::datalog::tests::ops::cols({ y, z })),
                                            Relation(tyr::datalog::tests::ops::cols({ x, z })) };
    inputs[0].insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(1)), ygg::encode_value(static_cast<ygg::uint_t>(2)) });
    inputs[2].insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(2)), ygg::encode_value(static_cast<ygg::uint_t>(3)) });
    const auto plan = tyr::datalog::database::compile({ relaxed->relaxation.queries[0].value(), relaxed->relaxation.queries[1].value() });
    auto evaluator = tyr::datalog::database::QueryEvaluator(layer(), plan);
    evaluator.evaluate(std::array { borrow(inputs[0]), borrow(inputs[2]) });
    EXPECT_EQ(evaluator.get_result(0).size(), 1);
    EXPECT_EQ(evaluator.get_result(1).size(), 1);
}
TEST(DatabaseRelaxation, ProjectedAwayConjunctsAreTestedOnceByBoundEvaluation)
{
    // π_{x,y}(E(x,y) ⋈ I(u,v)) with y bound: I(u,v) is an existence test, so each predecessor x is emitted once, not
    // once per row of I.
    auto queries = qb::repository();
    const Column u(3), v(4);
    const auto query = qb::project(queries, qb::join(queries, qb::input(queries, 0, { x, y }), qb::input(queries, 1, { u, v })), { x, y });
    constexpr auto objects = static_cast<ygg::uint_t>(50);
    auto edges = Relation(tyr::datalog::tests::ops::cols({ x, y }));
    auto identity = Relation(tyr::datalog::tests::ops::cols({ u, v }));
    for (ygg::uint_t o = 0; o < objects; ++o)
    {
        edges.insert(std::array { ygg::encode_value(o), ygg::encode_value(static_cast<ygg::uint_t>(0)) });
        identity.insert(std::array { ygg::encode_value(o), ygg::encode_value(o) });
    }
    auto evaluator = tyr::datalog::database::bound::QueryEvaluator(layer(), query);
    evaluator.bind_slot(0, borrow(edges));
    evaluator.bind_slot(1, borrow(identity));
    const auto predecessors = evaluator.prepare({ 1 });
    auto emitted = urd::Store<urd::sequence<ygg::uint_t>>();
    evaluator.evaluate(predecessors,
                       std::array { ygg::encode_value(static_cast<ygg::uint_t>(0)) },
                       [&](std::span<const ygg::Value> row)
                       {
                           emitted.push_back(ygg::decode_value<ygg::uint_t>(row[0]));
                           return true;
                       });
    std::ranges::sort(emitted);
    auto expected = urd::Store<urd::sequence<ygg::uint_t>>();
    for (ygg::uint_t o = 0; o < objects; ++o)
        expected.push_back(o);
    EXPECT_EQ(emitted, expected);
}

/// The y of each row a bound evaluation of `query` (columns x, y) emits for x = 0, in order; on_row asks to stop after
/// `limit` rows.
urd::Store<urd::sequence<ygg::uint_t>>
bound_successors(View query, const urd::Store<urd::sequence<const Relation*>>& inputs, size_t limit = std::numeric_limits<size_t>::max())
{
    auto evaluator = tyr::datalog::database::bound::QueryEvaluator(layer(), query);
    for (size_t slot = 0; slot < inputs.size(); ++slot)
        evaluator.bind_slot(slot, borrow(*inputs[slot]));
    const auto successors = evaluator.prepare({ 0 });
    auto emitted = urd::Store<urd::sequence<ygg::uint_t>>();
    evaluator.evaluate(successors,
                       std::array { ygg::encode_value(static_cast<ygg::uint_t>(0)) },
                       [&](std::span<const ygg::Value> row)
                       {
                           emitted.push_back(ygg::decode_value<ygg::uint_t>(row[1]));
                           return emitted.size() < limit;
                       });
    return emitted;
}

TEST(DatabaseRelaxation, VariablesReadByLaterFiltersAreNotExistenceTests)
{
    // E(0, y) has 10 rows and I(u, v) 3, so I is planned first and binds u, which only a later filter reads. A
    // step that stopped at I's first row would lose the other rows.
    auto queries = qb::repository();
    const Column u(3), v(4);
    const auto edges = qb::input(queries, 0, { x, y });
    const auto identity = qb::input(queries, 1, { u, v });
    auto e = Relation(tyr::datalog::tests::ops::cols({ x, y }));
    auto i = Relation(tyr::datalog::tests::ops::cols({ u, v }));
    auto blocked = Relation(tyr::datalog::tests::ops::cols({ x }));
    for (ygg::uint_t w = 0; w < 10; ++w)
        e.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(0)), ygg::encode_value(w) });
    for (ygg::uint_t w = 5; w < 8; ++w)
        i.insert(std::array { ygg::encode_value(w), ygg::encode_value(static_cast<ygg::uint_t>(0)) });
    blocked.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(5)) });

    // A later select_equal reads u.
    const auto equal = qb::project(queries, qb::select_equal(queries, qb::join(queries, edges, identity), u, y), { x, y });
    auto successors = bound_successors(equal, { &e, &i });
    std::ranges::sort(successors);
    EXPECT_EQ(successors, (urd::Store<urd::sequence<ygg::uint_t>> { 5, 6, 7 }));

    // A later antijoin reads u as an outer variable; it rejects I's first row, u = 5. I binds u, so it may repeat rows.
    const auto anti =
        qb::project(queries, qb::antijoin(queries, qb::join(queries, edges, identity), qb::rename(queries, qb::input(queries, 2, { x }), { u })), { x, y });
    successors = bound_successors(anti, { &e, &i, &blocked });
    std::ranges::sort(successors);
    const auto [first, last] = std::ranges::unique(successors);
    successors.erase(first, last);
    EXPECT_EQ(successors.size(), 10);
}

TEST(DatabaseRelaxation, ExistenceStepsSkipRowsTheirOwnFiltersReject)
{
    // π_{x,y}(E(x,y) ⋈ σ_{u=7} I(u,v)): I is an existence test whose first rows fail u = 7. It must still find a
    // matching row, and only one: (7, 0) and (7, 1) both match.
    auto queries = qb::repository();
    const Column u(3), v(4);
    const auto query = qb::project(
        queries,
        qb::join(queries, qb::input(queries, 0, { x, y }), qb::select_value(queries, qb::input(queries, 1, { u, v }), u, static_cast<ygg::uint_t>(7))),
        { x, y });
    auto e = Relation(tyr::datalog::tests::ops::cols({ x, y }));
    auto i = Relation(tyr::datalog::tests::ops::cols({ u, v }));
    for (ygg::uint_t w = 0; w < 10; ++w)
        e.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(0)), ygg::encode_value(w) });
    for (const auto& row : { std::array { ygg::encode_value(static_cast<ygg::uint_t>(5)), ygg::encode_value(static_cast<ygg::uint_t>(0)) },
                             std::array { ygg::encode_value(static_cast<ygg::uint_t>(6)), ygg::encode_value(static_cast<ygg::uint_t>(0)) },
                             std::array { ygg::encode_value(static_cast<ygg::uint_t>(7)), ygg::encode_value(static_cast<ygg::uint_t>(0)) },
                             std::array { ygg::encode_value(static_cast<ygg::uint_t>(7)), ygg::encode_value(static_cast<ygg::uint_t>(1)) } })
        i.insert(row);
    EXPECT_EQ(bound_successors(query, { &e, &i }).size(), 10);
}

TEST(DatabaseRelaxation, StopInsideAnExistenceStepEndsTheEvaluation)
{
    // E(0, y) has 5 rows and I(u, v) 50, so E is planned first and I is an existence test inside it. on_row stops at
    // the first row; the stop must not end only the existence step.
    auto queries = qb::repository();
    const Column u(3), v(4);
    const auto query = qb::project(queries, qb::join(queries, qb::input(queries, 0, { x, y }), qb::input(queries, 1, { u, v })), { x, y });
    auto e = Relation(tyr::datalog::tests::ops::cols({ x, y }));
    auto i = Relation(tyr::datalog::tests::ops::cols({ u, v }));
    for (ygg::uint_t w = 0; w < 5; ++w)
        e.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(0)), ygg::encode_value(w) });
    for (ygg::uint_t w = 0; w < 50; ++w)
        i.insert(std::array { ygg::encode_value(w), ygg::encode_value(w) });
    EXPECT_EQ(bound_successors(query, { &e, &i }, 1).size(), 1);
    EXPECT_EQ(bound_successors(query, { &e, &i }).size(), 5);
}

TEST(DatabaseRelaxation, SharedSubqueriesKeepTheRequestedColumnOrder)
{
    // π_y σ_{x=0} R relaxes R with the labels (y, x) first; the root R must still get R's relaxation as (x, y).
    auto queries = qb::repository();
    const auto r = qb::input(queries, 0, { x, y });
    const auto roots = std::array { qb::project(queries, qb::select_value(queries, r, x, static_cast<ygg::uint_t>(0)), { y }), r };
    const auto relaxed = relax(roots, { true, true, true, true });
    ASSERT_TRUE(relaxed->relaxation.queries[1].has_value());
    EXPECT_TRUE(std::ranges::equal(relaxed->relaxation.queries[1].value().columns() | std::views::transform(&ygg::ColumnLayout::label),
                                   (urd::Store<urd::sequence<Column>> { x, y })));
}

/// Whether the root's relaxation, evaluated on the known inputs, equals `expected` evaluated on all of them.
void expect_relaxes_to(View query, View expected, const std::array<bool, 4>& known, const std::array<Relation, 4>& inputs)
{
    const auto relaxed = relax(query, known);
    ASSERT_TRUE(relaxed->relaxation.queries[0].has_value());
    auto bound = urd::Store<urd::sequence<const Relation*>>();
    for (const auto slot : relaxed->relaxation.slots)
        bound.push_back(&inputs[slot]);
    const auto actual = evaluate(relaxed->relaxation.queries[0].value(), bound);
    const auto wanted = evaluate(expected, { &inputs[0], &inputs[1], &inputs[2], &inputs[3] });
    EXPECT_EQ(actual.size(), wanted.size());
    for (size_t i = 0; i < wanted.size(); ++i)
        EXPECT_TRUE(actual.contains(borrow(wanted).row(i)));
}

std::array<Relation, 4> sample_inputs()
{
    auto inputs = std::array<Relation, 4> { Relation(tyr::datalog::tests::ops::cols({ x, y })),
                                            Relation(tyr::datalog::tests::ops::cols({ x, y })),
                                            Relation(tyr::datalog::tests::ops::cols({ x, y })),
                                            Relation(tyr::datalog::tests::ops::cols({ y, z })) };
    // NOLINTNEXTLINE(bugprone-random-generator-seed): a fixed seed keeps the test reproducible
    auto random = std::mt19937(17);
    for (auto& input : inputs)
        for (ygg::uint_t left = 0; left < 4; ++left)
            for (ygg::uint_t right = 0; right < 4; ++right)
                if (random() % 2 != 0)
                    input.insert(std::array { ygg::encode_value(left), ygg::encode_value(right) });
    return inputs;
}

TEST(DatabaseRelaxation, MixedSubtrahendsKeepTheirKnownPart)
{
    // A subtrahend is under-approximated: its known part is subtracted, its unknown part is not.
    auto queries = qb::repository();
    const auto a = qb::input(queries, 0, { x, y });
    const auto b = qb::input(queries, 1, { x, y });
    const auto c = qb::input(queries, 2, { x, y });
    const auto known = std::array { true, true, false, true };
    const auto inputs = sample_inputs();
    expect_relaxes_to(qb::difference(queries, a, qb::union_(queries, b, c)), qb::difference(queries, a, b), known, inputs);
    expect_relaxes_to(qb::antijoin(queries, a, qb::union_(queries, b, c)), qb::antijoin(queries, a, b), known, inputs);
    // Nothing of B ⋈ C is certain, so nothing is subtracted.
    expect_relaxes_to(qb::difference(queries, a, qb::join(queries, b, c)), a, known, inputs);
}

TEST(DatabaseRelaxation, NestedSubtrahendsFlipPolarityTwice)
{
    // A \ (B \ π_{x,y}(C ⋈ E)) with E unknown: the inner subtrahend is over-approximated by C, so the outer one is
    // under-approximated by B \ C. With C unknown too, nothing of the outer subtrahend is certain.
    auto queries = qb::repository();
    const auto a = qb::input(queries, 0, { x, y });
    const auto b = qb::input(queries, 1, { x, y });
    const auto c = qb::input(queries, 2, { x, y });
    const auto e = qb::input(queries, 3, { y, z });
    const auto inner = qb::project(queries, qb::join(queries, c, e), { x, y });
    const auto query = qb::difference(queries, a, qb::difference(queries, b, inner));
    const auto inputs = sample_inputs();
    expect_relaxes_to(query, qb::difference(queries, a, qb::difference(queries, b, c)), { true, true, true, false }, inputs);
    expect_relaxes_to(query, a, { true, true, false, false }, inputs);
}

}  // namespace
