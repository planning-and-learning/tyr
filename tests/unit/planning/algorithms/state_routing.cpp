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
#include "planning/successors/successor_compositions.hpp"
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/planning/fdr_value.hpp"
#include "tyr/formalism/planning/parser.hpp"
#include "tyr/planning/axioms/axiom_evaluator.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/successors/ground/successor_generator.hpp"
#include "tyr/planning/tasks/ground/task.hpp"
#include "tyr/planning/heuristics/lmcut.hpp"
#include "tyr/planning/successors/lifted/successor_generator.hpp"
#include "tyr/planning/tasks/lifted/task.hpp"
#include "tyr/planning/nodes/node_view.hpp"
#include "tyr/planning/states/state_delta.hpp"
#include "tyr/planning/algorithms/state_routing/dist_hash.hpp"
#include "tyr/planning/algorithms/state_routing/dist_hash_concepts.hpp"
#include "tyr/planning/successors/successors.hpp"

#include <algorithm>
#include <concepts>
#include <filesystem>
#include <gtest/gtest.h>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <thread>
#include <tuple>
#include <urd/repr/values/unordered_flat_set.hpp>
#include <urd/store.hpp>
#include <utility>
#include <yggdrasil/execution/onetbb.hpp>

namespace tyr::tests
{
namespace
{

template<TaskKind Kind>
void expect_state_routing(const tyr::planning::TaskPtr<Kind>& task)
{
    auto execution_context = ygg::ExecutionContext::create(1);
    auto axiom_evaluator = std::make_shared<tyr::planning::AxiomEvaluator<Kind>>(task, execution_context);
    auto repository = tyr::planning::StateRepository<Kind>::create(task);
    auto generator = std::make_shared<tyr::planning::SuccessorGenerator<Kind>>(task, execution_context);

    auto worker_context = ygg::ExecutionContext::create(1);
    auto worker_axiom_evaluator = axiom_evaluator->make_worker(worker_context);
    auto worker_repository = repository->make_worker();
    auto worker = generator->make_worker(worker_context);

    const auto initial = tyr::tests::owned_initial_node(*repository, *axiom_evaluator);
    const auto worker_initial = tyr::tests::owned_initial_node(*worker_repository, *worker_axiom_evaluator);
    const auto dist_hash = tyr::planning::DistHash<Kind, tyr::planning::RandomDistHashTag>(17);

    const auto expected_source_index = tyr::tests::built_state(initial).pack().index();
    const auto expected_source_hash = dist_hash.hash(tyr::tests::built_state(initial));
    auto source = std::optional<tyr::planning::SharedStateBuilderView<Kind>>(::tyr::planning::share(*repository, ::tyr::planning::get_registered_state(*repository, expected_source_index)));
    auto transferred_source = source;
    source.reset();
    auto source_reader = std::jthread(
        [source = std::move(transferred_source), expected_source_index, expected_source_hash, &dist_hash]() mutable
        {
            EXPECT_EQ(source->pack().index(), expected_source_index);
            EXPECT_EQ(dist_hash.hash(*source), expected_source_hash);
            source.reset();
        });
    source_reader.join();

    auto thread_release_context = ygg::ExecutionContext::create(1);
    auto thread_release_evaluator = axiom_evaluator->make_worker(thread_release_context);
    auto thread_release_repository = repository->make_worker();
    auto thread_release_worker = generator->make_worker(thread_release_context);
    auto thread_released_state = ::tyr::planning::share(*thread_release_repository, ::tyr::planning::get_initial_state(*thread_release_repository, *thread_release_evaluator));
    {
        auto releaser = std::jthread([state = std::move(thread_released_state)] { EXPECT_FALSE(state.pack().index().is_none()); });
    }

    auto copy_assignment_context = ygg::ExecutionContext::create(1);
    auto copy_assignment_evaluator = axiom_evaluator->make_worker(copy_assignment_context);
    auto copy_assignment_repository = repository->make_worker();
    auto copy_assignment_worker = generator->make_worker(copy_assignment_context);
    auto copy_assigned_source = ::tyr::planning::share(*copy_assignment_repository, ::tyr::planning::get_initial_state(*copy_assignment_repository, *copy_assignment_evaluator));
    const auto registered_source = ::tyr::planning::share(*repository, ::tyr::planning::get_registered_state(*repository, expected_source_index));
    copy_assigned_source = registered_source;
    EXPECT_EQ(&copy_assigned_source.context(), repository.get());

    auto move_assignment_context = ygg::ExecutionContext::create(1);
    auto move_assignment_evaluator = axiom_evaluator->make_worker(move_assignment_context);
    auto move_assignment_repository = repository->make_worker();
    auto move_assignment_worker = generator->make_worker(move_assignment_context);
    auto move_assigned_source = ::tyr::planning::share(*move_assignment_repository, ::tyr::planning::get_initial_state(*move_assignment_repository, *move_assignment_evaluator));
    move_assigned_source = ::tyr::planning::share(*repository, ::tyr::planning::get_registered_state(*repository, expected_source_index));
    EXPECT_EQ(&move_assigned_source.context(), repository.get());

    EXPECT_EQ(dist_hash.hash(tyr::tests::built_state(initial)), dist_hash.hash(tyr::tests::built_state(worker_initial)));
    EXPECT_EQ(dist_hash.owner(tyr::tests::built_state(initial), 19), dist_hash.owner(tyr::tests::built_state(worker_initial), 19));
    EXPECT_EQ(dist_hash.owner(tyr::tests::built_state(initial), 1), urd::index<tyr::planning::Worker>(0));

    auto derived_only_difference = ::tyr::planning::get_state_builder(*repository);
    derived_only_difference->assign_unextended_part(tyr::tests::built_state(initial));
    derived_only_difference->set(urd::index<tyr::formalism::Atom<GroundTag, tyr::formalism::DerivedTag>>(0));
    EXPECT_EQ(dist_hash.hash(urd::resolve(*derived_only_difference, *repository)), dist_hash.hash(tyr::tests::built_state(initial)));

    auto first_numeric = ::tyr::planning::get_state_builder(*repository);
    auto second_numeric = ::tyr::planning::get_state_builder(*repository);
    first_numeric->assign_unextended_part(tyr::tests::built_state(initial));
    second_numeric->assign_unextended_part(tyr::tests::built_state(initial));
    const auto numeric_index = urd::index<tyr::formalism::FunctionTerm<GroundTag, tyr::formalism::FluentTag>>(0);
    const auto first_value = ygg::float_t { 1 };
    const auto second_value = first_value + ygg::FloatTolerance<ygg::float_t>::abs_epsilon * ygg::float_t { 0.25 };
    first_numeric->set(numeric_index, first_value);
    second_numeric->set(numeric_index, second_value);
    EXPECT_EQ(dist_hash.hash(urd::resolve(*first_numeric, *repository)), dist_hash.hash(urd::resolve(*second_numeric, *repository)));
    EXPECT_EQ(urd::resolve(*first_numeric->numeric_storage, *repository).get(numeric_index), first_value);
    EXPECT_EQ(urd::resolve(*second_numeric->numeric_storage, *repository).get(numeric_index), ygg::FloatTolerance<ygg::float_t>::canonicalize(second_value));
    second_numeric->set(numeric_index, ygg::float_t { 2 });
    EXPECT_NE(dist_hash.hash(urd::resolve(*first_numeric, *repository)), dist_hash.hash(urd::resolve(*second_numeric, *repository)));

    if constexpr (std::same_as<Kind, LiftedTag>)
    {
        auto canonical_facts = ::tyr::planning::get_state_builder(*repository);
        auto trailing_zero_facts = ::tyr::planning::get_state_builder(*repository);
        const auto low_variable = urd::index<tyr::formalism::FDRVariable<tyr::formalism::FluentTag>>(0);
        const auto high_variable = urd::index<tyr::formalism::FDRVariable<tyr::formalism::FluentTag>>(1000);
        canonical_facts->set(urd::data<tyr::formalism::FDRFact<tyr::formalism::FluentTag>>(low_variable, tyr::formalism::FDRValue { 1 }));
        trailing_zero_facts->set(urd::data<tyr::formalism::FDRFact<tyr::formalism::FluentTag>>(low_variable, tyr::formalism::FDRValue { 1 }));
        trailing_zero_facts->set(urd::data<tyr::formalism::FDRFact<tyr::formalism::FluentTag>>(high_variable, tyr::formalism::FDRValue { 1 }));
        trailing_zero_facts->set(urd::data<tyr::formalism::FDRFact<tyr::formalism::FluentTag>>(high_variable, tyr::formalism::FDRValue::none()));
        EXPECT_EQ(urd::resolve(*trailing_zero_facts->fact_storage, *repository).get(low_variable), tyr::formalism::FDRValue { 1 });
        EXPECT_EQ(dist_hash.hash(urd::resolve(*canonical_facts, *repository)), dist_hash.hash(urd::resolve(*trailing_zero_facts, *repository)));

        auto owners = urd::Store<urd::set<ygg::uint_t>> {};
        for (ygg::uint_t mask = 0; mask < 256; ++mask)
        {
            auto state = ::tyr::planning::get_state_builder(*repository);
            state->set(urd::data<tyr::formalism::FDRFact<tyr::formalism::FluentTag>>(urd::index<tyr::formalism::FDRVariable<tyr::formalism::FluentTag>>(44),
                                                                                     tyr::formalism::FDRValue { 1 }));
            for (ygg::uint_t bit = 0; bit < 8; ++bit)
                if (mask & (ygg::uint_t { 1 } << bit))
                    state->set(urd::data<tyr::formalism::FDRFact<tyr::formalism::FluentTag>>(
                        urd::index<tyr::formalism::FDRVariable<tyr::formalism::FluentTag>>(6 + bit),
                        tyr::formalism::FDRValue { 1 }));
            owners.insert(ygg::uint_t((dist_hash.owner(urd::resolve(*state, *repository), 16)).value));
        }
        EXPECT_EQ(owners.size(), 16);
    }

    const auto bindings = tyr::tests::get_applicable_action_bindings(*generator, initial, {});
    ASSERT_FALSE(bindings.empty());
    const auto action = bindings.front();
    const auto source_states_before_generation = ::tyr::planning::num_states(*repository);

    auto remote_state = ::tyr::planning::get_state_builder(*repository);
    auto delta = tyr::planning::StateDelta {};
    const auto remote_auxiliary_value = generator->generate_successor_state(initial, action, *remote_state, delta);
    axiom_evaluator->compute_extended_state(*remote_state, *repository);
    const auto remote_metric = tyr::planning::evaluate_successor_metric(*repository, *remote_state, remote_auxiliary_value);
    const auto remote_hash = dist_hash.hash(urd::resolve(*remote_state, *repository));
    EXPECT_EQ(::tyr::planning::num_states(*repository), source_states_before_generation);

    auto local_state = ::tyr::planning::get_state_builder(*repository);
    const auto local_auxiliary_value = generator->generate_successor_state(initial, action, *local_state, delta);
    const auto local_node = tyr::planning::OwnedNodeBuilderView<Kind>(
        tyr::planning::register_successor(*repository, *axiom_evaluator, std::move(local_state), local_auxiliary_value, initial, delta),
        repository.get());

    const auto compatibility_node = tyr::tests::get_successor_node(*generator, initial, action, *repository, *axiom_evaluator);
    EXPECT_EQ(local_node, compatibility_node);
    const auto source_states_after_local_registration = ::tyr::planning::num_states(*repository);

    auto owner_state = ::tyr::planning::get_state_builder(*worker_repository);
    using std::swap;
    swap(*owner_state, *remote_state);
    EXPECT_EQ(dist_hash.hash(urd::resolve(*owner_state, *worker_repository)), remote_hash);
    ::tyr::planning::register_extended_state(*worker_repository, *owner_state);
    const auto owner_node = tyr::planning::OwnedNodeBuilderView<Kind>(tyr::planning::NodeBuilder<Kind> { std::move(owner_state), remote_metric }, worker_repository.get());
    EXPECT_EQ(owner_node.get_metric(), compatibility_node.get_metric());
    EXPECT_EQ(::tyr::planning::num_states(*repository), source_states_after_local_registration);
    EXPECT_EQ(::tyr::planning::num_states(*worker_repository), 2);
    EXPECT_EQ(dist_hash.hash(::tyr::planning::share(*worker_repository, ::tyr::planning::get_registered_state(*worker_repository, tyr::tests::built_state(owner_node).pack().index()))), remote_hash);
}

template<TaskKind Kind>
void expect_lmcut_state_routing(const tyr::planning::TaskPtr<Kind>& task)
{
    auto execution_context = ygg::ExecutionContext::create(1);
    auto axiom_evaluator = std::make_shared<tyr::planning::AxiomEvaluator<Kind>>(task, execution_context);
    auto repository = tyr::planning::StateRepository<Kind>::create(task);
    auto generator = std::make_shared<tyr::planning::SuccessorGenerator<Kind>>(task, execution_context);
    const auto initial_node = tyr::tests::owned_initial_node(*repository, *axiom_evaluator);
    const auto initial = tyr::tests::built_state(initial_node);

    auto heuristic = tyr::planning::LMCutHeuristic<Kind>(task, ygg::ExecutionContext::create(1), CostMode::UNIT);
    const auto frontier = heuristic.compute_cut_frontier_atoms(initial);
    EXPECT_EQ(frontier.size(), 2);
    const auto find_frontier_atom = [&](std::string_view name)
    { return std::ranges::find_if(frontier, [&](const auto atom) { return atom.get_predicate().get_name().str() == name; }); };
    const auto landmark_it = find_frontier_atom("landmark");
    ASSERT_NE(landmark_it, frontier.end());
    EXPECT_NE(find_frontier_atom("goal"), frontier.end());
    EXPECT_EQ(find_frontier_atom("side"), frontier.end());

    const auto bindings = tyr::tests::get_applicable_action_bindings(*generator, initial_node, {});
    const auto reach_landmark = std::ranges::find_if(bindings, [](const auto binding) { return binding.get_relation().get_name().str() == "reach-landmark"; });
    ASSERT_NE(reach_landmark, bindings.end());
    auto side = std::optional<tyr::formalism::AtomView<GroundTag, tyr::formalism::FluentTag>> {};
    for (const auto conditional_effect : generator->ground_action(*reach_landmark).get_effects())
        for (const auto fact : conditional_effect.get_effect().template get_facts<tyr::formalism::PositiveTag>())
            if (const auto atom = fact.get_atom(); atom && atom->get_predicate().get_name().str() == "side")
                side = *atom;
    ASSERT_TRUE(side);

    auto first_hash = tyr::planning::DistHash<Kind, tyr::planning::LMCutDistHashTag>(17);
    auto second_hash = tyr::planning::DistHash<Kind, tyr::planning::LMCutDistHashTag>(17);
    first_hash.initialize(initial);
    second_hash.initialize(initial);
    EXPECT_THROW(first_hash.initialize(initial), std::logic_error);
    EXPECT_EQ(first_hash.hash(initial), second_hash.hash(initial));
    EXPECT_EQ(first_hash.owner(initial, 1), urd::index<tyr::planning::Worker>(0));

    auto side_state = ::tyr::planning::get_state_builder(*repository);
    side_state->assign_unextended_part(initial);
    side_state->set(task->get_fdr_context()->get_fact(side.value()));
    EXPECT_EQ(first_hash.hash(urd::resolve(*side_state, *repository)), first_hash.hash(initial));

    auto landmark_state = ::tyr::planning::get_state_builder(*repository);
    landmark_state->assign_unextended_part(initial);
    landmark_state->set(task->get_fdr_context()->get_fact(*landmark_it));
    EXPECT_NE(first_hash.hash(urd::resolve(*landmark_state, *repository)), first_hash.hash(initial));
}

}

TEST(TyrPlanningStateRoutingTest, RoutesGroundAndLiftedStatesWithoutProducerRegistration)
{
    const auto root = std::filesystem::path(BENCHMARKS_DIR);
    auto lifted_task = tyr::planning::Task<LiftedTag>::create(
        make_test_parser(root / "classical/tests/philosophers/domain.pddl").parse_task(root / "classical/tests/philosophers/test-1.pddl"));
    auto grounding_context = ygg::ExecutionContext::create(1);
    auto ground_task = lifted_task->instantiate_ground_task(*grounding_context).task;
    ASSERT_NE(ground_task, nullptr);

    expect_state_routing(ground_task);
    expect_state_routing(lifted_task);
}

TEST(TyrPlanningStateRoutingTest, UsesInitialLMCutFrontierAsStableDistributionFeatures)
{
    const auto root = std::filesystem::path(ROOT_DIR) / "tests/support/data/planning/state_routing";
    auto lifted_task = tyr::planning::Task<LiftedTag>::create(make_test_parser(root / "domain.pddl").parse_task(root / "problem.pddl"));
    auto grounding_context = ygg::ExecutionContext::create(1);
    auto ground_task = lifted_task->instantiate_ground_task(*grounding_context).task;
    ASSERT_NE(ground_task, nullptr);

    expect_lmcut_state_routing(ground_task);
    expect_lmcut_state_routing(lifted_task);
}

}
