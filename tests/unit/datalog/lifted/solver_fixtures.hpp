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

#ifndef TYR_TESTS_UNIT_DATALOG_LIFTED_SOLVER_FIXTURES_HPP_
#define TYR_TESTS_UNIT_DATALOG_LIFTED_SOLVER_FIXTURES_HPP_

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
#include "tyr/planning/lifted/programs/action.hpp"
#include "tyr/planning/lifted/programs/axiom.hpp"
#include "tyr/planning/lifted/programs/ground.hpp"
#include "tyr/planning/lifted/task.hpp"

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
    const auto target_assign_effect = intern(ygg::Data<tyr::formalism::datalog::NumericEffect<LiftedTag, tyr::formalism::FluentTag>>(
        tyr::formalism::NumericEffectOperatorKind::Assign,
        target_term.get_index(),
        ygg::Data<tyr::formalism::datalog::FunctionExpression<LiftedTag>>(source_term.get_index())));
    // (source) >= 0 reads source once more, next to the target assignment.
    const auto source_comparison = intern(ygg::Data<tyr::formalism::datalog::BinaryOperator<LiftedTag, tyr::formalism::BooleanOperatorKind>>(
        tyr::formalism::BooleanOperatorKind::Ge,
        ygg::Data<tyr::formalism::datalog::FunctionExpression<LiftedTag>>(source_term.get_index()),
        ygg::Data<tyr::formalism::datalog::FunctionExpression<LiftedTag>>(ygg::float_t(0))));
    const auto add_source_constraint = [&](ygg::Data<tyr::formalism::datalog::ConjunctiveCondition<LiftedTag>>& condition)
    {
        condition.numeric_constraints.emplace_back(
            tyr::formalism::BooleanOperatorKind::Ge,
            ygg::Data<tyr::formalism::datalog::BooleanOperator<LiftedTag>>::Variant(source_comparison.get_index()));
    };
    auto source_body_data = ygg::Data<tyr::formalism::datalog::ConjunctiveCondition<LiftedTag>>();
    add_source_constraint(source_body_data);
    const auto source_body = intern(std::move(source_body_data));

    auto target_rule_data = ygg::Data<tyr::formalism::datalog::Rule<LiftedTag, tyr::formalism::FunctionTag>>();
    target_rule_data.body = source_body.get_index();
    target_rule_data.head = ygg::Data<tyr::formalism::datalog::NumericEffectOperator<LiftedTag, tyr::formalism::FluentTag>>(
        tyr::formalism::NumericEffectOperatorKind::Assign,
        ygg::Data<tyr::formalism::datalog::NumericEffectOperator<LiftedTag, tyr::formalism::FluentTag>>::Variant(target_assign_effect.get_index()));
    target_rule_data.cost = 3;
    const auto target_rule = intern(std::move(target_rule_data));

    auto source_rule_data = ygg::Data<tyr::formalism::datalog::Rule<LiftedTag, tyr::formalism::FunctionTag>>();
    source_rule_data.body = empty_body.get_index();
    source_rule_data.head = ygg::Data<tyr::formalism::datalog::NumericEffectOperator<LiftedTag, tyr::formalism::FluentTag>>(
        tyr::formalism::NumericEffectOperatorKind::Assign,
        ygg::Data<tyr::formalism::datalog::NumericEffectOperator<LiftedTag, tyr::formalism::FluentTag>>::Variant(source_assign_effect.get_index()));
    source_rule_data.cost = 2;
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
    add_source_constraint(seed_body_data);
    const auto seed_body = intern(std::move(seed_body_data));
    auto seed_binding_data = ygg::Data<tyr::formalism::RelationBinding<tyr::formalism::Predicate<tyr::formalism::FluentTag>>>();
    seed_binding_data.relation = seed_predicate.get_index();
    seed_binding_data.objects.push_back(object.get_index());
    const auto seed_binding = intern(std::move(seed_binding_data));
    const auto ground_seed = intern(ygg::Data<tyr::formalism::datalog::Atom<GroundTag, tyr::formalism::FluentTag>>(seed_binding.get_index()));

    auto predicate_rule_data = ygg::Data<tyr::formalism::datalog::Rule<LiftedTag, tyr::formalism::PredicateTag>>();
    if (general_goal_rule)
        predicate_rule_data.variables.push_back(variable.get_index());
    predicate_rule_data.body = (general_goal_rule ? seed_body : source_body).get_index();
    predicate_rule_data.heads.push_back(goal_atom.get_index());
    predicate_rule_data.cost = 3;
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

