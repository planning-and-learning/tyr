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

#include "planning/parser.hpp"
#include "tyr/formalism/formalism.hpp"

#include <algorithm>
#include <filesystem>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <gtest/gtest.h>
#include <random>
#include <set>
#include <string>
#include <vector>

namespace tyr::tests
{
namespace
{
using formalism::planning::InvariantView;
using formalism::planning::InvariantViewList;
using formalism::planning::invariant::compute_invariant_order_greedy;
using formalism::planning::invariant::GreedyCriterion;

const auto tests_root = std::filesystem::path(BENCHMARKS_DIR) / "classical/tests";

/// Keeps the domain alive: the invariants are interned into its repository.
struct DomainInvariants
{
    formalism::planning::PlanningDomain domain;
    InvariantViewList invariants;

    explicit DomainInvariants(const std::filesystem::path& directory) :
        domain(make_test_parser(directory / "domain.pddl").get_domain()),
        invariants(formalism::planning::invariant::synthesize_invariants(domain.get_domain(), *domain.get_repository()))
    {
    }
};

std::set<std::string> predicate_names(InvariantView invariant)
{
    auto result = std::set<std::string> {};
    for (const auto atom : invariant.get_atoms())
        result.insert(fmt::format("{}", atom.get_predicate().get_name()));
    return result;
}

std::vector<std::set<std::string>> names(const InvariantViewList& order)
{
    auto result = std::vector<std::set<std::string>> {};
    for (const auto invariant : order)
        result.push_back(predicate_names(invariant));
    return result;
}

/// Every criterion: MaxGroupSize, MaxCoverage.
std::vector<InvariantViewList> all_orders(const InvariantViewList& invariants)
{
    return { compute_invariant_order_greedy(invariants, GreedyCriterion::MaxGroupSize),
             compute_invariant_order_greedy(invariants, GreedyCriterion::MaxCoverage) };
}

std::vector<std::filesystem::path> test_domains()
{
    auto result = std::vector<std::filesystem::path> {};
    for (const auto& entry : std::filesystem::directory_iterator(tests_root))
        if (std::filesystem::exists(entry.path() / "domain.pddl") && std::filesystem::exists(entry.path() / "test-1.pddl"))
            result.push_back(entry.path());
    std::ranges::sort(result);
    return result;
}

}

TEST(TyrFormalismPlanningInvariantOrderingTest, OrdersDependOnlyOnTheSetOfInvariants)
{
    for (const auto& directory : test_domains())
    {
        SCOPED_TRACE(directory.filename().string());
        const auto domain = DomainInvariants(directory);
        const auto orders = all_orders(domain.invariants);

        auto shuffled = domain.invariants;
        for (const auto seed : { 1U, 2U, 3U, 4U, 5U })
        {
            std::ranges::shuffle(shuffled, std::mt19937(seed));
            EXPECT_EQ(all_orders(shuffled), orders);
        }
    }
}

/// Two parsers of the same domain synthesize into separate repositories; the orders must agree.
TEST(TyrFormalismPlanningInvariantOrderingTest, OrdersAreStableAcrossParsers)
{
    for (const auto& directory : test_domains())
    {
        SCOPED_TRACE(directory.filename().string());
        const auto first = DomainInvariants(directory);
        const auto second = DomainInvariants(directory);
        for (const auto criterion : { GreedyCriterion::MaxGroupSize, GreedyCriterion::MaxCoverage })
            EXPECT_EQ(names(compute_invariant_order_greedy(first.invariants, criterion)), names(compute_invariant_order_greedy(second.invariants, criterion)));
    }
}

/// Parsing a task of the domain must not change the order of the domain's invariants.
TEST(TyrFormalismPlanningInvariantOrderingTest, OrdersDoNotChangeWhenTasksAreParsed)
{
    for (const auto& directory : test_domains())
    {
        SCOPED_TRACE(directory.filename().string());
        auto parser = make_test_parser(directory / "domain.pddl");
        const auto domain = parser.get_domain();
        const auto invariants = formalism::planning::invariant::synthesize_invariants(domain.get_domain(), *domain.get_repository());
        const auto before = all_orders(invariants);

        const auto task = parser.parse_task(directory / "test-1.pddl");

        EXPECT_EQ(all_orders(invariants), before);
    }
}

TEST(TyrFormalismPlanningInvariantOrderingTest, OrdersAreDistinctSynthesizedInvariants)
{
    for (const auto& directory : test_domains())
    {
        SCOPED_TRACE(directory.filename().string());
        const auto domain = DomainInvariants(directory);
        for (const auto& order : all_orders(domain.invariants))
        {
            for (size_t i = 0; i < order.size(); ++i)
            {
                EXPECT_NE(std::ranges::find(domain.invariants, order[i]), domain.invariants.end());
                EXPECT_EQ(std::ranges::find(order.begin() + i + 1, order.end(), order[i]), order.end());
            }
        }
    }
}

TEST(TyrFormalismPlanningInvariantOrderingTest, Ferry)
{
    const auto domain = DomainInvariants(tests_root / "ferry");

    using Names = std::vector<std::set<std::string>>;
    // Car positions have groups of locations + 1 atoms and cover cars * (locations + 1); ferry contents one group of cars + 1.
    EXPECT_EQ(names(compute_invariant_order_greedy(domain.invariants)), (Names { { "at", "on" }, { "at-ferry" } }));
    EXPECT_EQ(names(compute_invariant_order_greedy(domain.invariants, GreedyCriterion::MaxCoverage)), (Names { { "at", "on" }, { "at-ferry" } }));
}

TEST(TyrFormalismPlanningInvariantOrderingTest, Blocks)
{
    const auto domain = DomainInvariants(tests_root / "blocks_4");

    // "What is on the block" and "where is the block" are equally large; by name, {clear, holding, on} comes first.
    EXPECT_EQ(names(compute_invariant_order_greedy(domain.invariants)), (std::vector<std::set<std::string>> { { "clear", "holding", "on" } }));
    EXPECT_EQ(names(compute_invariant_order_greedy(domain.invariants, GreedyCriterion::MaxCoverage)),
              (std::vector<std::set<std::string>> { { "clear", "holding", "on" } }));
}

TEST(TyrFormalismPlanningInvariantOrderingTest, Airport)
{
    const auto domain = DomainInvariants(tests_root / "airport");

    using Names = std::vector<std::set<std::string>>;
    // One group of all moving and pushing airplanes (2n atoms) is larger than an airplane's state (n + 2 atoms), which
    // covers more atoms in total. MaxGroupSize takes the large group first, so is-parked is left to the airplane's state.
    EXPECT_EQ(names(compute_invariant_order_greedy(domain.invariants)),
              (Names { { "airborne", "at-segment" }, { "is-moving", "is-pushing" }, { "facing" }, { "is-moving", "is-parked", "is-pushing" } }));
    EXPECT_EQ(names(compute_invariant_order_greedy(domain.invariants, GreedyCriterion::MaxCoverage)),
              (Names { { "airborne", "at-segment" }, { "is-moving", "is-parked", "is-pushing" }, { "facing" } }));
}

}
