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
#include "solver_fixtures.hpp"
#include "tyr/datalog/declarations.hpp"
#include "tyr/datalog/fact_sets.hpp"
#include "tyr/datalog/lifted/contexts/program.hpp"
#include "tyr/datalog/lifted/workspaces/program.hpp"
#include "tyr/datalog/policies/aggregation.hpp"
#include "tyr/datalog/policies/annotation.hpp"
#include "tyr/datalog/policies/annotation_types.hpp"
#include "tyr/declarations.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/grounder.hpp"
#include "tyr/formalism/grounding_context.hpp"
#include "tyr/formalism/repository.hpp"
#include "tyr/planning/programs/lifted/action.hpp"
#include "tyr/planning/programs/axiom.hpp"
#include "tyr/planning/programs/lifted/ground.hpp"
#include "tyr/planning/tasks/lifted/task.hpp"

#include <algorithm>
#include <cstddef>
#include <gtest/gtest.h>
#include <stdexcept>
#include <string>
#include <tyr/datalog/lifted/program.hpp>
#include <urd/elements/copy.hpp>
#include <urd/elements/flat.hpp>
#include <urd/elements/index.hpp>
#include <urd/kinds.hpp>
#include <urd/repr/values/unordered_flat_set.hpp>
#include <urd/store.hpp>
#include <yggdrasil/core/closed_interval.hpp>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/execution/onetbb.hpp>
#include <yggdrasil/formalism/copy.hpp>
#include <yggdrasil/formalism/declarations.hpp>

