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

#include "tyr/datalog/solver.hpp"

#include "planning/parser.hpp"
#include "tyr/datalog/applicability_lifted.hpp"
#include "tyr/datalog/ground/program.hpp"
#include "tyr/datalog/lifted/consistency_graph.hpp"
#include "tyr/datalog/lifted/contexts/program.hpp"
#include "tyr/datalog/lifted/program.hpp"
#include "tyr/datalog/policies/annotation.hpp"
#include "tyr/formalism/datalog/canonicalization.hpp"
#include "tyr/formalism/datalog/copy.hpp"
#include "tyr/formalism/datalog/datas.hpp"
#include "tyr/formalism/datalog/grounder.hpp"
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
#include <initializer_list>
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
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>
#include <yggdrasil/serialization/json.hpp>
#include <yggdrasil/serialization/json_suite.hpp>

namespace tyr::tests
{
static_assert(tyr::formalism::datalog::Repository::thread_safe);

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

void append_atoms_by_predicate(const tyr::datalog::TaggedFactSets<tyr::formalism::FluentTag>& fact_sets, AtomsByPredicate& result)
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
    tyr::formalism::datalog::PredicateBindingView<tyr::formalism::FluentTag> goal;
    tyr::formalism::datalog::ConjunctiveConditionView<GroundTag> goals;
    tyr::formalism::datalog::FunctionBindingView<tyr::formalism::FluentTag> source;
    tyr::formalism::datalog::FunctionBindingView<tyr::formalism::FluentTag> target;
    tyr::formalism::datalog::RepositoryFactoryPtr factory;
    tyr::formalism::datalog::RepositoryPtr repository;
    tyr::datalog::Program<LiftedTag> program;

    LiftedNumericProgram(tyr::formalism::datalog::RepositoryFactoryPtr factory_,
                         tyr::formalism::datalog::RepositoryPtr repository_,
                         tyr::formalism::datalog::ProgramView<LiftedTag> program_view,
                         tyr::formalism::datalog::PredicateBindingView<tyr::formalism::FluentTag> goal_,
                         tyr::formalism::datalog::ConjunctiveConditionView<GroundTag> goals_,
                         tyr::formalism::datalog::FunctionBindingView<tyr::formalism::FluentTag> source_,
                         tyr::formalism::datalog::FunctionBindingView<tyr::formalism::FluentTag> target_) :
        goal(goal_),
        goals(goals_),
        source(source_),
        target(target_),
        factory(std::move(factory_)),
        repository(std::move(repository_)),
        program(program_view, repository, factory)
    {
    }
};

LiftedNumericProgram
make_lifted_numeric_program(bool include_source_rule = true, std::initializer_list<ygg::float_t> initial_source_values = {}, bool general_goal_rule = false)
{
    auto factory = std::make_shared<tyr::formalism::datalog::RepositoryFactory>();
    auto repository = factory->create_shared();
    const auto intern = [&]<typename T>(ygg::Data<T> data)
    {
        if constexpr (requires { tyr::formalism::datalog::canonicalize(data); })
            tyr::formalism::datalog::canonicalize(data);
        else
            tyr::formalism::canonicalize(data);
        return repository->insert(data).first;
    };
    const auto bind_function = [&](auto function)
    {
        auto data = ygg::Data<tyr::formalism::RelationBinding<tyr::formalism::Function<tyr::formalism::FluentTag>>>();
        data.relation = function.get_index();
        return intern(std::move(data));
    };
    const auto make_function_term = [&](auto function)
    {
        auto data = ygg::Data<tyr::formalism::datalog::FunctionTerm<LiftedTag, tyr::formalism::FluentTag>>();
        data.function = function.get_index();
        return intern(std::move(data));
    };

    const auto source_function = intern(ygg::Data<tyr::formalism::Function<tyr::formalism::FluentTag>>(std::string("source"), 0));
    const auto target_function = intern(ygg::Data<tyr::formalism::Function<tyr::formalism::FluentTag>>(std::string("target"), 0));
    const auto source = bind_function(source_function);
    const auto target = bind_function(target_function);
    const auto source_term = make_function_term(source_function);
    const auto target_term = make_function_term(target_function);
    const auto empty_body = intern(ygg::Data<tyr::formalism::datalog::ConjunctiveCondition<LiftedTag>>());

    const auto source_assign_effect = intern(ygg::Data<tyr::formalism::datalog::NumericEffect<LiftedTag, tyr::formalism::FluentTag>>(
        tyr::formalism::NumericEffectOperatorKind::Assign,
        source_term.get_index(),
        ygg::Data<tyr::formalism::datalog::FunctionExpression<LiftedTag>>(ygg::float_t(3))));
    const auto source_metric_effect = intern(ygg::Data<tyr::formalism::datalog::NumericEffect<LiftedTag, tyr::formalism::FluentTag>>(
        tyr::formalism::NumericEffectOperatorKind::Increase,
        source_term.get_index(),
        ygg::Data<tyr::formalism::datalog::FunctionExpression<LiftedTag>>(ygg::float_t(2))));
    const auto target_assign_effect = intern(ygg::Data<tyr::formalism::datalog::NumericEffect<LiftedTag, tyr::formalism::FluentTag>>(
        tyr::formalism::NumericEffectOperatorKind::Assign,
        target_term.get_index(),
        ygg::Data<tyr::formalism::datalog::FunctionExpression<LiftedTag>>(source_term.get_index())));
    const auto target_metric_effect = intern(ygg::Data<tyr::formalism::datalog::NumericEffect<LiftedTag, tyr::formalism::FluentTag>>(
        tyr::formalism::NumericEffectOperatorKind::Increase,
        target_term.get_index(),
        ygg::Data<tyr::formalism::datalog::FunctionExpression<LiftedTag>>(source_term.get_index())));
    auto target_rule_data = ygg::Data<tyr::formalism::datalog::Rule<LiftedTag, tyr::formalism::FunctionTag>>();
    target_rule_data.body = empty_body.get_index();
    target_rule_data.head = ygg::Data<tyr::formalism::datalog::NumericEffectOperator<LiftedTag, tyr::formalism::FluentTag>>(
        tyr::formalism::NumericEffectOperatorKind::Assign,
        ygg::Data<tyr::formalism::datalog::NumericEffectOperator<LiftedTag, tyr::formalism::FluentTag>>::Variant(target_assign_effect.get_index()));
    target_rule_data.metric_effects.emplace_back(
        tyr::formalism::NumericEffectOperatorKind::Increase,
        ygg::Data<tyr::formalism::datalog::NumericEffectOperator<LiftedTag, tyr::formalism::FluentTag>>::Variant(target_metric_effect.get_index()));
    const auto target_rule = intern(std::move(target_rule_data));

    auto source_rule_data = ygg::Data<tyr::formalism::datalog::Rule<LiftedTag, tyr::formalism::FunctionTag>>();
    source_rule_data.body = empty_body.get_index();
    source_rule_data.head = ygg::Data<tyr::formalism::datalog::NumericEffectOperator<LiftedTag, tyr::formalism::FluentTag>>(
        tyr::formalism::NumericEffectOperatorKind::Assign,
        ygg::Data<tyr::formalism::datalog::NumericEffectOperator<LiftedTag, tyr::formalism::FluentTag>>::Variant(source_assign_effect.get_index()));
    source_rule_data.metric_effects.emplace_back(
        tyr::formalism::NumericEffectOperatorKind::Increase,
        ygg::Data<tyr::formalism::datalog::NumericEffectOperator<LiftedTag, tyr::formalism::FluentTag>>::Variant(source_metric_effect.get_index()));
    const auto source_rule = intern(std::move(source_rule_data));

    const auto goal_predicate = intern(ygg::Data<tyr::formalism::Predicate<tyr::formalism::FluentTag>>(std::string("goal"), 0));
    auto goal_binding_data = ygg::Data<tyr::formalism::RelationBinding<tyr::formalism::Predicate<tyr::formalism::FluentTag>>>();
    goal_binding_data.relation = goal_predicate.get_index();
    const auto goal = intern(std::move(goal_binding_data));
    auto goal_atom_data = ygg::Data<tyr::formalism::datalog::Atom<LiftedTag, tyr::formalism::FluentTag>>();
    goal_atom_data.predicate = goal_predicate.get_index();
    const auto goal_atom = intern(std::move(goal_atom_data));
    const auto ground_goal = intern(ygg::Data<tyr::formalism::datalog::Atom<GroundTag, tyr::formalism::FluentTag>>(goal.get_index()));
    const auto goal_literal = intern(ygg::Data<tyr::formalism::datalog::Literal<GroundTag, tyr::formalism::FluentTag>>(ground_goal.get_index(), true));
    auto goals_data = ygg::Data<tyr::formalism::datalog::ConjunctiveCondition<GroundTag>>();
    goals_data.fluent_literals.push_back(goal_literal.get_index());
    const auto goals = intern(std::move(goals_data));

    const auto seed_predicate = intern(ygg::Data<tyr::formalism::Predicate<tyr::formalism::FluentTag>>(std::string("seed"), 1));
    const auto object = intern(ygg::Data<tyr::formalism::Object>(std::string("a")));
    const auto variable = intern(ygg::Data<tyr::formalism::Variable>(std::string("x")));
    auto seed_atom_data = ygg::Data<tyr::formalism::datalog::Atom<LiftedTag, tyr::formalism::FluentTag>>();
    seed_atom_data.predicate = seed_predicate.get_index();
    seed_atom_data.terms.emplace_back(tyr::formalism::ParameterIndex(0));
    const auto seed_atom = intern(std::move(seed_atom_data));
    const auto seed_literal = intern(ygg::Data<tyr::formalism::datalog::Literal<LiftedTag, tyr::formalism::FluentTag>>(seed_atom.get_index(), true));
    auto seed_body_data = ygg::Data<tyr::formalism::datalog::ConjunctiveCondition<LiftedTag>>();
    seed_body_data.variables.push_back(variable.get_index());
    seed_body_data.fluent_literals.push_back(seed_literal.get_index());
    const auto seed_body = intern(std::move(seed_body_data));
    auto seed_binding_data = ygg::Data<tyr::formalism::RelationBinding<tyr::formalism::Predicate<tyr::formalism::FluentTag>>>();
    seed_binding_data.relation = seed_predicate.get_index();
    seed_binding_data.objects.push_back(object.get_index());
    const auto seed_binding = intern(std::move(seed_binding_data));
    const auto ground_seed = intern(ygg::Data<tyr::formalism::datalog::Atom<GroundTag, tyr::formalism::FluentTag>>(seed_binding.get_index()));

    auto predicate_rule_data = ygg::Data<tyr::formalism::datalog::Rule<LiftedTag, tyr::formalism::PredicateTag>>();
    if (general_goal_rule)
        predicate_rule_data.variables.push_back(variable.get_index());
    predicate_rule_data.body = (general_goal_rule ? seed_body : empty_body).get_index();
    predicate_rule_data.heads.push_back(goal_atom.get_index());
    predicate_rule_data.metric_effects.emplace_back(
        tyr::formalism::NumericEffectOperatorKind::Increase,
        ygg::Data<tyr::formalism::datalog::NumericEffectOperator<LiftedTag, tyr::formalism::FluentTag>>::Variant(target_metric_effect.get_index()));
    const auto predicate_rule = intern(std::move(predicate_rule_data));

    auto program_data = ygg::Data<tyr::formalism::datalog::Program<LiftedTag>>();
    program_data.fluent_predicates.push_back(goal_predicate.get_index());
    if (general_goal_rule)
    {
        program_data.objects.push_back(object.get_index());
        program_data.fluent_predicates.push_back(seed_predicate.get_index());
        program_data.fluent_atoms.push_back(ground_seed.get_index());
    }
    program_data.fluent_functions.push_back(source_function.get_index());
    program_data.fluent_functions.push_back(target_function.get_index());
    if (initial_source_values.size() != 0)
    {
        const auto ground_source_term = intern(ygg::Data<tyr::formalism::datalog::FunctionTerm<GroundTag, tyr::formalism::FluentTag>>(source));
        for (const auto value : initial_source_values)
            program_data.fluent_fterm_values.push_back(
                intern(ygg::Data<tyr::formalism::datalog::FunctionTermValue<GroundTag, tyr::formalism::FluentTag>>(ground_source_term, value)).get_index());
    }
    program_data.predicate_rules.push_back(predicate_rule.get_index());
    if (include_source_rule)
        program_data.function_rules.push_back(source_rule.get_index());
    program_data.function_rules.push_back(target_rule.get_index());
    const auto program = intern(std::move(program_data));

    return LiftedNumericProgram(factory, repository, program, goal, goals, source, target);
}

