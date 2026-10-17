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
#include "tyr/planning/planning.hpp"

#include <algorithm>
#include <filesystem>
#include <fmt/format.h>
#include <gtest/gtest.h>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace tyr::tests
{
namespace
{
using formalism::planning::invariant::Invariant;
using formalism::planning::invariant::InvariantList;
using formalism::planning::invariant::MutexGroups;

const auto ferry_domain = std::filesystem::path(BENCHMARKS_DIR) / "classical/tests/ferry/domain.pddl";
const auto ferry_task = std::filesystem::path(BENCHMARKS_DIR) / "classical/tests/ferry/test-1.pddl";

std::set<std::string> predicate_names(const Invariant& invariant)
{
    auto result = std::set<std::string> {};
    for (const auto& atom : invariant.atoms)
        result.insert(fmt::format("{}", atom.predicate.get_name()));
    return result;
}

const Invariant& find_invariant(const InvariantList& invariants, const std::set<std::string>& names)
{
    const auto it = std::ranges::find_if(invariants, [&](const Invariant& invariant) { return predicate_names(invariant) == names; });
    if (it == invariants.end())
        throw std::logic_error(fmt::format("no invariant over {}", fmt::join(names, ", ")));
    return *it;
}

std::string text(const formalism::planning::AtomViewList<GroundTag, formalism::FluentTag>& atoms)
{
    auto names = std::vector<std::string> {};
    for (const auto atom : atoms)
    {
        auto objects = std::vector<std::string> {};
        for (const auto object : atom.get_objects())
            objects.push_back(fmt::format("{}", object.get_name()));
        names.push_back(fmt::format("{}({})", atom.get_predicate().get_name(), fmt::join(objects, ",")));
    }
    std::ranges::sort(names);
    return fmt::format("{}", fmt::join(names, " "));
}

std::multiset<std::string> group_texts(const MutexGroups& mutex_groups)
{
    auto result = std::multiset<std::string> {};
    for (const auto& group : mutex_groups.groups)
        result.insert(text(group.atoms));
    return result;
}

struct Grounded
{
    std::shared_ptr<planning::Task<LiftedTag>> lifted;
    planning::GroundTaskInstantiationResult result;
};

/// The ferry invariants of `lifted`'s domain: car position, ferry contents and ferry location.
struct FerryInvariants
{
    Invariant car;
    Invariant contents;
    Invariant location;

    explicit FerryInvariants(const planning::Task<LiftedTag>& lifted) :
        FerryInvariants(formalism::planning::invariant::synthesize_invariants(lifted.get_formalism_task().get_domain().get_domain()))
    {
    }

private:
    explicit FerryInvariants(const InvariantList& invariants) :
        car(find_invariant(invariants, { "at", "on" })),
        contents(find_invariant(invariants, { "empty-ferry", "on" })),
        location(find_invariant(invariants, { "at-ferry" }))
    {
    }
};

std::shared_ptr<planning::Task<LiftedTag>> parse_ferry() { return planning::Task<LiftedTag>::create(make_test_parser(ferry_domain).parse_task(ferry_task)); }

planning::GroundTaskInstantiationResult ground(planning::Task<LiftedTag>& lifted, std::optional<InvariantList> invariant_order)
{
    auto context = ygg::ExecutionContext::create(1);
    auto options = planning::GroundTaskInstantiationOptions {};
    options.invariant_order = std::move(invariant_order);
    return lifted.instantiate_ground_task(*context, options);
}

void expect_variables_match_groups(const planning::GroundTaskInstantiationResult& result)
{
    ASSERT_NE(result.task, nullptr);
    const auto& variables = result.task->get_fdr_context()->get_variables();
    ASSERT_EQ(variables.size(), result.mutex_groups.groups.size());
    for (size_t i = 0; i < variables.size(); ++i)
    {
        auto atoms = formalism::planning::AtomViewList<GroundTag, formalism::FluentTag> {};
        for (const auto atom : variables[i].get_atoms())
            atoms.push_back(atom);
        EXPECT_EQ(text(atoms), text(result.mutex_groups.groups[i].atoms));
    }
}

}

TEST(TyrPlanningInvariantOrderTest, FerryContentsFirst)
{
    auto lifted = parse_ferry();
    const auto invariants = FerryInvariants(*lifted);
    const auto result = ground(*lifted, InvariantList { invariants.contents, invariants.car, invariants.location });

    expect_variables_match_groups(result);
    EXPECT_EQ(group_texts(result.mutex_groups),
              (std::multiset<std::string> { "empty-ferry() on(car1) on(car2)",
                                            "at(car1,loc1) at(car1,loc3)",
                                            "at(car2,loc1) at(car2,loc3)",
                                            "at-ferry(loc1) at-ferry(loc3)" }));
    EXPECT_EQ(result.mutex_groups.groups.front().invariant_index, 0);
    EXPECT_TRUE(result.mutex_groups.groups.front().binding.empty());
    EXPECT_TRUE(result.mutex_groups.fallbacks.empty());
}

TEST(TyrPlanningInvariantOrderTest, CarPositionsFirstLeavesEmptyFerrySingle)
{
    auto lifted = parse_ferry();
    const auto invariants = FerryInvariants(*lifted);
    const auto result = ground(*lifted, InvariantList { invariants.car, invariants.location });

    expect_variables_match_groups(result);
    EXPECT_EQ(group_texts(result.mutex_groups),
              (std::multiset<std::string> { "at(car1,loc1) at(car1,loc3) on(car1)",
                                            "at(car2,loc1) at(car2,loc3) on(car2)",
                                            "at-ferry(loc1) at-ferry(loc3)",
                                            "empty-ferry()" }));
    for (const auto& group : result.mutex_groups.groups)
    {
        if (text(group.atoms) == "empty-ferry()")
            EXPECT_FALSE(group.invariant_index.has_value());
        else if (group.invariant_index == 0)
            EXPECT_EQ(group.binding.size(), 1);
    }
}

TEST(TyrPlanningInvariantOrderTest, EmptyOrderGivesOneVariablePerAtom)
{
    auto lifted = parse_ferry();
    const auto result = ground(*lifted, InvariantList {});

    expect_variables_match_groups(result);
    EXPECT_TRUE(result.mutex_groups.invariants.empty());
    for (const auto& group : result.mutex_groups.groups)
    {
        EXPECT_EQ(group.atoms.size(), 1);
        EXPECT_FALSE(group.invariant_index.has_value());
    }
}

TEST(TyrPlanningInvariantOrderTest, DefaultIsGreedyCoverWithProvenance)
{
    auto lifted = parse_ferry();
    const auto result = ground(*lifted, std::nullopt);

    expect_variables_match_groups(result);
    EXPECT_FALSE(result.mutex_groups.invariants.empty());
    EXPECT_TRUE(std::ranges::any_of(result.mutex_groups.groups, [](const auto& group) { return group.invariant_index.has_value(); }));
}

TEST(TyrPlanningInvariantOrderTest, InstanceWithTwoInitialAtomsFallsBackToSingleAtoms)
{
    auto lifted = planning::Task<LiftedTag>::create(make_test_parser(ferry_domain)
                                                        .parse_task(R"(
(define (problem ferry-two-positions)
  (:domain ferry)
  (:objects car1 car2 - car loc1 loc3 - location)
  (:init (empty-ferry) (at-ferry loc3) (at car1 loc1) (at car1 loc3) (at car2 loc3))
  (:goal (and (at car2 loc1))))
)",
                                                                    "ferry-two-positions.pddl"));
    const auto invariants = FerryInvariants(*lifted);
    const auto result = ground(*lifted, InvariantList { invariants.car, invariants.location });

    expect_variables_match_groups(result);
    ASSERT_EQ(result.mutex_groups.fallbacks.size(), 1);
    const auto& fallback = result.mutex_groups.fallbacks.front();
    EXPECT_EQ(fallback.invariant_index, 0);
    EXPECT_EQ(text(fallback.atoms), "at(car1,loc1) at(car1,loc3) on(car1)");
    // The violated instance's atoms do not fall through to another invariant.
    const auto groups = group_texts(result.mutex_groups);
    EXPECT_TRUE(groups.contains("at(car1,loc1)") && groups.contains("at(car1,loc3)") && groups.contains("on(car1)"));
    EXPECT_TRUE(groups.contains("at(car2,loc1) at(car2,loc3) on(car2)"));
}

