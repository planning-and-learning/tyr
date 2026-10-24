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
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/formatter.hpp"
#include "tyr/formalism/planning/invariants/synthesis.hpp"
#include "tyr/formalism/planning/parser.hpp"
#include "tyr/formalism/planning/planning_task.hpp"
#include "tyr/planning/ground/task.hpp"
#include "tyr/planning/lifted/task.hpp"
#include "tyr/planning/lifted/task_grounder_decl.hpp"

#include <algorithm>
#include <filesystem>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <functional>
#include <gtest/gtest.h>
#include <memory>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
#include <yggdrasil/execution/onetbb.hpp>
#include <yggdrasil/formatting/cista_formatters.hpp>

namespace tyr::tests
{
namespace
{
using formalism::planning::InvariantView;
using formalism::planning::InvariantViewList;

const auto ferry_domain = std::filesystem::path(BENCHMARKS_DIR) / "classical/tests/ferry/domain.pddl";
const auto ferry_task = std::filesystem::path(BENCHMARKS_DIR) / "classical/tests/ferry/test-1.pddl";

std::set<std::string> predicate_names(InvariantView invariant)
{
    auto result = std::set<std::string> {};
    for (const auto atom : invariant.get_atoms())
        result.insert(fmt::format("{}", atom.get_predicate().get_name()));
    return result;
}

InvariantView find_invariant(const InvariantViewList& invariants, const std::set<std::string>& names)
{
    const auto it = std::ranges::find_if(invariants, [&](InvariantView invariant) { return predicate_names(invariant) == names; });
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

/// A lifted task and its domain's invariants, interned into the domain's repository before the task is parsed.
struct ParsedTask
{
    std::shared_ptr<planning::Task<LiftedTag>> lifted;
    InvariantViewList invariants;
};

ParsedTask parse(const std::filesystem::path& domain_file,
                 const std::function<formalism::planning::PlanningTask<LiftedTag>(formalism::planning::Parser&)>& parse_task)
{
    auto parser = make_test_parser(domain_file);
    const auto domain = parser.get_domain();
    auto invariants = formalism::planning::invariant::synthesize_invariants(domain.get_domain(), *domain.get_repository());
    return ParsedTask { planning::Task<LiftedTag>::create(parse_task(parser)), std::move(invariants) };
}

ParsedTask parse_ferry()
{
    return parse(ferry_domain, [](formalism::planning::Parser& parser) { return parser.parse_task(ferry_task); });
}

/// The ferry invariants: car position, ferry contents and ferry location.
struct FerryInvariants
{
    InvariantView car;
    InvariantView contents;
    InvariantView location;

    explicit FerryInvariants(const InvariantViewList& invariants) :
        car(find_invariant(invariants, { "at", "on" })),
        contents(find_invariant(invariants, { "empty-ferry", "on" })),
        location(find_invariant(invariants, { "at-ferry" }))
    {
    }
};

planning::GroundTaskInstantiationResult ground(planning::Task<LiftedTag>& lifted, std::optional<InvariantViewList> invariant_order)
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
    auto ferry = parse_ferry();
    auto& lifted = ferry.lifted;
    const auto invariants = FerryInvariants(ferry.invariants);

    EXPECT_EQ(variable_texts(ground(*lifted, InvariantViewList { invariants.contents, invariants.car, invariants.location })),
              (std::multiset<std::string> { "empty-ferry() on(car1) on(car2)",
                                            "at(car1,loc1) at(car1,loc3)",
                                            "at(car2,loc1) at(car2,loc3)",
                                            "at-ferry(loc1) at-ferry(loc3)" }));
}

TEST(TyrPlanningInvariantOrderTest, CarPositionsFirstLeavesEmptyFerrySingle)
{
    auto ferry = parse_ferry();
    auto& lifted = ferry.lifted;
    const auto invariants = FerryInvariants(ferry.invariants);

    EXPECT_EQ(variable_texts(ground(*lifted, InvariantViewList { invariants.car, invariants.location })),
              (std::multiset<std::string> { "at(car1,loc1) at(car1,loc3) on(car1)",
                                            "at(car2,loc1) at(car2,loc3) on(car2)",
                                            "at-ferry(loc1) at-ferry(loc3)",
                                            "empty-ferry()" }));
}

TEST(TyrPlanningInvariantOrderTest, EmptyOrderGivesOneVariablePerAtom)
{
    auto lifted = parse_ferry().lifted;

    for (const auto& text : variable_texts(ground(*lifted, InvariantViewList {})))
        EXPECT_EQ(text.find(' '), std::string::npos) << text;
}

TEST(TyrPlanningInvariantOrderTest, DisabledInvariantSynthesisIgnoresTheOrder)
{
    auto ferry = parse_ferry();
    auto& lifted = ferry.lifted;
    const auto invariants = FerryInvariants(ferry.invariants);
    auto context = ygg::ExecutionContext::create(1);
    auto options = planning::GroundTaskInstantiationOptions {};
    options.disable_invariant_synthesis = true;
    options.invariant_order = InvariantViewList { invariants.contents };

    const auto result = lifted->instantiate_ground_task(*context, options);

    ASSERT_NE(result.task, nullptr);
    for (const auto& text : variable_texts(result))
        EXPECT_EQ(text.find(' '), std::string::npos) << text;
}

TEST(TyrPlanningInvariantOrderTest, ViolatedInstanceStaysSingle)
{
    auto ferry = parse(ferry_domain,
                       [](formalism::planning::Parser& parser)
                       {
                           return parser.parse_task(R"(
(define (problem ferry-two-positions)
  (:domain ferry)
  (:objects car1 car2 - car loc1 loc3 - location)
  (:init (empty-ferry) (at-ferry loc3) (at car1 loc1) (at car1 loc3) (at car2 loc3))
  (:goal (and (at car2 loc1))))
)",
                                                    "ferry-two-positions.pddl");
                       });
    auto& lifted = ferry.lifted;
    const auto invariants = FerryInvariants(ferry.invariants);