TEST(TyrDatalogLiftedBottomUpTest, FullEnumerationIsLimitedToNumericListeners)
{
    auto fixture = make_lifted_numeric_program(true, { 3 }, true);
    auto workspace = tyr::datalog::ProgramWorkspace<LiftedTag>(fixture.program);
    ASSERT_EQ(workspace.schedulers.size(), 1);

    const auto rule = fixture.program.get_program().get_rules<tyr::formalism::PredicateTag>().front();
    const auto seed = rule.get_body().get_literals<tyr::formalism::FluentTag>().front().get_atom().get_predicate().get_index();
    auto& scheduler = workspace.schedulers.front().get<tyr::formalism::PredicateTag>();

    scheduler.on_start_iteration();
    scheduler.on_generate(seed);
    scheduler.on_generate(fixture.target.get_index().relation);
    scheduler.on_finish_iteration();
    EXPECT_NE(std::ranges::find(scheduler.get_active_rules(), rule.get_index()), scheduler.get_active_rules().end());
    EXPECT_FALSE(scheduler.requires_full_enumeration(rule.get_index()));

    scheduler.on_start_iteration();
    scheduler.on_generate(fixture.source.get_index().relation);
    scheduler.on_finish_iteration();
    EXPECT_TRUE(scheduler.requires_full_enumeration(rule.get_index()));
}

struct LiftedPredicateProgram
{
    tyr::formalism::datalog::RepositoryFactoryPtr factory;
    tyr::formalism::datalog::RepositoryPtr repository;
    tyr::formalism::datalog::PredicateBindingView<tyr::formalism::FluentTag> goal;
    tyr::formalism::datalog::ConjunctiveConditionView<GroundTag> goals;
    tyr::formalism::datalog::RuleBindingView<tyr::formalism::PredicateTag> first_binding;
    tyr::formalism::datalog::RuleBindingView<tyr::formalism::PredicateTag> second_binding;
    tyr::formalism::datalog::FunctionBindingView<tyr::formalism::FluentTag> metric;
    tyr::formalism::datalog::FunctionBindingView<tyr::formalism::FluentTag> first_lifted_metric;
    tyr::formalism::datalog::FunctionBindingView<tyr::formalism::FluentTag> second_lifted_metric;
    tyr::datalog::Program<LiftedTag> program;

    LiftedPredicateProgram(tyr::formalism::datalog::RepositoryFactoryPtr factory_,
                           tyr::formalism::datalog::RepositoryPtr repository_,
                           tyr::formalism::datalog::ProgramView<LiftedTag> program_view,
                           tyr::formalism::datalog::PredicateBindingView<tyr::formalism::FluentTag> goal_,
                           tyr::formalism::datalog::ConjunctiveConditionView<GroundTag> goals_,
                           tyr::formalism::datalog::RuleBindingView<tyr::formalism::PredicateTag> first_binding_,
                           tyr::formalism::datalog::RuleBindingView<tyr::formalism::PredicateTag> second_binding_,
                           tyr::formalism::datalog::FunctionBindingView<tyr::formalism::FluentTag> metric_,
                           tyr::formalism::datalog::FunctionBindingView<tyr::formalism::FluentTag> first_lifted_metric_,
                           tyr::formalism::datalog::FunctionBindingView<tyr::formalism::FluentTag> second_lifted_metric_) :
        factory(std::move(factory_)),
        repository(std::move(repository_)),
        goal(goal_),
        goals(goals_),
        first_binding(first_binding_),
        second_binding(second_binding_),
        metric(metric_),
        first_lifted_metric(first_lifted_metric_),
        second_lifted_metric(second_lifted_metric_),
        program(program_view, repository, factory)
    {
    }
};

LiftedPredicateProgram make_lifted_predicate_program(bool include_lifted_metric_effect = false)
{
    auto factory = std::make_shared<tyr::formalism::datalog::RepositoryFactory>();
    auto repository = factory->create_shared();
    const auto intern = [&]<typename T>(ygg::Data<T> data)
    {
        if constexpr (requires { tyr::formalism::datalog::canonicalize(data); })
            tyr::formalism::datalog::canonicalize(data);
        else
            tyr::formalism::canonicalize(data);
        return repository->insert(data).first;
    };
    const auto source_predicate = intern(ygg::Data<tyr::formalism::Predicate<tyr::formalism::FluentTag>>(std::string("source"), 1));
    const auto goal_predicate = intern(ygg::Data<tyr::formalism::Predicate<tyr::formalism::FluentTag>>(std::string("goal"), 0));
    const auto metric_function = intern(ygg::Data<tyr::formalism::Function<tyr::formalism::FluentTag>>(std::string("metric"), 0));
    const auto lifted_metric_function = intern(ygg::Data<tyr::formalism::Function<tyr::formalism::FluentTag>>(std::string("lifted-metric"), 1));
    const auto first_object = intern(ygg::Data<tyr::formalism::Object>(std::string("a")));
    const auto second_object = intern(ygg::Data<tyr::formalism::Object>(std::string("b")));
    const auto variable = intern(ygg::Data<tyr::formalism::Variable>(std::string("x")));

    auto source_atom_data = ygg::Data<tyr::formalism::datalog::Atom<LiftedTag, tyr::formalism::FluentTag>>();
    source_atom_data.predicate = source_predicate.get_index();
    source_atom_data.terms.emplace_back(tyr::formalism::ParameterIndex(0));
    const auto source_atom = intern(std::move(source_atom_data));
    const auto source_literal = intern(ygg::Data<tyr::formalism::datalog::Literal<LiftedTag, tyr::formalism::FluentTag>>(source_atom.get_index(), true));
    auto goal_atom_data = ygg::Data<tyr::formalism::datalog::Atom<LiftedTag, tyr::formalism::FluentTag>>();
    goal_atom_data.predicate = goal_predicate.get_index();
    const auto goal_atom = intern(std::move(goal_atom_data));
    auto metric_term_data = ygg::Data<tyr::formalism::datalog::FunctionTerm<LiftedTag, tyr::formalism::FluentTag>>();
    metric_term_data.function = metric_function.get_index();
    const auto metric_term = intern(std::move(metric_term_data));
    auto lifted_metric_term_data = ygg::Data<tyr::formalism::datalog::FunctionTerm<LiftedTag, tyr::formalism::FluentTag>>();
    lifted_metric_term_data.function = lifted_metric_function.get_index();
    lifted_metric_term_data.terms.emplace_back(tyr::formalism::ParameterIndex(0));
    const auto lifted_metric_term = intern(std::move(lifted_metric_term_data));
    const auto metric_effect = intern(ygg::Data<tyr::formalism::datalog::NumericEffect<LiftedTag, tyr::formalism::FluentTag>>(
        tyr::formalism::NumericEffectOperatorKind::Increase,
        metric_term.get_index(),
        ygg::Data<tyr::formalism::datalog::FunctionExpression<LiftedTag>>(ygg::float_t(2))));
    const auto lifted_metric_effect = intern(ygg::Data<tyr::formalism::datalog::NumericEffect<LiftedTag, tyr::formalism::FluentTag>>(
        tyr::formalism::NumericEffectOperatorKind::Increase,
        lifted_metric_term.get_index(),
        ygg::Data<tyr::formalism::datalog::FunctionExpression<LiftedTag>>(ygg::float_t(3))));
    const auto lifted_rhs_metric_effect = intern(ygg::Data<tyr::formalism::datalog::NumericEffect<LiftedTag, tyr::formalism::FluentTag>>(
        tyr::formalism::NumericEffectOperatorKind::Increase,
        metric_term.get_index(),
        ygg::Data<tyr::formalism::datalog::FunctionExpression<LiftedTag>>(lifted_metric_term.get_index())));

    auto body_data = ygg::Data<tyr::formalism::datalog::ConjunctiveCondition<LiftedTag>>();
    body_data.variables.push_back(variable.get_index());
    body_data.fluent_literals.push_back(source_literal.get_index());
    const auto body = intern(std::move(body_data));
    auto rule_data = ygg::Data<tyr::formalism::datalog::Rule<LiftedTag, tyr::formalism::PredicateTag>>();
    rule_data.variables.push_back(variable.get_index());
    rule_data.body = body.get_index();
    rule_data.heads.push_back(goal_atom.get_index());
    rule_data.metric_effects.emplace_back(
        tyr::formalism::NumericEffectOperatorKind::Increase,
        ygg::Data<tyr::formalism::datalog::NumericEffectOperator<LiftedTag, tyr::formalism::FluentTag>>::Variant(metric_effect.get_index()));
    if (include_lifted_metric_effect)
    {
        rule_data.metric_effects.emplace_back(
            tyr::formalism::NumericEffectOperatorKind::Increase,
            ygg::Data<tyr::formalism::datalog::NumericEffectOperator<LiftedTag, tyr::formalism::FluentTag>>::Variant(metric_effect.get_index()));
        rule_data.metric_effects.emplace_back(
            tyr::formalism::NumericEffectOperatorKind::Increase,
            ygg::Data<tyr::formalism::datalog::NumericEffectOperator<LiftedTag, tyr::formalism::FluentTag>>::Variant(lifted_metric_effect.get_index()));
        rule_data.metric_effects.emplace_back(
            tyr::formalism::NumericEffectOperatorKind::Increase,
            ygg::Data<tyr::formalism::datalog::NumericEffectOperator<LiftedTag, tyr::formalism::FluentTag>>::Variant(lifted_rhs_metric_effect.get_index()));
    }
    const auto rule = intern(std::move(rule_data));

    const auto bind_rule = [&](auto object)
    {
        auto data = ygg::Data<tyr::formalism::RelationBinding<tyr::formalism::datalog::Rule<LiftedTag, tyr::formalism::PredicateTag>>>();
        data.relation = rule.get_index();
        data.objects.push_back(object.get_index());
        return intern(std::move(data));
    };
    const auto first_binding = bind_rule(first_object);
    const auto second_binding = bind_rule(second_object);

    const auto bind_function = [&](auto function, auto... objects)
    {
        auto data = ygg::Data<tyr::formalism::RelationBinding<tyr::formalism::Function<tyr::formalism::FluentTag>>>();
        data.relation = function.get_index();
        (data.objects.push_back(objects.get_index()), ...);
        return intern(std::move(data));
    };
    const auto metric = bind_function(metric_function);
    const auto first_lifted_metric = bind_function(lifted_metric_function, first_object);
    const auto second_lifted_metric = bind_function(lifted_metric_function, second_object);

    const auto bind_source = [&](auto object)
    {
        auto data = ygg::Data<tyr::formalism::RelationBinding<tyr::formalism::Predicate<tyr::formalism::FluentTag>>>();
        data.relation = source_predicate.get_index();
        data.objects.push_back(object.get_index());
        return intern(std::move(data));
    };
    const auto ground_first_source =
        intern(ygg::Data<tyr::formalism::datalog::Atom<GroundTag, tyr::formalism::FluentTag>>(bind_source(first_object).get_index()));
    const auto ground_second_source =
        intern(ygg::Data<tyr::formalism::datalog::Atom<GroundTag, tyr::formalism::FluentTag>>(bind_source(second_object).get_index()));
    auto goal_binding_data = ygg::Data<tyr::formalism::RelationBinding<tyr::formalism::Predicate<tyr::formalism::FluentTag>>>();
    goal_binding_data.relation = goal_predicate.get_index();
    const auto goal = intern(std::move(goal_binding_data));
    const auto ground_goal = intern(ygg::Data<tyr::formalism::datalog::Atom<GroundTag, tyr::formalism::FluentTag>>(goal.get_index()));
    const auto goal_literal = intern(ygg::Data<tyr::formalism::datalog::Literal<GroundTag, tyr::formalism::FluentTag>>(ground_goal.get_index(), true));
    auto goals_data = ygg::Data<tyr::formalism::datalog::ConjunctiveCondition<GroundTag>>();
    goals_data.fluent_literals.push_back(goal_literal.get_index());
    const auto goals = intern(std::move(goals_data));

    auto program_data = ygg::Data<tyr::formalism::datalog::Program<LiftedTag>>();
    program_data.objects = { first_object.get_index(), second_object.get_index() };
    program_data.fluent_predicates = { source_predicate.get_index(), goal_predicate.get_index() };
    program_data.fluent_functions = { metric_function.get_index() };
    if (include_lifted_metric_effect)
    {
        program_data.fluent_functions.push_back(lifted_metric_function.get_index());
        const auto first_term = intern(ygg::Data<tyr::formalism::datalog::FunctionTerm<GroundTag, tyr::formalism::FluentTag>>(first_lifted_metric));
        const auto second_term = intern(ygg::Data<tyr::formalism::datalog::FunctionTerm<GroundTag, tyr::formalism::FluentTag>>(second_lifted_metric));
        program_data.fluent_fterm_values = {
            intern(ygg::Data<tyr::formalism::datalog::FunctionTermValue<GroundTag, tyr::formalism::FluentTag>>(first_term, ygg::float_t(5))).get_index(),
            intern(ygg::Data<tyr::formalism::datalog::FunctionTermValue<GroundTag, tyr::formalism::FluentTag>>(second_term, ygg::float_t(7))).get_index()
        };
    }
    program_data.fluent_atoms = { ground_first_source.get_index(), ground_second_source.get_index() };
    program_data.predicate_rules = { rule.get_index() };
    const auto program = intern(std::move(program_data));

    return LiftedPredicateProgram(factory, repository, program, goal, goals, first_binding, second_binding, metric, first_lifted_metric, second_lifted_metric);
}