namespace tyr::tests
{
namespace
{

TEST(TyrDatalogLiftedBottomUpTest, PreparedContextMatchesOrdinaryContextAfterWorkspaceReuse)
{
    auto fixture = make_lifted_predicate_program(true);
    using AnnotationPolicy = tyr::datalog::MinCostAnnotationPolicy<tyr::datalog::SumAggregation>;
    using Context = tyr::datalog::ProgramExecutionContext<LiftedTag, AnnotationPolicy>;
    auto workspace = tyr::datalog::ProgramWorkspace<LiftedTag, AnnotationPolicy>(fixture.program);
    auto initial = Context(workspace);
    tyr::datalog::compute_model(initial);
    const auto expected_atoms = collect_atoms_by_predicate(workspace);
    const auto* expected_annotation = workspace.annotations.find(fixture.goal.index());
    ASSERT_NE(expected_annotation, nullptr);
    const auto expected_cost = tyr::datalog::get_cost(*expected_annotation);

    const auto insert_inputs = [&]
    {
        for (const auto atom : fixture.program.get_program().atoms<tyr::formalism::FluentTag>())
            tyr::datalog::insert(workspace.facts.fact_sets, atom.row().index());
        for (const auto fterm_value : fixture.program.get_program().fterm_values<tyr::formalism::FluentTag>())
            tyr::datalog::insert(workspace.facts.fact_sets.get<tyr::formalism::FluentTag>(),
                                 fterm_value.fterm().row().index(),
                                 ygg::ClosedInterval<ygg::float_t>(fterm_value.value(), fterm_value.value()));
    };
    for (size_t iteration = 0; iteration < 2; ++iteration)
    {
        workspace.cost_buckets.insert(99, fixture.goal.index());
        workspace.reset_evaluation();
        EXPECT_TRUE(workspace.cost_buckets.is_empty());
        insert_inputs();
        auto prepared = Context::from_reset_workspace(workspace, workspace.schedulers);
        tyr::datalog::compute_model(prepared);
        EXPECT_EQ(collect_atoms_by_predicate(workspace), expected_atoms);
        const auto* annotation = workspace.annotations.find(fixture.goal.index());
        ASSERT_NE(annotation, nullptr);
        EXPECT_EQ(tyr::datalog::get_cost(*annotation), expected_cost);

        // LM-cut also rebuilds a context with new facts without resetting its repository.
        workspace.facts.reset();
        insert_inputs();
        auto ordinary = Context(workspace);
        tyr::datalog::compute_model(ordinary);
        EXPECT_EQ(collect_atoms_by_predicate(workspace), expected_atoms);
        annotation = workspace.annotations.find(fixture.goal.index());
        ASSERT_NE(annotation, nullptr);
        EXPECT_EQ(tyr::datalog::get_cost(*annotation), expected_cost);
    }
}

TEST(TyrDatalogLiftedBottomUpTest, PreparedBindingSurvivesNestedGrounding)
{
    auto fixture = make_lifted_predicate_program();
    auto builder = tyr::formalism::Builder {};
    const auto program = fixture.program.get_program();
    const auto atom = program.rules()[0].body().literals<tyr::formalism::FluentTag>()[0].atom();
    const auto objects = program.objects();
    ASSERT_EQ(objects.size(), 2);
    auto indices = urd::flat_vector<urd::index<tyr::formalism::Object>> { objects[0].index() };
    auto context = tyr::formalism::GroundingContext { builder, *fixture.repository, indices };
    auto prepared = tyr::formalism::checkout<ygg::Row<tyr::formalism::Predicate<tyr::formalism::FluentTag>>>(builder);
    urd::store(prepared->relation, atom.predicate());
    tyr::formalism::ground(atom.terms(), context, prepared->values);

    indices[0] = objects[1].index();
    const auto nested = tyr::formalism::ground_binding(atom, context).first;
    EXPECT_EQ(nested.get<urd::index<tyr::formalism::Object>>(0), objects[1].index());
    ASSERT_EQ(prepared->values.size(), 1);
    EXPECT_EQ(prepared->values[0], objects[0].index().value);
    const auto retained = fixture.repository->insert(*prepared, builder).first;
    EXPECT_EQ(retained.get<urd::index<tyr::formalism::Object>>(0), objects[0].index());
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

    // The fixtures predate the predicates of the conditional effects, which are internal to the action and ground task programs.
    const auto without_effect_predicates = [](AtomsByPredicate atoms, const auto& effect_to_predicate)
    {
        auto effect_predicates = urd::Store<urd::set<std::string>> {};
        for (const auto& [_, predicate] : effect_to_predicate)
            effect_predicates.insert(predicate.name().str());
        erase_if(atoms, [&](const auto& entry) { return effect_predicates.contains(entry.predicate); });
        return atoms;
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
                const auto program = tyr::planning::AxiomEvaluatorProgram<tyr::LiftedTag>(task->get_formalism_task());
                return solve_program(program.get_datalog_program());
            }

            if (config == "applicable_action")
            {
                const auto action_program = tyr::planning::ApplicableActionProgram(task->get_formalism_task());
                auto action_workspace = tyr::datalog::ProgramWorkspace<LiftedTag>(action_program.get_datalog_program());
                if (task->has_axioms())
                {
                    const auto axiom_program = tyr::planning::AxiomEvaluatorProgram<tyr::LiftedTag>(task->get_formalism_task());
                    auto axiom_workspace = tyr::datalog::ProgramWorkspace<LiftedTag>(axiom_program.get_datalog_program());
                    solve_workspace(axiom_workspace);

                    const auto copy_facts = [&](const auto& axiom_facts)
                    {
                        for (ygg::uint_t set = 0; set < axiom_facts.predicate.size(); ++set)
                            for (const auto binding : tyr::datalog::rows(axiom_facts, set))
                                tyr::datalog::insert(action_workspace.facts.fact_sets,
                                                     urd::copy(binding, *action_workspace.workspace_repository, action_workspace.datalog_builder));
                    };
                    copy_facts(axiom_workspace.facts.fact_sets.get<tyr::formalism::FluentTag>());
                    copy_facts(axiom_workspace.facts.fact_sets.get<tyr::formalism::DerivedTag>());
                }

                solve_workspace(action_workspace);
                return without_effect_predicates(collect_atoms_by_predicate(action_workspace), action_program.get_conditional_effect_to_predicate_mapping());
            }

            if (config == "ground_task")
            {
                auto program = tyr::planning::GroundTaskProgram(task->get_formalism_task());
                auto workspace = tyr::datalog::ProgramWorkspace<LiftedTag>(program.get_datalog_program());
                auto context = tyr::datalog::ProgramExecutionContext(workspace);
                tyr::datalog::execute_model(context, *execution_context);
                return without_effect_predicates(collect_atoms_by_predicate(workspace), program.get_conditional_effect_to_predicate_mapping());
            }

            throw std::runtime_error("unknown bottom-up fixture configuration: " + std::string(config));
        };

        EXPECT_EQ(run_config(), expected);
    }
}

INSTANTIATE_TEST_SUITE_P(TyrDatalogLiftedBottomUpFixture,
                         BottomUpFixtureTest,
                         ::testing::ValuesIn(load_cases()),
                         [](const testing::TestParamInfo<BottomUpCase>& param_info) { return param_info.param.name; });

}
}
