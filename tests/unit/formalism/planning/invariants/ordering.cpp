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
using formalism::planning::MutableInvariant;
using formalism::planning::MutableInvariantList;
using formalism::planning::invariant::compute_invariant_order_exhaustive;
using formalism::planning::invariant::compute_invariant_order_greedy;

const auto tests_root = std::filesystem::path(BENCHMARKS_DIR) / "classical/tests";

/// Keeps the parsed task alive: the invariants' predicates refer to its repository.
struct DomainInvariants
{
    formalism::planning::PlanningTask<LiftedTag> task;
    MutableInvariantList invariants;

    explicit DomainInvariants(const std::filesystem::path& directory) :
        task(make_test_parser(directory / "domain.pddl").parse_task(directory / "test-1.pddl")),
        invariants(formalism::planning::invariant::synthesize_invariants(task.get_task().get_domain()))
    {
    }
};

std::set<std::string> predicate_names(const MutableInvariant& invariant)
{
    auto result = std::set<std::string> {};
    for (const auto& atom : invariant.atoms)
        result.insert(fmt::format("{}", atom.predicate.get_name()));
    return result;
}

std::vector<std::set<std::string>> names(const MutableInvariantList& order)
{
    auto result = std::vector<std::set<std::string>> {};
    for (const auto& invariant : order)
        result.push_back(predicate_names(invariant));
    return result;
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
        const auto greedy = compute_invariant_order_greedy(domain.invariants);
        const auto exhaustive = compute_invariant_order_exhaustive(domain.invariants);

        auto shuffled = domain.invariants;
        for (const auto seed : { 1U, 2U, 3U, 4U, 5U })
        {
            std::ranges::shuffle(shuffled, std::mt19937(seed));
            EXPECT_EQ(compute_invariant_order_greedy(shuffled), greedy);
            EXPECT_EQ(compute_invariant_order_exhaustive(shuffled), exhaustive);
        }
    }
}

TEST(TyrFormalismPlanningInvariantOrderingTest, OrdersAreDistinctSynthesizedInvariants)
{
    for (const auto& directory : test_domains())
    {
        SCOPED_TRACE(directory.filename().string());
        const auto domain = DomainInvariants(directory);
        for (const auto& order : { compute_invariant_order_greedy(domain.invariants), compute_invariant_order_exhaustive(domain.invariants) })
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

    // Car positions cover cars * (locations + 1) atoms, ferry contents only cars + 1.
    EXPECT_EQ(names(compute_invariant_order_greedy(domain.invariants)), (std::vector<std::set<std::string>> { { "at", "on" }, { "at-ferry" } }));
    // Ferry contents leave no atom single; car positions leave empty-ferry single.
    EXPECT_EQ(names(compute_invariant_order_exhaustive(domain.invariants)),
              (std::vector<std::set<std::string>> { { "empty-ferry", "on" }, { "at", "on" }, { "at-ferry" } }));
}

TEST(TyrFormalismPlanningInvariantOrderingTest, Blocks)
{
    const auto domain = DomainInvariants(tests_root / "blocks_4");

    // "Where is the block" and "what is on the block" cover equally many atoms; on(?x, ?y) with the rigid variable
    // first decides for "where is the block".
    EXPECT_EQ(names(compute_invariant_order_greedy(domain.invariants)), (std::vector<std::set<std::string>> { { "holding", "on", "on-table" } }));
    // Taking holding into the arm variable leaves fewer atoms single; the same tie-break picks on(?x, ?y).
    EXPECT_EQ(names(compute_invariant_order_exhaustive(domain.invariants)),
              (std::vector<std::set<std::string>> { { "arm-empty", "holding" }, { "holding", "on", "on-table" } }));
}

}
