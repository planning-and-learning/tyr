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

#include "tyr/datalog/lifted/bottom_up.hpp"

#include "planning/parser.hpp"
#include "tyr/datalog/lifted/contexts/program.hpp"
#include "tyr/datalog/lifted/programs/program.hpp"
#include "tyr/formalism/datalog/canonicalization.hpp"
#include "tyr/formalism/datalog/datas.hpp"
#include "tyr/planning/lifted/programs/ground.hpp"
#include "tyr/planning/planning.hpp"

#include <algorithm>
#include <array>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/json.hpp>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <memory>
#include <ostream>
#include <ranges>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>
#include <yggdrasil/serialization/json.hpp>
#include <yggdrasil/serialization/json_suite.hpp>

namespace d = tyr::datalog;
namespace f = tyr::formalism;
namespace fd = tyr::formalism::datalog;
namespace p = tyr::planning;

namespace tyr::tests
{
namespace
{
inline constexpr const char* kBottomUpFixture = "tests/fixtures/datalog/algorithms/lifted/bottom_up.json.gz";
inline constexpr const char* kBenchmarksFixture = "tests/fixtures/planning/benchmarks.json";
inline constexpr auto kConfigNames = std::array<std::string_view, 3> { "applicable_action", "axiom_evaluator", "ground_task" };

struct AtomBinding
{
    std::string objects;

    friend bool operator==(const AtomBinding&, const AtomBinding&) = default;
};

struct PredicateAtoms
{
    std::string predicate;
    ygg::uint_t arity;
    std::vector<AtomBinding> bindings;