TEST(TyrDatalogLiftedBottomUpTest, ConstRuleWorkspacePartitionsMetricEffectsByParameters)
{
    auto fixture = make_lifted_predicate_program(true);
    const auto& workspace = fixture.program.get_const_program_workspace().get_rules<tyr::formalism::PredicateTag>().front().value();

    EXPECT_EQ(workspace.get_rule().get_metric_effects().size(), 4);
    ASSERT_EQ(workspace.get_lifted_effects().size(), 2);
    ASSERT_EQ(workspace.get_nullary_effects().size(), 2);
    EXPECT_EQ(&workspace.get_lifted_effects().front().get_context(), &fixture.program.get_program_repository());
    EXPECT_EQ(&workspace.get_nullary_effects().front().get_context(), &fixture.program.get_program_repository());
}

TEST(TyrDatalogLiftedBottomUpTest, MixedMetricEffectsRespectExactTargetsAndRetainEvidence)
{
    auto fixture = make_lifted_predicate_program(true);
    auto metric_targets = tyr::datalog::RuleCostPolicy::MetricTargets {};
    metric_targets.insert(fixture.metric.get_index());
    metric_targets.insert(fixture.first_lifted_metric.get_index());
    using AnnotationPolicy = tyr::datalog::MinCostAnnotationPolicy<tyr::datalog::SumAggregation>;
    using Workspace = tyr::datalog::ProgramWorkspace<LiftedTag, AnnotationPolicy, tyr::datalog::NoTerminationPolicy, tyr::datalog::RuleCostPolicy>;
    auto workspace = Workspace(fixture.program, AnnotationPolicy(), tyr::datalog::NoTerminationPolicy(), tyr::datalog::RuleCostPolicy(&metric_targets));
    auto context = tyr::datalog::ProgramExecutionContext(workspace);

    tyr::datalog::compute_model(context);

    const auto* annotation = workspace.annotations.find(fixture.goal.get_index());
    ASSERT_NE(annotation, nullptr);
    const auto* witness = std::get_if<tyr::datalog::WitnessAnnotation<tyr::formalism::PredicateTag>>(annotation);
    ASSERT_NE(witness, nullptr);
    EXPECT_EQ(witness->get_rule_key(), fixture.second_binding.get_index());
    EXPECT_EQ(witness->get_cost(), 11);  // b: 2 + 2 + 0 (filtered target) + 7; a: 2 + 2 + 3 + 5 = 12.
    ASSERT_EQ(witness->get_numeric_supports().size(), 1);
    EXPECT_EQ(witness->get_numeric_supports().front().get_key(), fixture.second_lifted_metric.get_index());
    EXPECT_EQ(witness->get_numeric_supports().front().get_interval(), ygg::ClosedInterval<ygg::float_t>(7, 7));
    EXPECT_EQ(witness->get_numeric_supports().front().get_cost(), tyr::datalog::Cost(0));
}

TEST(TyrDatalogLiftedBottomUpTest, PreparedContextMatchesOrdinaryContextAfterWorkspaceReuse)
{
    auto fixture = make_lifted_predicate_program(true);
    using AnnotationPolicy = tyr::datalog::MinCostAnnotationPolicy<tyr::datalog::SumAggregation>;
    using Context = tyr::datalog::ProgramExecutionContext<LiftedTag, AnnotationPolicy>;
    auto workspace = tyr::datalog::ProgramWorkspace<LiftedTag, AnnotationPolicy>(fixture.program);
    auto initial = Context(workspace);
    tyr::datalog::compute_model(initial);
    const auto expected_atoms = collect_atoms_by_predicate(workspace);
    const auto* expected_annotation = workspace.annotations.find(fixture.goal.get_index());
    ASSERT_NE(expected_annotation, nullptr);
    const auto expected_cost = tyr::datalog::get_cost(*expected_annotation);

    const auto insert_inputs = [&]
    {
        for (const auto atom : fixture.program.get_program().get_atoms<tyr::formalism::FluentTag>())
            workspace.facts.fact_sets.predicate.insert(atom);
        workspace.facts.fact_sets.function.insert(fixture.program.get_program().get_fterm_values<tyr::formalism::FluentTag>());
    };
    for (size_t iteration = 0; iteration < 2; ++iteration)
    {
        workspace.cost_buckets.insert(99, fixture.goal.get_index());
        workspace.reset_evaluation();
        EXPECT_TRUE(workspace.cost_buckets.is_empty());
        insert_inputs();
        auto prepared = Context::from_reset_workspace(workspace, workspace.schedulers);
        tyr::datalog::compute_model(prepared);
        EXPECT_EQ(collect_atoms_by_predicate(workspace), expected_atoms);
        const auto* annotation = workspace.annotations.find(fixture.goal.get_index());
        ASSERT_NE(annotation, nullptr);
        EXPECT_EQ(tyr::datalog::get_cost(*annotation), expected_cost);

        // LM-cut also rebuilds a context with new facts without resetting its repository.
        workspace.facts.reset();
        insert_inputs();
        auto ordinary = Context(workspace);
        tyr::datalog::compute_model(ordinary);
        EXPECT_EQ(collect_atoms_by_predicate(workspace), expected_atoms);
        annotation = workspace.annotations.find(fixture.goal.get_index());
        ASSERT_NE(annotation, nullptr);
        EXPECT_EQ(tyr::datalog::get_cost(*annotation), expected_cost);
    }
}

TEST(TyrDatalogLiftedBottomUpTest, PreparedBindingSurvivesNestedGrounding)
{
    auto fixture = make_lifted_predicate_program();
    auto builder = tyr::formalism::datalog::Builder {};
    const auto program = fixture.program.get_program();
    const auto atom = program.get_rules<tyr::formalism::PredicateTag>()[0].get_body().get_literals<tyr::formalism::FluentTag>()[0].get_atom();
    const auto objects = program.get_objects();
    ASSERT_EQ(objects.size(), 2);
    auto indices = ygg::IndexList<tyr::formalism::Object> { objects[0].get_index() };
    auto context = tyr::formalism::datalog::GrounderContext { builder, *fixture.repository, indices };
    auto prepared = tyr::formalism::datalog::checkout<tyr::formalism::RelationBinding<tyr::formalism::Predicate<tyr::formalism::FluentTag>>>(builder);
    prepared->relation = atom.get_predicate().get_index();
    tyr::formalism::datalog::ground(atom.get_terms(), context, prepared->objects);

    indices[0] = objects[1].get_index();
    const auto nested = tyr::formalism::datalog::ground_binding(atom, context).first;
    EXPECT_EQ(nested.get_objects()[0], objects[1]);
    ASSERT_EQ(prepared->objects.size(), 1);
    EXPECT_EQ(prepared->objects[0], objects[0].get_index());
    const auto retained = tyr::formalism::datalog::insert(*fixture.repository, *prepared).first;
    EXPECT_EQ(retained.get_objects()[0], objects[0]);
}

class BottomUpFixtureTest : public ::testing::TestWithParam<BottomUpCase>
{
};

