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

#include "solver_fixtures.hpp"

#include <algorithm>
#include <set>
#include <string>
#include <gtest/gtest.h>
#include "tyr/formalism/datalog/interning.hpp"
#include <yggdrasil/execution/onetbb.hpp>

namespace tyr::tests
{
namespace
{

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

    // The fixtures predate the predicates of the conditional effects, which are internal to the action and ground task programs.
    const auto without_effect_predicates = [](AtomsByPredicate atoms, const auto& action_to_effect_rules, const auto& repository)
    {
        // An effect rule's last head is its effect predicate.
        auto effect_predicates = std::set<std::string> {};
        for (const auto& [_, rules] : action_to_effect_rules)
            for (const auto rule : rules)
                effect_predicates.insert(ygg::make_view(rule, repository).get_heads().back().get_predicate().get_name().str());
        std::erase_if(atoms, [&](const auto& entry) { return effect_predicates.contains(entry.predicate); });
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
                return without_effect_predicates(collect_atoms_by_predicate(action_workspace),
                                                 action_program.get_action_to_effect_rules_mapping(),
                                                 action_program.get_datalog_program().get_program_repository());
            }

            if (config == "ground_task")
            {
                auto program = tyr::planning::GroundTaskProgram(task->get_task());
                auto workspace = tyr::datalog::ProgramWorkspace<LiftedTag>(program.get_datalog_program());
                auto context = tyr::datalog::ProgramExecutionContext(workspace);
                tyr::datalog::execute_model(context, *execution_context);
                return without_effect_predicates(collect_atoms_by_predicate(workspace),
                                                 program.get_action_to_effect_rules_mapping(),
                                                 program.get_datalog_program().get_program_repository());
            }

            throw std::runtime_error("unknown bottom-up fixture configuration: " + std::string(config));
        };

        EXPECT_EQ(run_config(), expected);
    }
}

INSTANTIATE_TEST_SUITE_P(TyrDatalogLiftedBottomUpFixture,
                         BottomUpFixtureTest,
                         ::testing::ValuesIn(load_cases()),
                         [](const testing::TestParamInfo<BottomUpCase>& info) { return info.param.name; });

}
}
