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
using formalism::planning::MutableInvariant;
using formalism::planning::MutableInvariantList;

const auto ferry_domain = std::filesystem::path(BENCHMARKS_DIR) / "classical/tests/ferry/domain.pddl";
const auto ferry_task = std::filesystem::path(BENCHMARKS_DIR) / "classical/tests/ferry/test-1.pddl";

std::set<std::string> predicate_names(const MutableInvariant& invariant)
{
    auto result = std::set<std::string> {};
    for (const auto& atom : invariant.atoms)
        result.insert(fmt::format("{}", atom.predicate.get_name()));
    return result;
}

const MutableInvariant& find_invariant(const MutableInvariantList& invariants, const std::set<std::string>& names)
{
    const auto it = std::ranges::find_if(invariants, [&](const MutableInvariant& invariant) { return predicate_names(invariant) == names; });
    if (it == invariants.end())
        throw std::logic_error(fmt::format("no invariant over {}", fmt::join(names, ", ")));
    return *it;
}

/// The FDR variables of a ground task, each as its sorted atoms.
std::multiset<std::string> variable_texts(const planning::GroundTaskInstantiationResult& result)
{
    auto texts = std::multiset<std::string> {};
    for (const auto variable : result.task->get_fdr_context()->get_variables())
    {
        auto names = std::vector<std::string> {};
        for (const auto atom : variable.get_atoms())
        {
            auto objects = std::vector<std::string> {};
            for (const auto object : atom.get_objects())
                objects.push_back(fmt::format("{}", object.get_name()));
            names.push_back(fmt::format("{}({})", atom.get_predicate().get_name(), fmt::join(objects, ",")));
        }
        std::ranges::sort(names);
        texts.insert(fmt::format("{}", fmt::join(names, " ")));
    }
    return texts;
}

/// The ferry invariants of `lifted`'s domain: car position, ferry contents and ferry location.
struct FerryInvariants
{
    MutableInvariant car;
    MutableInvariant contents;
    MutableInvariant location;

    explicit FerryInvariants(const planning::Task<LiftedTag>& lifted) :
        FerryInvariants(formalism::planning::invariant::synthesize_invariants(lifted.get_formalism_task().get_domain().get_domain()))
    {
    }

private:
    explicit FerryInvariants(const MutableInvariantList& invariants) :
        car(find_invariant(invariants, { "at", "on" })),
        contents(find_invariant(invariants, { "empty-ferry", "on" })),
        location(find_invariant(invariants, { "at-ferry" }))
    {
    }
};

std::shared_ptr<planning::Task<LiftedTag>> parse_ferry() { return planning::Task<LiftedTag>::create(make_test_parser(ferry_domain).parse_task(ferry_task)); }

planning::GroundTaskInstantiationResult ground(planning::Task<LiftedTag>& lifted, std::optional<MutableInvariantList> invariant_order)
{
    auto context = ygg::ExecutionContext::create(1);
    auto options = planning::GroundTaskInstantiationOptions {};
    options.invariant_order = std::move(invariant_order);
    auto result = lifted.instantiate_ground_task(*context, options);
    if (!result.task)
        throw std::logic_error("grounding failed");
    return result;
}

}

TEST(TyrPlanningInvariantOrderTest, FerryContentsFirst)
{
    auto lifted = parse_ferry();
    const auto invariants = FerryInvariants(*lifted);

    EXPECT_EQ(variable_texts(ground(*lifted, MutableInvariantList { invariants.contents, invariants.car, invariants.location })),
              (std::multiset<std::string> { "empty-ferry() on(car1) on(car2)",
                                            "at(car1,loc1) at(car1,loc3)",
                                            "at(car2,loc1) at(car2,loc3)",
                                            "at-ferry(loc1) at-ferry(loc3)" }));
}

