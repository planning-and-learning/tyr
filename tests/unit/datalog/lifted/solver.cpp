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

#include "tyr/datalog/lifted/solver.hpp"

#include "planning/parser.hpp"
#include "tyr/datalog/lifted/contexts/program.hpp"
#include "tyr/datalog/lifted/policies/annotation.hpp"
#include "tyr/datalog/lifted/programs/program.hpp"
#include "tyr/formalism/datalog/canonicalization.hpp"
#include "tyr/formalism/datalog/datas.hpp"
#include "tyr/formalism/datalog/merge.hpp"
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
#include <oneapi/tbb/parallel_for.h>
#include <oneapi/tbb/task_arena.h>
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
static_assert(fd::Repository::thread_safe);

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

struct LiftedNumericProgram
{
    fd::PredicateBindingView<f::FluentTag> goal;
    fd::FunctionBindingView<f::FluentTag> source;
    fd::FunctionBindingView<f::FluentTag> target;
    d::Program<LiftedTag> program;

    LiftedNumericProgram(fd::RepositoryFactoryPtr factory_,
                         fd::RepositoryPtr repository_,
                         fd::ProgramView<LiftedTag> program_view,
                         fd::PredicateBindingView<f::FluentTag> goal_,
                         fd::FunctionBindingView<f::FluentTag> source_,
                         fd::FunctionBindingView<f::FluentTag> target_) :
        goal(goal_),
        source(source_),
        target(target_),
        program(program_view, std::move(repository_), std::move(factory_))
    {
    }
};

LiftedNumericProgram make_lifted_numeric_program()
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
    const auto bind_function = [&](auto function)
    {
        auto data = ygg::Data<f::RelationBinding<f::Function<f::FluentTag>>>();
        data.relation = function.get_index();
        return intern(std::move(data));
    };
    const auto make_function_term = [&](auto function)
    {
        auto data = ygg::Data<fd::FunctionTerm<f::FluentTag>>();
        data.function = function.get_index();
        return intern(std::move(data));
    };

    const auto source_function = intern(ygg::Data<f::Function<f::FluentTag>>(std::string("source"), 0));
    const auto target_function = intern(ygg::Data<f::Function<f::FluentTag>>(std::string("target"), 0));
    const auto source = bind_function(source_function);
    const auto target = bind_function(target_function);
    const auto source_term = make_function_term(source_function);
    const auto target_term = make_function_term(target_function);
    const auto source_ground_term = intern(ygg::Data<fd::GroundFunctionTerm<f::FluentTag>>(source.get_index()));
    const auto source_value = intern(ygg::Data<fd::GroundFunctionTermValue<f::FluentTag>>(source_ground_term.get_index(), 3));
    const auto empty_body = intern(ygg::Data<fd::ConjunctiveCondition>());

    const auto assign_effect = intern(ygg::Data<fd::NumericEffect<f::FluentTag>>(f::NumericEffectOperatorKind::Assign,
                                                                                 target_term.get_index(),
                                                                                 ygg::Data<fd::FunctionExpression>(source_term.get_index())));
    auto function_rule_data = ygg::Data<fd::Rule<f::FunctionTag>>();
    function_rule_data.body = empty_body.get_index();
    function_rule_data.head =
        ygg::Data<fd::NumericEffectOperator<f::FluentTag>>(f::NumericEffectOperatorKind::Assign,
                                                           ygg::Data<fd::NumericEffectOperator<f::FluentTag>>::Variant(assign_effect.get_index()));
    const auto function_rule = intern(std::move(function_rule_data));

    const auto goal_predicate = intern(ygg::Data<f::Predicate<f::FluentTag>>(std::string("goal"), 0));
    auto goal_binding_data = ygg::Data<f::RelationBinding<f::Predicate<f::FluentTag>>>();
    goal_binding_data.relation = goal_predicate.get_index();
    const auto goal = intern(std::move(goal_binding_data));
    auto goal_atom_data = ygg::Data<fd::Atom<f::FluentTag>>();
    goal_atom_data.predicate = goal_predicate.get_index();
    const auto goal_atom = intern(std::move(goal_atom_data));

    const auto metric_effect = intern(ygg::Data<fd::NumericEffect<f::FluentTag>>(f::NumericEffectOperatorKind::Increase,
                                                                                 target_term.get_index(),
                                                                                 ygg::Data<fd::FunctionExpression>(source_term.get_index())));
    auto predicate_rule_data = ygg::Data<fd::Rule<f::PredicateTag>>();
    predicate_rule_data.body = empty_body.get_index();
    predicate_rule_data.head = goal_atom.get_index();
    predicate_rule_data.metric_effects.emplace_back(f::NumericEffectOperatorKind::Increase,
                                                    ygg::Data<fd::NumericEffectOperator<f::FluentTag>>::Variant(metric_effect.get_index()));
    const auto predicate_rule = intern(std::move(predicate_rule_data));

    auto program_data = ygg::Data<fd::Program>();
    program_data.fluent_predicates.push_back(goal_predicate.get_index());
    program_data.fluent_functions.push_back(source_function.get_index());
    program_data.fluent_functions.push_back(target_function.get_index());
    program_data.fluent_fterm_values.push_back(source_value.get_index());
    program_data.predicate_rules.push_back(predicate_rule.get_index());
    program_data.function_rules.push_back(function_rule.get_index());
    const auto program = intern(std::move(program_data));

    return LiftedNumericProgram(factory, repository, program, goal, source, target);
}

