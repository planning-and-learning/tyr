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

#include <concepts>
#include <filesystem>
#include <gtest/gtest.h>
#include <memory>
#include <optional>
#include <thread>
#include <tuple>
#include <utility>
#include <yggdrasil/execution/onetbb.hpp>

namespace p = tyr::planning;
namespace f = tyr::formalism;
namespace fp = tyr::formalism::planning;

namespace tyr::tests
{
namespace
{

template<TaskKind Kind>
void expect_state_routing(const p::TaskPtr<Kind>& task)
{
    auto execution_context = ygg::ExecutionContext::create(1);
    auto axiom_evaluator = p::AxiomEvaluatorFactory<Kind>().create(task, execution_context);
    auto repository = p::StateRepositoryFactory<Kind>().create(task, axiom_evaluator);
    auto generator = p::SuccessorGeneratorFactory<Kind>().create(task, execution_context, repository);

    auto worker_context = ygg::ExecutionContext::create(1);
    auto worker = generator->make_worker(worker_context);
    const auto worker_repository = worker->get_state_repository();

    const auto initial = generator->get_initial_node();
    const auto worker_initial = worker->get_initial_node();
    const auto dist_hash = p::DistHash<Kind, p::RandomDistHashTag>(17);

    const auto expected_source_index = initial.get_state().get_index();
    const auto expected_source_hash = dist_hash.hash(initial.get_state().get_state_builder());
    auto source = std::optional<p::StateView<Kind>>(repository->get_registered_state(expected_source_index));
    auto transferred_source = source;
    source.reset();
    auto source_reader = std::jthread(
        [source = std::move(transferred_source), expected_source_index, expected_source_hash, &dist_hash]() mutable
        {
            EXPECT_EQ(source->get_index(), expected_source_index);
            EXPECT_EQ(dist_hash.hash(source->get_state_builder()), expected_source_hash);
            source.reset();
        });
    source_reader.join();

    auto thread_release_worker = generator->make_worker(ygg::ExecutionContext::create(1));
    auto thread_released_state = thread_release_worker->get_initial_node().get_state();
    const auto thread_release_owner = std::weak_ptr(thread_released_state.get_state_repository());
    thread_release_worker.reset();
    EXPECT_FALSE(thread_release_owner.expired());
    {
        auto releaser = std::jthread([state = std::move(thread_released_state)] { EXPECT_FALSE(state.get_index().is_max()); });
    }
    EXPECT_TRUE(thread_release_owner.expired());

    auto copy_assignment_worker = generator->make_worker(ygg::ExecutionContext::create(1));
    auto copy_assigned_source = copy_assignment_worker->get_initial_node().get_state();
    const auto copy_assignment_owner = std::weak_ptr(copy_assigned_source.get_state_repository());
    copy_assignment_worker.reset();
    EXPECT_FALSE(copy_assignment_owner.expired());
    copy_assigned_source = initial.get_state();
    EXPECT_TRUE(copy_assignment_owner.expired());
    EXPECT_EQ(copy_assigned_source.get_state_repository(), repository);

    auto move_assignment_worker = generator->make_worker(ygg::ExecutionContext::create(1));
    auto move_assigned_source = move_assignment_worker->get_initial_node().get_state();
    const auto move_assignment_owner = std::weak_ptr(move_assigned_source.get_state_repository());
    move_assignment_worker.reset();
    EXPECT_FALSE(move_assignment_owner.expired());
    move_assigned_source = repository->get_registered_state(expected_source_index);
    EXPECT_TRUE(move_assignment_owner.expired());
    EXPECT_EQ(move_assigned_source.get_state_repository(), repository);

    EXPECT_EQ(dist_hash.hash(initial.get_state().get_state_builder()), dist_hash.hash(worker_initial.get_state().get_state_builder()));
    EXPECT_EQ(dist_hash.owner(initial.get_state().get_state_builder(), 19), dist_hash.owner(worker_initial.get_state().get_state_builder(), 19));
    EXPECT_EQ(dist_hash.owner(initial.get_state().get_state_builder(), 1), ygg::Index<p::Worker>(0));

    auto derived_only_difference = repository->get_state_builder();
    derived_only_difference->assign_unextended_part(initial.get_state().get_state_builder());
    derived_only_difference->set(ygg::Index<fp::GroundAtom<f::DerivedTag>>(0));
    EXPECT_EQ(dist_hash.hash(*derived_only_difference), dist_hash.hash(initial.get_state().get_state_builder()));

    auto first_numeric = repository->get_state_builder();
    auto second_numeric = repository->get_state_builder();
    first_numeric->assign_unextended_part(initial.get_state().get_state_builder());
    second_numeric->assign_unextended_part(initial.get_state().get_state_builder());
    const auto numeric_index = ygg::Index<fp::GroundFunctionTerm<f::FluentTag>>(0);
    const auto first_value = ygg::float_t { 1 };
    const auto second_value = first_value + ygg::FloatTolerance<ygg::float_t>::abs_epsilon * ygg::float_t { 0.25 };
    first_numeric->set(numeric_index, first_value);
    second_numeric->set(numeric_index, second_value);
    EXPECT_EQ(dist_hash.hash(*first_numeric), dist_hash.hash(*second_numeric));
    EXPECT_EQ(first_numeric->get(numeric_index), first_value);
    EXPECT_EQ(second_numeric->get(numeric_index), ygg::FloatTolerance<ygg::float_t>::canonicalize(second_value));
    second_numeric->set(numeric_index, ygg::float_t { 2 });
    EXPECT_NE(dist_hash.hash(*first_numeric), dist_hash.hash(*second_numeric));

    if constexpr (std::same_as<Kind, ::tyr::LiftedTag>)
    {
        auto canonical_facts = repository->get_state_builder();
        auto trailing_zero_facts = repository->get_state_builder();
        const auto low_variable = ygg::Index<fp::FDRVariable<f::FluentTag>>(0);
        const auto high_variable = ygg::Index<fp::FDRVariable<f::FluentTag>>(1000);
        canonical_facts->set(ygg::Data<fp::FDRFact<f::FluentTag>>(low_variable, fp::FDRValue { 1 }));
        trailing_zero_facts->set(ygg::Data<fp::FDRFact<f::FluentTag>>(low_variable, fp::FDRValue { 1 }));
        trailing_zero_facts->set(ygg::Data<fp::FDRFact<f::FluentTag>>(high_variable, fp::FDRValue { 1 }));
        trailing_zero_facts->set(ygg::Data<fp::FDRFact<f::FluentTag>>(high_variable, fp::FDRValue::none()));
        EXPECT_EQ(trailing_zero_facts->get(low_variable), fp::FDRValue { 1 });
        EXPECT_EQ(dist_hash.hash(*canonical_facts), dist_hash.hash(*trailing_zero_facts));
    }

    const auto bindings = generator->get_applicable_action_bindings(initial);
    ASSERT_FALSE(bindings.empty());
    const auto action = bindings.front();
    const auto source_states_before_generation = repository->num_states();

    auto remote_state = repository->get_state_builder();
    const auto remote_result = generator->generate_successor_state(initial, action, *remote_state);
    const auto remote_hash = dist_hash.hash(*remote_state);
    EXPECT_EQ(repository->num_states(), source_states_before_generation);

    auto local_state = repository->get_state_builder();
    const auto local_result = generator->generate_successor_state(initial, action, *local_state);
    const auto local_node = generator->finalize_successor_state(std::move(local_state), local_result);

    const auto compatibility_node = generator->get_successor_node(initial, action);
    EXPECT_EQ(local_node, compatibility_node);
    const auto source_states_after_local_finalize = repository->num_states();

    auto owner_state = worker_repository->get_state_builder();
    using std::swap;
    swap(*owner_state, *remote_state);
    EXPECT_EQ(dist_hash.hash(*owner_state), remote_hash);
    const auto owner_node = worker->finalize_successor_state(std::move(owner_state), remote_result);
    EXPECT_EQ(owner_node.get_metric(), compatibility_node.get_metric());
    EXPECT_EQ(repository->num_states(), source_states_after_local_finalize);
    EXPECT_EQ(worker_repository->num_states(), 2);
    EXPECT_EQ(dist_hash.hash(worker_repository->get_registered_state(owner_node.get_state().get_index()).get_state_builder()), remote_hash);
}

}

TEST(TyrPlanningStateRoutingTest, RoutesGroundAndLiftedStatesWithoutProducerRegistration)
{
    const auto root = std::filesystem::path(BENCHMARKS_DIR);
    auto lifted_task = p::Task<LiftedTag>::create(
        make_test_parser(root / "classical/tests/philosophers/domain.pddl").parse_task(root / "classical/tests/philosophers/test-1.pddl"));
    auto grounding_context = ygg::ExecutionContext::create(1);
    auto ground_task = lifted_task->instantiate_ground_task(*grounding_context).task;
    ASSERT_NE(ground_task, nullptr);

    expect_state_routing(ground_task);
    expect_state_routing(lifted_task);
}

}
