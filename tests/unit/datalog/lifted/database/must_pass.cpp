/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "tyr/datalog/lifted/database/semantics/must_pass.hpp"
#include <urd/repr/sequences/sequence.hpp>
#include <urd/repr/values/unordered_flat_set.hpp>
#include <urd/store.hpp>

#include "query_helpers.hpp"
#include "tyr/datalog/lifted/database/semantics/relation_view.hpp"
#include "yggdrasil/formalism/relation_view.hpp"

#include <gtest/gtest.h>
#include <map>
#include <optional>
#include <random>
#include <set>
#include <tuple>

namespace
{
using Column = urd::index<ygg::Column>;
using Relation = urd::builder<ygg::Relation<ygg::Schema>>;
using ygg::uint_t;
namespace qb = tyr::datalog::tests::qb;
using tyr::datalog::tests::distances::borrow;

const Column key(20), u(0), v(1);

using Pairs = std::set<std::pair<ygg::uint_t, ygg::uint_t>>;  ///< (key, vertex)

const Column probe(30);

/// All objects 0..objects-1 as probes.
Relation all_probes(ygg::uint_t objects)
{
    auto probes = Relation(tyr::datalog::tests::ops::cols({ probe }));
    for (ygg::uint_t object = 0; object < objects; ++object)
        probes.insert(std::array { ygg::encode_value(object) });
    return probes;
}

/// Must-pass objects of keyed edges (key, from..., to...) between keyed sources and targets (key, vertex...).
Pairs must_pass(const Relation& edges, const Relation& sources, const Relation& targets, const Relation& probes)
{
    auto queries = qb::repository();
    const auto query = qb::input(queries, 0, edges.columns());
    const auto plan = tyr::datalog::database::MustPassPlan(sources.columns(), query.columns(), targets.columns(), probes.columns(), 1);
    const ygg::FreeRelationRepository layer;
    auto computation = tyr::datalog::database::MustPass(layer, query, plan);
    auto out = Relation(plan.output_columns());
    const auto view = borrow(layer, edges);
    computation.must_pass([&](size_t) { return view; }, borrow(layer, sources), borrow(layer, targets), borrow(layer, probes), out);
    auto result = Pairs();
    for (const auto row : layer.view(out).rows())
        result.emplace(ygg::decode_value<ygg::uint_t>(row.value(0)), ygg::decode_value<ygg::uint_t>(row.value(1)));
    EXPECT_EQ(result.size(), out.size()) << "every must-pass object is reported once";
    return result;
}

Pairs must_pass(const Relation& edges, const Relation& sources, const Relation& targets) { return must_pass(edges, sources, targets, all_probes(10)); }

using Vertex = urd::Store<urd::sequence<ygg::uint_t>>;

/// Whether a target of `key` is reachable from one of its sources without visiting a vertex containing `removed`.
bool reachable(const urd::Store<urd::sequence<std::tuple<ygg::uint_t, Vertex, Vertex>>>& edges,
               const urd::Store<urd::sequence<std::pair<ygg::uint_t, Vertex>>>& sources,
               const urd::Store<urd::sequence<std::pair<ygg::uint_t, Vertex>>>& targets,
               ygg::uint_t of,
               std::optional<ygg::uint_t> removed)
{
    const auto allowed = [&](const Vertex& vertex) { return !removed || std::ranges::find(vertex, *removed) == vertex.end(); };
    auto frontier = urd::Store<urd::sequence<Vertex>>();
    auto seen = urd::Store<urd::set<Vertex>>();
    for (const auto& [k, s] : sources)
        if (k == of && allowed(s) && seen.insert(s).second)
            frontier.push_back(s);
    while (!frontier.empty())
    {
        const auto vertex = frontier.back();
        frontier.pop_back();
        for (const auto& [k, t] : targets)
            if (k == of && t == vertex)
                return true;
        for (const auto& [k, from, to] : edges)
            if (k == of && from == vertex && allowed(to) && seen.insert(to).second)
                frontier.push_back(to);
    }
    return false;
}

TEST(DatabaseMustPass, ObjectsOnEveryPathPerKey)
{
    // Key 0: a diamond 1 -> {2, 3} -> 4 -> 5; key 1: the same vertex numbers, but 2 -> 4 is the only route.
    auto edges = Relation(tyr::datalog::tests::ops::cols({ key, u, v }));
    for (const auto& [k, from, to] : urd::Store<urd::sequence<std::tuple<ygg::uint_t, ygg::uint_t, ygg::uint_t>>> { { 0, 1, 2 },
                                                                                                                    { 0, 1, 3 },
                                                                                                                    { 0, 2, 4 },
                                                                                                                    { 0, 3, 4 },
                                                                                                                    { 0, 4, 5 },
                                                                                                                    { 1, 1, 2 },
                                                                                                                    { 1, 2, 4 },
                                                                                                                    { 1, 3, 4 } })
        edges.insert(std::array { ygg::encode_value(k), ygg::encode_value(from), ygg::encode_value(to) });
    auto sources = Relation(tyr::datalog::tests::ops::cols({ key, u }));
    sources.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(0)), ygg::encode_value(static_cast<ygg::uint_t>(1)) });
    sources.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(1)), ygg::encode_value(static_cast<ygg::uint_t>(1)) });
    sources.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(2)), ygg::encode_value(static_cast<ygg::uint_t>(1)) });  // Key 2 has no edges and no targets.
    auto targets = Relation(tyr::datalog::tests::ops::cols({ key, u }));
    targets.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(0)), ygg::encode_value(static_cast<ygg::uint_t>(5)) });
    targets.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(1)), ygg::encode_value(static_cast<ygg::uint_t>(4)) });
    targets.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(3)), ygg::encode_value(static_cast<ygg::uint_t>(4)) });  // Key 3 has no sources.
    EXPECT_EQ(must_pass(edges, sources, targets), (Pairs { { 0, 1 }, { 0, 4 }, { 0, 5 }, { 1, 1 }, { 1, 2 }, { 1, 4 } }));
}