TEST_P(BottomUpFixtureTest, InitialStateAtomsMatchFixture)
{
    const auto& test_case = GetParam();
    auto execution_context = ygg::ExecutionContext::create(1);
    auto task = tyr::planning::Task<LiftedTag>::create(make_test_parser(test_case.domain_file).parse_task(test_case.task_file));

    const auto solve_workspace = [&](auto& workspace)
    {
        auto context = tyr::datalog::ProgramExecutionContext(workspace);
        tyr::datalog::execute_model(context, *execution_context);
    };

    const auto solve_program = [&](const auto& program)
    {
        auto workspace = tyr::datalog::ProgramWorkspace<LiftedTag>(program);
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
                const auto program = tyr::planning::AxiomEvaluatorProgram<LiftedTag>(task->get_task());
                return solve_program(program.get_datalog_program());
            }

            if (config == "applicable_action")
            {
                const auto action_program = tyr::planning::ApplicableActionProgram<LiftedTag>(task->get_task());
                auto action_workspace = tyr::datalog::ProgramWorkspace<LiftedTag>(action_program.get_datalog_program());
                if (task->has_axioms())
                {
                    const auto axiom_program = tyr::planning::AxiomEvaluatorProgram<LiftedTag>(task->get_task());
                    auto axiom_workspace = tyr::datalog::ProgramWorkspace<LiftedTag>(axiom_program.get_datalog_program());
                    solve_workspace(axiom_workspace);

                    auto merge_context = tyr::formalism::datalog::CopyContext { action_workspace.datalog_builder, action_workspace.workspace_repository };
                    for (const auto& set : axiom_workspace.facts.fact_sets.predicate.get_sets())
                        for (const auto binding : set.get_bindings())
                            action_workspace.facts.fact_sets.predicate.insert(tyr::formalism::datalog::copy(binding, merge_context).first);
                }

                solve_workspace(action_workspace);
                return collect_atoms_by_predicate(action_workspace);
            }

            if (config == "ground_task")
            {
                auto program = tyr::planning::GroundTaskProgram(task->get_task());
                auto workspace = tyr::datalog::ProgramWorkspace<LiftedTag>(program.get_datalog_program());
                auto context = tyr::datalog::ProgramExecutionContext(workspace);
                tyr::datalog::execute_model(context, *execution_context);
                return collect_atoms_by_predicate(workspace);
            }

            throw std::runtime_error("unknown bottom-up fixture configuration: " + std::string(config));
        };

        EXPECT_EQ(run_config(), expected);
    }
}

TEST(TyrDatalogLiftedBottomUpTest, NoAnnotationPolicyComputesZeroCostNumericReachability)
{
    auto fixture = make_lifted_numeric_program();
    auto workspace = tyr::datalog::ProgramWorkspace<LiftedTag>(fixture.program);
    auto context = tyr::datalog::ProgramExecutionContext(workspace);

    tyr::datalog::compute_model(context);

    const auto expected = ygg::ClosedInterval<ygg::float_t>(3, 3);
    EXPECT_TRUE(workspace.facts.fact_sets.predicate.contains(fixture.goal));
    EXPECT_EQ(workspace.facts.fact_sets.function[fixture.target], expected);
    EXPECT_EQ(workspace.annotations.find(fixture.goal.get_index()), nullptr);
    EXPECT_EQ(workspace.numeric_annotations.size(), 0);
}

TEST(TyrDatalogLiftedBottomUpTest, NoAnnotationPolicyIgnoresUnavailableMetricInputs)
{
    auto fixture = make_lifted_numeric_program(false);
    auto workspace = tyr::datalog::ProgramWorkspace<LiftedTag>(fixture.program);
    auto context = tyr::datalog::ProgramExecutionContext(workspace);

    tyr::datalog::compute_model(context);

    EXPECT_TRUE(workspace.facts.fact_sets.predicate.contains(fixture.goal));
    EXPECT_TRUE(empty(workspace.facts.fact_sets.function[fixture.target]));
}

TEST(TyrDatalogLiftedBottomUpTest, SingleCoreTerminationExposesOnlyOptimalGoalFrontierAchievers)
{
    auto fixture = make_lifted_predicate_program();
    auto termination_policy = tyr::datalog::TerminationPolicy<tyr::datalog::MaxAggregation>();
    termination_policy.set_goals(fixture.goals);
    auto cost_policy = tyr::datalog::RuleCostOverridePolicy<LiftedTag>();
    cost_policy.set_cost(fixture.first_binding.get_index(), tyr::datalog::Cost(1));
    using Workspace = tyr::datalog::ProgramWorkspace<LiftedTag,
                                                     tyr::datalog::MinCostAnnotationWithAchieversPolicy<tyr::datalog::MaxAggregation>,
                                                     tyr::datalog::TerminationPolicy<tyr::datalog::MaxAggregation>,
                                                     tyr::datalog::RuleCostOverridePolicy<LiftedTag>>;
    auto workspace =
        Workspace(fixture.program, tyr::datalog::MinCostAnnotationWithAchieversPolicy<tyr::datalog::MaxAggregation>(), termination_policy, cost_policy);
    auto context = tyr::datalog::ProgramExecutionContext(workspace);
    auto execution_context = ygg::ExecutionContext::create(1);

    tyr::datalog::execute_model(context, *execution_context);

    ASSERT_TRUE(workspace.facts.fact_sets.predicate.contains(fixture.goal));
    const auto* achievers = workspace.annotation_policy.find_achievers(fixture.goal.get_index());
    ASSERT_NE(achievers, nullptr);
    ASSERT_EQ(achievers->size(), 1);
    EXPECT_EQ(achievers->front().get_rule_key(), fixture.first_binding.get_index());
}

TEST(TyrDatalogLiftedBottomUpTest, SingleCoreRetainsFirstEqualCostWitness)
{
    auto fixture = make_lifted_predicate_program();
    using Workspace =
        tyr::datalog::ProgramWorkspace<LiftedTag, tyr::datalog::MinCostAnnotationPolicy<tyr::datalog::MaxAggregation>, tyr::datalog::NoTerminationPolicy>;
    auto workspace = Workspace(fixture.program);
    auto context = tyr::datalog::ProgramExecutionContext(workspace);
    auto execution_context = ygg::ExecutionContext::create(1);

    tyr::datalog::execute_model(context, *execution_context);

    const auto* annotation = workspace.annotations.find(fixture.goal.get_index());
    ASSERT_NE(annotation, nullptr);
    const auto* witness = std::get_if<tyr::datalog::WitnessAnnotation<tyr::formalism::PredicateTag>>(annotation);
    ASSERT_NE(witness, nullptr);
    EXPECT_EQ(witness->get_rule_key(), fixture.first_binding.get_index());
}

TEST(TyrDatalogLiftedBottomUpTest, MissingLiteralBindingUsesClosedWorldPolarityWithoutInterning)
{
    auto fixture = make_lifted_predicate_program();
    auto object_data = ygg::Data<tyr::formalism::Object>(std::string("missing"));
    tyr::formalism::canonicalize(object_data);
    const auto object = fixture.repository->insert(object_data).first;

    const auto rule = fixture.program.get_program().get_rules<tyr::formalism::PredicateTag>().front();
    const auto positive = rule.get_body().get_literals<tyr::formalism::FluentTag>().front();
    auto negative_data = ygg::Data<tyr::formalism::datalog::Literal<LiftedTag, tyr::formalism::FluentTag>>(positive.get_atom().get_index(), false);
    tyr::formalism::datalog::canonicalize(negative_data);
    const auto negative = fixture.repository->insert(negative_data).first;

    auto workspace = tyr::datalog::ProgramWorkspace<LiftedTag>(fixture.program);
    workspace.binding.push_back(object.get_index());
    auto grounder = tyr::formalism::datalog::GrounderContext { workspace.datalog_builder, workspace.workspace_repository, workspace.binding };
    const auto fact_sets = tyr::datalog::FactSets { workspace.const_workspace.facts.fact_sets, workspace.facts.fact_sets };
    const auto applicability_context = tyr::datalog::ApplicabilityContext { fact_sets, grounder };
    const auto relation = positive.get_atom().get_predicate().get_index();
    const auto num_bindings = workspace.workspace_repository.size(relation);

    EXPECT_FALSE(tyr::datalog::is_applicable(positive, applicability_context));
    EXPECT_TRUE(tyr::datalog::is_applicable(negative, applicability_context));
    EXPECT_EQ(workspace.workspace_repository.size(relation), num_bindings);
}

TEST(TyrDatalogLiftedBottomUpTest, SumAnnotationPricesEachNumericSupportOccurrence)
{
    auto fixture = make_lifted_numeric_program();
    using AnnotationPolicy = tyr::datalog::MinCostAnnotationPolicy<tyr::datalog::SumAggregation>;
    using Workspace = tyr::datalog::ProgramWorkspace<LiftedTag, AnnotationPolicy, tyr::datalog::NoTerminationPolicy>;
    auto workspace = Workspace(fixture.program);
    auto context = tyr::datalog::ProgramExecutionContext(workspace);

    tyr::datalog::compute_model(context);

    const auto interval = ygg::ClosedInterval<ygg::float_t>(3, 3);
    const auto* annotation = workspace.numeric_annotations.find(fixture.target.get_index(), interval);
    ASSERT_NE(annotation, nullptr);
    const auto* witness = std::get_if<tyr::datalog::WitnessAnnotation<tyr::formalism::FunctionTag>>(annotation);
    ASSERT_NE(witness, nullptr);
    EXPECT_EQ(witness->get_cost(), 7);
    ASSERT_EQ(witness->get_numeric_supports().size(), 1);
    EXPECT_EQ(witness->get_numeric_supports().front().get_key(), fixture.source.get_index());
    EXPECT_EQ(witness->get_numeric_supports().front().get_interval(), interval);
    EXPECT_EQ(witness->get_numeric_supports().front().get_cost(), 2);
}

TEST(TyrDatalogLiftedBottomUpTest, StoredNumericCertificateCanStillBecomeAvailable)
{
    auto fixture = make_lifted_numeric_program();
    using AnnotationPolicy = tyr::datalog::MinCostAnnotationPolicy<tyr::datalog::SumAggregation>;
    using Workspace = tyr::datalog::ProgramWorkspace<LiftedTag, AnnotationPolicy, tyr::datalog::NoTerminationPolicy>;
    auto workspace = Workspace(fixture.program);
    auto context = tyr::datalog::ProgramExecutionContext(workspace);
    const auto source_interval = ygg::ClosedInterval<ygg::float_t>(3, 3);
    workspace.numeric_annotations.insert(fixture.source.get_index(), source_interval, tyr::datalog::BaseAnnotation(2));

    tyr::datalog::compute_model(context);

    EXPECT_EQ(workspace.facts.fact_sets.function[fixture.source], source_interval);
    EXPECT_EQ(workspace.facts.fact_sets.function[fixture.target], source_interval);
}

TEST(TyrDatalogLiftedBottomUpTest, RepricesTentativePredicateAfterAvailableNumericCertificateImproves)
{
    auto fixture = make_lifted_numeric_program(true, { 3 }, true);
    ASSERT_EQ(fixture.program.get_program().get_rules<tyr::formalism::PredicateTag>().front().get_arity(), 1);
    using AnnotationPolicy = tyr::datalog::MinCostAnnotationPolicy<tyr::datalog::SumAggregation>;
    using TerminationPolicy = tyr::datalog::TerminationPolicy<tyr::datalog::SumAggregation>;
    using Workspace = tyr::datalog::ProgramWorkspace<LiftedTag, AnnotationPolicy, TerminationPolicy>;
    auto termination_policy = TerminationPolicy();
    termination_policy.set_goals(fixture.goals);
    auto workspace = Workspace(fixture.program, AnnotationPolicy(), termination_policy);
    auto context = tyr::datalog::ProgramExecutionContext(workspace);
    const auto source_interval = ygg::ClosedInterval<ygg::float_t>(3, 3);
    workspace.numeric_annotations.clear();
    workspace.numeric_annotations.insert(fixture.source.get_index(), source_interval, tyr::datalog::BaseAnnotation(5));

    tyr::datalog::compute_model(context);

    const auto* source = workspace.numeric_annotations.find(fixture.source.get_index(), source_interval);
    const auto* goal = workspace.annotations.find(fixture.goal.get_index());
    ASSERT_NE(source, nullptr);
    ASSERT_NE(goal, nullptr);
    EXPECT_TRUE(workspace.facts.fact_sets.predicate.contains(fixture.goal));
    EXPECT_EQ(tyr::datalog::get_cost(*source), 2);
    EXPECT_EQ(tyr::datalog::get_cost(*goal), 5);
}

