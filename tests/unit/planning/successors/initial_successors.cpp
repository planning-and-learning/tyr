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

// The successors of each test task's initial node, ground and lifted each, against the fixture
// (tests/support/generators/planning/successors/generate_initial_successors.py): for_each_successor's successors serialized by
// yggdrasil's dictionaries (initial_successor_tables.hpp), in the generator's order (it is deterministic); and that
// expanding the node again once warm allocates nothing. A task tyr does not parse yet fails to parse with the fixture's
// reason.

#include "planning/parser.hpp"
#include "planning/successors/initial_successor_tables.hpp"
#include "tyr/planning/axioms/axiom_evaluator.hpp"
#include "tyr/planning/states/state_repository.hpp"
#include "tyr/planning/successors/ground/successor_generator.hpp"
#include "tyr/planning/successors/lifted/successor_generator.hpp"
#include "tyr/planning/successors/successor_view.hpp"
#include "tyr/planning/successors/successors.hpp"
#include "tyr/planning/tasks/ground/task.hpp"
#include "tyr/planning/tasks/lifted/task.hpp"

#include <boost/json.hpp>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <exception>
#include <filesystem>
#include <gtest/gtest.h>
#include <optional>
#include <string>
#include <urd/elements/clone.hpp>
#include <urd/testing/allocations.hpp>
#include <utility>
#include <vector>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/execution/onetbb.hpp>
#include <yggdrasil/serialization/dictionaries.hpp>
#include <yggdrasil/serialization/json.hpp>
#include <yggdrasil/serialization/json_suite.hpp>

