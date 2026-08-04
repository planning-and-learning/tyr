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
#include "tyr/formalism/planning/parser.hpp"
#include "tyr/planning/planning.hpp"
#include "tyr/planning/search_space/parallel.hpp"
#include "tyr/planning/search_space/sequential.hpp"

#include <filesystem>
#include <gtest/gtest.h>
#include <limits>
#include <memory>
#include <optional>
#include <ranges>
#include <span>
#include <string_view>
#include <vector>
#include <yggdrasil/containers/segmented_vector.hpp>
#include <yggdrasil/execution/onetbb.hpp>

namespace p = tyr::planning;

namespace tyr::tests
{
namespace
{

template<TaskKind Kind>
struct SequentialSearchNode
{
    ygg::float_t g_value;
    ygg::Index<p::State<Kind>> parent_state;
};

template<TaskKind Kind>
struct ParallelSearchNode
{
    ygg::float_t g_value;
    p::WorkerStateIndex<Kind> parent_state;
};

template<typename SearchNode>
SearchNode& get_or_create_search_node(ygg::uint_t index, ygg::SegmentedVector<SearchNode>& search_nodes, const SearchNode& default_node)
{
    while (search_nodes.size() <= index)
        search_nodes.push_back(default_node);
    return search_nodes[index];
}

template<TaskKind Kind>
p::StateView<Kind> copy_state(const p::StateView<Kind>& source, p::StateRepository<Kind>& destination)
{
    auto builder = destination.get_state_builder();
    builder->assign_unextended_part(source.get_state_builder());
    return destination.register_state(std::move(builder));
}

template<TaskKind Kind>
auto find_successor(const p::LabeledNodeList<Kind>& successors,
                    std::string_view action,
                    std::optional<ygg::Index<p::State<Kind>>> excluded_state = std::nullopt)
{
    return std::ranges::find_if(successors,
                                [&](const auto& successor) {
                                    return successor.label.get_relation().get_name().str() == action
                                           && (!excluded_state || successor.node.get_state().get_index() != *excluded_state);
                                });
}

template<TaskKind Kind>
void expect_sequential_reconstruction(const p::TaskPtr<Kind>& task)
{
    auto execution_context = ygg::ExecutionContext::create(1);
    auto axiom_evaluator = p::AxiomEvaluatorFactory<Kind>().create(task, execution_context);
    auto repository = p::StateRepositoryFactory<Kind>().create(task, axiom_evaluator);
    auto generator = p::SuccessorGeneratorFactory<Kind>().create(task, execution_context, repository);

    const auto start = generator->get_initial_node();
    const auto successors = generator->get_labeled_successor_nodes(start);
    const auto selected = find_successor(successors, "pick");
    ASSERT_NE(selected, successors.end());

    const auto g_value = p::compute_successor_g_value(start.get_metric(), selected->node.get_metric(), ::tyr::CostMode::GENERAL);
    const auto final_node = p::Node<Kind>(selected->node.get_state(), g_value);
    const auto default_node = SequentialSearchNode<Kind> { std::numeric_limits<ygg::float_t>::infinity(), ygg::Index<p::State<Kind>>::max() };
    auto search_nodes = ygg::SegmentedVector<SequentialSearchNode<Kind>> {};
    get_or_create_search_node(ygg::uint_t(start.get_state().get_index()), search_nodes, default_node) =
        SequentialSearchNode<Kind> { start.get_metric(), ygg::Index<p::State<Kind>>::max() };
    auto& final_search_node = get_or_create_search_node(ygg::uint_t(final_node.get_state().get_index()), search_nodes, default_node);
    final_search_node = SequentialSearchNode<Kind> { g_value, start.get_state().get_index() };

    const auto num_states = repository->num_states();
    const auto plan = p::PlanReconstructionPolicy<p::SequentialSearch>::extract_total_ordered_plan(final_search_node,
                                                                                                   final_node,
                                                                                                   search_nodes,
                                                                                                   *generator,
                                                                                                   ::tyr::CostMode::GENERAL);

    ASSERT_EQ(plan.get_length(), 1);
    EXPECT_EQ(repository->num_states(), num_states);
    EXPECT_EQ(plan.get_start_node().get_state().get_state_repository(), repository);
    EXPECT_EQ(plan.get_start_node().get_state().get_index(), start.get_state().get_index());
    EXPECT_EQ(plan.get_labeled_succ_nodes().front().node.get_state().get_state_repository(), repository);
    EXPECT_EQ(plan.get_labeled_succ_nodes().front().node.get_state().get_index(), final_node.get_state().get_index());
}

template<TaskKind Kind>
void expect_parallel_reconstruction(const p::TaskPtr<Kind>& task)
{
    auto execution_context = ygg::ExecutionContext::create(1);
    auto axiom_evaluator = p::AxiomEvaluatorFactory<Kind>().create(task, execution_context);
    auto caller_repository = p::StateRepositoryFactory<Kind>().create(task, axiom_evaluator);
    auto caller_generator = p::SuccessorGeneratorFactory<Kind>().create(task, execution_context, caller_repository);
    auto plan = std::optional<p::Plan<Kind>> {};
    auto first_repository = std::weak_ptr<p::StateRepository<Kind>> {};
    auto second_repository = std::weak_ptr<p::StateRepository<Kind>> {};

    {
        auto first_generator = caller_generator->make_worker(ygg::ExecutionContext::create(1));
        auto second_generator = caller_generator->make_worker(ygg::ExecutionContext::create(1));
        const auto first_repository_owner = first_generator->get_state_repository();
        const auto second_repository_owner = second_generator->get_state_repository();
        first_repository = first_repository_owner;
        second_repository = second_repository_owner;

        const auto start = first_generator->get_initial_node();
        const auto start_successors = first_generator->get_labeled_successor_nodes(start);
        const auto selected_middle = find_successor(start_successors, "pick");
        ASSERT_NE(selected_middle, start_successors.end());
        const auto middle_g_value = p::compute_successor_g_value(start.get_metric(), selected_middle->node.get_metric(), ::tyr::CostMode::GENERAL);
        const auto middle_state = copy_state(selected_middle->node.get_state(), *second_repository_owner);
        const auto middle = p::Node<Kind>(middle_state, middle_g_value);

        const auto middle_successors = second_generator->get_labeled_successor_nodes(middle);
        const auto selected_final = find_successor(middle_successors, "move", std::optional { middle_state.get_index() });
        ASSERT_NE(selected_final, middle_successors.end());
        const auto final_g_value = p::compute_successor_g_value(middle_g_value, selected_final->node.get_metric(), ::tyr::CostMode::GENERAL);
        const auto final_state = copy_state(selected_final->node.get_state(), *first_repository_owner);

        const auto root = p::WorkerStateIndex<Kind> { ygg::Index<p::Worker>(0), start.get_state().get_index() };
        const auto middle_index = p::WorkerStateIndex<Kind> { ygg::Index<p::Worker>(1), middle_state.get_index() };
        const auto final = p::WorkerStateIndex<Kind> { ygg::Index<p::Worker>(0), final_state.get_index() };
        const auto no_parent = p::WorkerStateIndex<Kind> { ygg::Index<p::Worker>::max(), ygg::Index<p::State<Kind>>::max() };
        const auto default_node = ParallelSearchNode<Kind> { std::numeric_limits<ygg::float_t>::infinity(), no_parent };
        auto first_search_nodes = ygg::SegmentedVector<ParallelSearchNode<Kind>> {};
        auto second_search_nodes = ygg::SegmentedVector<ParallelSearchNode<Kind>> {};
        get_or_create_search_node(ygg::uint_t(root.state), first_search_nodes, default_node) = ParallelSearchNode<Kind> { start.get_metric(), no_parent };
        get_or_create_search_node(ygg::uint_t(final.state), first_search_nodes, default_node) = ParallelSearchNode<Kind> { final_g_value, middle_index };
        get_or_create_search_node(ygg::uint_t(middle_index.state), second_search_nodes, default_node) = ParallelSearchNode<Kind> { middle_g_value, root };

        const auto workers = std::vector<p::WorkerSearchSpaceView<Kind, ParallelSearchNode<Kind>>> {
            { *first_generator, first_search_nodes },
            { *second_generator, second_search_nodes },
        };
        EXPECT_EQ(caller_repository->num_states(), 0);
        plan = p::PlanReconstructionPolicy<p::ParallelSearch>::extract_total_ordered_plan(
            final,
            std::span<const p::WorkerSearchSpaceView<Kind, ParallelSearchNode<Kind>>>(workers),
            *caller_generator,
            ::tyr::CostMode::GENERAL);
        EXPECT_EQ(caller_repository->num_states(), 3);
    }

    ASSERT_TRUE(plan.has_value());
    ASSERT_EQ(plan->get_length(), 2);
    EXPECT_TRUE(first_repository.expired());
    EXPECT_TRUE(second_repository.expired());
    EXPECT_EQ(plan->get_start_node().get_state().get_state_repository(), caller_repository);
    EXPECT_EQ(plan->get_labeled_succ_nodes()[0].node.get_state().get_state_repository(), caller_repository);
    EXPECT_EQ(plan->get_labeled_succ_nodes()[1].node.get_state().get_state_repository(), caller_repository);
    EXPECT_EQ(plan->get_labeled_succ_nodes()[0].label.get_relation().get_name().str(), "pick");
    EXPECT_EQ(plan->get_labeled_succ_nodes()[1].label.get_relation().get_name().str(), "move");
}

template<TaskKind Kind>
void expect_plan_reconstruction(const p::TaskPtr<Kind>& task)
{
    expect_sequential_reconstruction(task);
    expect_parallel_reconstruction(task);
}

}

TEST(TyrPlanningPlanReconstructionTest, ReconstructsGroundAndLiftedWorkerTrajectories)
{
    const auto root = std::filesystem::path(BENCHMARKS_DIR);
    auto lifted_task =
        p::Task<LiftedTag>::create(make_test_parser(root / "classical/tests/gripper/domain.pddl").parse_task(root / "classical/tests/gripper/test-1.pddl"));
    auto grounding_context = ygg::ExecutionContext::create(1);
    auto ground_task = lifted_task->instantiate_ground_task(*grounding_context).task;
    ASSERT_NE(ground_task, nullptr);

    expect_plan_reconstruction(ground_task);
    expect_plan_reconstruction(lifted_task);
}

}