    friend bool operator==(const PredicateAtoms&, const PredicateAtoms&) = default;
};

using AtomsByPredicate = std::vector<PredicateAtoms>;

struct BottomUpCase
{
    std::string name;
    std::filesystem::path domain_file;
    std::filesystem::path task_file;
    std::array<AtomsByPredicate, kConfigNames.size()> configs;
};

void PrintTo(const BottomUpCase& test_case, std::ostream* os) { *os << test_case.name << " (" << test_case.task_file << ')'; }

AtomBinding parse_atom_binding(const boost::json::object& object) { return AtomBinding { ygg::common::as_string(object, "objects", "atom binding") }; }

PredicateAtoms parse_predicate_atoms(const boost::json::object& object)
{
    auto bindings = std::vector<AtomBinding> {};
    for (const auto& value : ygg::common::as_array(object, "bindings", "predicate atoms"))
        bindings.push_back(parse_atom_binding(ygg::common::as_object(value, "atom binding")));
    std::ranges::sort(bindings, {}, &AtomBinding::objects);

    return PredicateAtoms { ygg::common::as_string(object, "predicate", "predicate atoms"),
                            boost::json::value_to<ygg::uint_t>(object.at("arity")),
                            std::move(bindings) };
}

AtomsByPredicate parse_atoms_by_predicate(const boost::json::object& object)
{
    auto result = AtomsByPredicate {};
    for (const auto& value : ygg::common::as_array(object, "atoms", "configuration"))
        result.push_back(parse_predicate_atoms(ygg::common::as_object(value, "predicate atoms")));
    return result;
}

size_t get_config_index(std::string_view config)
{
    const auto it = std::ranges::find(kConfigNames, config);
    if (it == kConfigNames.end())
        throw std::runtime_error("unknown bottom-up configuration: " + std::string(config));
    return static_cast<size_t>(it - kConfigNames.begin());
}

BottomUpCase parse_case(const boost::json::object& object)
{
    auto result = BottomUpCase { ygg::common::as_string(object, "name", "case"),
                                 ygg::common::resolve_path(std::filesystem::path(BENCHMARKS_DIR), ygg::common::as_string(object, "domain_file", "case")),
                                 ygg::common::resolve_path(std::filesystem::path(BENCHMARKS_DIR), ygg::common::as_string(object, "task_file", "case")),
                                 {} };

    auto seen = std::array<bool, kConfigNames.size()> {};

    if (const auto* configs = object.if_contains("configs"))
        for (const auto& [name, value] : configs->as_object())
        {
            const auto config = std::string(name);
            const auto index = get_config_index(config);
            if (seen[index])
                throw std::runtime_error("duplicate bottom-up configuration in " + result.name + ": " + config);
            seen[index] = true;
            result.configs[index] = parse_atoms_by_predicate(value.as_object());
        }

    if (!std::ranges::all_of(seen, [](bool value) { return value; }))
        throw std::runtime_error("bottom-up configurations do not match the three-config matrix in " + result.name);

    return result;
}

std::vector<BottomUpCase> load_cases()
{
    const auto fixture_path = ygg::common::root_path() / kBottomUpFixture;
    auto file = std::ifstream(fixture_path, std::ios::binary);
    if (!file)
        throw std::runtime_error("failed to open gzip JSON fixture: " + fixture_path.string());

    auto input = boost::iostreams::filtering_istream {};
    input.push(boost::iostreams::gzip_decompressor {});
    input.push(file);
    auto contents = std::ostringstream {};
    boost::iostreams::copy(input, contents);

    const auto suite = boost::json::parse(contents.str());
    const auto& suite_object = ygg::common::as_object(suite, "suite");
    const auto benchmarks = ygg::common::load_json_file(ygg::common::root_path() / kBenchmarksFixture);
    const auto& benchmark_object = ygg::common::as_object(benchmarks, "benchmark suite");
    const auto& fixture_cases = ygg::common::as_array(suite_object, "cases", "suite");
    const auto& benchmark_cases = ygg::common::as_array(benchmark_object, "cases", "benchmark suite");
    if (fixture_cases.size() != benchmark_cases.size())
        throw std::runtime_error("bottom-up fixture does not cover the benchmark manifest");

    auto result = std::vector<BottomUpCase> {};
    for (size_t i = 0; i < fixture_cases.size(); ++i)
    {
        const auto& fixture_case = ygg::common::as_object(fixture_cases[i], "case");
        const auto& benchmark_case = ygg::common::as_object(benchmark_cases[i], "benchmark case");
        for (const auto key : { "name", "domain_file", "task_file" })
            if (ygg::common::as_string(fixture_case, key, "case") != ygg::common::as_string(benchmark_case, key, "benchmark case"))
                throw std::runtime_error("bottom-up fixture case does not match the benchmark manifest at index " + std::to_string(i));
        result.push_back(parse_case(fixture_case));
    }
    return result;
}

void append_atoms_by_predicate(const d::TaggedFactSets<f::FluentTag>& fact_sets, AtomsByPredicate& result)
{
    for (const auto& set : fact_sets.predicate.get_sets())
    {
        auto bindings = std::vector<AtomBinding> {};
        for (const auto binding : set.get_bindings())
            bindings.push_back(AtomBinding { ygg::to_string(binding.get_objects()) });
        std::ranges::sort(bindings, {}, &AtomBinding::objects);

        const auto predicate = set.get_predicate();
        result.push_back(PredicateAtoms { predicate.get_name().str(), predicate.get_arity(), std::move(bindings) });
    }
}

template<typename Workspace>
AtomsByPredicate collect_atoms_by_predicate(const Workspace& workspace)
{
    auto result = AtomsByPredicate {};
    append_atoms_by_predicate(workspace.facts.fact_sets, result);
    return result;
}

class BottomUpFixtureTest : public ::testing::TestWithParam<BottomUpCase>
{
};

TEST_P(BottomUpFixtureTest, InitialStateAtomsMatchAtOneAndMaximumThreads)
{
    const auto& test_case = GetParam();

    for (const auto num_threads : { ygg::ExecutionContext::uint_t(1), ygg::ExecutionContext::get_max_num_threads() })
    {
        auto execution_context = ygg::ExecutionContext::create(num_threads);
        auto task = p::Task<::tyr::LiftedTag>::create(make_test_parser(test_case.domain_file).parse_task(test_case.task_file));
        auto axiom_evaluator = p::AxiomEvaluatorFactory<::tyr::LiftedTag>().create(task, execution_context);
        auto state_repository = p::StateRepositoryFactory<::tyr::LiftedTag>().create(task, axiom_evaluator);
        auto successor_generator = p::SuccessorGeneratorFactory<::tyr::LiftedTag>().create(task, execution_context, state_repository);
        const auto initial_node = successor_generator->get_initial_node();

        for (size_t config_index = 0; config_index < kConfigNames.size(); ++config_index)
        {
            const auto config = kConfigNames[config_index];
            const auto& expected = test_case.configs[config_index];
            SCOPED_TRACE("threads=" + std::to_string(num_threads) + ", config=" + std::string(config));

            if (config == "axiom_evaluator" && !axiom_evaluator)
            {
                EXPECT_TRUE(expected.empty());
                continue;
            }

            const auto run_config = [&]() -> AtomsByPredicate
            {
                if (config == "axiom_evaluator")
                    return collect_atoms_by_predicate(axiom_evaluator->get_workspace());

                if (config == "applicable_action")
                {
                    successor_generator->get_applicable_action_bindings(initial_node);
                    return collect_atoms_by_predicate(successor_generator->get_workspace());
                }

                if (config == "ground_task")
                {
                    auto program = p::GroundTaskProgram(task->get_task());
                    auto workspace = d::ProgramWorkspace<::tyr::LiftedTag>(program.get_datalog_program());
                    auto context = d::ProgramExecutionContext(workspace);
                    execution_context->arena().execute([&] { d::solve_bottom_up(context); });
                    return collect_atoms_by_predicate(workspace);
                }

                throw std::runtime_error("unknown bottom-up fixture configuration: " + std::string(config));
            };

            EXPECT_EQ(run_config(), expected);
        }
    }
}

TEST(TyrDatalogLiftedBottomUpTest, RejectedCanonicalTiesDoNotInternUnneededBindings)
{
    auto factory = std::make_shared<fd::RepositoryFactory>();
    auto repository = factory->create_shared();
    const auto intern = [&]<typename T>(ygg::Data<T> data)
    {
        if constexpr (requires { fd::canonicalize(data); })
            fd::canonicalize(data);
        else
            f::canonicalize(data);
        return repository->get_or_create(data).first;
    };

    const auto source = intern(ygg::Data<f::Predicate<f::FluentTag>>(std::string("source"), 1));
    const auto goal = intern(ygg::Data<f::Predicate<f::FluentTag>>(std::string("goal"), 0));
    const auto variable = intern(ygg::Data<f::Variable>(std::string("x")));

    auto source_atom_data = ygg::Data<fd::Atom<f::FluentTag>>();
    source_atom_data.predicate = source.get_index();
    source_atom_data.terms.emplace_back(f::ParameterIndex(0));
    const auto source_atom = intern(std::move(source_atom_data));

    const auto source_literal = intern(ygg::Data<fd::Literal<f::FluentTag>>(source_atom.get_index(), true));
    auto body_data = ygg::Data<fd::ConjunctiveCondition>();
    body_data.variables.push_back(variable.get_index());
    body_data.fluent_literals.push_back(source_literal.get_index());
    const auto body = intern(std::move(body_data));

    auto goal_atom_data = ygg::Data<fd::Atom<f::FluentTag>>();
    goal_atom_data.predicate = goal.get_index();
    const auto goal_atom = intern(std::move(goal_atom_data));

    auto rule_data = ygg::Data<fd::Rule<f::PredicateTag>>();
    rule_data.variables.push_back(variable.get_index());
    rule_data.body = body.get_index();
    rule_data.head = goal_atom.get_index();
    const auto rule = intern(std::move(rule_data));

    auto program_data = ygg::Data<fd::Program>();
    program_data.fluent_predicates.push_back(source.get_index());
    program_data.fluent_predicates.push_back(goal.get_index());
    program_data.predicate_rules.push_back(rule.get_index());
    for (const auto* name : { "a", "b", "c", "d" })
    {
        const auto object = intern(ygg::Data<f::Object>(std::string(name)));
        program_data.objects.push_back(object.get_index());

        auto binding_data = ygg::Data<f::RelationBinding<f::Predicate<f::FluentTag>>>();
        binding_data.relation = source.get_index();
        binding_data.objects.push_back(object.get_index());
        const auto binding = intern(std::move(binding_data));
        program_data.fluent_atoms.push_back(intern(ygg::Data<fd::GroundAtom<f::FluentTag>>(binding.get_index())).get_index());
    }
    const auto program_view = intern(std::move(program_data));

    auto program = d::Program<LiftedTag>(program_view, repository, factory);
    const auto& const_rule_workspace = program.get_const_program_workspace().get_rules<f::PredicateTag>().front().value();
    EXPECT_EQ(&const_rule_workspace.get_nullary_condition().get_context(), &program.get_program_repository());

    using OrPolicy = d::OrAnnotationPolicy<LiftedTag>;
    using AndPolicy = d::AndAnnotationPolicy<LiftedTag, d::SumAggregation>;
    using Termination = d::NoTerminationPolicy<LiftedTag>;
    auto workspace = d::ProgramWorkspace<LiftedTag, OrPolicy, AndPolicy, Termination>(program);
    auto context = d::ProgramExecutionContext(workspace);
    d::solve_bottom_up(context);

    const auto& rule_repository = workspace.get_rules<f::PredicateTag>().front()->worker.front().solve.program_overlay_repository;
    EXPECT_EQ(rule_repository.size(rule.get_index()), 1);
    EXPECT_EQ(rule_repository.size(goal.get_index()), 0);
}

TEST(TyrDatalogLiftedBottomUpTest, ProgramWorkspacesOwnIndependentRepositories)
{
    auto factory = std::make_shared<fd::RepositoryFactory>();
    auto repository = factory->create_shared();
    auto program_data = ygg::Data<fd::Program> {};
    fd::canonicalize(program_data);
    const auto program_view = repository->get_or_create(program_data).first;
    const auto program = d::Program<LiftedTag>(program_view, repository, factory);

    auto first = d::ProgramWorkspace<LiftedTag>(program);
    auto second = d::ProgramWorkspace<LiftedTag>(program);

    EXPECT_NE(&first.workspace_repository, &second.workspace_repository);
    EXPECT_EQ(&first.workspace_repository.get_root(), &program.get_program_repository());
    EXPECT_EQ(&second.workspace_repository.get_root(), &program.get_program_repository());

    auto variable = ygg::Data<f::Variable>(std::string("workspace-only"));
    EXPECT_TRUE(first.workspace_repository.get_or_create(variable).second);
    EXPECT_FALSE(second.workspace_repository.find(variable).has_value());
}

TEST(TyrDatalogLiftedBottomUpTest, PredicateAnnotationsUseRelationAndRowIndices)
{
    auto factory = std::make_shared<fd::RepositoryFactory>();
    auto repository = factory->create_shared();
    const auto intern = [&]<typename T>(ygg::Data<T> data)
    {
        f::canonicalize(data);
        return repository->get_or_create(data).first;
    };

    const auto first_predicate = intern(ygg::Data<f::Predicate<f::FluentTag>>(std::string("first"), 1));
    const auto second_predicate = intern(ygg::Data<f::Predicate<f::FluentTag>>(std::string("second"), 1));
    const auto first_object = intern(ygg::Data<f::Object>(std::string("a")));
    const auto second_object = intern(ygg::Data<f::Object>(std::string("b")));
    const auto make_binding = [&](auto predicate, auto object)
    {
        auto data = ygg::Data<f::RelationBinding<f::Predicate<f::FluentTag>>>();
        data.relation = predicate.get_index();
        data.objects.push_back(object.get_index());
        return intern(std::move(data));
    };

    const auto first = make_binding(first_predicate, first_object);
    const auto hole = make_binding(first_predicate, second_object);
    const auto second = make_binding(second_predicate, first_object);
    auto annotations = d::SelectedPredicateAnnotations<LiftedTag>();

    annotations.insert_or_assign(first, d::BaseAnnotation<LiftedTag>(3));
    annotations.insert_or_assign(second, d::BaseAnnotation<LiftedTag>(5));
    EXPECT_EQ(d::get_cost(*annotations.find(first)), 3);
    EXPECT_EQ(d::get_cost(*std::as_const(annotations).find(second)), 5);
    EXPECT_EQ(annotations.find(hole), nullptr);

    annotations.insert_or_assign(first, d::BaseAnnotation<LiftedTag>(2));
    EXPECT_EQ(d::get_cost(*annotations.find(first)), 2);

    annotations.clear();
    EXPECT_EQ(annotations.find(first), nullptr);
    EXPECT_EQ(annotations.find(second), nullptr);
}

INSTANTIATE_TEST_SUITE_P(TyrDatalogLiftedBottomUpFixture,
                         BottomUpFixtureTest,
                         ::testing::ValuesIn(load_cases()),
                         [](const testing::TestParamInfo<BottomUpCase>& info) { return info.param.name; });

}
}
