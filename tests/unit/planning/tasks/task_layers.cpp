/*
 * Copyright (C) 2026 Dominik Drexler
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

#include "planning/parser.hpp"
#include "planning/successors/successor_compositions.hpp"
#include "tyr/formalism/planning/fdr_context.hpp"
#include "tyr/formalism/planning/parser.hpp"
#include "tyr/formalism/repository.hpp"
#include "tyr/planning/axioms/axiom_evaluator.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/states/state_repository.hpp"
#include "tyr/planning/successors/lifted/successor_generator.hpp"
#include "tyr/planning/successors/successors.hpp"
#include "tyr/planning/tasks/lifted/task.hpp"

#include <filesystem>
#include <gtest/gtest.h>
#include <memory>
#include <utility>
#include <yggdrasil/execution/onetbb.hpp>

namespace tyr::tests
{

/// Two planning Tasks of one formalism task: each grounds and registers into its own layers, over the frozen task's
/// repository and FDR context, which neither changes.
TEST(TyrPlanningTaskTest, TasksOfOneFormalismTaskWriteIntoTheirOwnLayers)
{
    const auto root = std::filesystem::path(BENCHMARKS_DIR);
    const auto formalism_task =
        make_test_parser(root / "classical/tests/philosophers/domain.pddl").parse_task(root / "classical/tests/philosophers/test-1.pddl");
    const auto frozen_variables = formalism_task.get_fdr_context()->get_variables().size();

    auto first = tyr::planning::Task<LiftedTag>::create(formalism_task);
    auto second = tyr::planning::Task<LiftedTag>::create(formalism_task);
    EXPECT_EQ(first->get_repository()->parent(), formalism_task.get_repository().get());
    EXPECT_EQ(second->get_repository()->parent(), formalism_task.get_repository().get());
    EXPECT_NE(first->get_repository(), second->get_repository());
    EXPECT_EQ(first->get_fdr_context()->get_variables().size(), frozen_variables);

    // The frozen task's facts, found through a layer.
    for (const auto atom : formalism_task.get_task().atoms<formalism::FluentTag>())
        EXPECT_EQ(std::as_const(*first->get_fdr_context()).get_fact(atom), formalism_task.get_fdr_context()->get_fact(atom));

    auto context = ygg::ExecutionContext::create(1);
    auto axiom_evaluator = std::make_shared<tyr::planning::AxiomEvaluator<LiftedTag>>(first, context);
    auto state_repository = tyr::planning::StateRepository<LiftedTag>::create(first);
    auto successor_generator = std::make_shared<tyr::planning::SuccessorGenerator<LiftedTag>>(first, context);
    const auto initial = tyr::tests::owned_initial_node(*state_repository, *axiom_evaluator);
    ASSERT_FALSE(tyr::tests::get_labeled_successor_nodes(*successor_generator, initial, {}, *state_repository, *axiom_evaluator).empty());

    EXPECT_GT(first->get_fdr_context()->get_variables().size(), frozen_variables);
    EXPECT_EQ(second->get_fdr_context()->get_variables().size(), frozen_variables);
    EXPECT_EQ(formalism_task.get_fdr_context()->get_variables().size(), frozen_variables);
}

}