    // car1 is at two locations initially, so its instance is not a mutex; it does not fall through to ferry contents.
    const auto variables = variable_texts(ground(*lifted, InvariantViewList { invariants.car, invariants.contents, invariants.location }));

    EXPECT_TRUE(variables.contains("at(car1,loc1)") && variables.contains("at(car1,loc3)") && variables.contains("on(car1)"));
    EXPECT_TRUE(variables.contains("at(car2,loc1) at(car2,loc3) on(car2)"));
    EXPECT_TRUE(variables.contains("empty-ferry()"));
}

TEST(TyrPlanningInvariantOrderTest, RejectsDuplicateInvariants)
{
    auto ferry = parse_ferry();
    auto& lifted = ferry.lifted;
    const auto invariants = FerryInvariants(ferry.invariants);
    EXPECT_THROW(ground(*lifted, InvariantViewList { invariants.car, invariants.car }), std::invalid_argument);
}

TEST(TyrPlanningInvariantOrderTest, RejectsInvariantsOfAnotherParserOfTheSameDomain)
{
    auto ferry = parse_ferry();
    const auto other = parse_ferry();
    const auto foreign = FerryInvariants(other.invariants);

    // Interned into the other parser's repository, so they are not this domain's invariants.
    EXPECT_THROW(ground(*ferry.lifted, InvariantViewList { foreign.contents, foreign.car, foreign.location }), std::invalid_argument);
}

TEST(TyrPlanningInvariantOrderTest, RejectsInvariantsOfAnotherDomain)
{
    auto ferry = parse_ferry();
    const auto blocks_directory = std::filesystem::path(BENCHMARKS_DIR) / "classical/tests/blocks_4";
    const auto blocks =
        parse(blocks_directory / "domain.pddl", [&](formalism::planning::Parser& parser) { return parser.parse_task(blocks_directory / "test-1.pddl"); });

    EXPECT_THROW(ground(*ferry.lifted, InvariantViewList { blocks.invariants.front() }), std::invalid_argument);
}

}