struct LiftedPredicateProgram
{
    tyr::formalism::datalog::RepositoryFactoryPtr factory;
    tyr::formalism::datalog::RepositoryPtr repository;
    tyr::formalism::datalog::PredicateBindingView<tyr::formalism::FluentTag> goal;
    tyr::formalism::datalog::ConjunctiveConditionView<GroundTag> goals;
    tyr::formalism::datalog::RuleBindingView<tyr::formalism::PredicateTag> first_binding;
    tyr::formalism::datalog::RuleBindingView<tyr::formalism::PredicateTag> second_binding;
    tyr::datalog::Program<LiftedTag> program;

    LiftedPredicateProgram(tyr::formalism::datalog::RepositoryFactoryPtr factory_,
                           tyr::formalism::datalog::RepositoryPtr repository_,
                           tyr::formalism::datalog::ProgramView<LiftedTag> program_view,
                           tyr::formalism::datalog::PredicateBindingView<tyr::formalism::FluentTag> goal_,
                           tyr::formalism::datalog::ConjunctiveConditionView<GroundTag> goals_,
                           tyr::formalism::datalog::RuleBindingView<tyr::formalism::PredicateTag> first_binding_,
                           tyr::formalism::datalog::RuleBindingView<tyr::formalism::PredicateTag> second_binding_) :
        factory(std::move(factory_)),
        repository(std::move(repository_)),
        goal(goal_),
        goals(goals_),
        first_binding(first_binding_),
        second_binding(second_binding_),
        program(program_view, repository, factory)
    {
    }
};

/// goal <- source(?x) with cost 2, and with include_numeric_constraint also (value ?x) >= 0 where (value a) = 5 and (value b) = 7.
LiftedPredicateProgram make_lifted_predicate_program(bool include_numeric_constraint = false)
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
    const auto value_function = intern(ygg::Data<tyr::formalism::Function<tyr::formalism::FluentTag>>(std::string("value"), 1));
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

    auto body_data = ygg::Data<tyr::formalism::datalog::ConjunctiveCondition<LiftedTag>>();
    body_data.variables.push_back(variable.get_index());
    body_data.fluent_literals.push_back(source_literal.get_index());
    if (include_numeric_constraint)
    {
        auto value_term_data = ygg::Data<tyr::formalism::datalog::FunctionTerm<LiftedTag, tyr::formalism::FluentTag>>();
        value_term_data.function = value_function.get_index();
        value_term_data.terms.emplace_back(tyr::formalism::ParameterIndex(0));
        const auto value_term = intern(std::move(value_term_data));
        const auto comparison = intern(ygg::Data<tyr::formalism::datalog::BinaryOperator<LiftedTag, tyr::formalism::BooleanOperatorKind>>(
            tyr::formalism::BooleanOperatorKind::Ge,
            ygg::Data<tyr::formalism::datalog::FunctionExpression<LiftedTag>>(value_term.get_index()),
            ygg::Data<tyr::formalism::datalog::FunctionExpression<LiftedTag>>(ygg::float_t(0))));
        body_data.numeric_constraints.emplace_back(tyr::formalism::BooleanOperatorKind::Ge,
                                                   ygg::Data<tyr::formalism::datalog::BooleanOperator<LiftedTag>>::Variant(comparison.get_index()));
    }
    const auto body = intern(std::move(body_data));
    auto rule_data = ygg::Data<tyr::formalism::datalog::Rule<LiftedTag, tyr::formalism::PredicateTag>>();
    rule_data.variables.push_back(variable.get_index());
    rule_data.body = body.get_index();
    rule_data.heads.push_back(goal_atom.get_index());
    rule_data.cost = 2;
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
    if (include_numeric_constraint)
    {
        const auto bind_value = [&](auto object)
        {
            auto data = ygg::Data<tyr::formalism::RelationBinding<tyr::formalism::Function<tyr::formalism::FluentTag>>>();
            data.relation = value_function.get_index();
            data.objects.push_back(object.get_index());
            return intern(ygg::Data<tyr::formalism::datalog::FunctionTerm<GroundTag, tyr::formalism::FluentTag>>(intern(std::move(data))));
        };
        program_data.fluent_functions = { value_function.get_index() };
        program_data.fluent_fterm_values = {
            intern(ygg::Data<tyr::formalism::datalog::FunctionTermValue<GroundTag, tyr::formalism::FluentTag>>(bind_value(first_object), ygg::float_t(5))).get_index(),
            intern(ygg::Data<tyr::formalism::datalog::FunctionTermValue<GroundTag, tyr::formalism::FluentTag>>(bind_value(second_object), ygg::float_t(7))).get_index()
        };
    }
    program_data.fluent_atoms = { ground_first_source.get_index(), ground_second_source.get_index() };
    program_data.predicate_rules = { rule.get_index() };
    const auto program = intern(std::move(program_data));

    return LiftedPredicateProgram(factory, repository, program, goal, goals, first_binding, second_binding);
}

}  // namespace
}  // namespace tyr::tests

#endif
