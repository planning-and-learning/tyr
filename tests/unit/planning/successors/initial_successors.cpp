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

// The successors of each test task's initial node, ground and lifted, against the fixture
// (tests/support/generators/planning/successors/generate_initial_successors.py): each successor's label and
// distributions, each outcome's probability, failure, metric and state, in the generator's form and order; and that
// expanding it again once warm allocates nothing. A task tyr does not parse yet fails to parse with the fixture's reason.

#include "planning/parser.hpp"
#include "tyr/formalism/planning/formatter.hpp"
#include "tyr/formalism/repository.hpp"
#include "tyr/planning/axioms/axiom_evaluator.hpp"
#include "tyr/planning/formatter.hpp"
#include "tyr/planning/states/state_repository.hpp"
#include "tyr/planning/states/state_view.hpp"
#include "tyr/planning/successors/ground/successor_generator.hpp"
#include "tyr/planning/successors/lifted/successor_generator.hpp"
#include "tyr/planning/successors/successor_view.hpp"
#include "tyr/planning/successors/successors.hpp"
#include "tyr/planning/tasks/ground/task.hpp"
#include "tyr/planning/tasks/lifted/task.hpp"

#include <algorithm>
#include <boost/json.hpp>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fmt/format.h>
#include <gtest/gtest.h>
#include <limits>
#include <optional>
#include <string>
#include <tuple>
#include <utility>
#include <vector>
#include <urd/elements/view.hpp>
#include <urd/testing/allocations.hpp>
#include <yggdrasil/execution/onetbb.hpp>
#include <yggdrasil/serialization/json.hpp>
#include <yggdrasil/serialization/json_suite.hpp>

