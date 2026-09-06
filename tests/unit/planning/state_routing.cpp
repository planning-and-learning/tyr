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

#include "planning/metric.hpp"
#include "planning/parser.hpp"
#include "tyr/formalism/planning/parser.hpp"
#include "tyr/planning/planning.hpp"

#include <algorithm>
#include <concepts>
#include <filesystem>
#include <gtest/gtest.h>
#include <memory>
#include <optional>
#include <set>
#include <stdexcept>
#include <string_view>
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
    auto repository = p::StateRepositoryFactory<Kind>().create(task);
    auto generator = p::SuccessorGeneratorFactory<Kind>().create(task, execution_context);

    auto worker_context = ygg::ExecutionContext::create(1);
    auto worker_axiom_evaluator = axiom_evaluator->make_worker(worker_context);
    auto worker_repository = repository->make_worker();
    auto worker = generator->make_worker(worker_context);

    const auto initial = generator->get_initial_node(*repository, *axiom_evaluator);
    const auto worker_initial = worker->get_initial_node(*worker_repository, *worker_axiom_evaluator);
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

    auto thread_release_context = ygg::ExecutionContext::create(1);
    auto thread_release_evaluator = axiom_evaluator->make_worker(thread_release_context);
    auto thread_release_repository = repository->make_worker();
    auto thread_release_worker = generator->make_worker(thread_release_context);
    auto thread_released_state = thread_release_worker->get_initial_node(*thread_release_repository, *thread_release_evaluator).get_state();
    const auto thread_release_owner = std::weak_ptr(thread_released_state.get_state_repository());
    thread_release_worker.reset();
    thread_release_evaluator.reset();
    thread_release_repository.reset();
    EXPECT_FALSE(thread_release_owner.expired());
    {
        auto releaser = std::jthread([state = std::move(thread_released_state)] { EXPECT_FALSE(state.get_index().is_max()); });
    }
    EXPECT_TRUE(thread_release_owner.expired());

    auto copy_assignment_context = ygg::ExecutionContext::create(1);
    auto copy_assignment_evaluator = axiom_evaluator->make_worker(copy_assignment_context);
    auto copy_assignment_repository = repository->make_worker();
    auto copy_assignment_worker = generator->make_worker(copy_assignment_context);
    auto copy_assigned_source = copy_assignment_worker->get_initial_node(*copy_assignment_repository, *copy_assignment_evaluator).get_state();
    const auto copy_assignment_owner = std::weak_ptr(copy_assigned_source.get_state_repository());
    copy_assignment_worker.reset();
    copy_assignment_evaluator.reset();
    copy_assignment_repository.reset();
    EXPECT_FALSE(copy_assignment_owner.expired());
    copy_assigned_source = initial.get_state();
    EXPECT_TRUE(copy_assignment_owner.expired());
    EXPECT_EQ(copy_assigned_source.get_state_repository(), repository);

    auto move_assignment_context = ygg::ExecutionContext::create(1);
    auto move_assignment_evaluator = axiom_evaluator->make_worker(move_assignment_context);
    auto move_assignment_repository = repository->make_worker();
    auto move_assignment_worker = generator->make_worker(move_assignment_context);
    auto move_assigned_source = move_assignment_worker->get_initial_node(*move_assignment_repository, *move_assignment_evaluator).get_state();
    const auto move_assignment_owner = std::weak_ptr(move_assigned_source.get_state_repository());
    move_assignment_worker.reset();
    move_assignment_evaluator.reset();
    move_assignment_repository.reset();
    EXPECT_FALSE(move_assignment_owner.expired());
    move_assigned_source = repository->get_registered_state(expected_source_index);
    EXPECT_TRUE(move_assignment_owner.expired());
    EXPECT_EQ(move_assigned_source.get_state_repository(), repository);

    EXPECT_EQ(dist_hash.hash(initial.get_state().get_state_builder()), dist_hash.hash(worker_initial.get_state().get_state_builder()));
    EXPECT_EQ(dist_hash.owner(initial.get_state().get_state_builder(), 19), dist_hash.owner(worker_initial.get_state().get_state_builder(), 19));
    EXPECT_EQ(dist_hash.owner(initial.get_state().get_state_builder(), 1), ygg::Index<p::Worker>(0));

    auto derived_only_difference = repository->get_state_builder();
    derived_only_difference->assign_unextended_part(initial.get_state().get_state_builder());
    derived_only_difference->set(ygg::Index<fp::Atom<GroundTag, f::DerivedTag>>(0));
    EXPECT_EQ(dist_hash.hash(*derived_only_difference), dist_hash.hash(initial.get_state().get_state_builder()));

    auto first_numeric = repository->get_state_builder();
    auto second_numeric = repository->get_state_builder();
    first_numeric->assign_unextended_part(initial.get_state().get_state_builder());
    second_numeric->assign_unextended_part(initial.get_state().get_state_builder());
    const auto numeric_index = ygg::Index<fp::FunctionTerm<GroundTag, f::FluentTag>>(0);
    const auto first_value = ygg::float_t { 1 };
    const auto second_value = first_value + ygg::FloatTolerance<ygg::float_t>::abs_epsilon * ygg::float_t { 0.25 };
    first_numeric->set(numeric_index, first_value);
    second_numeric->set(numeric_index, second_value);
    EXPECT_EQ(dist_hash.hash(*first_numeric), dist_hash.hash(*second_numeric));
    EXPECT_EQ(first_numeric->get(numeric_index), first_value);
    EXPECT_EQ(second_numeric->get(numeric_index), ygg::FloatTolerance<ygg::float_t>::canonicalize(second_value));
    second_numeric->set(numeric_index, ygg::float_t { 2 });
    EXPECT_NE(dist_hash.hash(*first_numeric), dist_hash.hash(*second_numeric));

    if constexpr (std::same_as<Kind, LiftedTag>)
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

        auto owners = std::set<ygg::uint_t> {};
        for (ygg::uint_t mask = 0; mask < 256; ++mask)
        {
            auto state = repository->get_state_builder();
            state->set(ygg::Data<fp::FDRFact<f::FluentTag>>(ygg::Index<fp::FDRVariable<f::FluentTag>>(44), fp::FDRValue { 1 }));
            for (ygg::uint_t bit = 0; bit < 8; ++bit)
                if (mask & (ygg::uint_t { 1 } << bit))
                    state->set(ygg::Data<fp::FDRFact<f::FluentTag>>(ygg::Index<fp::FDRVariable<f::FluentTag>>(6 + bit), fp::FDRValue { 1 }));
            owners.insert(ygg::uint_t(dist_hash.owner(*state, 16)));
        }
        EXPECT_EQ(owners.size(), 16);
    }

    const auto bindings = generator->get_applicable_action_bindings(initial);
    ASSERT_FALSE(bindings.empty());
    const auto action = bindings.front();
    const auto source_states_before_generation = repository->num_states();

    auto remote_state = repository->get_state_builder();
    const auto remote_result = generator->generate_successor_state(initial, action, *remote_state);
    axiom_evaluator->compute_extended_state(*remote_state);
    const auto remote_metric = p::evaluate_successor_metric(*task, *remote_state, remote_result.auxiliary_value);
    const auto remote_hash = dist_hash.hash(*remote_state);
    EXPECT_EQ(repository->num_states(), source_states_before_generation);

    auto local_state = repository->get_state_builder();
    const auto local_result = generator->generate_successor_state(initial, action, *local_state);
    const auto local_node = generator->finalize_successor_state(*repository, *axiom_evaluator, std::move(local_state), local_result);

    const auto compatibility_node = generator->get_successor_node(initial, action, *repository, *axiom_evaluator);
    EXPECT_EQ(local_node, compatibility_node);
    const auto source_states_after_local_registration = repository->num_states();

    auto owner_state = worker_repository->get_state_builder();
    using std::swap;
    swap(*owner_state, *remote_state);
    EXPECT_EQ(dist_hash.hash(*owner_state), remote_hash);
    const auto owner_node = p::Node<Kind>(worker_repository->register_extended_state(std::move(owner_state)), remote_metric);
    EXPECT_EQ(owner_node.get_metric(), compatibility_node.get_metric());
    EXPECT_EQ(repository->num_states(), source_states_after_local_registration);
    EXPECT_EQ(worker_repository->num_states(), 2);
    EXPECT_EQ(dist_hash.hash(worker_repository->get_registered_state(owner_node.get_state().get_index()).get_state_builder()), remote_hash);
}

