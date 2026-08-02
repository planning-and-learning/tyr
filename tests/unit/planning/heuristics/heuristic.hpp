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

#ifndef TYR_TESTS_PLANNING_HEURISTICS_HEURISTIC_HPP_
#define TYR_TESTS_PLANNING_HEURISTICS_HEURISTIC_HPP_

#include "planning/parser.hpp"
#include "tyr/formalism/formalism.hpp"
#include "tyr/planning/planning.hpp"

#include <algorithm>
#include <boost/json.hpp>
#include <filesystem>
#include <gtest/gtest.h>
#include <limits>
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <type_traits>
#include <vector>
#include <yggdrasil/serialization/json.hpp>
#include <yggdrasil/serialization/json_suite.hpp>

namespace p = tyr::planning;

namespace tyr::tests
{

struct HeuristicExpectation
{
    ::tyr::CostMode cost_mode;
    std::optional<ygg::float_t> h;
};

struct HeuristicCase
{
    std::string name;
    std::filesystem::path domain_file;
    std::filesystem::path task_file;
    std::vector<std::pair<std::string, HeuristicExpectation>> configs;
};

inline void PrintTo(const HeuristicCase& test_case, std::ostream* os) { *os << test_case.name << " (" << test_case.task_file << ")"; }

inline std::optional<ygg::float_t> parse_optional_float(const boost::json::object& object, const char* key)
{
    const auto* value = object.if_contains(key);
    if (!value)
        return std::nullopt;
    return boost::json::value_to<ygg::float_t>(*value);
}

inline ::tyr::CostMode parse_cost_mode(const std::string& key)
{
    if (key == "unit")
        return ::tyr::CostMode::UNIT;
    if (key == "general")
        return ::tyr::CostMode::GENERAL;
    throw std::runtime_error("parse_cost_mode(...): unknown cost mode: " + key);
}

inline HeuristicExpectation parse_expectation(const std::string& key, const boost::json::object& object)
{
    return HeuristicExpectation { parse_cost_mode(key), parse_optional_float(object, "h") };
}

inline HeuristicCase parse_case(const boost::json::object& suite, const boost::json::object& object)
{
    auto result = HeuristicCase { ygg::common::as_string(object, "name", "case"),
                                  ygg::common::resolve_path(std::filesystem::path(BENCHMARKS_DIR), ygg::common::as_string(object, "domain_file", "case")),
                                  ygg::common::resolve_path(std::filesystem::path(BENCHMARKS_DIR), ygg::common::as_string(object, "task_file", "case")),
                                  {} };
    if (const auto* configs_value = object.if_contains("configs"))
        for (const auto& [key, value] : configs_value->as_object())
            result.configs.emplace_back(std::string(key), parse_expectation(std::string(key), value.as_object()));
    return result;
}

inline std::vector<HeuristicCase> load_cases()
{
    const auto suite = ygg::common::load_json_file(ygg::common::root_path() / kHeuristicFixture);
    const auto& suite_object = ygg::common::as_object(suite, "suite");
    auto result = std::vector<HeuristicCase> {};
    for (const auto& case_value : ygg::common::as_array(suite_object, "cases", "suite"))
        result.push_back(parse_case(suite_object, ygg::common::as_object(case_value, "case")));
    return result;
}

template<::tyr::TaskKind Kind>
struct HeuristicContext
{
    std::shared_ptr<ygg::ExecutionContext> execution_context;
    p::TaskPtr<Kind> task;
    p::SuccessorGeneratorPtr<Kind> successor_generator;
};

template<::tyr::TaskKind Kind>
HeuristicContext<Kind> create_heuristic_context(const std::filesystem::path& domain_file, const std::filesystem::path& task_file)
{
    auto execution_context = ygg::ExecutionContext::create(1);
    auto lifted_task = p::Task<::tyr::LiftedTag>::create(make_test_parser(domain_file).parse_task(task_file));

    auto task = p::TaskPtr<Kind> {};
    if constexpr (std::is_same_v<Kind, ::tyr::LiftedTag>)
        task = std::move(lifted_task);
    else
        task = lifted_task->instantiate_ground_task(*execution_context).task;

    auto axiom_evaluator = p::AxiomEvaluatorFactory<Kind>().create(task, execution_context);
    auto state_repository = p::StateRepositoryFactory<Kind>().create(task, axiom_evaluator);
    auto successor_generator = p::SuccessorGeneratorFactory<Kind>().create(task, execution_context, state_repository);

    return HeuristicContext<Kind> { std::move(execution_context), std::move(task), std::move(successor_generator) };
}

inline bool should_check(const HeuristicExpectation& expectation) { return expectation.h.has_value(); }

template<::tyr::TaskKind Kind>
HeuristicContext<Kind> create_preferred_action_reset_context()
{
    const auto fixture_dir = ygg::common::root_path() / "tests/fixtures/planning/heuristics/preferred_action_reset";
    return create_heuristic_context<Kind>(fixture_dir / "domain.pddl", fixture_dir / "problem.pddl");
}

template<::tyr::TaskKind Kind>
void expect_preferred_actions_reset_after_dead_end()
{
    auto context = create_preferred_action_reset_context<Kind>();
    auto heuristic = p::FFRPGHeuristic<Kind>::create(context.task, context.execution_context);
    const auto initial_node = context.successor_generator->get_initial_node();

    EXPECT_EQ(heuristic->evaluate(initial_node.get_state()), 1);
    EXPECT_FALSE(heuristic->get_preferred_actions().empty());

    const auto successors = context.successor_generator->get_labeled_successor_nodes(initial_node);
    const auto dead_end = std::ranges::find_if(successors, [](const auto& successor) { return successor.label.get_relation().get_name().str() == "consume"; });
    ASSERT_NE(dead_end, successors.end());
    EXPECT_EQ(heuristic->evaluate(dead_end->node.get_state()), std::numeric_limits<ygg::float_t>::infinity());
    EXPECT_TRUE(heuristic->get_preferred_actions().empty());
}

template<::tyr::TaskKind Kind>
void expect_builtin_set_goal_reconfigures_evaluator()
{
    auto context = create_preferred_action_reset_context<Kind>();
    auto heuristic = p::AddRPGHeuristic<Kind>::create(context.task, context.execution_context);
    const auto initial_node = context.successor_generator->get_initial_node();

    EXPECT_EQ(heuristic->evaluate(initial_node.get_state()), 1);

    const auto successors = context.successor_generator->get_labeled_successor_nodes(initial_node);
    const auto achieve = std::ranges::find_if(successors, [](const auto& successor) { return successor.label.get_relation().get_name().str() == "achieve"; });
    ASSERT_NE(achieve, successors.end());
    heuristic->set_goal(context.successor_generator->ground_action(achieve->label).get_condition());

    EXPECT_EQ(heuristic->evaluate(initial_node.get_state()), 0);
}

inline void expect_optional_eq(ygg::float_t actual, std::optional<ygg::float_t> expected)
{
    if (expected)
    {
        EXPECT_DOUBLE_EQ(static_cast<double>(actual), static_cast<double>(*expected));
    }
}

class HeuristicFixtureTest : public ::testing::TestWithParam<HeuristicCase>
{
};

TEST_P(HeuristicFixtureTest, InitialStateMatchesFixture)
{
    const auto& test_case = GetParam();
    auto context = create_heuristic_context<HeuristicTaskKind>(test_case.domain_file, test_case.task_file);
    const auto initial_state = context.successor_generator->get_initial_node().get_state();

    for (const auto& [key, expectation] : test_case.configs)
    {
        if (!should_check(expectation))
            continue;

        SCOPED_TRACE(key);
        auto heuristic = TestedHeuristic<HeuristicTaskKind>::create(context.task, context.execution_context, expectation.cost_mode);
        const auto value = heuristic->evaluate(initial_state);

        ASSERT_NE(value, std::numeric_limits<ygg::float_t>::infinity());
        expect_optional_eq(value, expectation.h);
    }
}

INSTANTIATE_TEST_SUITE_P(TyrPlanningHeuristicFixture,
                         HeuristicFixtureTest,
                         ::testing::ValuesIn(load_cases()),
                         [](const testing::TestParamInfo<HeuristicCase>& info) { return info.param.name; });

}

#endif