namespace tyr::tests
{
namespace
{

/// A fixture case: its task, and either the parser's reason for not parsing it or its serialized successors, by kind.
struct InitialSuccessorsCase
{
    std::string name;
    std::filesystem::path domain_file;
    std::filesystem::path task_file;
    std::optional<std::string> skipped;
    boost::json::value lifted;
    boost::json::value ground;
};

std::vector<InitialSuccessorsCase> load_cases()
{
    const auto suite = ygg::common::load_json_file(ygg::common::root_path() / "tests/support/data/planning/successors/initial_successors.json");
    auto result = std::vector<InitialSuccessorsCase> {};
    for (const auto& value : ygg::common::as_array(ygg::common::as_object(suite, "suite"), "cases", "suite"))
    {
        const auto& object = ygg::common::as_object(value, "case");
        auto test_case =
            InitialSuccessorsCase { ygg::common::as_string(object, "name", "case"),
                                    ygg::common::resolve_path(std::filesystem::path(BENCHMARKS_DIR), ygg::common::as_string(object, "domain_file", "case")),
                                    ygg::common::resolve_path(std::filesystem::path(BENCHMARKS_DIR), ygg::common::as_string(object, "task_file", "case")),
                                    std::nullopt,
                                    {},
                                    {} };
        if (object.contains("skipped"))
            test_case.skipped = ygg::common::as_string(object, "skipped", "case");
        else
        {
            test_case.lifted = object.at("lifted");
            test_case.ground = object.at("ground");
        }
        result.push_back(std::move(test_case));
    }
    return result;
}

/// The first place two JSON values differ ("" where none): numbers compared within the float tolerance of the repository
/// (ygg::FloatTolerance: a build's arithmetic may differ in its last bits), everything else exactly.
std::string first_difference(const boost::json::value& actual, const boost::json::value& expected, const std::string& path = "")
{
    if (actual.is_number() && expected.is_number())
    {
        const auto lhs = actual.to_number<double>();
        const auto rhs = expected.to_number<double>();
        return lhs == rhs || std::abs(lhs - rhs) <= ygg::FloatTolerance<double>::tolerance(lhs, rhs) ? "" : path;
    }
    if (actual.kind() != expected.kind())
        return path;
    if (actual.is_object())
    {
        const auto& lhs = actual.as_object();
        const auto& rhs = expected.as_object();
        if (lhs.size() != rhs.size())
            return path;
        for (const auto& [key, value] : lhs)
        {
            const auto* other = rhs.if_contains(key);
            if (other == nullptr)
                return path + "/" + std::string(key);
            if (auto difference = first_difference(value, *other, path + "/" + std::string(key)); !difference.empty())
                return difference;
        }
        return "";
    }
    if (actual.is_array())
    {
        const auto& lhs = actual.as_array();
        const auto& rhs = expected.as_array();
        if (lhs.size() != rhs.size())
            return path;
        for (std::size_t i = 0; i < lhs.size(); ++i)
            if (auto difference = first_difference(lhs[i], rhs[i], path + "[" + std::to_string(i) + "]"); !difference.empty())
                return difference;
        return "";
    }
    return actual == expected ? "" : path;
}

/// The successors of the task's initial node as the generator writes them (for_each_successor), serialized: each a reference into the tables, and the
/// tables (initial_successor_tables.hpp).
template<TaskKind Kind>
boost::json::value serialized_initial_successors(const planning::TaskPtr<Kind>& task, const ygg::ExecutionContextPtr& execution_context)
{
    auto repository = planning::StateRepository<Kind>::create(task);
    auto axiom_evaluator = planning::AxiomEvaluator<Kind>(task, execution_context);
    auto generator = planning::SuccessorGenerator<Kind>(task, execution_context);
    const auto initial = planning::get_initial_node(*repository, axiom_evaluator);
    auto dictionaries = ygg::serialization::Dictionaries {};
    register_initial_successor_tables<Kind>(dictionaries);
    // The generator's successors as it writes them (builders), each kept as a clone: they outlive the tables, which read
    // them (each as its borrowed view).
    auto successors = std::vector<planning::OwnedSuccessorBuilderView<planning::GeneralTag, Kind>> {};
    const auto keep = [&](const planning::TransitionLabel<Kind>& /*transition*/, planning::SuccessorBuilder<planning::GeneralTag, Kind>& successor)
    {
        // The axiom step, without interning: a transition whose outcomes all fail there is not applicable.
        if (planning::extend_outcomes(generator, *repository, axiom_evaluator, successor))
            successors.emplace_back(urd::clone(successor), repository.get());
        return true;
    };
    planning::for_each_successor<planning::GeneralTag>(generator, initial, keep);
    auto serialized = boost::json::array {};
    for (const planning::SuccessorBuilderView<planning::GeneralTag, Kind>& successor : successors)
        serialized.push_back(dictionaries.serialize(successor));
    return boost::json::object { { "successors", std::move(serialized) }, { "tables", dictionaries.tables() } };
}

/// Expanding the initial node again, once its builders have grown, allocates and frees nothing: its successors are written
/// into the repository's reused builders and none is kept.
template<TaskKind Kind>
void expect_no_allocations_after_warmup(const planning::TaskPtr<Kind>& task, const ygg::ExecutionContextPtr& execution_context)
{
    if constexpr (!urd::testing::counts_allocations)
        return;
    auto repository = planning::StateRepository<Kind>::create(task);
    auto axiom_evaluator = planning::AxiomEvaluator<Kind>(task, execution_context);
    auto generator = planning::SuccessorGenerator<Kind>(task, execution_context);
    const auto initial = planning::get_initial_node(*repository, axiom_evaluator);
    const auto visit = [](const planning::TransitionLabel<Kind>& /*transition*/, planning::SuccessorBuilder<planning::GeneralTag, Kind>& /*successor*/)
    { return true; };
    const auto expand = [&] { return planning::for_each_successor<planning::GeneralTag>(generator, initial, visit); };
    ASSERT_TRUE(expand());
    ASSERT_TRUE(expand());
    {
        const auto counted = urd::testing::Counted();
        static_cast<void>(expand());
    }
    const auto counts = urd::testing::counts();
    EXPECT_EQ(counts.allocations, 0);
    EXPECT_EQ(counts.deallocations, 0);
}

class InitialSuccessorsTest : public ::testing::TestWithParam<InitialSuccessorsCase>
{
};

TEST_P(InitialSuccessorsTest, SuccessorsMatchFixtureGroundAndLifted)
{
    const auto& test_case = GetParam();
    if (test_case.skipped)
    {
        // Still unsupported, for the same reason: once tyr parses it, the fixture is regenerated.
        try
        {
            static_cast<void>(make_test_parser(test_case.domain_file).parse_task(test_case.task_file));
            ADD_FAILURE() << test_case.name << " parses now: regenerate the fixture";
        }
        catch (const std::exception& error)
        {
            EXPECT_EQ(std::string(error.what()), *test_case.skipped);
        }
        return;
    }
    const auto execution_context = ygg::ExecutionContext::create(1);
    const auto lifted = planning::Task<LiftedTag>::create(make_test_parser(test_case.domain_file).parse_task(test_case.task_file));
    EXPECT_EQ(first_difference(serialized_initial_successors<LiftedTag>(lifted, execution_context), test_case.lifted), "") << "lifted";
    {
        SCOPED_TRACE("lifted");
        expect_no_allocations_after_warmup<LiftedTag>(lifted, execution_context);
    }
    const auto ground = lifted->instantiate_ground_task(*execution_context).task;
    ASSERT_TRUE(ground);
    EXPECT_EQ(first_difference(serialized_initial_successors<GroundTag>(ground, execution_context), test_case.ground), "") << "ground";
    {
        SCOPED_TRACE("ground");
        expect_no_allocations_after_warmup<GroundTag>(ground, execution_context);
    }
}

INSTANTIATE_TEST_SUITE_P(TyrPlanningInitialSuccessors,
                         InitialSuccessorsTest,
                         ::testing::ValuesIn(load_cases()),
                         [](const ::testing::TestParamInfo<InitialSuccessorsCase>& info)
                         {
                             // A test name of letters, digits and underscores.
                             auto name = info.param.name;
                             for (auto& character : name)
                                 if (!std::isalnum(static_cast<unsigned char>(character)))
                                     character = '_';
                             return name;
                         });
}
}