TEST(TyrDatalogLiftedBottomUpTest, GroundAndLiftedShareResolvedNumericCandidateSemantics)
{
    auto fixture = make_lifted_numeric_program(false, { 1, 10 });
    const auto source_interval = ygg::ClosedInterval<ygg::float_t>(1, 10);
    const auto low_support_interval = ygg::ClosedInterval<ygg::float_t>(1, 2);
    const auto interior_support_interval = ygg::ClosedInterval<ygg::float_t>(4, 6);
    const auto high_support_interval = ygg::ClosedInterval<ygg::float_t>(8, 10);
    const auto expect_source_annotation_order = [&](const auto* entries)
    {
        ASSERT_NE(entries, nullptr);
        ASSERT_EQ(entries->size(), 3);
        EXPECT_EQ(entries->at(0).interval, interior_support_interval);
        EXPECT_EQ(tyr::datalog::get_cost(entries->at(0).annotation), 1);
        EXPECT_EQ(entries->at(1).interval, low_support_interval);
        EXPECT_EQ(tyr::datalog::get_cost(entries->at(1).annotation), 2);
        EXPECT_EQ(entries->at(2).interval, high_support_interval);
        EXPECT_EQ(tyr::datalog::get_cost(entries->at(2).annotation), 2);
    };

    using LiftedAnnotationPolicy = tyr::datalog::MinCostAnnotationPolicy<tyr::datalog::SumAggregation>;
    using LiftedWorkspace = tyr::datalog::ProgramWorkspace<LiftedTag, LiftedAnnotationPolicy, tyr::datalog::NoTerminationPolicy>;
    auto lifted_workspace = LiftedWorkspace(fixture.program);
    auto lifted_context = tyr::datalog::ProgramExecutionContext(lifted_workspace);
    lifted_workspace.numeric_annotations.clear();
    lifted_workspace.numeric_annotations.insert(fixture.source.get_index(),
                                                high_support_interval,
                                                tyr::datalog::BaseAnnotation(tyr::datalog::Cost(2)));
    lifted_workspace.numeric_annotations.insert(fixture.source.get_index(),
                                                interior_support_interval,
                                                tyr::datalog::BaseAnnotation(tyr::datalog::Cost(1)));
    lifted_workspace.numeric_annotations.insert(fixture.source.get_index(), low_support_interval, tyr::datalog::BaseAnnotation(tyr::datalog::Cost(2)));
    expect_source_annotation_order(lifted_workspace.numeric_annotations.find_entries(fixture.source.get_index().relation, fixture.source.get_index().row));

    tyr::datalog::compute_model(lifted_context);

    EXPECT_EQ((lifted_workspace.workspace_repository.template size<tyr::formalism::datalog::Rule<GroundTag, tyr::formalism::PredicateTag>>()), 0);
    EXPECT_EQ((lifted_workspace.workspace_repository.template size<tyr::formalism::datalog::Rule<GroundTag, tyr::formalism::FunctionTag>>()), 0);

    auto builder = tyr::formalism::datalog::Builder {};
    auto binding = ygg::IndexList<tyr::formalism::Object> {};
    auto grounder = tyr::formalism::datalog::GrounderContext { builder, *fixture.repository, binding };
    auto ground_program_data = ygg::Data<tyr::formalism::datalog::Program<GroundTag>> {};
    const auto lifted_program = fixture.program.get_program();
    for (const auto predicate : lifted_program.get_predicates<tyr::formalism::FluentTag>())
        ground_program_data.fluent_predicates.push_back(predicate.get_index());
    for (const auto function : lifted_program.get_functions<tyr::formalism::FluentTag>())
        ground_program_data.fluent_functions.push_back(function.get_index());
    for (const auto value : lifted_program.get_fterm_values<tyr::formalism::FluentTag>())
        ground_program_data.fluent_fterm_values.push_back(value.get_index());
    for (const auto rule : lifted_program.get_rules<tyr::formalism::PredicateTag>())
        ground_program_data.predicate_rules.push_back(tyr::formalism::datalog::ground(rule, grounder).first.get_index());
    for (const auto rule : lifted_program.get_rules<tyr::formalism::FunctionTag>())
        ground_program_data.function_rules.push_back(tyr::formalism::datalog::ground(rule, grounder).first.get_index());
    tyr::formalism::datalog::canonicalize(ground_program_data);
    const auto ground_program_view = fixture.repository->insert(ground_program_data).first;
    auto ground_program = tyr::datalog::Program<GroundTag>(ground_program_view, fixture.repository, fixture.factory);

    using GroundAnnotationPolicy = tyr::datalog::MinCostAnnotationPolicy<tyr::datalog::SumAggregation>;
    using GroundWorkspace = tyr::datalog::ProgramWorkspace<GroundTag, GroundAnnotationPolicy, tyr::datalog::NoTerminationPolicy>;
    auto ground_workspace = GroundWorkspace(ground_program);
    auto ground_context = tyr::datalog::ProgramExecutionContext(ground_workspace);

    auto ground_source_data = ygg::Data<tyr::formalism::datalog::FunctionTerm<GroundTag, tyr::formalism::FluentTag>>(fixture.source);
    auto ground_target_data = ygg::Data<tyr::formalism::datalog::FunctionTerm<GroundTag, tyr::formalism::FluentTag>>(fixture.target);
    const auto ground_source = fixture.repository->insert(ground_source_data).first;
    const auto ground_target = fixture.repository->insert(ground_target_data).first;
    ground_workspace.numeric_annotations.clear();
    ground_workspace.numeric_annotations.insert(ground_source.get_row().get_index(),
                                                high_support_interval,
                                                tyr::datalog::BaseAnnotation(tyr::datalog::Cost(2)));
    ground_workspace.numeric_annotations.insert(ground_source.get_row().get_index(),
                                                interior_support_interval,
                                                tyr::datalog::BaseAnnotation(tyr::datalog::Cost(1)));
    ground_workspace.numeric_annotations.insert(ground_source.get_row().get_index(),
                                                low_support_interval,
                                                tyr::datalog::BaseAnnotation(tyr::datalog::Cost(2)));
    expect_source_annotation_order(ground_workspace.numeric_annotations.find_entries(ground_source.get_row().get_index()));

    tyr::datalog::compute_model(ground_context);

    EXPECT_TRUE(lifted_workspace.facts.fact_sets.predicate.contains(fixture.goal));
    EXPECT_TRUE(ground_workspace.facts.fact_sets.predicate.contains(fixture.goal));
    EXPECT_EQ(lifted_workspace.facts.fact_sets.function[fixture.source], source_interval);
    EXPECT_EQ(ground_workspace.facts.fact_sets.function[ground_source], source_interval);
    EXPECT_EQ(lifted_workspace.facts.fact_sets.function[fixture.target], source_interval);
    EXPECT_EQ(ground_workspace.facts.fact_sets.function[ground_target], source_interval);

    const auto* lifted_annotation = lifted_workspace.numeric_annotations.find(fixture.target.get_index(), source_interval);
    const auto* ground_annotation = ground_workspace.numeric_annotations.find(ground_target.get_row().get_index(), source_interval);
    ASSERT_NE(lifted_annotation, nullptr);
    ASSERT_NE(ground_annotation, nullptr);
    const auto* lifted_witness = std::get_if<tyr::datalog::WitnessAnnotation<tyr::formalism::FunctionTag>>(lifted_annotation);
    const auto* ground_witness = std::get_if<tyr::datalog::WitnessAnnotation<tyr::formalism::FunctionTag>>(ground_annotation);
    ASSERT_NE(lifted_witness, nullptr);
    ASSERT_NE(ground_witness, nullptr);
    EXPECT_EQ(lifted_witness->get_cost(), 5);
    EXPECT_EQ(ground_witness->get_cost(), 5);
    ASSERT_EQ(lifted_witness->get_numeric_supports().size(), 2);
    ASSERT_EQ(ground_witness->get_numeric_supports().size(), 2);
    for (size_t i = 0; i < ground_witness->get_numeric_supports().size(); ++i)
    {
        const auto& ground_support = ground_witness->get_numeric_supports()[i];
        const auto& lifted_support = lifted_witness->get_numeric_supports()[i];
        EXPECT_EQ(ground_support.get_key(), lifted_support.get_key());
        EXPECT_EQ(ground_support.get_key(), ground_source.get_row().get_index());
        EXPECT_EQ(ground_support.get_interval(), lifted_support.get_interval());
        EXPECT_EQ(ground_support.get_cost(), lifted_support.get_cost());
    }
    EXPECT_EQ(lifted_witness->get_numeric_supports()[0].get_interval(), low_support_interval);
    EXPECT_EQ(lifted_witness->get_numeric_supports()[0].get_cost(), 2);
    EXPECT_EQ(lifted_witness->get_numeric_supports()[1].get_interval(), high_support_interval);
    EXPECT_EQ(lifted_witness->get_numeric_supports()[1].get_cost(), 2);
}