TEST(DatabaseMustPass, SeveralSourcesAndTargets)
{
    // 1 -> 3, 2 -> 3, 3 -> 4, 3 -> 5: from {1, 2} to {4, 5} only 3 must be passed.
    auto edges = Relation(tyr::datalog::tests::ops::cols({ key, u, v }));
    for (const auto& [from, to] : urd::Store<urd::sequence<std::pair<ygg::uint_t, ygg::uint_t>>> { { 1, 3 }, { 2, 3 }, { 3, 4 }, { 3, 5 } })
        edges.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(0)), ygg::encode_value(from), ygg::encode_value(to) });
    auto sources = Relation(tyr::datalog::tests::ops::cols({ key, u }));
    auto targets = Relation(tyr::datalog::tests::ops::cols({ key, u }));
    for (const auto vertex : { static_cast<ygg::uint_t>(1), static_cast<ygg::uint_t>(2) })
        sources.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(0)), ygg::encode_value(vertex) });
    for (const auto vertex : { static_cast<ygg::uint_t>(4), static_cast<ygg::uint_t>(5) })
        targets.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(0)), ygg::encode_value(vertex) });
    EXPECT_EQ(must_pass(edges, sources, targets), (Pairs { { 0, 3 } }));
    // A source that is a target: the empty path passes nothing else.
    targets.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(0)), ygg::encode_value(static_cast<ygg::uint_t>(1)) });
    sources.clear();
    sources.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(0)), ygg::encode_value(static_cast<ygg::uint_t>(1)) });
    EXPECT_EQ(must_pass(edges, sources, targets), (Pairs { { 0, 1 } }));
}

TEST(DatabaseMustPass, UnreachableTargetsHaveNoMustPassVertices)
{
    auto edges = Relation(tyr::datalog::tests::ops::cols({ key, u, v }));
    edges.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(0)), ygg::encode_value(static_cast<ygg::uint_t>(1)), ygg::encode_value(static_cast<ygg::uint_t>(2)) });
    auto sources = Relation(tyr::datalog::tests::ops::cols({ key, u }));
    sources.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(0)), ygg::encode_value(static_cast<ygg::uint_t>(1)) });
    auto targets = Relation(tyr::datalog::tests::ops::cols({ key, u }));
    targets.insert(std::array { ygg::encode_value(static_cast<ygg::uint_t>(0)), ygg::encode_value(static_cast<ygg::uint_t>(3)) });
    EXPECT_TRUE(must_pass(edges, sources, targets).empty());
}

class DatabaseMustPassRandom : public testing::TestWithParam<unsigned>
{
};