class BottomUpFixtureTest : public ::testing::TestWithParam<BottomUpCase>
{
};

TEST_P(BottomUpFixtureTest, InitialStateAtomsMatchFixture)
{
    const auto& test_case = GetParam();
    auto execution_context = ygg::ExecutionContext::create(1);
    auto task = p::Task<::tyr::LiftedTag>::create(make_test_parser(test_case.domain_file).parse_task(test_case.task_file));

    const auto solve_workspace = [&](auto& workspace)
    {
        auto context = d::ProgramExecutionContext(workspace);
        execution_context->arena().execute([&] { d::compute_model(context); });
    };

    const auto solve_program = [&](const auto& program)
    {
        auto workspace = d::ProgramWorkspace<::tyr::LiftedTag>(program);
        solve_workspace(workspace);
        return collect_atoms_by_predicate(workspace);
    };

    for (size_t config_index = 0; config_index < kConfigNames.size(); ++config_index)
    {
        const auto config = kConfigNames[config_index];
        const auto& expected = test_case.configs[config_index];
        SCOPED_TRACE("config=" + std::string(config));

        if (config == "axiom_evaluator" && !task->has_axioms())
        {
            EXPECT_TRUE(expected.empty());
            continue;
        }

        const auto run_config = [&]() -> AtomsByPredicate
        {
            if (config == "axiom_evaluator")
            {
                const auto program = p::AxiomEvaluatorProgram<::tyr::LiftedTag>(task->get_task());
                return solve_program(program.get_datalog_program());
            }

            if (config == "applicable_action")
            {
                const auto action_program = p::ApplicableActionProgram<::tyr::LiftedTag>(task->get_task());
                auto action_workspace = d::ProgramWorkspace<::tyr::LiftedTag>(action_program.get_datalog_program());
                if (task->has_axioms())
                {
                    const auto axiom_program = p::AxiomEvaluatorProgram<::tyr::LiftedTag>(task->get_task());
                    auto axiom_workspace = d::ProgramWorkspace<::tyr::LiftedTag>(axiom_program.get_datalog_program());
                    solve_workspace(axiom_workspace);

                    auto merge_context = fd::MergeContext { action_workspace.datalog_builder, action_workspace.workspace_repository };
                    for (const auto& set : axiom_workspace.facts.fact_sets.predicate.get_sets())
                        for (const auto binding : set.get_bindings())
                            action_workspace.facts.fact_sets.predicate.insert(fd::merge_d2d(binding, merge_context).first);
                }

                solve_workspace(action_workspace);
                return collect_atoms_by_predicate(action_workspace);
            }

            if (config == "ground_task")
            {
                auto program = p::GroundTaskProgram(task->get_task());
                auto workspace = d::ProgramWorkspace<::tyr::LiftedTag>(program.get_datalog_program());
                auto context = d::ProgramExecutionContext(workspace);
                execution_context->arena().execute([&] { d::compute_model(context); });
                return collect_atoms_by_predicate(workspace);
            }

            throw std::runtime_error("unknown bottom-up fixture configuration: " + std::string(config));
        };

        EXPECT_EQ(run_config(), expected);
    }
}