TEST(TyrDatalogLiftedBottomUpTest, RepeatedArgumentsNewAtomRecordsExistingHeadAchiever)
{
    auto factory = std::make_shared<tyr::formalism::datalog::RepositoryFactory>();
    auto repository = factory->create_shared();
    const auto intern = [&]<typename T>(ygg::Data<T> data)
    {
        if constexpr (requires { tyr::formalism::datalog::canonicalize(data); })
            tyr::formalism::datalog::canonicalize(data);
        else
            tyr::formalism::canonicalize(data);
        return repository->insert(data).first;
    };

    const auto diagonal = intern(ygg::Data<tyr::formalism::Predicate<tyr::formalism::FluentTag>>(std::string("diagonal"), 2));
    const auto seed = intern(ygg::Data<tyr::formalism::Predicate<tyr::formalism::FluentTag>>(std::string("seed"), 1));
    const auto goal_predicate = intern(ygg::Data<tyr::formalism::Predicate<tyr::formalism::FluentTag>>(std::string("goal"), 0));
    const auto a = intern(ygg::Data<tyr::formalism::Object>(std::string("a")));
    const auto b = intern(ygg::Data<tyr::formalism::Object>(std::string("b")));
    const auto x = intern(ygg::Data<tyr::formalism::Variable>(std::string("x")));
    const auto y = intern(ygg::Data<tyr::formalism::Variable>(std::string("y")));

    const auto make_atom = [&](auto predicate, std::initializer_list<tyr::formalism::ParameterIndex> terms)
    {
        auto data = ygg::Data<tyr::formalism::datalog::Atom<LiftedTag, tyr::formalism::FluentTag>>();
        data.predicate = predicate.get_index();
        for (const auto term : terms)
            data.terms.emplace_back(term);
        return intern(std::move(data));
    };
    const auto make_literal = [&](auto atom)
    { return intern(ygg::Data<tyr::formalism::datalog::Literal<LiftedTag, tyr::formalism::FluentTag>>(atom.get_index(), true)); };
    const auto make_initial_atom = [&](auto predicate, std::initializer_list<ygg::Index<tyr::formalism::Object>> objects)
    {
        auto binding_data = ygg::Data<tyr::formalism::RelationBinding<tyr::formalism::Predicate<tyr::formalism::FluentTag>>>();
        binding_data.relation = predicate.get_index();
        binding_data.objects.insert(binding_data.objects.end(), objects.begin(), objects.end());
        return intern(ygg::Data<tyr::formalism::datalog::Atom<GroundTag, tyr::formalism::FluentTag>>(intern(std::move(binding_data)).get_index()));
    };

    const auto derive_seed_atom = make_atom(seed, { tyr::formalism::ParameterIndex(0) });
    const auto derive_diagonal_atom = make_atom(diagonal, { tyr::formalism::ParameterIndex(0), tyr::formalism::ParameterIndex(0) });
    auto derive_body_data = ygg::Data<tyr::formalism::datalog::ConjunctiveCondition<LiftedTag>>();
    derive_body_data.variables.push_back(y.get_index());
    derive_body_data.fluent_literals.push_back(make_literal(derive_seed_atom).get_index());
    const auto derive_body = intern(std::move(derive_body_data));
    auto derive_rule_data = ygg::Data<tyr::formalism::datalog::Rule<LiftedTag, tyr::formalism::PredicateTag>>();
    derive_rule_data.variables.push_back(y.get_index());
    derive_rule_data.body = derive_body.get_index();
    derive_rule_data.heads.push_back(derive_diagonal_atom.get_index());
    const auto derive_rule = intern(std::move(derive_rule_data));

    const auto goal_diagonal_atom = make_atom(diagonal, { tyr::formalism::ParameterIndex(0), tyr::formalism::ParameterIndex(0) });
    const auto goal_pair_atom = make_atom(diagonal, { tyr::formalism::ParameterIndex(0), tyr::formalism::ParameterIndex(1) });
    const auto goal_seed_atom = make_atom(seed, { tyr::formalism::ParameterIndex(1) });
    const auto goal_atom = make_atom(goal_predicate, {});
    auto goal_body_data = ygg::Data<tyr::formalism::datalog::ConjunctiveCondition<LiftedTag>>();
    goal_body_data.variables.push_back(x.get_index());
    goal_body_data.variables.push_back(y.get_index());
    goal_body_data.fluent_literals.push_back(make_literal(goal_diagonal_atom).get_index());
    goal_body_data.fluent_literals.push_back(make_literal(goal_pair_atom).get_index());
    goal_body_data.fluent_literals.push_back(make_literal(goal_seed_atom).get_index());
    const auto goal_body = intern(std::move(goal_body_data));
    auto goal_rule_data = ygg::Data<tyr::formalism::datalog::Rule<LiftedTag, tyr::formalism::PredicateTag>>();
    goal_rule_data.variables.push_back(x.get_index());
    goal_rule_data.variables.push_back(y.get_index());
    goal_rule_data.body = goal_body.get_index();
    goal_rule_data.heads.push_back(goal_atom.get_index());
    const auto goal_rule = intern(std::move(goal_rule_data));
    const auto conflicting_goal_condition =
        tyr::datalog::create_overapproximation_conflicting_conjunctive_condition(2, goal_rule.get_body(), *repository).first;
    const auto conflicting_literals = conflicting_goal_condition.get_literals<tyr::formalism::FluentTag>();
    ASSERT_EQ(conflicting_literals.size(), 1);
    EXPECT_EQ(conflicting_literals.front().get_atom().get_index(), goal_diagonal_atom.get_index());

    const auto initial_aa = make_initial_atom(diagonal, { a.get_index(), a.get_index() });
    const auto initial_ab = make_initial_atom(diagonal, { a.get_index(), b.get_index() });
    const auto initial_ba = make_initial_atom(diagonal, { b.get_index(), a.get_index() });
    const auto initial_seed = make_initial_atom(seed, { b.get_index() });

    auto goal_binding_data = ygg::Data<tyr::formalism::RelationBinding<tyr::formalism::Predicate<tyr::formalism::FluentTag>>>();
    goal_binding_data.relation = goal_predicate.get_index();
    const auto goal = intern(std::move(goal_binding_data));

    auto program_data = ygg::Data<tyr::formalism::datalog::Program<LiftedTag>>();
    program_data.objects = { a.get_index(), b.get_index() };
    program_data.fluent_predicates = { diagonal.get_index(), seed.get_index(), goal_predicate.get_index() };
    program_data.fluent_atoms = { initial_aa.get_index(), initial_ab.get_index(), initial_ba.get_index(), initial_seed.get_index() };
    program_data.predicate_rules = { derive_rule.get_index(), goal_rule.get_index() };
    const auto program_view = intern(std::move(program_data));
    auto program = tyr::datalog::Program<LiftedTag>(program_view, repository, factory);

    using AnnotationPolicy = tyr::datalog::MinCostAnnotationWithAchieversPolicy<tyr::datalog::MaxAggregation>;
    using TerminationPolicy = tyr::datalog::TerminationPolicy<tyr::datalog::MaxAggregation>;
    auto workspace = tyr::datalog::ProgramWorkspace<LiftedTag, AnnotationPolicy, TerminationPolicy>(program);
    auto context = tyr::datalog::ProgramExecutionContext(workspace);
    tyr::datalog::compute_model(context);

    ASSERT_TRUE(workspace.facts.fact_sets.predicate.contains(goal));
    const auto* achievers = workspace.annotation_policy.find_achievers(goal.get_index());
    ASSERT_NE(achievers, nullptr);
    ASSERT_EQ(achievers->size(), 2);

    auto bindings = std::vector<std::vector<ygg::Index<tyr::formalism::Object>>>();
    for (const auto& achiever : *achievers)
    {
        EXPECT_EQ(achiever.get_rule_key().relation, goal_rule.get_index());
        auto& binding = bindings.emplace_back();
        for (const auto object : ygg::make_view(achiever.get_rule_key(), workspace.workspace_repository).get_objects())
            binding.push_back(object.get_index());
    }
    std::ranges::sort(bindings);
    const auto expected = std::vector<std::vector<ygg::Index<tyr::formalism::Object>>> { { a.get_index(), b.get_index() }, { b.get_index(), b.get_index() } };
    EXPECT_EQ(bindings, expected);
}

TEST(TyrDatalogLiftedBottomUpTest, NewHigherArityAtomsGenerateBindingsWithoutDeltaEdges)
{
    // h(x,y,z) :- t(x,y,z), u(x,y,z). The pairwise projections of t and u hold initially through other atoms, so the
    // consistency graph of the h rule has the clique (a,b,c) before t(a,b,c) and u(a,b,c) are derived in the same
    // iteration. Only the new atoms can generate h(a,b,c), and only the one of the first literal may (Debug checks duplicates).
    auto factory = std::make_shared<tyr::formalism::datalog::RepositoryFactory>();
    auto repository = factory->create_shared();
    const auto intern = [&]<typename T>(ygg::Data<T> data)
    {
        if constexpr (requires { tyr::formalism::datalog::canonicalize(data); })
            tyr::formalism::datalog::canonicalize(data);
        else
            tyr::formalism::canonicalize(data);
        return repository->insert(data).first;
    };

    using FluentPredicate = tyr::formalism::Predicate<tyr::formalism::FluentTag>;
    const auto t = intern(ygg::Data<FluentPredicate>(std::string("t"), 3));
    const auto u = intern(ygg::Data<FluentPredicate>(std::string("u"), 3));
    const auto h = intern(ygg::Data<FluentPredicate>(std::string("h"), 3));
    const auto s = intern(ygg::Data<FluentPredicate>(std::string("s"), 1));
    const auto m = intern(ygg::Data<FluentPredicate>(std::string("m"), 2));
    const auto a = intern(ygg::Data<tyr::formalism::Object>(std::string("a")));
    const auto b = intern(ygg::Data<tyr::formalism::Object>(std::string("b")));
    const auto c = intern(ygg::Data<tyr::formalism::Object>(std::string("c")));
    const auto d = intern(ygg::Data<tyr::formalism::Object>(std::string("d")));
    const auto x = intern(ygg::Data<tyr::formalism::Variable>(std::string("x")));
    const auto y = intern(ygg::Data<tyr::formalism::Variable>(std::string("y")));
    const auto z = intern(ygg::Data<tyr::formalism::Variable>(std::string("z")));
    const auto p0 = tyr::formalism::ParameterIndex(0);
    const auto p1 = tyr::formalism::ParameterIndex(1);
    const auto p2 = tyr::formalism::ParameterIndex(2);

    const auto make_atom = [&](auto predicate, std::initializer_list<tyr::formalism::ParameterIndex> terms)
    {
        auto data = ygg::Data<tyr::formalism::datalog::Atom<LiftedTag, tyr::formalism::FluentTag>>();
        data.predicate = predicate.get_index();
        for (const auto term : terms)
            data.terms.emplace_back(term);
        return intern(std::move(data));
    };
    const auto make_literal = [&](auto atom)
    { return intern(ygg::Data<tyr::formalism::datalog::Literal<LiftedTag, tyr::formalism::FluentTag>>(atom.get_index(), true)); };
    const auto make_binding = [&](auto predicate, std::initializer_list<ygg::Index<tyr::formalism::Object>> objects)
    {
        auto data = ygg::Data<tyr::formalism::RelationBinding<FluentPredicate>>();
        data.relation = predicate.get_index();
        data.objects.insert(data.objects.end(), objects.begin(), objects.end());
        return intern(std::move(data));
    };
    const auto make_initial_atom = [&](auto predicate, std::initializer_list<ygg::Index<tyr::formalism::Object>> objects)
    { return intern(ygg::Data<tyr::formalism::datalog::Atom<GroundTag, tyr::formalism::FluentTag>>(make_binding(predicate, objects).get_index())); };
    const auto make_predicate_rule = [&](auto head, std::vector<ygg::Index<tyr::formalism::datalog::Literal<LiftedTag, tyr::formalism::FluentTag>>> literals)
    {
        auto body = ygg::Data<tyr::formalism::datalog::ConjunctiveCondition<LiftedTag>>();
        body.variables = { x.get_index(), y.get_index(), z.get_index() };
        body.fluent_literals.insert(body.fluent_literals.end(), literals.begin(), literals.end());
        auto rule = ygg::Data<tyr::formalism::datalog::Rule<LiftedTag, tyr::formalism::PredicateTag>>();
        rule.variables = { x.get_index(), y.get_index(), z.get_index() };
        rule.body = intern(std::move(body)).get_index();
        rule.heads.push_back(head.get_index());
        return intern(std::move(rule));
    };

    const auto seeded = std::vector { make_literal(make_atom(s, { p0 })).get_index(), make_literal(make_atom(m, { p1, p2 })).get_index() };
    const auto derive_t = make_predicate_rule(make_atom(t, { p0, p1, p2 }), seeded);
    const auto derive_u = make_predicate_rule(make_atom(u, { p0, p1, p2 }), seeded);
    const auto derive_h = make_predicate_rule(make_atom(h, { p0, p1, p2 }),
                                              { make_literal(make_atom(t, { p0, p1, p2 })).get_index(), make_literal(make_atom(u, { p0, p1, p2 })).get_index() });

    auto program_data = ygg::Data<tyr::formalism::datalog::Program<LiftedTag>>();
    program_data.objects = { a.get_index(), b.get_index(), c.get_index(), d.get_index() };
    program_data.fluent_predicates = { t.get_index(), u.get_index(), h.get_index(), s.get_index(), m.get_index() };
    for (const auto predicate : { t, u })
    {
        // Projections (a,b), (b,c) and (a,c) of the triple (a,b,c), each through another atom.
        program_data.fluent_atoms.push_back(make_initial_atom(predicate, { a.get_index(), b.get_index(), d.get_index() }).get_index());
        program_data.fluent_atoms.push_back(make_initial_atom(predicate, { d.get_index(), b.get_index(), c.get_index() }).get_index());
        program_data.fluent_atoms.push_back(make_initial_atom(predicate, { a.get_index(), d.get_index(), c.get_index() }).get_index());
    }
    program_data.fluent_atoms.push_back(make_initial_atom(s, { a.get_index() }).get_index());
    program_data.fluent_atoms.push_back(make_initial_atom(m, { b.get_index(), c.get_index() }).get_index());
    program_data.predicate_rules = { derive_t.get_index(), derive_u.get_index(), derive_h.get_index() };
    auto program = tyr::datalog::Program<LiftedTag>(intern(std::move(program_data)), repository, factory);

    using AnnotationPolicy = tyr::datalog::MinCostAnnotationWithAchieversPolicy<tyr::datalog::MaxAggregation>;
    using TerminationPolicy = tyr::datalog::TerminationPolicy<tyr::datalog::MaxAggregation>;
    auto workspace = tyr::datalog::ProgramWorkspace<LiftedTag, AnnotationPolicy, TerminationPolicy>(program);
    auto context = tyr::datalog::ProgramExecutionContext(workspace);
    tyr::datalog::compute_model(context);

    const auto target = make_binding(h, { a.get_index(), b.get_index(), c.get_index() });
    ASSERT_TRUE(workspace.facts.fact_sets.predicate.contains(target.get_index()));
    const auto* achievers = workspace.annotation_policy.find_achievers(target.get_index());
    ASSERT_NE(achievers, nullptr);
    EXPECT_EQ(achievers->size(), 1);
    for (const auto other : { make_binding(h, { a.get_index(), b.get_index(), d.get_index() }),
                              make_binding(h, { d.get_index(), b.get_index(), c.get_index() }),
                              make_binding(h, { a.get_index(), d.get_index(), c.get_index() }) })
        EXPECT_TRUE(workspace.facts.fact_sets.predicate.contains(other.get_index()));

    // The h rule rejected (a,b,c) before t(a,b,c) existed, so the new atom, not a delta edge, generated it.
    const auto& h_rule = workspace.predicate_rules[ygg::uint_t(derive_h.get_index())];
    ASSERT_NE(h_rule, nullptr);
    EXPECT_GT(h_rule->worker.front().solve.statistics.num_rejected_rules, 0);
}