TEST(TyrPlanningInvariantOrderTest, RejectsDuplicateInvariants)
{
    auto lifted = parse_ferry();
    const auto invariants = FerryInvariants(*lifted);
    EXPECT_THROW(ground(*lifted, InvariantList { invariants.car, invariants.car }), std::invalid_argument);
}

TEST(TyrPlanningInvariantOrderTest, AcceptsInvariantsOfAnotherParserOfTheSameDomain)
{
    auto lifted = parse_ferry();
    auto other = parse_ferry();
    const auto own = FerryInvariants(*lifted);
    const auto foreign = FerryInvariants(*other);

    const auto expected = ground(*lifted, InvariantList { own.contents, own.car, own.location });
    const auto actual = ground(*lifted, InvariantList { foreign.contents, foreign.car, foreign.location });

    EXPECT_EQ(group_texts(actual.mutex_groups), group_texts(expected.mutex_groups));
}

TEST(TyrPlanningInvariantOrderTest, RejectsInvariantsOfAnotherDomain)
{
    auto lifted = parse_ferry();
    auto blocks = planning::Task<LiftedTag>::create(make_test_parser(std::filesystem::path(BENCHMARKS_DIR) / "classical/tests/blocks_4/domain.pddl")
                                                        .parse_task(std::filesystem::path(BENCHMARKS_DIR) / "classical/tests/blocks_4/test-1.pddl"));
    const auto blocks_invariants = formalism::planning::invariant::synthesize_invariants(blocks->get_formalism_task().get_domain().get_domain());
    // Ferry has no invariant with three parts, so this one cannot coincide with a ferry invariant.
    const auto it = std::ranges::find_if(blocks_invariants, [](const Invariant& invariant) { return invariant.atoms.size() == 3; });
    ASSERT_NE(it, blocks_invariants.end());

    EXPECT_THROW(ground(*lifted, InvariantList { *it }), std::invalid_argument);
}

}