TEST_P(DatabaseMustPassRandom, MatchesObjectRemoval)
{
    // Random keyed graphs over vertices of one and of two objects, against the definition: x must be passed iff a target
    // is reachable, but not without the vertices containing x. Only probed objects are reported.
    auto rng = std::mt19937(GetParam());
    constexpr auto keys = static_cast<ygg::uint_t>(3);
    constexpr auto objects = static_cast<ygg::uint_t>(5);
    for (const auto k : { static_cast<size_t>(1), static_cast<size_t>(2) })
        for (size_t round = 0; round < 20; ++round)
        {
            const auto random_vertex = [&]
            {
                auto vertex = Vertex();
                for (size_t c = 0; c < k; ++c)
                    vertex.push_back(static_cast<ygg::uint_t>(rng() % (k == 1 ? 9 : objects)));
                return vertex;
            };
            auto vertex_columns = urd::Store<urd::sequence<Column>> { key };
            auto edge_columns = urd::Store<urd::sequence<Column>> { key };
            for (size_t c = 0; c < k; ++c)
            {
                vertex_columns.emplace_back(static_cast<ygg::uint_t>(c));
                edge_columns.emplace_back(static_cast<ygg::uint_t>(c));
            }
            for (size_t c = 0; c < k; ++c)
                edge_columns.emplace_back(static_cast<ygg::uint_t>(k + c));
            auto edge_list = urd::Store<urd::sequence<std::tuple<ygg::uint_t, Vertex, Vertex>>>();
            auto source_list = urd::Store<urd::sequence<std::pair<ygg::uint_t, Vertex>>>();
            auto target_list = urd::Store<urd::sequence<std::pair<ygg::uint_t, Vertex>>>();
            auto edges = Relation(tyr::datalog::tests::qb::columns_of(std::span<const Column>(edge_columns)));
            auto sources = Relation(tyr::datalog::tests::qb::columns_of(std::span<const Column>(vertex_columns)));
            auto targets = Relation(tyr::datalog::tests::qb::columns_of(std::span<const Column>(vertex_columns)));
            const auto row = [](ygg::uint_t of, const Vertex& a, const Vertex& b = {})
            {
                auto values = urd::Store<urd::sequence<ygg::uint_t>> { of };
                values.insert(values.end(), a.begin(), a.end());
                values.insert(values.end(), b.begin(), b.end());
                return values;
            };
            const auto insert = [](Relation& relation, const urd::Store<urd::sequence<ygg::uint_t>>& values)
            {
                auto encoded = urd::Store<urd::sequence<ygg::Value>>();
                for (const auto value : values)
                    encoded.push_back(ygg::encode_value(value));
                if (relation.contains(encoded))
                    return false;
                relation.insert(encoded);
                return true;
            };
            for (ygg::uint_t of = 0; of < keys; ++of)
            {
                for (size_t e = 0; e < 16; ++e)
                {
                    const auto from = random_vertex(), to = random_vertex();
                    if (insert(edges, row(of, from, to)))
                        edge_list.emplace_back(of, from, to);
                }
                for (size_t i = 0; i < 1 + rng() % 2; ++i)
                    if (const auto s = random_vertex(); insert(sources, row(of, s)))
                        source_list.emplace_back(of, s);
                for (size_t i = 0; i < 1 + rng() % 2; ++i)
                    if (const auto t = random_vertex(); insert(targets, row(of, t)))
                        target_list.emplace_back(of, t);
            }
            const auto probed = urd::Store<urd::set<ygg::uint_t>> { 0, 2, 3, 4, 5, 7 };
            auto probes = Relation(tyr::datalog::tests::ops::cols({ probe }));
            for (const auto x : probed)
                probes.insert(std::array { ygg::encode_value(x) });
            auto expected = Pairs();
            for (ygg::uint_t of = 0; of < keys; ++of)
                if (reachable(edge_list, source_list, target_list, of, std::nullopt))
                    for (const auto x : probed)
                        if (!reachable(edge_list, source_list, target_list, of, x))
                            expected.emplace(of, x);
            ASSERT_EQ(must_pass(edges, sources, targets, probes), expected) << "k " << k << " round " << round;
        }
}

INSTANTIATE_TEST_SUITE_P(Seeds, DatabaseMustPassRandom, testing::Range(1u, 9u));

}  // namespace
