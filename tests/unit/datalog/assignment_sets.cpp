/*
 * Copyright (C) 2025-2026 Dominik Drexler
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include "tyr/datalog/lifted/kckp/assignment_sets.hpp"
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>

#include "tyr/analysis/variable_domain.hpp"
#include "tyr/formalism/declarations.hpp"
#include <yggdrasil/formalism/row_data.hpp>
#include "tyr/formalism/datalog/interning.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/function_data.hpp"
#include "tyr/formalism/object_data.hpp"
#include "tyr/formalism/predicate_data.hpp"

#include <algorithm>
#include <gtest/gtest.h>
#include <initializer_list>
#include <string>
#include <vector>
#include <yggdrasil/core/closed_interval.hpp>

namespace tyr::tests
{
namespace
{
using ObjectIndex = urd::index<tyr::formalism::Object>;

tyr::analysis::VariableDomain domain(std::initializer_list<ygg::uint_t> objects)
{
    auto result = tyr::analysis::VariableDomain {};
    for (const auto object : objects)
        result.objects.emplace_back(object);
    return result;
}

template<typename Relation>
auto make_binding(tyr::formalism::datalog::Repository& repository,
                  urd::index_view<Relation, tyr::formalism::datalog::Repository> relation,
                  std::initializer_list<ObjectIndex> objects)
{
    auto data = urd::data<ygg::Row<Relation>> {};
    data.relation = relation.index();
    for (const auto object : objects)
        data.values.push_back(object.value);
    return ygg::formalism::insert(repository, data).first;
}

/// What every projection of the binding allows: the intersection of its empty, vertex and edge assignments.
template<typename Set, typename Binding>
ygg::ClosedInterval<ygg::float_t> interval_of(const Set& set, Binding binding)
{
    const auto objects = binding.template get<ObjectIndex>();
    auto result = set[tyr::datalog::EmptyAssignment {}];
    for (ygg::uint_t i = 0; i < objects.size(); ++i)
    {
        result = intersect(result, set[tyr::datalog::VertexAssignment(tyr::formalism::ParameterIndex(i), objects[i])]);
        for (ygg::uint_t j = i + 1; j < objects.size(); ++j)
            result = intersect(result,
                               set[tyr::datalog::EdgeAssignment(tyr::formalism::ParameterIndex(i),
                                                                objects[i],
                                                                tyr::formalism::ParameterIndex(j),
                                                                objects[j])]);
    }
    return result;
}

template<typename T>
auto intern(tyr::formalism::datalog::Repository& repository, urd::data<T> data)
{
    return ygg::formalism::insert(repository, data).first;
}

struct Fixture
{
    tyr::formalism::datalog::Repository repository = tyr::formalism::datalog::Repository();
    urd::Store<urd::sequence<tyr::formalism::datalog::ObjectView>> objects;

    Fixture()
    {
        for (ygg::uint_t i = 0; i < 5; ++i)
            objects.push_back(intern(repository, urd::data<tyr::formalism::Object>("o" + std::to_string(i))));
    }

    [[nodiscard]] tyr::analysis::VariableDomainList domains() const
    {
        return { domain({ static_cast<ygg::uint_t>(objects[0].index().value), static_cast<ygg::uint_t>(objects[2].index().value) }),
                 domain({ static_cast<ygg::uint_t>(objects[1].index().value) }),
                 domain({ static_cast<ygg::uint_t>(objects[0].index().value),
                          static_cast<ygg::uint_t>(objects[3].index().value),
                          static_cast<ygg::uint_t>(objects[4].index().value) }) };
    }
};
}

TEST(TyrDatalogLiftedAssignmentSets, CompactRanksAreDenseAndReportMissingAssignments)
{
    const auto domains = tyr::analysis::VariableDomainList { domain({ 0, 2 }), domain({ 1 }), domain({ 0, 3, 4 }) };
    const auto hash = tyr::datalog::PerfectAssignmentHash(domains, 5);
    const auto coordinates = urd::Store<urd::sequence<urd::Store<urd::sequence<ObjectIndex>>>> {
        { ObjectIndex(0), ObjectIndex(2) },
        { ObjectIndex(1) },
        { ObjectIndex(0), ObjectIndex(3), ObjectIndex(4) },
    };
    auto ranks = urd::Store<urd::sequence<size_t>> { tyr::datalog::EmptyAssignment::rank };

    for (ygg::uint_t i = 0; i < coordinates.size(); ++i)
        for (const auto object : coordinates[i])
            ranks.push_back(hash.get_rank(tyr::datalog::VertexAssignment(tyr::formalism::ParameterIndex(i), object)));

    for (ygg::uint_t i = 0; i < coordinates.size(); ++i)
        for (ygg::uint_t j = i + 1; j < coordinates.size(); ++j)
            for (const auto first_object : coordinates[i])
                for (const auto second_object : coordinates[j])
                    ranks.push_back(hash.get_rank(
                        tyr::datalog::EdgeAssignment(tyr::formalism::ParameterIndex(i), first_object, tyr::formalism::ParameterIndex(j), second_object)));

    std::ranges::sort(ranks);
    ASSERT_EQ(hash.size(), 18);
    ASSERT_EQ(ranks.size(), hash.size());
    for (size_t rank = 0; rank < ranks.size(); ++rank)
        EXPECT_EQ(ranks[rank], rank);

    EXPECT_FALSE(hash.find_rank(tyr::datalog::VertexAssignment(tyr::formalism::ParameterIndex(0), ObjectIndex(1))));
    EXPECT_FALSE(
        hash.find_rank(tyr::datalog::EdgeAssignment(tyr::formalism::ParameterIndex(0), ObjectIndex(1), tyr::formalism::ParameterIndex(1), ObjectIndex(1))));
    EXPECT_EQ(tyr::datalog::PerfectAssignmentHash({}, 5).size(), 1);
    EXPECT_EQ(tyr::datalog::PerfectAssignmentHash({ domain({ 0, 4 }) }, 5).size(), 3);
    EXPECT_EQ(tyr::datalog::PerfectAssignmentHash({ domain({}) }, 5).size(), 1);
}

TEST(TyrDatalogLiftedAssignmentSets, PredicateInsertionUsesOnlyCompleteAssignmentsAndResets)
{
    auto fixture = Fixture {};
    const auto predicate = intern(fixture.repository, urd::data<tyr::formalism::Predicate<tyr::formalism::FluentTag>>(std::string("p"), 3));
    const auto binding = make_binding<tyr::formalism::Predicate<tyr::formalism::FluentTag>>(
        fixture.repository,
        predicate,
        { fixture.objects[0].index(), fixture.objects[1].index(), fixture.objects[3].index() });
    auto set = tyr::datalog::PredicateAssignmentSet<tyr::formalism::FluentTag>(predicate, fixture.domains(), fixture.objects.size());

    set.insert(binding);

    EXPECT_EQ(set.size(), 18);
    EXPECT_EQ(set.get_set().size(), 6);
    EXPECT_TRUE(set.at(tyr::datalog::VertexAssignment(tyr::formalism::ParameterIndex(0), fixture.objects[0].index())));
    EXPECT_TRUE(set.at(tyr::datalog::EdgeAssignment(tyr::formalism::ParameterIndex(0),
                                                    fixture.objects[0].index(),
                                                    tyr::formalism::ParameterIndex(2),
                                                    fixture.objects[3].index())));
    EXPECT_FALSE(set.at(tyr::datalog::VertexAssignment(tyr::formalism::ParameterIndex(0), fixture.objects[2].index())));
    EXPECT_FALSE(set[tyr::datalog::VertexAssignment(tyr::formalism::ParameterIndex(0), fixture.objects[1].index())]);
    EXPECT_FALSE(set[tyr::datalog::EdgeAssignment(tyr::formalism::ParameterIndex(0),
                                                  fixture.objects[1].index(),
                                                  tyr::formalism::ParameterIndex(1),
                                                  fixture.objects[1].index())]);

    set.reset();
    const auto out_of_domain = make_binding<tyr::formalism::Predicate<tyr::formalism::FluentTag>>(
        fixture.repository,
        predicate,
        { fixture.objects[1].index(), fixture.objects[1].index(), fixture.objects[3].index() });
    set.insert(out_of_domain);
    EXPECT_EQ(set.get_set().size(), 3);
    EXPECT_FALSE(set[tyr::datalog::VertexAssignment(tyr::formalism::ParameterIndex(0), fixture.objects[1].index())]);
    EXPECT_TRUE(set.at(tyr::datalog::VertexAssignment(tyr::formalism::ParameterIndex(1), fixture.objects[1].index())));
    EXPECT_TRUE(set.at(tyr::datalog::VertexAssignment(tyr::formalism::ParameterIndex(2), fixture.objects[3].index())));
    EXPECT_TRUE(set.at(tyr::datalog::EdgeAssignment(tyr::formalism::ParameterIndex(1),
                                                    fixture.objects[1].index(),
                                                    tyr::formalism::ParameterIndex(2),
                                                    fixture.objects[3].index())));

    set.reset();
    EXPECT_TRUE(set.get_set().empty());
}

TEST(TyrDatalogLiftedAssignmentSets, FunctionInsertionTracksHullsAndEmptyAssignments)
{
    auto fixture = Fixture {};
    const auto function = intern(fixture.repository, urd::data<tyr::formalism::Function<tyr::formalism::FluentTag>>(std::string("f"), 3));
    const auto first = make_binding<tyr::formalism::Function<tyr::formalism::FluentTag>>(
        fixture.repository,
        function,
        { fixture.objects[0].index(), fixture.objects[1].index(), fixture.objects[3].index() });
    const auto second = make_binding<tyr::formalism::Function<tyr::formalism::FluentTag>>(
        fixture.repository,
        function,
        { fixture.objects[0].index(), fixture.objects[1].index(), fixture.objects[4].index() });
    const auto out_of_domain = make_binding<tyr::formalism::Function<tyr::formalism::FluentTag>>(
        fixture.repository,
        function,
        { fixture.objects[1].index(), fixture.objects[1].index(), fixture.objects[3].index() });
    auto set = tyr::datalog::FunctionAssignmentSet<tyr::formalism::FluentTag>(function, fixture.domains(), fixture.objects.size());
    const auto first_interval = ygg::ClosedInterval<ygg::float_t>(2, 4);
    const auto second_interval = ygg::ClosedInterval<ygg::float_t>(1, 6);
    const auto projected_interval = ygg::ClosedInterval<ygg::float_t>(8, 9);

    EXPECT_EQ(set.size(), 18);
    EXPECT_TRUE(empty(set[tyr::datalog::EmptyAssignment {}]));
    EXPECT_TRUE(empty(set[tyr::datalog::VertexAssignment(tyr::formalism::ParameterIndex(0), fixture.objects[1].index())]));
    EXPECT_TRUE(set.insert(first, first_interval));
    EXPECT_EQ(interval_of(set, first), first_interval);
    EXPECT_TRUE(set.insert(second, second_interval));
    EXPECT_FALSE(set.insert(second, second_interval));
    EXPECT_EQ(set[tyr::datalog::EmptyAssignment {}], second_interval);
    EXPECT_EQ(set.at(tyr::datalog::VertexAssignment(tyr::formalism::ParameterIndex(0), fixture.objects[0].index())), second_interval);
    EXPECT_EQ(set.at(tyr::datalog::EdgeAssignment(tyr::formalism::ParameterIndex(0),
                                                  fixture.objects[0].index(),
                                                  tyr::formalism::ParameterIndex(1),
                                                  fixture.objects[1].index())),
              second_interval);
    EXPECT_EQ(interval_of(set, first), first_interval);
    EXPECT_EQ(interval_of(set, second), second_interval);
    EXPECT_TRUE(empty(set[tyr::datalog::EdgeAssignment(tyr::formalism::ParameterIndex(0),
                                                       fixture.objects[1].index(),
                                                       tyr::formalism::ParameterIndex(1),
                                                       fixture.objects[1].index())]));

    set.reset();
    EXPECT_TRUE(empty(set[tyr::datalog::EmptyAssignment {}]));
    EXPECT_TRUE(empty(interval_of(set, first)));
    EXPECT_TRUE(set.insert(out_of_domain, projected_interval));
    EXPECT_EQ(set[tyr::datalog::EmptyAssignment {}], projected_interval);
    EXPECT_TRUE(empty(interval_of(set, out_of_domain)));
    EXPECT_TRUE(empty(set[tyr::datalog::VertexAssignment(tyr::formalism::ParameterIndex(0), fixture.objects[1].index())]));
    EXPECT_EQ(set.at(tyr::datalog::VertexAssignment(tyr::formalism::ParameterIndex(1), fixture.objects[1].index())), projected_interval);
    EXPECT_EQ(set.at(tyr::datalog::VertexAssignment(tyr::formalism::ParameterIndex(2), fixture.objects[3].index())), projected_interval);
    EXPECT_EQ(set.at(tyr::datalog::EdgeAssignment(tyr::formalism::ParameterIndex(1),
                                                  fixture.objects[1].index(),
                                                  tyr::formalism::ParameterIndex(2),
                                                  fixture.objects[3].index())),
              projected_interval);

    set.reset();
    EXPECT_TRUE(empty(set[tyr::datalog::EmptyAssignment {}]));

    const auto nullary = intern(fixture.repository, urd::data<tyr::formalism::Function<tyr::formalism::FluentTag>>(std::string("constant"), 0));
    const auto nullary_binding = make_binding<tyr::formalism::Function<tyr::formalism::FluentTag>>(fixture.repository, nullary, {});
    auto nullary_set = tyr::datalog::FunctionAssignmentSet<tyr::formalism::FluentTag>(nullary, {}, fixture.objects.size());
    EXPECT_EQ(nullary_set.size(), 1);
    EXPECT_TRUE(empty(interval_of(nullary_set, nullary_binding)));
    EXPECT_TRUE(nullary_set.insert(nullary_binding, static_cast<ygg::float_t>(7)));
    EXPECT_EQ(interval_of(nullary_set, nullary_binding), ygg::ClosedInterval<ygg::float_t>(7, 7));
}
TEST(TyrDatalogLiftedAssignmentSets, TouchedBlocksAndPreparedRanksMatchScalarRanks)
{
    auto fixture = Fixture {};
    constexpr ygg::uint_t arity = 6;
    const auto predicate = intern(fixture.repository, urd::data<tyr::formalism::Predicate<tyr::formalism::FluentTag>>(std::string("wide"), arity));
    const auto function = intern(fixture.repository, urd::data<tyr::formalism::Function<tyr::formalism::FluentTag>>(std::string("wide-value"), arity));
    const auto domains = tyr::analysis::VariableDomainList(arity, domain({ 0, 2, 4 }));
    auto predicates = tyr::datalog::PredicateAssignmentSet<tyr::formalism::FluentTag>(predicate, domains, fixture.objects.size());
    auto functions = tyr::datalog::FunctionAssignmentSet<tyr::formalism::FluentTag>(function, domains, fixture.objects.size());
    ASSERT_GT(predicates.get_hash().size(), 64);  // ranks over more than one 64-bit word

    for (const auto objects :
         { std::initializer_list<ObjectIndex> { ObjectIndex(0), ObjectIndex(2), ObjectIndex(4), ObjectIndex(0), ObjectIndex(2), ObjectIndex(4) },
           std::initializer_list<ObjectIndex> { ObjectIndex(1), ObjectIndex(2), ObjectIndex(1), ObjectIndex(4), ObjectIndex(1), ObjectIndex(0) } })
    {
        const auto predicate_binding = make_binding<tyr::formalism::Predicate<tyr::formalism::FluentTag>>(fixture.repository, predicate, objects);
        const auto function_binding = make_binding<tyr::formalism::Function<tyr::formalism::FluentTag>>(fixture.repository, function, objects);
        const auto interval = ygg::ClosedInterval<ygg::float_t> { 2, 5 };
        predicates.insert(predicate_binding);
        predicates.insert(predicate_binding);
        EXPECT_TRUE(functions.insert(function_binding, interval));
        EXPECT_FALSE(functions.insert(function_binding, interval));
        auto expected = std::vector<bool>(predicates.size(), false);
        const auto indices = urd::Store<urd::sequence<ObjectIndex>>(objects);
        for (ygg::uint_t i = 0; i < arity; ++i)
        {
            const auto vertex = tyr::datalog::VertexAssignment(tyr::formalism::ParameterIndex(i), indices[i]);
            if (const auto rank = predicates.get_hash().find_rank(vertex))
            {
                expected[*rank] = true;
                EXPECT_EQ(functions.at(vertex), interval);
            }
            for (ygg::uint_t j = i + 1; j < arity; ++j)
            {
                const auto edge = tyr::datalog::EdgeAssignment(tyr::formalism::ParameterIndex(i), indices[i], tyr::formalism::ParameterIndex(j), indices[j]);
                if (const auto rank = predicates.get_hash().find_rank(edge))
                {
                    expected[*rank] = true;
                    EXPECT_EQ(functions.at(edge), interval);
                }
            }
        }
        for (size_t i = 0; i < expected.size(); ++i)
            EXPECT_EQ(predicates.get_set().contains(i), expected[i]);
        EXPECT_EQ(predicates.get_set().size(), static_cast<size_t>(std::ranges::count(expected, true)));
        predicates.reset();
        predicates.reset();
        functions.reset();
        EXPECT_TRUE(predicates.get_set().empty());
        EXPECT_TRUE(empty(functions[tyr::datalog::EmptyAssignment {}]));
    }
}

}