TEST(TyrDatalogLiftedBottomUpTest, NoAnnotationPolicyKeepsNumericSemanticsWithoutStores)
{
    auto fixture = make_lifted_numeric_program();
    auto workspace = d::ProgramWorkspace<LiftedTag>(fixture.program);
    auto context = d::ProgramExecutionContext(workspace);

    d::compute_model(context);

    const auto expected = ygg::ClosedInterval<ygg::float_t>(3, 3);
    EXPECT_TRUE(workspace.facts.fact_sets.predicate.contains(fixture.goal));
    EXPECT_EQ(workspace.facts.fact_sets.function[fixture.target], expected);
    EXPECT_EQ(workspace.annotations.find(fixture.goal), nullptr);
    EXPECT_EQ(workspace.numeric_annotations.size(), 0);
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

    using AnnotationPolicy = d::MinCostAnnotationPolicy<LiftedTag, d::SumAggregation>;
    using Termination = d::NoTerminationPolicy<LiftedTag>;
    auto workspace = d::ProgramWorkspace<LiftedTag, AnnotationPolicy, Termination>(program);
    auto context = d::ProgramExecutionContext(workspace);
    d::compute_model(context);

    EXPECT_EQ(workspace.workspace_repository.size(rule.get_index()), 1);
    EXPECT_EQ(workspace.workspace_repository.size(goal.get_index()), 1);
}