TEST(TyrPlanningInvariantOrderTest, CarPositionsFirstLeavesEmptyFerrySingle)
{
    auto lifted = parse_ferry();
    const auto invariants = FerryInvariants(*lifted);

    EXPECT_EQ(variable_texts(ground(*lifted, MutableInvariantList { invariants.car, invariants.location })),
              (std::multiset<std::string> { "at(car1,loc1) at(car1,loc3) on(car1)",
                                            "at(car2,loc1) at(car2,loc3) on(car2)",
                                            "at-ferry(loc1) at-ferry(loc3)",
                                            "empty-ferry()" }));
}

TEST(TyrPlanningInvariantOrderTest, EmptyOrderGivesOneVariablePerAtom)
{
    auto lifted = parse_ferry();

    for (const auto& text : variable_texts(ground(*lifted, MutableInvariantList {})))
        EXPECT_EQ(text.find(' '), std::string::npos) << text;
}

TEST(TyrPlanningInvariantOrderTest, DisabledInvariantSynthesisIgnoresTheOrder)
{
    auto lifted = parse_ferry();
    const auto invariants = FerryInvariants(*lifted);
    auto context = ygg::ExecutionContext::create(1);
    auto options = planning::GroundTaskInstantiationOptions {};
    options.disable_invariant_synthesis = true;
    options.invariant_order = MutableInvariantList { invariants.contents };

    const auto result = lifted->instantiate_ground_task(*context, options);

    ASSERT_NE(result.task, nullptr);
    for (const auto& text : variable_texts(result))
        EXPECT_EQ(text.find(' '), std::string::npos) << text;
}

TEST(TyrPlanningInvariantOrderTest, ViolatedInstanceStaysSingle)
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

    // car1 is at two locations initially, so its instance is not a mutex; it does not fall through to ferry contents.
    const auto variables = variable_texts(ground(*lifted, MutableInvariantList { invariants.car, invariants.contents, invariants.location }));

    EXPECT_TRUE(variables.contains("at(car1,loc1)") && variables.contains("at(car1,loc3)") && variables.contains("on(car1)"));
    EXPECT_TRUE(variables.contains("at(car2,loc1) at(car2,loc3) on(car2)"));
    EXPECT_TRUE(variables.contains("empty-ferry()"));
}

TEST(TyrPlanningInvariantOrderTest, RejectsDuplicateInvariants)
{
    auto lifted = parse_ferry();
    const auto invariants = FerryInvariants(*lifted);
    EXPECT_THROW(ground(*lifted, MutableInvariantList { invariants.car, invariants.car }), std::invalid_argument);
}

TEST(TyrPlanningInvariantOrderTest, AcceptsInvariantsOfAnotherParserOfTheSameDomain)
{
    auto lifted = parse_ferry();
    auto other = parse_ferry();
    const auto own = FerryInvariants(*lifted);
    const auto foreign = FerryInvariants(*other);

    EXPECT_EQ(variable_texts(ground(*lifted, MutableInvariantList { foreign.contents, foreign.car, foreign.location })),
              variable_texts(ground(*lifted, MutableInvariantList { own.contents, own.car, own.location })));
}

TEST(TyrPlanningInvariantOrderTest, RejectsInvariantsOfAnotherDomain)
{
    auto lifted = parse_ferry();
    auto blocks = planning::Task<LiftedTag>::create(make_test_parser(std::filesystem::path(BENCHMARKS_DIR) / "classical/tests/blocks_4/domain.pddl")
                                                        .parse_task(std::filesystem::path(BENCHMARKS_DIR) / "classical/tests/blocks_4/test-1.pddl"));
    const auto blocks_invariants = formalism::planning::invariant::synthesize_invariants(blocks->get_formalism_task().get_domain().get_domain());
    // Ferry has no invariant with three parts, so this one cannot coincide with a ferry invariant.
    const auto it = std::ranges::find_if(blocks_invariants, [](const MutableInvariant& invariant) { return invariant.atoms.size() == 3; });
    ASSERT_NE(it, blocks_invariants.end());

    EXPECT_THROW(ground(*lifted, MutableInvariantList { *it }), std::invalid_argument);
}

}
