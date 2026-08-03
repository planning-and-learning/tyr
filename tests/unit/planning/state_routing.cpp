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

#include <filesystem>
#include <future>
#include <gtest/gtest.h>
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
struct TestMetadata
{
    p::InternalStateID<Kind> parent;
    ygg::uint_t marker;
};

template<TaskKind Kind>
struct TransferResult
{
    ygg::Index<p::State<Kind>> state;
    ygg::float_t metric;
    ygg::hash_t hash;
};

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

    EXPECT_EQ(dist_hash.hash(initial.get_state().get_state_builder()), dist_hash.hash(worker_initial.get_state().get_state_builder()));
    EXPECT_EQ(dist_hash.owner(initial.get_state().get_state_builder(), 19), dist_hash.owner(worker_initial.get_state().get_state_builder(), 19));
    EXPECT_EQ(dist_hash.owner(initial.get_state().get_state_builder(), 1), 0);

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
    EXPECT_EQ(second_numeric->get(numeric_index), second_value);
    second_numeric->set(numeric_index, ygg::float_t { 2 });
    EXPECT_NE(dist_hash.hash(*first_numeric), dist_hash.hash(*second_numeric));

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
    auto router = p::SingleWorkerStateRouter<Kind, p::RandomDistHashTag, TestMetadata<Kind>>(17);
    router.send(std::move(local_state), local_result, action, TestMetadata<Kind> { p::InternalStateID<Kind> { 0, initial.get_state().get_index() }, 42 });
    const auto routed = router.receive(*generator);
    EXPECT_EQ(routed.labeled_node.label, action);
    EXPECT_EQ(routed.metadata.parent.state, initial.get_state().get_index());
    EXPECT_EQ(routed.metadata.parent.worker, 0);
    EXPECT_EQ(routed.metadata.marker, 42);

    const auto compatibility_node = generator->get_successor_node(initial, action);
    EXPECT_EQ(routed.labeled_node.node, compatibility_node);
    const auto source_states_after_local_route = repository->num_states();

    auto transfer_pool = p::StateTransferPool<Kind> {};
    auto transferred_state = transfer_pool.export_state(std::move(remote_state));
    auto result_promise = std::promise<TransferResult<Kind>> {};
    auto result_future = result_promise.get_future();
    auto receiver = std::jthread(
        [&, transferred_state = std::move(transferred_state)]() mutable
        {
            auto owner_state = transfer_pool.import_state(*worker_repository, std::move(transferred_state));
            const auto imported_hash = dist_hash.hash(*owner_state);
            const auto owner_node = worker->finalize_successor_state(std::move(owner_state), remote_result);
            result_promise.set_value(TransferResult<Kind> { owner_node.get_state().get_index(), owner_node.get_metric(), imported_hash });
        });
    receiver.join();

    const auto transferred_result = result_future.get();
    EXPECT_EQ(transferred_result.hash, remote_hash);
    EXPECT_EQ(transferred_result.metric, compatibility_node.get_metric());
    EXPECT_EQ(repository->num_states(), source_states_after_local_route);
    EXPECT_EQ(worker_repository->num_states(), 2);
    EXPECT_EQ(dist_hash.hash(worker_repository->get_registered_state(transferred_result.state).get_state_builder()), remote_hash);
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