TEST(TyrDatalogLiftedBottomUpTest, ProgramWorkspacesOwnIndependentRepositories)
{
    auto factory = std::make_shared<fd::RepositoryFactory>();
    auto repository = factory->create_shared();
    const auto intern = [&]<typename T>(ygg::Data<T> data)
    {
        f::canonicalize(data);
        return repository->get_or_create(data).first;
    };

    const auto predicate = intern(ygg::Data<f::Predicate<f::FluentTag>>(std::string("workspace-predicate"), 1));
    const auto object = intern(ygg::Data<f::Object>(std::string("workspace-object")));
    auto program_data = ygg::Data<fd::Program> {};
    program_data.fluent_predicates.push_back(predicate.get_index());
    program_data.objects.push_back(object.get_index());
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

    auto second_variable = ygg::Data<f::Variable>(std::string("second-workspace-only"));
    EXPECT_TRUE(second.workspace_repository.get_or_create(second_variable).second);

    auto binding_data = ygg::Data<f::RelationBinding<f::Predicate<f::FluentTag>>>();
    binding_data.relation = predicate.get_index();
    binding_data.objects.push_back(object.get_index());
    const auto binding = first.workspace_repository.get_or_create(binding_data).first;
    const auto binding_index = binding.get_index();
    first.annotations.insert_or_assign(binding, d::BaseAnnotation<LiftedTag>(3));

    first.reset_evaluation();

    EXPECT_FALSE(first.workspace_repository.find(ygg::Data<f::Variable>(std::string("workspace-only"))).has_value());
    EXPECT_TRUE(second.workspace_repository.find(ygg::Data<f::Variable>(std::string("second-workspace-only"))).has_value());
    EXPECT_EQ(&first.workspace_repository.get_root(), &program.get_program_repository());

    auto reused_binding_data = ygg::Data<f::RelationBinding<f::Predicate<f::FluentTag>>>();
    reused_binding_data.relation = predicate.get_index();
    reused_binding_data.objects.push_back(object.get_index());
    const auto reused_binding = first.workspace_repository.get_or_create(reused_binding_data).first;
    EXPECT_EQ(reused_binding.get_index(), binding_index);
    EXPECT_EQ(first.annotations.find(reused_binding), nullptr);
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
    auto annotations = d::PredicateAnnotations<LiftedTag>();

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

TEST(TyrDatalogLiftedBottomUpTest, DeltaAnnotationsAreConcurrentAndReusable)
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
    const auto function = intern(ygg::Data<f::Function<f::FluentTag>>(std::string("value"), 1));

    auto first_bindings = std::vector<fd::PredicateBindingView<f::FluentTag>> {};
    auto second_bindings = std::vector<fd::PredicateBindingView<f::FluentTag>> {};
    auto function_bindings = std::vector<fd::FunctionBindingView<f::FluentTag>> {};
    for (size_t i = 0; i < 64; ++i)
    {
        const auto object = intern(ygg::Data<f::Object>("o" + std::to_string(i)));
        const auto make_predicate_binding = [&](auto predicate)
        {
            auto data = ygg::Data<f::RelationBinding<f::Predicate<f::FluentTag>>> {};
            data.relation = predicate.get_index();
            data.objects.push_back(object.get_index());
            return intern(std::move(data));
        };
        const auto make_function_binding = [&](auto function_)
        {
            auto data = ygg::Data<f::RelationBinding<f::Function<f::FluentTag>>> {};
            data.relation = function_.get_index();
            data.objects.push_back(object.get_index());
            return intern(std::move(data));
        };
        first_bindings.push_back(make_predicate_binding(first_predicate));
        second_bindings.push_back(make_predicate_binding(second_predicate));
        function_bindings.push_back(make_function_binding(function));
    }

    auto delta_annotations = d::DeltaPredicateAnnotations<LiftedTag>(2);
    auto delta_numeric_annotations = d::DeltaFunctionAnnotations<LiftedTag>(1);
    const auto interval = ygg::ClosedInterval<ygg::float_t>(0, 1);
    auto arena = oneapi::tbb::task_arena(8);
    arena.execute(
        [&]
        {
            oneapi::tbb::parallel_for(size_t(0),
                                      size_t(8),
                                      [&](size_t worker)
                                      {
                                          const auto predicate_cost = d::Cost(8 - worker);
                                          const auto function_cost = d::Cost(worker < 4 ? 3 : 5);
                                          for (size_t i = first_bindings.size(); i-- > 0;)
                                          {
                                              delta_annotations.insert_if_better(first_bindings[i], d::BaseAnnotation<LiftedTag>(predicate_cost));
                                              delta_annotations.insert_if_better(second_bindings[i], d::BaseAnnotation<LiftedTag>(predicate_cost));
                                              delta_numeric_annotations.insert(function_bindings[i], interval, d::BaseAnnotation<LiftedTag>(function_cost));
                                          }
                                      });
        });

    for (size_t i = 0; i < first_bindings.size(); ++i)
    {
        ASSERT_NE(delta_annotations.find(first_bindings[i]), nullptr);
        ASSERT_NE(delta_annotations.find(second_bindings[i]), nullptr);
        EXPECT_EQ(d::get_cost(*delta_annotations.find(first_bindings[i])), 1);
        EXPECT_EQ(d::get_cost(*delta_annotations.find(second_bindings[i])), 1);

        const auto* entries = delta_numeric_annotations.find_entries(function_bindings[i]);
        ASSERT_NE(entries, nullptr);
        ASSERT_EQ(entries->size(), 1);
        EXPECT_EQ(d::get_cost(entries->at(0).annotation), 3);
    }

    const auto second_interval = ygg::ClosedInterval<ygg::float_t>(1, 2);
    EXPECT_TRUE(delta_numeric_annotations.insert(function_bindings.back(), second_interval, d::BaseAnnotation<LiftedTag>(4)));
    EXPECT_FALSE(delta_numeric_annotations.insert(function_bindings.back(), second_interval, d::BaseAnnotation<LiftedTag>(5)));
    EXPECT_TRUE(delta_numeric_annotations.insert(function_bindings.back(), second_interval, d::BaseAnnotation<LiftedTag>(2)));
    ASSERT_EQ(delta_numeric_annotations.find_entries(function_bindings.back())->size(), 2);
    ASSERT_NE(delta_numeric_annotations.find(function_bindings.back(), second_interval), nullptr);
    EXPECT_EQ(d::get_cost(*delta_numeric_annotations.find(function_bindings.back(), second_interval)), 2);

    auto numeric_annotations = d::FunctionAnnotations<LiftedTag>();
    numeric_annotations.insert(function_bindings.back(), interval, d::BaseAnnotation<LiftedTag>(5));
    numeric_annotations.insert(function_bindings.back(), interval, d::BaseAnnotation<LiftedTag>(3));
    numeric_annotations.insert(function_bindings.back(), interval, d::BaseAnnotation<LiftedTag>(4));
    numeric_annotations.insert(function_bindings.back(), interval, d::BaseAnnotation<LiftedTag>(ygg::ClosedInterval<ygg::float_t>(1, 1), 3));
    numeric_annotations.insert(function_bindings.back(), second_interval, d::BaseAnnotation<LiftedTag>(2));
    numeric_annotations.insert(function_bindings.back(), interval, d::BaseAnnotation<LiftedTag>(1));
    const auto* numeric_entries = numeric_annotations.find_entries(function.get_index(), function_bindings.back().get_index().row);
    ASSERT_NE(numeric_entries, nullptr);
    ASSERT_EQ(numeric_entries->size(), 2);
    EXPECT_EQ(numeric_entries->at(0).interval, interval);
    EXPECT_EQ(d::get_cost(numeric_entries->at(0).annotation), 1);
    EXPECT_EQ(numeric_entries->at(1).interval, second_interval);
    EXPECT_EQ(d::get_cost(numeric_entries->at(1).annotation), 2);
    EXPECT_EQ(d::get_cost(*numeric_annotations.find(function_bindings.back(), interval)), 1);
    EXPECT_EQ(d::get_metric(*numeric_annotations.find(function_bindings.back(), interval)), ygg::ClosedInterval<ygg::float_t>());

    delta_annotations.clear();
    delta_numeric_annotations.clear();
    EXPECT_EQ(delta_annotations.find(first_bindings.back()), nullptr);
    EXPECT_EQ(delta_numeric_annotations.find_entries(function_bindings.back()), nullptr);

    delta_annotations.insert_if_better(first_bindings.back(), d::BaseAnnotation<LiftedTag>(7));
    delta_numeric_annotations.insert(function_bindings.back(), interval, d::BaseAnnotation<LiftedTag>(7));
    EXPECT_EQ(d::get_cost(*delta_annotations.find(first_bindings.back())), 7);
    ASSERT_NE(delta_numeric_annotations.find_entries(function_bindings.back()), nullptr);
    EXPECT_EQ(delta_numeric_annotations.find_entries(function_bindings.back())->size(), 1);
}

INSTANTIATE_TEST_SUITE_P(TyrDatalogLiftedBottomUpFixture,
                         BottomUpFixtureTest,
                         ::testing::ValuesIn(load_cases()),
                         [](const testing::TestParamInfo<BottomUpCase>& info) { return info.param.name; });

}
}