TEST(TyrDatalogLiftedBottomUpTest, RejectedCanonicalTiesDoNotInternUnneededBindings)
{
    auto factory = std::make_shared<tyr::formalism::datalog::RepositoryFactory>();
    auto repository = factory->create_shared();
    const auto intern = [&]<typename T>(ygg::Data<T> data)
    {
        if constexpr (requires { tyr::formalism::datalog::canonicalize(data); })
            tyr::formalism::datalog::canonicalize(data);
        else
            tyr::formalism::canonicalize(data);
        return repository->insert(data).first;
    };

    const auto source = intern(ygg::Data<tyr::formalism::Predicate<tyr::formalism::FluentTag>>(std::string("source"), 1));
    const auto goal = intern(ygg::Data<tyr::formalism::Predicate<tyr::formalism::FluentTag>>(std::string("goal"), 0));
    const auto variable = intern(ygg::Data<tyr::formalism::Variable>(std::string("x")));

    auto source_atom_data = ygg::Data<tyr::formalism::datalog::Atom<LiftedTag, tyr::formalism::FluentTag>>();
    source_atom_data.predicate = source.get_index();
    source_atom_data.terms.emplace_back(tyr::formalism::ParameterIndex(0));
    const auto source_atom = intern(std::move(source_atom_data));

    const auto source_literal = intern(ygg::Data<tyr::formalism::datalog::Literal<LiftedTag, tyr::formalism::FluentTag>>(source_atom.get_index(), true));
    auto body_data = ygg::Data<tyr::formalism::datalog::ConjunctiveCondition<LiftedTag>>();
    body_data.variables.push_back(variable.get_index());
    body_data.fluent_literals.push_back(source_literal.get_index());
    const auto body = intern(std::move(body_data));

    auto goal_atom_data = ygg::Data<tyr::formalism::datalog::Atom<LiftedTag, tyr::formalism::FluentTag>>();
    goal_atom_data.predicate = goal.get_index();
    const auto goal_atom = intern(std::move(goal_atom_data));

    auto rule_data = ygg::Data<tyr::formalism::datalog::Rule<LiftedTag, tyr::formalism::PredicateTag>>();
    rule_data.variables.push_back(variable.get_index());
    rule_data.body = body.get_index();
    rule_data.heads.push_back(goal_atom.get_index());
    const auto rule = intern(std::move(rule_data));

    auto program_data = ygg::Data<tyr::formalism::datalog::Program<LiftedTag>>();
    program_data.fluent_predicates.push_back(source.get_index());
    program_data.fluent_predicates.push_back(goal.get_index());
    program_data.predicate_rules.push_back(rule.get_index());
    for (const auto* name : { "a", "b", "c", "d" })
    {
        const auto object = intern(ygg::Data<tyr::formalism::Object>(std::string(name)));
        program_data.objects.push_back(object.get_index());

        auto binding_data = ygg::Data<tyr::formalism::RelationBinding<tyr::formalism::Predicate<tyr::formalism::FluentTag>>>();
        binding_data.relation = source.get_index();
        binding_data.objects.push_back(object.get_index());
        const auto binding = intern(std::move(binding_data));
        program_data.fluent_atoms.push_back(
            intern(ygg::Data<tyr::formalism::datalog::Atom<GroundTag, tyr::formalism::FluentTag>>(binding.get_index())).get_index());
    }
    const auto program_view = intern(std::move(program_data));

    auto program = tyr::datalog::Program<LiftedTag>(program_view, repository, factory);
    const auto& const_rule_workspace = program.get_const_program_workspace().get_rules<tyr::formalism::PredicateTag>().front().value();
    EXPECT_EQ(&const_rule_workspace.get_nullary_condition().get_context(), &program.get_program_repository());

    using AnnotationPolicy = tyr::datalog::MinCostAnnotationPolicy<tyr::datalog::SumAggregation>;
    using Termination = tyr::datalog::NoTerminationPolicy;
    auto workspace = tyr::datalog::ProgramWorkspace<LiftedTag, AnnotationPolicy, Termination>(program);
    auto context = tyr::datalog::ProgramExecutionContext(workspace);
    tyr::datalog::compute_model(context);

    EXPECT_EQ(workspace.workspace_repository.size(rule.get_index()), 1);
    EXPECT_EQ(workspace.workspace_repository.size(goal.get_index()), 1);
}

TEST(TyrDatalogLiftedBottomUpTest, ProgramWorkspacesOwnIndependentRepositories)
{
    auto factory = std::make_shared<tyr::formalism::datalog::RepositoryFactory>();
    auto repository = factory->create_shared();
    const auto intern = [&]<typename T>(ygg::Data<T> data)
    {
        tyr::formalism::canonicalize(data);
        return repository->insert(data).first;
    };

    const auto predicate = intern(ygg::Data<tyr::formalism::Predicate<tyr::formalism::FluentTag>>(std::string("workspace-predicate"), 1));
    const auto object = intern(ygg::Data<tyr::formalism::Object>(std::string("workspace-object")));
    auto program_data = ygg::Data<tyr::formalism::datalog::Program<LiftedTag>> {};
    program_data.fluent_predicates.push_back(predicate.get_index());
    program_data.objects.push_back(object.get_index());
    tyr::formalism::datalog::canonicalize(program_data);
    const auto program_view = repository->insert(program_data).first;
    const auto program = tyr::datalog::Program<LiftedTag>(program_view, repository, factory);

    auto first = tyr::datalog::ProgramWorkspace<LiftedTag>(program);
    auto second = tyr::datalog::ProgramWorkspace<LiftedTag>(program);

    EXPECT_NE(&first.workspace_repository, &second.workspace_repository);
    EXPECT_EQ(&first.workspace_repository.get_root(), &program.get_program_repository());
    EXPECT_EQ(&second.workspace_repository.get_root(), &program.get_program_repository());

    auto variable = ygg::Data<tyr::formalism::Variable>(std::string("workspace-only"));
    EXPECT_TRUE(first.workspace_repository.insert(variable).second);
    EXPECT_FALSE(second.workspace_repository.find(variable).has_value());

    auto second_variable = ygg::Data<tyr::formalism::Variable>(std::string("second-workspace-only"));
    EXPECT_TRUE(second.workspace_repository.insert(second_variable).second);

    auto binding_data = ygg::Data<tyr::formalism::RelationBinding<tyr::formalism::Predicate<tyr::formalism::FluentTag>>>();
    binding_data.relation = predicate.get_index();
    binding_data.objects.push_back(object.get_index());
    const auto binding = first.workspace_repository.insert(binding_data).first;
    const auto binding_index = binding.get_index();
    first.annotations.insert_or_assign(binding.get_index(), tyr::datalog::BaseAnnotation(3));

    first.reset_evaluation();

    EXPECT_FALSE(first.workspace_repository.find(ygg::Data<tyr::formalism::Variable>(std::string("workspace-only"))).has_value());
    EXPECT_TRUE(second.workspace_repository.find(ygg::Data<tyr::formalism::Variable>(std::string("second-workspace-only"))).has_value());
    EXPECT_EQ(&first.workspace_repository.get_root(), &program.get_program_repository());

    auto reused_binding_data = ygg::Data<tyr::formalism::RelationBinding<tyr::formalism::Predicate<tyr::formalism::FluentTag>>>();
    reused_binding_data.relation = predicate.get_index();
    reused_binding_data.objects.push_back(object.get_index());
    const auto reused_binding = first.workspace_repository.insert(reused_binding_data).first;
    EXPECT_EQ(reused_binding.get_index(), binding_index);
    EXPECT_EQ(first.annotations.find(reused_binding.get_index()), nullptr);
}

TEST(TyrDatalogLiftedBottomUpTest, PredicateAnnotationsUseRelationAndRowIndices)
{
    auto factory = std::make_shared<tyr::formalism::datalog::RepositoryFactory>();
    auto repository = factory->create_shared();
    const auto intern = [&]<typename T>(ygg::Data<T> data)
    {
        tyr::formalism::canonicalize(data);
        return repository->insert(data).first;
    };

    const auto first_predicate = intern(ygg::Data<tyr::formalism::Predicate<tyr::formalism::FluentTag>>(std::string("first"), 1));
    const auto second_predicate = intern(ygg::Data<tyr::formalism::Predicate<tyr::formalism::FluentTag>>(std::string("second"), 1));
    const auto first_object = intern(ygg::Data<tyr::formalism::Object>(std::string("a")));
    const auto second_object = intern(ygg::Data<tyr::formalism::Object>(std::string("b")));
    const auto make_binding = [&](auto predicate, auto object)
    {
        auto data = ygg::Data<tyr::formalism::RelationBinding<tyr::formalism::Predicate<tyr::formalism::FluentTag>>>();
        data.relation = predicate.get_index();
        data.objects.push_back(object.get_index());
        return intern(std::move(data));
    };

    const auto first = make_binding(first_predicate, first_object);
    const auto hole = make_binding(first_predicate, second_object);
    const auto second = make_binding(second_predicate, first_object);
    auto annotations = tyr::datalog::PredicateAnnotations<>();

    annotations.insert_or_assign(first.get_index(), tyr::datalog::BaseAnnotation(3));
    annotations.insert_or_assign(second.get_index(), tyr::datalog::BaseAnnotation(5));
    EXPECT_EQ(tyr::datalog::get_cost(*annotations.find(first.get_index())), 3);
    EXPECT_EQ(tyr::datalog::get_cost(*std::as_const(annotations).find(second.get_index())), 5);
    EXPECT_EQ(annotations.find(hole.get_index()), nullptr);

    annotations.insert_or_assign(first.get_index(), tyr::datalog::BaseAnnotation(2));
    EXPECT_EQ(tyr::datalog::get_cost(*annotations.find(first.get_index())), 2);

    annotations.clear();
    EXPECT_EQ(annotations.find(first.get_index()), nullptr);
    EXPECT_EQ(annotations.find(second.get_index()), nullptr);
}

