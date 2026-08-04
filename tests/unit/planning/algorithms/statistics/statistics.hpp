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

#ifndef TYR_TESTS_PLANNING_ALGORITHMS_SEARCH_STATISTICS_HPP_
#define TYR_TESTS_PLANNING_ALGORITHMS_SEARCH_STATISTICS_HPP_

#include "planning/parser.hpp"
#include "tyr/formalism/planning/formatter.hpp"

#include <boost/json.hpp>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <gtest/gtest.h>
#include <memory>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <string>
#include <tyr/formalism/formalism.hpp>
#include <tyr/planning/planning.hpp>
#include <utility>
#include <vector>
#include <yggdrasil/serialization/json.hpp>
#include <yggdrasil/serialization/json_suite.hpp>

namespace tyr::tests
{

struct PlanStatistics
{
    ygg::float_t cost;
    uint64_t length;
    std::vector<std::string> actions;
};

/// Expected outcome of one search configuration: event-handler counters plus an optional plan.
/// Pinning these fixes the exact search trajectory, making them cross-platform determinism regression tests.
struct SearchStatistics
{
    planning::ProgressStatistics::Snapshot statistics;
    std::optional<PlanStatistics> plan;
};

/// Parse the four counters from an object that carries them inline (returns nullopt when absent).
inline std::optional<planning::ProgressStatistics::Snapshot> parse_optional_counters(const boost::json::object& object)
{
    if (!object.if_contains("num_generated"))
        return std::nullopt;
    return planning::ProgressStatistics::Snapshot { boost::json::value_to<uint64_t>(object.at("num_generated")),
                                                    boost::json::value_to<uint64_t>(object.at("num_expanded")),
                                                    boost::json::value_to<uint64_t>(object.at("num_deadends")),
                                                    boost::json::value_to<uint64_t>(object.at("num_pruned")) };
}

inline void expect_counters(const planning::ProgressStatistics::Snapshot& expected, const planning::Statistics& actual)
{
    EXPECT_EQ(actual.get_num_generated(), expected.get_num_generated());
    EXPECT_EQ(actual.get_num_expanded(), expected.get_num_expanded());
    EXPECT_EQ(actual.get_num_deadends(), expected.get_num_deadends());
    EXPECT_EQ(actual.get_num_pruned(), expected.get_num_pruned());
}

inline std::optional<PlanStatistics> parse_optional_plan(const boost::json::object& object)
{
    const auto* value = object.if_contains("plan");
    if (!value)
        return std::nullopt;

    const auto& plan = value->as_object();
    auto actions = std::vector<std::string> {};
    for (const auto& action : ygg::common::as_array(plan, "actions", "plan"))
        actions.push_back(boost::json::value_to<std::string>(action));

    return PlanStatistics { boost::json::value_to<ygg::float_t>(plan.at("cost")), boost::json::value_to<uint64_t>(plan.at("length")), std::move(actions) };
}

inline std::optional<SearchStatistics> parse_optional_statistics(const boost::json::object& object, const char* key)
{
    const auto* value = object.if_contains(key);
    if (!value)
        return std::nullopt;
    const auto& stats = value->as_object();
    return SearchStatistics { *parse_optional_counters(stats), parse_optional_plan(stats) };
}

template<TaskKind Kind>
std::vector<std::string> plan_actions(const planning::Plan<Kind>& plan)
{
    auto result = std::vector<std::string> {};
    for (const auto& labeled_node : plan.get_labeled_succ_nodes())
        result.push_back(fmt::format("{}", std::make_pair(labeled_node.label, formalism::planning::PlanFormatting())));
    return result;
}

template<TaskKind Kind>
void expect_plan(const PlanStatistics& expected, const planning::Plan<Kind>& actual)
{
    EXPECT_EQ(actual.get_length(), expected.length);
    EXPECT_DOUBLE_EQ(static_cast<double>(actual.get_cost()), static_cast<double>(expected.cost));
    EXPECT_EQ(plan_actions(actual), expected.actions);
}

template<TaskKind Kind>
void expect_statistics(const SearchStatistics& expected, const planning::Statistics& actual, const planning::SearchResult<Kind>& result)
{
    expect_counters(expected.statistics, actual);

    ASSERT_EQ(result.plan.has_value(), expected.plan.has_value());
    if (expected.plan)
        expect_plan(*expected.plan, *result.plan);
}

inline planning::SearchStatus parse_search_status(const std::string& status)
{
    if (status == "SOLVED")
        return planning::SearchStatus::SOLVED;
    if (status == "EXHAUSTED")
        return planning::SearchStatus::EXHAUSTED;
    if (status == "CYCLE")
        return planning::SearchStatus::CYCLE;
    if (status == "UNSOLVABLE")
        return planning::SearchStatus::UNSOLVABLE;
    if (status == "OUT_OF_TIME")
        return planning::SearchStatus::OUT_OF_TIME;
    if (status == "OUT_OF_MEMORY")
        return planning::SearchStatus::OUT_OF_MEMORY;
    if (status == "OUT_OF_STATES")
        return planning::SearchStatus::OUT_OF_STATES;
    if (status == "FAILED")
        return planning::SearchStatus::FAILED;
    if (status == "IN_PROGRESS")
        return planning::SearchStatus::IN_PROGRESS;

    throw std::runtime_error("parse_search_status(...): unknown search status: " + status);
}

/// One fixture case: expected statistics per configuration key. Split fixtures use "default"
/// for heuristic-free searches and keys such as "hmax_unit" otherwise; any case key whose
/// object value holds a "num_generated" member is parsed as a configuration.
struct SearchCase
{
    std::string name;
    std::filesystem::path domain_file;
    std::filesystem::path task_file;
    std::vector<std::pair<std::string, SearchStatistics>> configs;
};

inline void PrintTo(const SearchCase& test_case, std::ostream* os) { *os << test_case.name << " (" << test_case.task_file << ")"; }

inline SearchCase parse_search_case(const boost::json::object& suite, const boost::json::object& object)
{
    auto result = SearchCase { ygg::common::as_string(object, "name", "case"),
                               ygg::common::resolve_path(std::filesystem::path(BENCHMARKS_DIR), ygg::common::as_string(object, "domain_file", "case")),
                               ygg::common::resolve_path(std::filesystem::path(BENCHMARKS_DIR), ygg::common::as_string(object, "task_file", "case")),
                               {} };
    for (const auto& [key, value] : object)
        if (value.is_object() && value.as_object().if_contains("num_generated"))
            result.configs.emplace_back(std::string(key), *parse_optional_statistics(object, std::string(key).c_str()));
    return result;
}

inline std::vector<SearchCase> load_search_cases(const std::filesystem::path& fixture)
{
    const auto suite = ygg::common::load_json_file(ygg::common::root_path() / fixture);
    const auto& suite_object = ygg::common::as_object(suite, "suite");
    auto result = std::vector<SearchCase> {};
    for (const auto& case_value : ygg::common::as_array(suite_object, "cases", "suite"))
        result.push_back(parse_search_case(suite_object, ygg::common::as_object(case_value, "case")));
    return result;
}

template<TaskKind Kind>
struct SearchContext
{
    std::shared_ptr<ygg::ExecutionContext> execution_context;
    planning::TaskPtr<Kind> task;
    planning::SuccessorGeneratorPtr<Kind> successor_generator;
};

template<TaskKind Kind>
SearchContext<Kind> create_search_context(const std::filesystem::path& domain_file, const std::filesystem::path& task_file)
{
    auto execution_context = ygg::ExecutionContext::create(1);
    auto lifted_task = planning::Task<LiftedTag>::create(make_test_parser(domain_file).parse_task(task_file));

    auto task = planning::TaskPtr<Kind> {};
    if constexpr (std::is_same_v<Kind, LiftedTag>)
        task = std::move(lifted_task);
    else
        task = lifted_task->instantiate_ground_task(*execution_context).task;

    auto axiom_evaluator = planning::AxiomEvaluatorFactory<Kind>().create(task, execution_context);
    auto state_repository = planning::StateRepositoryFactory<Kind>().create(task, axiom_evaluator);
    auto successor_generator = planning::SuccessorGeneratorFactory<Kind>().create(task, execution_context, state_repository);

    return SearchContext<Kind> { std::move(execution_context), std::move(task), std::move(successor_generator) };
}

template<TaskKind Kind>
void expect_repository_statistics(const SearchContext<Kind>& context, const planning::Statistics& statistics)
{
    const auto& state_repository = *context.successor_generator->get_state_repository();
    const auto& task_repository = *context.task->get_repository();

    EXPECT_EQ(statistics.get_num_registered_states(), state_repository.num_states());
    EXPECT_EQ(statistics.get_state_storage_memory_usage(), state_repository.memory_usage());
    EXPECT_EQ(statistics.get_action_bindings_memory_usage(), task_repository.template memory_usage<formalism::RelationBinding<formalism::planning::Action>>());
    EXPECT_EQ(statistics.get_predicate_bindings_memory_usage(),
              task_repository.template memory_usage<formalism::RelationBinding<formalism::Predicate<formalism::StaticTag>>>()
                  + task_repository.template memory_usage<formalism::RelationBinding<formalism::Predicate<formalism::FluentTag>>>()
                  + task_repository.template memory_usage<formalism::RelationBinding<formalism::Predicate<formalism::DerivedTag>>>());
    EXPECT_EQ(statistics.get_axiom_bindings_memory_usage(), task_repository.template memory_usage<formalism::RelationBinding<formalism::planning::Axiom>>());
    EXPECT_EQ(statistics.get_function_bindings_memory_usage(),
              task_repository.template memory_usage<formalism::RelationBinding<formalism::Function<formalism::StaticTag>>>()
                  + task_repository.template memory_usage<formalism::RelationBinding<formalism::Function<formalism::FluentTag>>>()
                  + task_repository.template memory_usage<formalism::RelationBinding<formalism::Function<formalism::AuxiliaryTag>>>());
}

inline std::pair<std::string, CostMode> parse_costed_heuristic_key(const std::string& key)
{
    const auto pos = key.rfind('_');
    if (pos == std::string::npos)
        throw std::invalid_argument("parse_costed_heuristic_key(...): missing cost suffix in key: " + key);

    const auto suffix = key.substr(pos + 1);
    if (suffix == "unit")
        return { key.substr(0, pos), CostMode::UNIT };
    if (suffix == "general")
        return { key.substr(0, pos), CostMode::GENERAL };

    throw std::invalid_argument("parse_costed_heuristic_key(...): unknown cost suffix in key: " + key);
}

template<TaskKind Kind>
planning::HeuristicPtr<Kind> create_search_heuristic(const std::string& name, const SearchContext<Kind>& context, CostMode cost_mode)
{
    if (name == "blind")
        return planning::BlindHeuristic<Kind>::create();
    if (name == "hmax")
        return planning::MaxRPGHeuristic<Kind>::create(context.task, context.execution_context, cost_mode);
    if (name == "hadd")
        return planning::AddRPGHeuristic<Kind>::create(context.task, context.execution_context, cost_mode);
    if (name == "hff")
        return planning::FFRPGHeuristic<Kind>::create(context.task, context.execution_context, cost_mode);
    if (name == "hlmcut")
        return planning::LMCutHeuristic<Kind>::create(context.task, context.execution_context, cost_mode);
    throw std::invalid_argument("create_search_heuristic(...): unknown heuristic: " + name);
}

}

#endif