namespace tyr::tests
{
namespace
{
/// A time point's earliest and latest time (infinity: unbounded).
using Window = std::pair<double, double>;
/// An open action: its plan step, its start time point and its duration bounds.
using OpenEntry = std::tuple<std::string, std::uint32_t, double, double>;
/// The temporal part: its open actions, its timed initial literals that happened, its windows.
using Temporal = std::tuple<std::vector<OpenEntry>, std::uint32_t, std::vector<Window>>;
/// A state: its fluent atoms, its derived atoms, its fluent function values (each sorted), its temporal part.
using StateEntry = std::tuple<std::vector<std::string>, std::vector<std::string>, std::vector<std::pair<std::string, double>>, Temporal>;
/// An outcome: its probability, whether it fails, its metric and its state.
using Outcome = std::tuple<double, bool, double, StateEntry>;
/// A successor: its label and its distributions.
using Successor = std::pair<std::string, std::vector<std::vector<Outcome>>>;

/// A number rounded to the fixture's precision, so that both sides compare exactly.
double rounded(double value)
{
    if (std::isinf(value))
        return value;
    return std::round(value * 1e9) / 1e9 + 0.0;
}

/// The generator's order: a distribution's outcomes sorted, a successor's distributions sorted, the successors sorted.
std::vector<Successor> sorted(std::vector<Successor> successors)
{
    for (auto& [label, distributions] : successors)
    {
        for (auto& distribution : distributions)
            std::ranges::sort(distribution);
        std::ranges::sort(distributions);
    }
    std::ranges::sort(successors);
    return successors;
}

struct InitialSuccessorsCase
{
    std::string name;
    std::filesystem::path domain_file;
    std::filesystem::path task_file;
    /// The parser's reason where tyr does not parse the task yet, else its successors.
    std::optional<std::string> skipped;
    std::vector<Successor> successors;
};

std::vector<std::string> strings_of(const boost::json::value& value)
{
    auto result = std::vector<std::string> {};
    for (const auto& element : value.as_array())
        result.emplace_back(element.as_string());
    return result;
}

StateEntry state_of_json(const boost::json::object& state)
{
    auto fterm_values = std::vector<std::pair<std::string, double>> {};
    for (const auto& pair : state.at("fluent_fterm_values").as_array())
        fterm_values.emplace_back(std::string(pair.as_array().at(0).as_string()), rounded(pair.as_array().at(1).to_number<double>()));
    const auto& temporal = state.at("temporal").as_object();
    auto open = std::vector<OpenEntry> {};
    for (const auto& entry : temporal.at("open").as_array())
    {
        const auto& object = entry.as_object();
        open.emplace_back(std::string(object.at("action").as_string()),
                          object.at("start").to_number<std::uint32_t>(),
                          rounded(object.at("min_duration").to_number<double>()),
                          object.at("max_duration").is_null() ? std::numeric_limits<double>::infinity() : rounded(object.at("max_duration").to_number<double>()));
    }
    auto windows = std::vector<Window> {};
    for (const auto& window : temporal.at("windows").as_array())
    {
        const auto& bounds = window.as_array();
        windows.emplace_back(rounded(bounds.at(0).to_number<double>()),
                             bounds.at(1).is_null() ? std::numeric_limits<double>::infinity() : rounded(bounds.at(1).to_number<double>()));
    }
    return { strings_of(state.at("fluent_atoms")),
             strings_of(state.at("derived_atoms")),
             std::move(fterm_values),
             Temporal { std::move(open), temporal.at("til_progress").to_number<std::uint32_t>(), std::move(windows) } };
}

std::vector<InitialSuccessorsCase> load_cases()
{
    const auto suite = ygg::common::load_json_file(ygg::common::root_path() / "tests/support/data/planning/successors/initial_successors.json");
    auto result = std::vector<InitialSuccessorsCase> {};
    for (const auto& value : ygg::common::as_array(ygg::common::as_object(suite, "suite"), "cases", "suite"))
    {
        const auto& object = ygg::common::as_object(value, "case");
        auto test_case = InitialSuccessorsCase { ygg::common::as_string(object, "name", "case"),
                                                 ygg::common::resolve_path(std::filesystem::path(BENCHMARKS_DIR), ygg::common::as_string(object, "domain_file", "case")),
                                                 ygg::common::resolve_path(std::filesystem::path(BENCHMARKS_DIR), ygg::common::as_string(object, "task_file", "case")),
                                                 std::nullopt,
                                                 {} };
        if (object.contains("skipped"))
            test_case.skipped = ygg::common::as_string(object, "skipped", "case");
        else
            for (const auto& entry : object.at("successors").as_array())
            {
                auto successor = Successor { std::string(entry.as_object().at("label").as_string()), {} };
                for (const auto& distribution : entry.as_object().at("distributions").as_array())
                {
                    auto& outcomes = successor.second.emplace_back();
                    for (const auto& outcome_value : distribution.as_array())
                    {
                        const auto& outcome = outcome_value.as_object();
                        outcomes.emplace_back(rounded(outcome.at("probability").to_number<double>()),
                                              outcome.at("failed").as_bool(),
                                              rounded(outcome.at("metric").to_number<double>()),
                                              state_of_json(outcome.at("state").as_object()));
                    }
                }
                test_case.successors.push_back(std::move(successor));
            }
        test_case.successors = sorted(std::move(test_case.successors));
        result.push_back(std::move(test_case));
    }
    return result;
}

/// An action binding as a plan step: its action's original name and arguments.
template<typename Binding>
std::string plan_step(const Binding& binding)
{ return fmt::format("{}", std::make_pair(binding, formalism::PlanFormatting())); }

template<TaskKind Kind, typename State>
StateEntry state_of(const State& state, const formalism::Repository& repository)
{
    auto fluent_atoms = std::vector<std::string> {};
    for (const auto atom : state.template get_atoms<formalism::FluentTag>())
        fluent_atoms.push_back(fmt::format("{}", atom));
    std::ranges::sort(fluent_atoms);
    auto derived_atoms = std::vector<std::string> {};
    for (const auto atom : state.template get_atoms<formalism::DerivedTag>())
        derived_atoms.push_back(fmt::format("{}", atom));
    std::ranges::sort(derived_atoms);
    auto fterm_values = std::vector<std::pair<std::string, double>> {};
    for (const auto& [term, value] : state.template get_fterm_values<formalism::FluentTag>())
        fterm_values.emplace_back(fmt::format("{}", term), rounded(value));
    std::ranges::sort(fterm_values);

    const auto& parts = urd::built_view(state.get_temporal_storage()).get_parts();
    auto open = std::vector<OpenEntry> {};
    for (const auto& open_action : parts.open)
        open.emplace_back(plan_step(urd::resolve(open_action.action, repository)),
                          open_action.start,
                          rounded(open_action.min_duration),
                          rounded(open_action.max_duration));
    auto windows = std::vector<Window> {};
    for (std::size_t t = 0; t < parts.network.size(); ++t)
        windows.emplace_back(rounded(parts.network.earliest(t)), rounded(parts.network.latest(t)));
    return { std::move(fluent_atoms), std::move(derived_atoms), std::move(fterm_values), Temporal { std::move(open), parts.til_progress, std::move(windows) } };
}

template<TaskKind Kind>
std::vector<Successor> initial_successors(const planning::TaskPtr<Kind>& task, const ygg::ExecutionContextPtr& execution_context)
{
    auto repository = planning::StateRepository<Kind>::create(task);
    auto axiom_evaluator = planning::AxiomEvaluator<Kind>(task, execution_context);
    auto generator = planning::SuccessorGenerator<Kind>(task, execution_context);
    const auto initial = planning::get_initial_node(*repository, axiom_evaluator);
    const auto& formalism_repository = *task->get_repository();
    auto result = std::vector<Successor> {};
    for (const auto& successor : planning::get_successors(generator, *repository, axiom_evaluator, initial))
    {
        const auto label = successor.get_label();
        auto text = fmt::format("{}", label.get_kind());
        if (const auto binding = label.get_binding())
            urd::visit_view(*binding, [&](const auto& row) { text += " " + plan_step(row); });
        if (label.get_kind() == planning::TransitionKind::END)
            text += fmt::format(" #{}", label.get_open());
        auto& entry = result.emplace_back(std::move(text), std::vector<std::vector<Outcome>> {});
        for (const auto distribution : successor.get_distributions())
        {
            auto& outcomes = entry.second.emplace_back();
            for (const auto outcome : distribution.get_outcomes())
            {
                const auto node = outcome.get_node();
                outcomes.emplace_back(rounded(outcome.get_probability()),
                                      outcome.get_failed(),
                                      rounded(node.get_metric()),
                                      state_of<Kind>(urd::built_view(node.get_state()), formalism_repository));
            }
        }
    }
    return sorted(std::move(result));
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
    const auto visit = [](planning::SuccessorBuilder<Kind>& /*successor*/) { return true; };
    const auto expand = [&] { return planning::for_each_successor(generator, initial, axiom_evaluator, visit); };
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
    EXPECT_EQ(initial_successors<LiftedTag>(lifted, execution_context), test_case.successors) << "lifted";
    {
        SCOPED_TRACE("lifted");
        expect_no_allocations_after_warmup<LiftedTag>(lifted, execution_context);
    }
    const auto ground = lifted->instantiate_ground_task(*execution_context).task;
    ASSERT_TRUE(ground);
    EXPECT_EQ(initial_successors<GroundTag>(ground, execution_context), test_case.successors) << "ground";
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