TEST(TyrDatalogLiftedBottomUpTest, DeltaAnnotationsAreConcurrentAndReusable)
{
    auto factory = std::make_shared<tyr::formalism::datalog::RepositoryFactory>();
    auto repository = factory->create_shared();
    const auto intern = [&]<typename T>(ygg::Data<T> data)
    {
        tyr::formalism::canonicalize(data);
        return repository->insert(data).first;
    };

    const auto first_predicate = intern(ygg::Data<tyr::formalism::Predicate<tyr::formalism::FluentTag>>(std::string("first"), 1));
    const auto second_predicate = intern(ygg::Data<tyr::formalism::Predicate<tyr::formalism::FluentTag>>(std::string("second"), 1));
    const auto function = intern(ygg::Data<tyr::formalism::Function<tyr::formalism::FluentTag>>(std::string("value"), 1));

    auto first_bindings = std::vector<tyr::formalism::datalog::PredicateBindingView<tyr::formalism::FluentTag>> {};
    auto second_bindings = std::vector<tyr::formalism::datalog::PredicateBindingView<tyr::formalism::FluentTag>> {};
    auto function_bindings = std::vector<tyr::formalism::datalog::FunctionBindingView<tyr::formalism::FluentTag>> {};
    for (size_t i = 0; i < 64; ++i)
    {
        const auto object = intern(ygg::Data<tyr::formalism::Object>("o" + std::to_string(i)));
        const auto make_predicate_binding = [&](auto predicate)
        {
            auto data = ygg::Data<tyr::formalism::RelationBinding<tyr::formalism::Predicate<tyr::formalism::FluentTag>>> {};
            data.relation = predicate.get_index();
            data.objects.push_back(object.get_index());
            return intern(std::move(data));
        };
        const auto make_function_binding = [&](auto function_)
        {
            auto data = ygg::Data<tyr::formalism::RelationBinding<tyr::formalism::Function<tyr::formalism::FluentTag>>> {};
            data.relation = function_.get_index();
            data.objects.push_back(object.get_index());
            return intern(std::move(data));
        };
        first_bindings.push_back(make_predicate_binding(first_predicate));
        second_bindings.push_back(make_predicate_binding(second_predicate));
        function_bindings.push_back(make_function_binding(function));
    }

    using SequentialMap = tyr::datalog::DenseRelationMap<tyr::formalism::PredicateTag, tyr::datalog::Cost>;
    using ConcurrentMap = tyr::datalog::DenseRelationMap<tyr::formalism::PredicateTag, tyr::datalog::Cost, true>;
    static_assert(!SequentialMap::thread_safe);
    static_assert(ConcurrentMap::thread_safe);
    static_assert(!tyr::datalog::PredicateAnnotations<>::thread_safe);
    static_assert(tyr::datalog::PredicateAnnotations<true>::thread_safe);
    static_assert(!tyr::datalog::FunctionAnnotations<>::thread_safe);
    static_assert(tyr::datalog::FunctionAnnotations<true>::thread_safe);
    static_assert(std::is_default_constructible_v<tyr::datalog::PredicateAnnotations<>>);
    static_assert(!std::is_default_constructible_v<tyr::datalog::PredicateAnnotations<true>>);
    static_assert(std::is_default_constructible_v<tyr::datalog::FunctionAnnotations<>>);
    static_assert(!std::is_default_constructible_v<tyr::datalog::FunctionAnnotations<true>>);
    static_assert(std::is_copy_constructible_v<SequentialMap>);
    static_assert(!std::is_copy_constructible_v<ConcurrentMap>);
    static_assert(std::is_nothrow_move_constructible_v<ConcurrentMap>);
    static_assert(std::is_nothrow_move_assignable_v<ConcurrentMap>);
    const auto retain_min = [](auto& map, auto binding, tyr::datalog::Cost candidate)
    {
        map.update(binding.get_index(),
                   [&](auto& incumbent, bool initialized)
                   {
                       if (!initialized || candidate < incumbent)
                           incumbent = candidate;
                   });
    };
    auto sequential_map = SequentialMap(2);
    auto concurrent_map = ConcurrentMap(2);
    for (size_t worker = 0; worker < 8; ++worker)
        retain_min(sequential_map, first_bindings.front(), tyr::datalog::Cost(8 - worker));

    auto delta_annotations = tyr::datalog::PredicateAnnotations<true>(2);
    auto delta_numeric_annotations = tyr::datalog::FunctionAnnotations<true>(1);
    const auto interval = ygg::ClosedInterval<ygg::float_t>(0, 1);
    auto arena = oneapi::tbb::task_arena(8);
    arena.execute(
        [&]
        {
            oneapi::tbb::parallel_for(size_t(0),
                                      size_t(8),
                                      [&](size_t worker)
                                      {
                                          const auto predicate_cost = tyr::datalog::Cost(8 - worker);
                                          const auto function_cost = tyr::datalog::Cost(worker < 4 ? 3 : 5);
                                          retain_min(concurrent_map, first_bindings.front(), predicate_cost);
                                          for (size_t i = first_bindings.size(); i-- > 0;)
                                          {
                                              delta_annotations.insert_if_better(first_bindings[i].get_index(), tyr::datalog::BaseAnnotation(predicate_cost));
                                              delta_annotations.insert_if_better(second_bindings[i].get_index(), tyr::datalog::BaseAnnotation(predicate_cost));
                                              delta_numeric_annotations.insert(function_bindings[i].get_index(), interval, tyr::datalog::BaseAnnotation(function_cost));
                                          }
                                      });
        });

    ASSERT_NE(sequential_map.find(first_bindings.front().get_index()), nullptr);
    ASSERT_NE(concurrent_map.find(first_bindings.front().get_index()), nullptr);
    EXPECT_EQ(*sequential_map.find(first_bindings.front().get_index()), *concurrent_map.find(first_bindings.front().get_index()));

    for (size_t i = 0; i < first_bindings.size(); ++i)
    {
        ASSERT_NE(delta_annotations.find(first_bindings[i].get_index()), nullptr);
        ASSERT_NE(delta_annotations.find(second_bindings[i].get_index()), nullptr);
        EXPECT_EQ(tyr::datalog::get_cost(*delta_annotations.find(first_bindings[i].get_index())), 1);
        EXPECT_EQ(tyr::datalog::get_cost(*delta_annotations.find(second_bindings[i].get_index())), 1);

        const auto* entries = delta_numeric_annotations.find_entries(function_bindings[i].get_index());
        ASSERT_NE(entries, nullptr);
        ASSERT_EQ(entries->size(), 1);
        EXPECT_EQ(tyr::datalog::get_cost(entries->at(0).annotation), 3);
    }

    const auto second_interval = ygg::ClosedInterval<ygg::float_t>(1, 2);
    EXPECT_TRUE(delta_numeric_annotations.insert(function_bindings.back().get_index(), second_interval, tyr::datalog::BaseAnnotation(4)));
    EXPECT_FALSE(delta_numeric_annotations.insert(function_bindings.back().get_index(), second_interval, tyr::datalog::BaseAnnotation(5)));
    EXPECT_TRUE(delta_numeric_annotations.insert(function_bindings.back().get_index(), second_interval, tyr::datalog::BaseAnnotation(2)));
    ASSERT_EQ(delta_numeric_annotations.find_entries(function_bindings.back().get_index())->size(), 2);
    ASSERT_NE(delta_numeric_annotations.find(function_bindings.back().get_index(), second_interval), nullptr);
    EXPECT_EQ(tyr::datalog::get_cost(*delta_numeric_annotations.find(function_bindings.back().get_index(), second_interval)), 2);

    auto numeric_annotations = tyr::datalog::FunctionAnnotations<>();
    numeric_annotations.insert(function_bindings.back().get_index(), interval, tyr::datalog::BaseAnnotation(5));
    numeric_annotations.insert(function_bindings.back().get_index(), interval, tyr::datalog::BaseAnnotation(3));
    numeric_annotations.insert(function_bindings.back().get_index(), interval, tyr::datalog::BaseAnnotation(4));
    numeric_annotations.insert(function_bindings.back().get_index(), interval, tyr::datalog::BaseAnnotation(3));
    numeric_annotations.insert(function_bindings.back().get_index(), second_interval, tyr::datalog::BaseAnnotation(2));
    numeric_annotations.insert(function_bindings.back().get_index(), interval, tyr::datalog::BaseAnnotation(1));
    const auto* numeric_entries = numeric_annotations.find_entries(function.get_index(), function_bindings.back().get_index().row);
    ASSERT_NE(numeric_entries, nullptr);
    ASSERT_EQ(numeric_entries->size(), 2);
    EXPECT_EQ(numeric_entries->at(0).interval, interval);
    EXPECT_EQ(tyr::datalog::get_cost(numeric_entries->at(0).annotation), 1);
    EXPECT_EQ(numeric_entries->at(1).interval, second_interval);
    EXPECT_EQ(tyr::datalog::get_cost(numeric_entries->at(1).annotation), 2);
    EXPECT_EQ(tyr::datalog::get_cost(*numeric_annotations.find(function_bindings.back().get_index(), interval)), 1);

    delta_annotations.clear();
    delta_numeric_annotations.clear();
    sequential_map.clear();
    concurrent_map.clear();
    EXPECT_EQ(delta_annotations.find(first_bindings.back().get_index()), nullptr);
    EXPECT_EQ(delta_numeric_annotations.find_entries(function_bindings.back().get_index()), nullptr);
    EXPECT_EQ(sequential_map.find(first_bindings.front().get_index()), nullptr);
    EXPECT_EQ(concurrent_map.find(first_bindings.front().get_index()), nullptr);

    delta_annotations.insert_if_better(first_bindings.back().get_index(), tyr::datalog::BaseAnnotation(7));
    delta_numeric_annotations.insert(function_bindings.back().get_index(), interval, tyr::datalog::BaseAnnotation(7));
    retain_min(sequential_map, first_bindings.front(), tyr::datalog::Cost(7));
    retain_min(concurrent_map, first_bindings.front(), tyr::datalog::Cost(7));
    EXPECT_EQ(tyr::datalog::get_cost(*delta_annotations.find(first_bindings.back().get_index())), 7);
    ASSERT_NE(delta_numeric_annotations.find_entries(function_bindings.back().get_index()), nullptr);
    EXPECT_EQ(delta_numeric_annotations.find_entries(function_bindings.back().get_index())->size(), 1);
    EXPECT_EQ(*sequential_map.find(first_bindings.front().get_index()), *concurrent_map.find(first_bindings.front().get_index()));
}

INSTANTIATE_TEST_SUITE_P(TyrDatalogLiftedBottomUpFixture,
                         BottomUpFixtureTest,
                         ::testing::ValuesIn(load_cases()),
                         [](const testing::TestParamInfo<BottomUpCase>& info) { return info.param.name; });

}
}