template<TaskKind Kind>
void expect_lmcut_state_routing(const p::TaskPtr<Kind>& task)
{
    auto execution_context = ygg::ExecutionContext::create(1);
    auto axiom_evaluator = p::AxiomEvaluatorFactory<Kind>().create(task, execution_context);
    auto repository = p::StateRepositoryFactory<Kind>().create(task);
    auto generator = p::SuccessorGeneratorFactory<Kind>().create(task, execution_context);
    const auto initial_node = generator->get_initial_node(*repository, *axiom_evaluator);
    const auto& initial = initial_node.get_state();

    auto heuristic = p::LMCutHeuristic<Kind>(task, ygg::ExecutionContext::create(1), CostMode::UNIT);
    const auto frontier = heuristic.compute_cut_frontier_atoms(initial.get_state_builder());
    EXPECT_EQ(frontier.size(), 2);
    const auto find_frontier_atom = [&](std::string_view name)
    { return std::ranges::find_if(frontier, [&](const auto atom) { return atom.get_predicate().get_name().str() == name; }); };
    const auto landmark_it = find_frontier_atom("landmark");
    ASSERT_NE(landmark_it, frontier.end());
    EXPECT_NE(find_frontier_atom("goal"), frontier.end());
    EXPECT_EQ(find_frontier_atom("side"), frontier.end());

    const auto bindings = generator->get_applicable_action_bindings(initial_node);
    const auto reach_landmark = std::ranges::find_if(bindings, [](const auto binding) { return binding.get_relation().get_name().str() == "reach-landmark"; });
    ASSERT_NE(reach_landmark, bindings.end());
    auto side = std::optional<fp::AtomView<GroundTag, f::FluentTag>> {};
    for (const auto conditional_effect : generator->ground_action(*reach_landmark).get_effects())
        for (const auto fact : conditional_effect.get_effect().template get_facts<f::PositiveTag>())
            if (const auto atom = fact.get_atom(); atom && atom->get_predicate().get_name().str() == "side")
                side = *atom;
    ASSERT_TRUE(side);

    auto first_hash = p::DistHash<Kind, p::LMCutDistHashTag>(17);
    auto second_hash = p::DistHash<Kind, p::LMCutDistHashTag>(17);
    first_hash.initialize(initial);
    second_hash.initialize(initial);
    EXPECT_THROW(first_hash.initialize(initial), std::logic_error);
    EXPECT_EQ(first_hash.hash(initial.get_state_builder()), second_hash.hash(initial.get_state_builder()));
    EXPECT_EQ(first_hash.owner(initial.get_state_builder(), 1), ygg::Index<p::Worker>(0));

    auto side_state = repository->get_state_builder();
    side_state->assign_unextended_part(initial.get_state_builder());
    side_state->set(task->get_fdr_context()->get_fact(*side).get_data());
    EXPECT_EQ(first_hash.hash(*side_state), first_hash.hash(initial.get_state_builder()));

    auto landmark_state = repository->get_state_builder();
    landmark_state->assign_unextended_part(initial.get_state_builder());
    landmark_state->set(task->get_fdr_context()->get_fact(*landmark_it).get_data());
    EXPECT_NE(first_hash.hash(*landmark_state), first_hash.hash(initial.get_state_builder()));
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

TEST(TyrPlanningStateRoutingTest, UsesInitialLMCutFrontierAsStableDistributionFeatures)
{
    const auto root = std::filesystem::path(ROOT_DIR) / "tests/fixtures/planning/state_routing";
    auto lifted_task = p::Task<LiftedTag>::create(make_test_parser(root / "domain.pddl").parse_task(root / "problem.pddl"));
    auto grounding_context = ygg::ExecutionContext::create(1);
    auto ground_task = lifted_task->instantiate_ground_task(*grounding_context).task;
    ASSERT_NE(ground_task, nullptr);

    expect_lmcut_state_routing(ground_task);
    expect_lmcut_state_routing(lifted_task);
}

}
