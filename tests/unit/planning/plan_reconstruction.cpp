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

#include <array>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <gtest/gtest.h>
#include <limits>
#include <memory>
#include <optional>
#include <ranges>
#include <semaphore>
#include <span>
#include <stdexcept>
#include <string_view>
#include <thread>
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

template<TaskKind Kind>
class CountingGBFSWorkerEventHandler final : public p::gbfs_lazy::WorkerEventHandler<Kind>
{
public:
    explicit CountingGBFSWorkerEventHandler(p::Statistics& statistics) : m_statistics(statistics) {}

    void on_expand_node(const p::Node<Kind>&) override { m_statistics.increment_num_expanded(); }
    void on_generate_transition(const p::Node<Kind>&, const p::LabeledNode<Kind>&, p::TransitionOutcome outcome) override
    {
        if (outcome == p::TransitionOutcome::OPENED || outcome == p::TransitionOutcome::GOAL)
            m_statistics.increment_num_accepted_successors();
        else if (outcome == p::TransitionOutcome::PRUNED)
            m_statistics.increment_num_pruned();
    }

private:
    p::Statistics& m_statistics;
};

template<TaskKind Kind>
class UnsupportedParallelPruningStrategy final : public p::PruningStrategy<Kind>
{
};

template<TaskKind Kind>
class UnsupportedParallelGoalStrategy final : public p::GoalStrategy<Kind>
{
public:
    bool is_static_goal_satisfied(const p::Task<Kind>&) override { return true; }
    bool is_dynamic_goal_satisfied(const p::StateView<Kind>&, const ygg::Builder<p::State<Kind>>&) override { return false; }
};

template<TaskKind Kind>
class CountingGBFSEventHandler final : public p::gbfs_lazy::EventHandler<Kind>
{
public:
    explicit CountingGBFSEventHandler(const p::StateRepository<Kind>* caller_repository) : m_caller_repository(caller_repository) {}

    void on_start_search(const p::Node<Kind>&, ygg::float_t) override {}
    void on_new_best_h_value(ygg::float_t) override {}

    void on_end_search(p::SearchStatus status, const p::Statistics& statistics) override
    {
        end_status = status;
        end_statistics = statistics;
    }

    void on_solved(const p::Plan<Kind>& plan) override
    {
        solved_plan_length = plan.get_length();
        plan_materialized = plan.get_start_node().get_state().get_state_repository().get() == m_caller_repository;
        for (const auto& successor : plan.get_labeled_succ_nodes())
            plan_materialized &= successor.node.get_state().get_state_repository().get() == m_caller_repository;
    }

    p::gbfs_lazy::WorkerEventHandlerPtr<Kind> make_worker(ygg::Index<p::Worker> index) override
    {
        ++num_workers_created;
        return std::make_unique<CountingGBFSWorkerEventHandler<Kind>>(worker_statistics.at(ygg::uint_t(index)));
    }

    p::Statistics totals() const
    {
        auto result = p::Statistics {};
        for (const auto& statistics : worker_statistics)
            result.add(statistics);
        return result;
    }

    std::array<p::Statistics, 2> worker_statistics;
    size_t num_workers_created = 0;
    bool plan_materialized = false;
    size_t solved_plan_length = 0;
    std::optional<p::SearchStatus> end_status;
    std::optional<p::Statistics> end_statistics;

private:
    const p::StateRepository<Kind>* m_caller_repository;
};

template<TaskKind Kind>
class CountingBrFSWorkerEventHandler final : public p::brfs::WorkerEventHandler<Kind>
{
public:
    explicit CountingBrFSWorkerEventHandler(p::Statistics& statistics) : m_statistics(statistics) {}

    void on_expand_node(const p::Node<Kind>&) override { m_statistics.increment_num_expanded(); }
    void on_generate_transition(const p::Node<Kind>&, const p::LabeledNode<Kind>&, p::TransitionOutcome outcome) override
    {
        if (outcome == p::TransitionOutcome::OPENED || outcome == p::TransitionOutcome::GOAL)
            m_statistics.increment_num_accepted_successors();
        else if (outcome == p::TransitionOutcome::PRUNED)
            m_statistics.increment_num_pruned();
    }

private:
    p::Statistics& m_statistics;
};

template<TaskKind Kind>
class CountingBrFSEventHandler final : public p::brfs::EventHandler<Kind>
{
public:
    void on_start_search(const p::Node<Kind>&) override { ++num_starts; }
    void on_finish_layer(ygg::uint_t layer, const p::Statistics& statistics) override
    {
        finished_layers.push_back(layer);
        layer_statistics.push_back(statistics);
    }
    void on_end_search(p::SearchStatus status, const p::Statistics& statistics) override
    {
        end_status = status;
        end_statistics = statistics;
    }
    void on_solved(const p::Plan<Kind>& plan) override { solved_plan_length = plan.get_length(); }

    p::brfs::WorkerEventHandlerPtr<Kind> make_worker(ygg::Index<p::Worker> index) override
    {
        ++num_workers_created;
        return std::make_unique<CountingBrFSWorkerEventHandler<Kind>>(worker_statistics.at(ygg::uint_t(index)));
    }

    std::array<p::Statistics, 2> worker_statistics;
    std::vector<ygg::uint_t> finished_layers;
    std::vector<p::Statistics> layer_statistics;
    size_t num_starts = 0;
    size_t num_workers_created = 0;
    size_t solved_plan_length = 0;
    std::optional<p::SearchStatus> end_status;
    std::optional<p::Statistics> end_statistics;
};

template<TaskKind Kind>
class OwnerAccessGuard
{
public:
    static constexpr size_t num_workers = 4;

    OwnerAccessGuard(p::StateRepositoryMode mode, uint64_t seed) : m_mode(mode), m_hash(seed) {}

    void access(ygg::Index<p::Worker> worker, const p::StateView<Kind>& state)
    {
        auto& lane = m_lanes.at(ygg::uint_t(worker));
        if (lane.active.test_and_set(std::memory_order_acquire))
            lane.reentered.store(true, std::memory_order_relaxed);

        if (owner(state) != worker)
            lane.wrong_owner.store(true, std::memory_order_relaxed);
        observe_repository(lane, state);
        lane.num_accesses.fetch_add(1, std::memory_order_relaxed);
        std::this_thread::yield();
        lane.active.clear(std::memory_order_release);
    }

    void access_transition(ygg::Index<p::Worker> worker, const p::StateView<Kind>& source, const p::StateView<Kind>& target)
    {
        const auto source_owner = owner(source);
        observe_repository(m_lanes.at(ygg::uint_t(source_owner)), source);
        if (source_owner != worker)
        {
            m_num_remote_transitions.fetch_add(1, std::memory_order_relaxed);
            if (m_mode == p::StateRepositoryMode::SHARED && source.get_state_repository() != target.get_state_repository())
                m_wrong_shared_remote_repository.store(true, std::memory_order_relaxed);
        }
        access(worker, target);
    }

    void count_duplicate() noexcept { m_num_duplicates.fetch_add(1, std::memory_order_relaxed); }

    void expect_valid() const
    {
        auto num_accesses = uint64_t { 0 };
        for (const auto& lane : m_lanes)
        {
            EXPECT_FALSE(lane.reentered.load(std::memory_order_relaxed));
            EXPECT_FALSE(lane.wrong_owner.load(std::memory_order_relaxed));
            EXPECT_FALSE(lane.wrong_repository.load(std::memory_order_relaxed));
            num_accesses += lane.num_accesses.load(std::memory_order_relaxed);
        }
        EXPECT_GT(num_accesses, 0);
        EXPECT_GT(m_num_remote_transitions.load(std::memory_order_relaxed), 0);
        EXPECT_GT(m_num_duplicates.load(std::memory_order_relaxed), 0);
        EXPECT_FALSE(m_wrong_shared_remote_repository.load(std::memory_order_relaxed));
    }

private:
    struct Lane
    {
        std::atomic_flag active = ATOMIC_FLAG_INIT;
        std::atomic<bool> reentered { false };
        std::atomic<bool> wrong_owner { false };
        std::atomic<bool> wrong_repository { false };
        std::atomic<const p::StateRepository<Kind>*> repository { nullptr };
        std::atomic<uint64_t> num_accesses { 0 };
    };

    ygg::Index<p::Worker> owner(const p::StateView<Kind>& state) const
    {
        if (m_mode == p::StateRepositoryMode::SHARED)
            return ygg::Index<p::Worker>(static_cast<ygg::uint_t>(ygg::uint_t(state.get_index()) % num_workers));
        return m_hash.owner(state.get_state_builder(), num_workers);
    }

    void observe_repository(Lane& lane, const p::StateView<Kind>& state) const
    {
        if (m_mode == p::StateRepositoryMode::SHARED)
            return;

        auto* expected_repository = static_cast<const p::StateRepository<Kind>*>(nullptr);
        const auto* repository = state.get_state_repository().get();
        if (!lane.repository.compare_exchange_strong(expected_repository, repository, std::memory_order_relaxed) && expected_repository != repository)
            lane.wrong_repository.store(true, std::memory_order_relaxed);
    }

    p::StateRepositoryMode m_mode;
    p::DistHash<Kind, p::RandomDistHashTag> m_hash;
    std::array<Lane, num_workers> m_lanes;
    std::atomic<uint64_t> m_num_remote_transitions { 0 };
    std::atomic<uint64_t> m_num_duplicates { 0 };
    std::atomic<bool> m_wrong_shared_remote_repository { false };
};

template<TaskKind Kind>
class GuardedBrFSWorkerEventHandler final : public p::brfs::WorkerEventHandler<Kind>
{
public:
    GuardedBrFSWorkerEventHandler(ygg::Index<p::Worker> worker, std::shared_ptr<OwnerAccessGuard<Kind>> guard) : m_worker(worker), m_guard(std::move(guard)) {}

    void on_expand_node(const p::Node<Kind>& node) override { m_guard->access(m_worker, node.get_state()); }

    void on_generate_transition(const p::Node<Kind>& source, const p::LabeledNode<Kind>& target, p::TransitionOutcome outcome) override
    {
        m_guard->access_transition(m_worker, source.get_state(), target.node.get_state());
        if (outcome == p::TransitionOutcome::DUPLICATE)
            m_guard->count_duplicate();
    }

private:
    ygg::Index<p::Worker> m_worker;
    std::shared_ptr<OwnerAccessGuard<Kind>> m_guard;
};

template<TaskKind Kind>
class GuardedBrFSEventHandler final : public p::brfs::EventHandler<Kind>
{
public:
    explicit GuardedBrFSEventHandler(std::shared_ptr<OwnerAccessGuard<Kind>> guard) : m_guard(std::move(guard)) {}

    void on_start_search(const p::Node<Kind>&) override {}
    void on_finish_layer(ygg::uint_t, const p::Statistics&) override {}
    void on_end_search(p::SearchStatus, const p::Statistics&) override {}
    void on_solved(const p::Plan<Kind>&) override {}

    p::brfs::WorkerEventHandlerPtr<Kind> make_worker(ygg::Index<p::Worker> worker) override
    {
        return std::make_unique<GuardedBrFSWorkerEventHandler<Kind>>(worker, m_guard);
    }

private:
    std::shared_ptr<OwnerAccessGuard<Kind>> m_guard;
};

template<TaskKind Kind>
class GuardedPruningStrategy final : public p::PruningStrategy<Kind>
{
public:
    explicit GuardedPruningStrategy(std::shared_ptr<OwnerAccessGuard<Kind>> guard) : m_guard(std::move(guard)) {}

    GuardedPruningStrategy(ygg::Index<p::Worker> worker, std::shared_ptr<OwnerAccessGuard<Kind>> guard) : m_worker(worker), m_guard(std::move(guard)) {}

    p::PruningStrategyPtr<Kind> make_worker(ygg::Index<p::Worker> worker) const override { return std::make_shared<GuardedPruningStrategy>(worker, m_guard); }

    bool should_prune_state(const p::StateView<Kind>& state) override
    {
        m_guard->access(m_worker, state);
        return false;
    }

    bool should_prune_successor_state(const p::StateView<Kind>& source, const p::StateView<Kind>& state, bool) override
    {
        m_guard->access_transition(m_worker, source, state);
        return false;
    }

private:
    ygg::Index<p::Worker> m_worker { 0 };
    std::shared_ptr<OwnerAccessGuard<Kind>> m_guard;
};

struct AStarGoalGate
{
    std::binary_semaphore cheap_path { 0 };
    std::atomic<bool> expensive_goal_seen { false };
    std::atomic<uint64_t> num_expanded_goals { 0 };
};

template<TaskKind Kind>
class GatedAStarWorkerEventHandler final : public p::astar_eager::WorkerEventHandler<Kind>
{
public:
    explicit GatedAStarWorkerEventHandler(AStarGoalGate& gate) : m_gate(gate) {}

    void on_expand_node(const p::Node<Kind>& node) override
    {
        if (node.get_metric() == 1 && !m_gate.cheap_path.try_acquire_for(std::chrono::seconds(5)))
            throw std::runtime_error("Timed out waiting for the expensive goal.");
    }

    void on_generate_transition(const p::Node<Kind>&, const p::LabeledNode<Kind>& successor, p::TransitionOutcome outcome) override
    {
        if (outcome == p::TransitionOutcome::GOAL && successor.node.get_metric() == 100)
        {
            m_gate.expensive_goal_seen.store(true, std::memory_order_relaxed);
            m_gate.cheap_path.release();
        }
    }

    void on_expand_goal_node(const p::Node<Kind>&) override { m_gate.num_expanded_goals.fetch_add(1, std::memory_order_relaxed); }

private:
    AStarGoalGate& m_gate;
};

template<TaskKind Kind>
class GatedAStarEventHandler final : public p::astar_eager::EventHandler<Kind>
{
public:
    void on_start_search(const p::Node<Kind>&, ygg::float_t) override {}
    void on_end_search(p::SearchStatus, const p::Statistics&) override {}
    void on_solved(const p::Plan<Kind>&) override {}

    p::astar_eager::WorkerEventHandlerPtr<Kind> make_worker(ygg::Index<p::Worker>) override
    {
        return std::make_unique<GatedAStarWorkerEventHandler<Kind>>(gate);
    }

    AStarGoalGate gate;
};

struct AStarLayerGate
{
    std::binary_semaphore lower_started { 0 };
    std::binary_semaphore higher_started { 0 };
    std::atomic<bool> higher_expanded { false };
};

template<TaskKind Kind>
class LayeredAStarWorkerEventHandler final : public p::astar_eager::WorkerEventHandler<Kind>
{
public:
    explicit LayeredAStarWorkerEventHandler(AStarLayerGate& gate) : m_gate(gate) {}

    void on_expand_node(const p::Node<Kind>& node) override
    {
        if (node.get_metric() == 1)
        {
            m_gate.lower_started.release();
            static_cast<void>(m_gate.higher_started.try_acquire_for(std::chrono::milliseconds(100)));
        }
        else if (node.get_metric() == 100)
        {
            if (!m_gate.lower_started.try_acquire_for(std::chrono::seconds(5)))
                throw std::runtime_error("Timed out waiting for the lower f-layer.");
            m_gate.higher_expanded.store(true, std::memory_order_relaxed);
            m_gate.higher_started.release();
        }
    }

private:
    AStarLayerGate& m_gate;
};

template<TaskKind Kind>
class LayeredAStarEventHandler final : public p::astar_eager::EventHandler<Kind>
{
public:
    void on_start_search(const p::Node<Kind>&, ygg::float_t) override {}
    void on_end_search(p::SearchStatus, const p::Statistics&) override {}
    void on_solved(const p::Plan<Kind>&) override {}

    p::astar_eager::WorkerEventHandlerPtr<Kind> make_worker(ygg::Index<p::Worker>) override
    {
        return std::make_unique<LayeredAStarWorkerEventHandler<Kind>>(gate);
    }

    AStarLayerGate gate;
};

template<TaskKind Kind>
class ThrowingAStarWorkerEventHandler final : public p::astar_eager::WorkerEventHandler<Kind>
{
public:
    void on_expand_node(const p::Node<Kind>&) override { throw std::runtime_error("Worker event failure."); }
};

template<TaskKind Kind>
class ThrowingAStarEventHandler final : public p::astar_eager::EventHandler<Kind>
{
public:
    void on_start_search(const p::Node<Kind>&, ygg::float_t) override {}
    void on_end_search(p::SearchStatus, const p::Statistics&) override {}
    void on_solved(const p::Plan<Kind>&) override {}

    p::astar_eager::WorkerEventHandlerPtr<Kind> make_worker(ygg::Index<p::Worker>) override
    {
        return std::make_unique<ThrowingAStarWorkerEventHandler<Kind>>();
    }
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

template<TaskKind Kind>
void expect_parallel_lazy_gbfs(const p::TaskPtr<Kind>& task, p::StateRepositoryMode state_repository_mode)
{
    auto execution_context = ygg::ExecutionContext::create(1);
    auto axiom_evaluator = p::AxiomEvaluatorFactory<Kind>().create(task, execution_context);
    auto repository = p::StateRepositoryFactory<Kind>().create(task, axiom_evaluator);
    auto generator = p::SuccessorGeneratorFactory<Kind>().create(task, execution_context, repository);
    auto heuristic = p::BlindHeuristic<Kind>::create();

    auto options = p::gbfs_lazy::Options<Kind> {};
    options.num_search_workers = 2;
    options.state_repository_mode = state_repository_mode;
    const auto result = p::gbfs_lazy::find_solution(*task, *generator, *heuristic, options);

    ASSERT_EQ(result.status, p::SearchStatus::SOLVED);
    ASSERT_TRUE(result.plan);
    ASSERT_TRUE(result.goal_node);
    ASSERT_FALSE(result.plan->empty());

    const auto expect_caller_state = [&](const auto& node)
    {
        const auto& state = node.get_state();
        EXPECT_EQ(state.get_state_repository(), repository);
        EXPECT_EQ(state.get_state_builder().get_index(), state.get_index());
        EXPECT_EQ(repository->get_registered_state(state.get_index()), state);
    };
    expect_caller_state(result.plan->get_start_node());
    for (const auto& successor : result.plan->get_labeled_succ_nodes())
        expect_caller_state(successor.node);
    expect_caller_state(*result.goal_node);

    auto goal = p::ConjunctiveGoalStrategy<Kind>(*task);
    EXPECT_TRUE(goal.is_dynamic_goal_satisfied(result.plan->get_start_node().get_state(), result.goal_node->get_state()));

    options.num_search_workers = 0;
    EXPECT_THROW(p::gbfs_lazy::find_solution(*task, *generator, *heuristic, options), std::invalid_argument);

    if constexpr (std::numeric_limits<size_t>::max() > std::numeric_limits<ygg::uint_t>::max())
    {
        options.num_search_workers = static_cast<size_t>(std::numeric_limits<ygg::uint_t>::max()) + 1;
        EXPECT_THROW(p::gbfs_lazy::find_solution(*task, *generator, *heuristic, options), std::invalid_argument);
    }

    options.num_search_workers = 2;
    options.pruning_strategy = p::PruningStrategy<Kind>::create();
    EXPECT_EQ(p::gbfs_lazy::find_solution(*task, *generator, *heuristic, options).status, p::SearchStatus::SOLVED);

    options.pruning_strategy = nullptr;
    options.goal_strategy = p::ConjunctiveGoalStrategy<Kind>::create(*task);
    EXPECT_EQ(p::gbfs_lazy::find_solution(*task, *generator, *heuristic, options).status, p::SearchStatus::SOLVED);

    options.goal_strategy = nullptr;
    options.pruning_strategy = std::make_shared<UnsupportedParallelPruningStrategy<Kind>>();
    EXPECT_THROW(p::gbfs_lazy::find_solution(*task, *generator, *heuristic, options), std::invalid_argument);

    options.pruning_strategy = nullptr;
    options.goal_strategy = std::make_shared<UnsupportedParallelGoalStrategy<Kind>>();
    EXPECT_THROW(p::gbfs_lazy::find_solution(*task, *generator, *heuristic, options), std::invalid_argument);

    options.goal_strategy = nullptr;
    options.max_num_states = 0;
    EXPECT_EQ(p::gbfs_lazy::find_solution(*task, *generator, *heuristic, options).status, p::SearchStatus::OUT_OF_STATES);

    options.max_num_states = 1;
    EXPECT_EQ(p::gbfs_lazy::find_solution(*task, *generator, *heuristic, options).status, p::SearchStatus::OUT_OF_STATES);

    options.max_num_states = std::numeric_limits<ygg::uint_t>::max();
    options.max_time = std::chrono::steady_clock::duration::zero();
    EXPECT_EQ(p::gbfs_lazy::find_solution(*task, *generator, *heuristic, options).status, p::SearchStatus::OUT_OF_TIME);
}

template<TaskKind Kind>
void expect_parallel_brfs(const p::TaskPtr<Kind>& task, p::StateRepositoryMode state_repository_mode)
{
    auto execution_context = ygg::ExecutionContext::create(1);
    auto axiom_evaluator = p::AxiomEvaluatorFactory<Kind>().create(task, execution_context);
    auto repository = p::StateRepositoryFactory<Kind>().create(task, axiom_evaluator);
    auto generator = p::SuccessorGeneratorFactory<Kind>().create(task, execution_context, repository);

    auto options = p::brfs::Options<Kind> {};
    options.num_search_workers = 2;
    options.state_repository_mode = state_repository_mode;
    const auto result = p::brfs::find_solution(*task, *generator, options);

    ASSERT_EQ(result.status, p::SearchStatus::SOLVED);
    ASSERT_TRUE(result.plan);
    ASSERT_TRUE(result.goal_node);
    ASSERT_FALSE(result.plan->empty());
    EXPECT_EQ(result.plan->get_cost(), result.plan->get_length());
    EXPECT_EQ(result.plan->get_start_node().get_state().get_state_repository(), repository);
    for (const auto& successor : result.plan->get_labeled_succ_nodes())
        EXPECT_EQ(successor.node.get_state().get_state_repository(), repository);
    EXPECT_EQ(result.goal_node->get_state().get_state_repository(), repository);

    ASSERT_EQ(result.worker_statistics.size(), 2);
    auto num_registered_states = uint64_t { 0 };
    auto state_storage_memory_usage = size_t { 0 };
    for (const auto& statistics : result.worker_statistics)
    {
        num_registered_states += statistics.get_num_registered_states();
        state_storage_memory_usage += statistics.get_state_storage_memory_usage();
    }
    EXPECT_EQ(num_registered_states, result.statistics.get_num_registered_states());
    EXPECT_EQ(state_storage_memory_usage, result.statistics.get_state_storage_memory_usage());

    options.num_search_workers = 0;
    EXPECT_THROW(p::brfs::find_solution(*task, *generator, options), std::invalid_argument);

    options.num_search_workers = 2;
    options.max_num_states = 0;
    EXPECT_EQ(p::brfs::find_solution(*task, *generator, options).status, p::SearchStatus::OUT_OF_STATES);

    options.max_num_states = 1;
    EXPECT_EQ(p::brfs::find_solution(*task, *generator, options).status, p::SearchStatus::OUT_OF_STATES);

    options.max_num_states = std::numeric_limits<ygg::uint_t>::max();
    options.max_time = std::chrono::steady_clock::duration::zero();
    EXPECT_EQ(p::brfs::find_solution(*task, *generator, options).status, p::SearchStatus::OUT_OF_TIME);
}

template<TaskKind Kind>
void expect_brfs_events(const p::TaskPtr<Kind>& task, p::StateRepositoryMode state_repository_mode, size_t num_workers)
{
    auto execution_context = ygg::ExecutionContext::create(1);
    auto axiom_evaluator = p::AxiomEvaluatorFactory<Kind>().create(task, execution_context);
    auto repository = p::StateRepositoryFactory<Kind>().create(task, axiom_evaluator);
    auto generator = p::SuccessorGeneratorFactory<Kind>().create(task, execution_context, repository);
    auto event_handler = std::make_shared<CountingBrFSEventHandler<Kind>>();

    auto options = p::brfs::Options<Kind> {};
    options.event_handler = event_handler;
    options.num_search_workers = num_workers;
    options.state_repository_mode = state_repository_mode;
    const auto result = p::brfs::find_solution(*task, *generator, options);

    ASSERT_EQ(result.status, p::SearchStatus::SOLVED);
    ASSERT_TRUE(result.plan);
    EXPECT_EQ(event_handler->num_starts, 1);
    EXPECT_EQ(event_handler->num_workers_created, num_workers);
    EXPECT_EQ(event_handler->solved_plan_length, result.plan->get_length());
    ASSERT_TRUE(event_handler->end_status);
    EXPECT_EQ(*event_handler->end_status, result.status);
    ASSERT_TRUE(event_handler->end_statistics);
    EXPECT_EQ(event_handler->end_statistics->get_num_accepted_successors(), result.statistics.get_num_accepted_successors());
    EXPECT_EQ(event_handler->end_statistics->get_num_expanded(), result.statistics.get_num_expanded());

    auto worker_totals = p::Statistics {};
    for (const auto& statistics : event_handler->worker_statistics)
        worker_totals.add(statistics);
    EXPECT_EQ(worker_totals.get_num_accepted_successors(), result.statistics.get_num_accepted_successors());
    EXPECT_EQ(worker_totals.get_num_expanded(), result.statistics.get_num_expanded());
    EXPECT_EQ(worker_totals.get_num_pruned(), result.statistics.get_num_pruned());

    ASSERT_EQ(event_handler->finished_layers.size(), event_handler->layer_statistics.size());
    ASSERT_FALSE(event_handler->finished_layers.empty());
    EXPECT_EQ(event_handler->finished_layers.front(), 0);
    EXPECT_TRUE(std::ranges::is_sorted(event_handler->finished_layers));
    for (size_t i = 1; i < event_handler->layer_statistics.size(); ++i)
    {
        EXPECT_EQ(event_handler->finished_layers[i], event_handler->finished_layers[i - 1] + 1);
        EXPECT_LE(event_handler->layer_statistics[i - 1].get_num_accepted_successors(), event_handler->layer_statistics[i].get_num_accepted_successors());
        EXPECT_LE(event_handler->layer_statistics[i - 1].get_num_expanded(), event_handler->layer_statistics[i].get_num_expanded());
    }
}

template<TaskKind Kind>
void expect_aggregated_worker_statistics(const p::SearchResult<Kind>& result, size_t num_workers)
{
    ASSERT_EQ(result.worker_statistics.size(), num_workers);
    auto totals = p::Statistics {};
    for (const auto& worker : result.worker_statistics)
    {
        totals.add(worker);
        EXPECT_EQ(worker.get_search_time(), result.statistics.get_search_time());
    }
    EXPECT_EQ(totals.get_num_accepted_successors(), result.statistics.get_num_accepted_successors());
    EXPECT_EQ(totals.get_num_expanded(), result.statistics.get_num_expanded());
    EXPECT_EQ(totals.get_num_deadends(), result.statistics.get_num_deadends());
    EXPECT_EQ(totals.get_num_pruned(), result.statistics.get_num_pruned());
    EXPECT_EQ(totals.get_num_generated_successors(), result.statistics.get_num_generated_successors());
    EXPECT_EQ(totals.get_num_transferred_successors(), result.statistics.get_num_transferred_successors());
    EXPECT_EQ(totals.get_idle_time(), result.statistics.get_idle_time());
}

template<TaskKind Kind>
void expect_parallel_iw(const p::TaskPtr<Kind>& task, p::StateRepositoryMode state_repository_mode)
{
    auto execution_context = ygg::ExecutionContext::create(1);
    auto axiom_evaluator = p::AxiomEvaluatorFactory<Kind>().create(task, execution_context);
    auto repository = p::StateRepositoryFactory<Kind>().create(task, axiom_evaluator);
    auto generator = p::SuccessorGeneratorFactory<Kind>().create(task, execution_context, repository);
    auto brfs_solver = p::brfs::Solver<Kind> { task, generator };
    brfs_solver.options.num_search_workers = 2;
    brfs_solver.options.state_repository_mode = state_repository_mode;

    const auto iw_result = p::iw::find_solution(brfs_solver, 2);
    ASSERT_EQ(iw_result.status, p::SearchStatus::SOLVED);
    ASSERT_TRUE(iw_result.plan);
    EXPECT_EQ(iw_result.plan->get_start_node().get_state().get_state_repository(), repository);
    expect_aggregated_worker_statistics(iw_result, 2);
}

template<TaskKind Kind>
void expect_parallel_siw_outer_orchestration(const p::TaskPtr<Kind>& task)
{
    auto execution_context = ygg::ExecutionContext::create(1);
    auto axiom_evaluator = p::AxiomEvaluatorFactory<Kind>().create(task, execution_context);
    auto repository = p::StateRepositoryFactory<Kind>().create(task, axiom_evaluator);
    auto generator = p::SuccessorGeneratorFactory<Kind>().create(task, execution_context, repository);
    auto brfs_solver = p::brfs::Solver<Kind> { task, generator };
    brfs_solver.options.num_search_workers = 2;
    brfs_solver.options.state_repository_mode = p::StateRepositoryMode::SHARED;

    auto iw_solver = p::iw::Solver<Kind> { brfs_solver, 2 };
    auto event_handler = p::siw::DefaultEventHandler<Kind>::create();
    auto options = p::siw::Options<Kind> {};
    options.event_handler = event_handler;
    const auto siw_result = p::siw::find_solution(iw_solver, options);
    ASSERT_EQ(siw_result.status, p::SearchStatus::SOLVED);
    ASSERT_TRUE(siw_result.plan);
    EXPECT_GT(event_handler->get_statistics().get_num_solved_subsearches(), 1);
    EXPECT_EQ(siw_result.plan->get_start_node().get_state().get_state_repository(), repository);
    for (const auto& successor : siw_result.plan->get_labeled_succ_nodes())
        EXPECT_EQ(successor.node.get_state().get_state_repository(), repository);
    expect_aggregated_worker_statistics(siw_result, 2);
}

template<TaskKind Kind>
void expect_parallel_astar(const p::TaskPtr<Kind>& task, p::StateRepositoryMode state_repository_mode, p::astar_eager::ParallelSearchMode parallel_search_mode)
{
    auto execution_context = ygg::ExecutionContext::create(1);
    auto axiom_evaluator = p::AxiomEvaluatorFactory<Kind>().create(task, execution_context);
    auto repository = p::StateRepositoryFactory<Kind>().create(task, axiom_evaluator);
    auto generator = p::SuccessorGeneratorFactory<Kind>().create(task, execution_context, repository);
    auto heuristic = p::BlindHeuristic<Kind>::create();

    auto options = p::astar_eager::Options<Kind> {};
    options.num_search_workers = 2;
    options.state_repository_mode = state_repository_mode;
    options.parallel_search_mode = parallel_search_mode;
    const auto result = p::astar_eager::find_solution(*task, *generator, *heuristic, options);

    ASSERT_EQ(result.status, p::SearchStatus::SOLVED);
    ASSERT_TRUE(result.plan);
    ASSERT_TRUE(result.goal_node);
    ASSERT_FALSE(result.plan->empty());
    EXPECT_EQ(result.plan->get_start_node().get_state().get_state_repository(), repository);
    for (const auto& successor : result.plan->get_labeled_succ_nodes())
        EXPECT_EQ(successor.node.get_state().get_state_repository(), repository);
    EXPECT_EQ(result.goal_node->get_state().get_state_repository(), repository);

    ASSERT_EQ(result.worker_statistics.size(), 2);
    auto num_registered_states = uint64_t { 0 };
    auto state_storage_memory_usage = size_t { 0 };
    for (const auto& statistics : result.worker_statistics)
    {
        num_registered_states += statistics.get_num_registered_states();
        state_storage_memory_usage += statistics.get_state_storage_memory_usage();
    }
    EXPECT_EQ(num_registered_states, result.statistics.get_num_registered_states());
    EXPECT_EQ(state_storage_memory_usage, result.statistics.get_state_storage_memory_usage());
    if (state_repository_mode == p::StateRepositoryMode::SHARED)
    {
        EXPECT_EQ(result.worker_statistics[0].get_num_registered_states(), (result.statistics.get_num_registered_states() + 1) / 2);
        EXPECT_EQ(result.worker_statistics[1].get_num_registered_states(), result.statistics.get_num_registered_states() / 2);
        EXPECT_EQ(result.worker_statistics[0].get_state_storage_memory_usage(), result.statistics.get_state_storage_memory_usage());
        EXPECT_EQ(result.worker_statistics[1].get_state_storage_memory_usage(), 0);
    }
}

template<TaskKind Kind>
void expect_lazy_gbfs_worker_events(const p::TaskPtr<Kind>& task, p::StateRepositoryMode state_repository_mode, size_t num_search_workers)
{
    auto execution_context = ygg::ExecutionContext::create(1);
    auto axiom_evaluator = p::AxiomEvaluatorFactory<Kind>().create(task, execution_context);
    auto repository = p::StateRepositoryFactory<Kind>().create(task, axiom_evaluator);
    auto generator = p::SuccessorGeneratorFactory<Kind>().create(task, execution_context, repository);
    auto heuristic = p::BlindHeuristic<Kind>::create();
    auto event_handler = std::make_shared<CountingGBFSEventHandler<Kind>>(repository.get());

    auto options = p::gbfs_lazy::Options<Kind> {};
    options.event_handler = event_handler;
    options.num_search_workers = num_search_workers;
    options.state_repository_mode = state_repository_mode;
    const auto result = p::gbfs_lazy::find_solution(*task, *generator, *heuristic, options);

    ASSERT_EQ(result.status, p::SearchStatus::SOLVED);
    ASSERT_TRUE(result.plan);
    EXPECT_EQ(event_handler->num_workers_created, num_search_workers);

    const auto totals = event_handler->totals();
    EXPECT_EQ(totals.get_num_accepted_successors(), result.statistics.get_num_accepted_successors());
    EXPECT_EQ(totals.get_num_expanded(), result.statistics.get_num_expanded());
    EXPECT_EQ(totals.get_num_pruned(), result.statistics.get_num_pruned());

    ASSERT_TRUE(event_handler->end_status);
    EXPECT_EQ(*event_handler->end_status, result.status);
    ASSERT_TRUE(event_handler->end_statistics);
    EXPECT_EQ(event_handler->end_statistics->get_num_accepted_successors(), result.statistics.get_num_accepted_successors());
    EXPECT_EQ(event_handler->end_statistics->get_num_expanded(), result.statistics.get_num_expanded());
    EXPECT_EQ(event_handler->end_statistics->get_num_deadends(), result.statistics.get_num_deadends());
    EXPECT_EQ(event_handler->end_statistics->get_num_pruned(), result.statistics.get_num_pruned());
    EXPECT_EQ(event_handler->end_statistics->get_search_time(), result.statistics.get_search_time());

    EXPECT_TRUE(event_handler->plan_materialized);
    EXPECT_EQ(event_handler->solved_plan_length, result.plan->get_length());

    ASSERT_EQ(result.worker_statistics.size(), num_search_workers);
    auto worker_totals = p::Statistics {};
    auto num_registered_states = uint64_t { 0 };
    auto state_storage_memory_usage = size_t { 0 };
    for (const auto& statistics : result.worker_statistics)
    {
        worker_totals.add(statistics);
        num_registered_states += statistics.get_num_registered_states();
        state_storage_memory_usage += statistics.get_state_storage_memory_usage();
        EXPECT_LE(statistics.get_idle_time(), std::chrono::duration_cast<std::chrono::nanoseconds>(statistics.get_search_time()));
    }
    EXPECT_EQ(worker_totals.get_num_accepted_successors(), result.statistics.get_num_accepted_successors());
    EXPECT_EQ(worker_totals.get_num_expanded(), result.statistics.get_num_expanded());
    EXPECT_EQ(worker_totals.get_num_deadends(), result.statistics.get_num_deadends());
    EXPECT_EQ(worker_totals.get_num_pruned(), result.statistics.get_num_pruned());
    EXPECT_EQ(worker_totals.get_idle_time(), result.statistics.get_idle_time());
    EXPECT_EQ(num_registered_states, result.statistics.get_num_registered_states());
    EXPECT_EQ(state_storage_memory_usage, result.statistics.get_state_storage_memory_usage());
    if (num_search_workers > 1 && state_repository_mode == p::StateRepositoryMode::SHARED)
    {
        EXPECT_EQ(result.worker_statistics[0].get_num_registered_states(), (result.statistics.get_num_registered_states() + 1) / 2);
        EXPECT_EQ(result.worker_statistics[1].get_num_registered_states(), result.statistics.get_num_registered_states() / 2);
        EXPECT_EQ(result.worker_statistics[0].get_state_storage_memory_usage(), result.statistics.get_state_storage_memory_usage());
        EXPECT_EQ(result.worker_statistics[1].get_state_storage_memory_usage(), 0);
    }
}

template<TaskKind Kind>
std::optional<uint64_t> find_weighted_astar_seed(p::SuccessorGenerator<Kind>& generator)
{
    const auto start = generator.get_initial_node();
    auto direct = std::optional<decltype(generator.get_state_repository()->get_state_builder())> {};
    auto via = std::optional<decltype(generator.get_state_repository()->get_state_builder())> {};
    for (const auto action : generator.get_applicable_action_bindings(start))
    {
        auto state = generator.get_state_repository()->get_state_builder();
        const auto result = generator.generate_successor_state(start, action, *state);
        if (result.auxiliary_value == 100)
            direct.emplace(std::move(state));
        else if (result.auxiliary_value == 1)
            via.emplace(std::move(state));
    }
    if (!direct || !via)
        return std::nullopt;

    for (uint64_t seed = 0; seed < 1024; ++seed)
    {
        const auto hash = p::DistHash<Kind, p::RandomDistHashTag>(seed);
        const auto start_owner = hash.owner(start.get_state().get_state_builder(), 2);
        if (hash.owner(**direct, 2) == start_owner && hash.owner(**via, 2) != start_owner)
            return seed;
    }
    return std::nullopt;
}

template<TaskKind Kind>
void expect_parallel_astar_keeps_searching_after_first_goal(const p::TaskPtr<Kind>& task,
                                                            p::StateRepositoryMode state_repository_mode,
                                                            p::astar_eager::ParallelSearchMode mode)
{
    auto execution_context = ygg::ExecutionContext::create(1);
    auto axiom_evaluator = p::AxiomEvaluatorFactory<Kind>().create(task, execution_context);
    auto repository = p::StateRepositoryFactory<Kind>().create(task, axiom_evaluator);
    auto generator = p::SuccessorGeneratorFactory<Kind>().create(task, execution_context, repository);
    auto heuristic = p::BlindHeuristic<Kind>::create();
    const auto seed = state_repository_mode == p::StateRepositoryMode::HASH_DISTRIBUTED ? find_weighted_astar_seed(*generator) : std::optional<uint64_t> { 0 };
    ASSERT_TRUE(seed);

    auto event_handler = std::make_shared<GatedAStarEventHandler<Kind>>();
    auto options = p::astar_eager::Options<Kind> {};
    options.event_handler = event_handler;
    options.num_search_workers = 2;
    options.state_repository_mode = state_repository_mode;
    options.dist_hash_mode = p::DistHashMode::RANDOM;
    options.parallel_search_mode = mode;
    options.random_seed = *seed;
    const auto result = p::astar_eager::find_solution(*task, *generator, *heuristic, options);

    ASSERT_EQ(result.status, p::SearchStatus::SOLVED);
    ASSERT_TRUE(result.plan);
    ASSERT_TRUE(result.goal_node);
    EXPECT_TRUE(event_handler->gate.expensive_goal_seen.load(std::memory_order_relaxed));
    EXPECT_EQ(event_handler->gate.num_expanded_goals.load(std::memory_order_relaxed), 0);
    EXPECT_EQ(result.plan->get_cost(), 2);
    EXPECT_EQ(result.plan->get_length(), 2);
    ASSERT_EQ(result.worker_statistics.size(), 2);
    for (const auto& statistics : result.worker_statistics)
    {
        EXPECT_GT(statistics.get_num_registered_states(), 0);
    }
    if (state_repository_mode == p::StateRepositoryMode::HASH_DISTRIBUTED)
        EXPECT_GT(result.worker_statistics[1].get_state_storage_memory_usage(), 0);
    else
        EXPECT_EQ(result.worker_statistics[1].get_state_storage_memory_usage(), 0);
    EXPECT_EQ(result.plan->get_start_node().get_state().get_state_repository(), repository);
    for (const auto& successor : result.plan->get_labeled_succ_nodes())
        EXPECT_EQ(successor.node.get_state().get_state_repository(), repository);
    EXPECT_EQ(result.goal_node->get_state().get_state_repository(), repository);
}

template<TaskKind Kind>
void expect_parallel_astar_coordinates_f_layers(const p::TaskPtr<Kind>& task,
                                                p::StateRepositoryMode state_repository_mode,
                                                p::astar_eager::ParallelSearchMode mode,
                                                bool expect_higher_expansion)
{
    auto execution_context = ygg::ExecutionContext::create(1);
    auto axiom_evaluator = p::AxiomEvaluatorFactory<Kind>().create(task, execution_context);
    auto repository = p::StateRepositoryFactory<Kind>().create(task, axiom_evaluator);
    auto generator = p::SuccessorGeneratorFactory<Kind>().create(task, execution_context, repository);
    auto heuristic = p::BlindHeuristic<Kind>::create();
    const auto seed = state_repository_mode == p::StateRepositoryMode::HASH_DISTRIBUTED ? find_weighted_astar_seed(*generator) : std::optional<uint64_t> { 0 };
    ASSERT_TRUE(seed);

    auto event_handler = std::make_shared<LayeredAStarEventHandler<Kind>>();
    auto options = p::astar_eager::Options<Kind> {};
    options.event_handler = event_handler;
    options.goal_strategy = p::ExhaustiveGoalStrategy<Kind>::create();
    options.num_search_workers = 2;
    options.state_repository_mode = state_repository_mode;
    options.dist_hash_mode = p::DistHashMode::RANDOM;
    options.parallel_search_mode = mode;
    options.random_seed = *seed;
    const auto result = p::astar_eager::find_solution(*task, *generator, *heuristic, options);

    EXPECT_EQ(result.status, p::SearchStatus::EXHAUSTED);
    EXPECT_EQ(event_handler->gate.higher_expanded.load(std::memory_order_relaxed), expect_higher_expansion);
    EXPECT_EQ(result.statistics.get_num_expanded(), expect_higher_expansion ? 4 : 3);
}

template<TaskKind Kind>
void expect_synchronous_parallel_astar_stops_all_workers(const p::TaskPtr<Kind>& task)
{
    auto execution_context = ygg::ExecutionContext::create(1);
    auto axiom_evaluator = p::AxiomEvaluatorFactory<Kind>().create(task, execution_context);
    auto repository = p::StateRepositoryFactory<Kind>().create(task, axiom_evaluator);
    auto generator = p::SuccessorGeneratorFactory<Kind>().create(task, execution_context, repository);
    auto heuristic = p::BlindHeuristic<Kind>::create();
    const auto seed = find_weighted_astar_seed(*generator);
    ASSERT_TRUE(seed);
    auto options = p::astar_eager::Options<Kind> {};
    options.event_handler = std::make_shared<LayeredAStarEventHandler<Kind>>();
    options.goal_strategy = p::ExhaustiveGoalStrategy<Kind>::create();
    options.num_search_workers = 2;
    options.state_repository_mode = p::StateRepositoryMode::HASH_DISTRIBUTED;
    options.dist_hash_mode = p::DistHashMode::RANDOM;
    options.parallel_search_mode = p::astar_eager::ParallelSearchMode::SYNCHRONOUS;
    options.random_seed = *seed;
    options.max_time = std::chrono::milliseconds(50);
    EXPECT_EQ(p::astar_eager::find_solution(*task, *generator, *heuristic, options).status, p::SearchStatus::OUT_OF_TIME);

    options.max_time = std::nullopt;
    options.event_handler = std::make_shared<ThrowingAStarEventHandler<Kind>>();
    EXPECT_THROW(p::astar_eager::find_solution(*task, *generator, *heuristic, options), std::runtime_error);
}

template<TaskKind Kind>
void expect_parallel_lazy_gbfs_exhaustion(const p::TaskPtr<Kind>& task, p::StateRepositoryMode state_repository_mode)
{
    auto execution_context = ygg::ExecutionContext::create(1);
    auto axiom_evaluator = p::AxiomEvaluatorFactory<Kind>().create(task, execution_context);
    auto repository = p::StateRepositoryFactory<Kind>().create(task, axiom_evaluator);
    auto generator = p::SuccessorGeneratorFactory<Kind>().create(task, execution_context, repository);
    auto heuristic = p::BlindHeuristic<Kind>::create();
    auto options = p::gbfs_lazy::Options<Kind> {};
    options.num_search_workers = 2;
    options.state_repository_mode = state_repository_mode;

    const auto result = p::gbfs_lazy::find_solution(*task, *generator, *heuristic, options);
    EXPECT_EQ(result.status, p::SearchStatus::EXHAUSTED);
    EXPECT_GT(result.statistics.get_num_expanded(), 0);
    EXPECT_GT(result.statistics.get_num_accepted_successors(), 0);
}

template<TaskKind Kind>
void expect_parallel_brfs_exhaustion(const p::TaskPtr<Kind>& task, p::StateRepositoryMode state_repository_mode)
{
    constexpr auto kind_name = std::same_as<Kind, GroundTag> ? "ground" : "lifted";
    SCOPED_TRACE(kind_name);
    SCOPED_TRACE(state_repository_mode == p::StateRepositoryMode::HASH_DISTRIBUTED ? "hash-distributed" : "shared");

    auto sequential_execution_context = ygg::ExecutionContext::create(1);
    auto sequential_axiom_evaluator = p::AxiomEvaluatorFactory<Kind>().create(task, sequential_execution_context);
    auto sequential_repository = p::StateRepositoryFactory<Kind>().create(task, sequential_axiom_evaluator);
    auto sequential_generator = p::SuccessorGeneratorFactory<Kind>().create(task, sequential_execution_context, sequential_repository);
    auto sequential_options = p::brfs::Options<Kind> {};
    sequential_options.goal_strategy = p::ExhaustiveGoalStrategy<Kind>::create();
    const auto sequential_result = p::brfs::find_solution(*task, *sequential_generator, sequential_options);

    auto execution_context = ygg::ExecutionContext::create(1);
    auto axiom_evaluator = p::AxiomEvaluatorFactory<Kind>().create(task, execution_context);
    auto repository = p::StateRepositoryFactory<Kind>().create(task, axiom_evaluator);
    auto generator = p::SuccessorGeneratorFactory<Kind>().create(task, execution_context, repository);
    constexpr auto seed = uint64_t { 0 };
    auto guard = std::make_shared<OwnerAccessGuard<Kind>>(state_repository_mode, seed);
    auto options = p::brfs::Options<Kind> {};
    options.event_handler = std::make_shared<GuardedBrFSEventHandler<Kind>>(guard);
    options.pruning_strategy = std::make_shared<GuardedPruningStrategy<Kind>>(guard);
    options.goal_strategy = p::ExhaustiveGoalStrategy<Kind>::create();
    options.num_search_workers = OwnerAccessGuard<Kind>::num_workers;
    options.state_repository_mode = state_repository_mode;
    options.dist_hash_mode = p::DistHashMode::RANDOM;
    options.random_seed = seed;

    const auto result = p::brfs::find_solution(*task, *generator, options);
    guard->expect_valid();

    EXPECT_EQ(sequential_result.status, p::SearchStatus::EXHAUSTED);
    EXPECT_EQ(result.status, p::SearchStatus::EXHAUSTED);
    EXPECT_EQ(result.statistics.get_num_registered_states(), sequential_result.statistics.get_num_registered_states());
    EXPECT_EQ(result.statistics.get_num_expanded(), sequential_result.statistics.get_num_expanded());
    EXPECT_EQ(result.statistics.get_num_accepted_successors(), sequential_result.statistics.get_num_accepted_successors());
    EXPECT_EQ(result.statistics.get_num_pruned(), 0);
    EXPECT_EQ(result.worker_statistics.size(), OwnerAccessGuard<Kind>::num_workers);
    EXPECT_GT(sequential_result.statistics.get_num_generated_successors(), 0);
    EXPECT_EQ(sequential_result.statistics.get_num_transferred_successors(), 0);
    EXPECT_DOUBLE_EQ(sequential_result.statistics.get_communication_overhead(), 0.0);
    EXPECT_GT(result.statistics.get_num_generated_successors(), 0);
    EXPECT_GT(result.statistics.get_num_transferred_successors(), 0);
    EXPECT_LE(result.statistics.get_num_transferred_successors(), result.statistics.get_num_generated_successors());
    EXPECT_DOUBLE_EQ(result.statistics.get_communication_overhead(),
                     static_cast<double>(result.statistics.get_num_transferred_successors())
                         / static_cast<double>(result.statistics.get_num_generated_successors()));

    auto worker_totals = p::Statistics {};
    for (const auto& statistics : result.worker_statistics)
        worker_totals.add(statistics);
    EXPECT_EQ(worker_totals.get_num_generated_successors(), result.statistics.get_num_generated_successors());
    EXPECT_EQ(worker_totals.get_num_transferred_successors(), result.statistics.get_num_transferred_successors());
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

TEST(TyrPlanningPlanReconstructionTest, ParallelLazyGBFSMaterializesGroundAndLiftedPlansInCallerRepository)
{
    const auto root = std::filesystem::path(BENCHMARKS_DIR);
    auto lifted_task =
        p::Task<LiftedTag>::create(make_test_parser(root / "classical/tests/gripper/domain.pddl").parse_task(root / "classical/tests/gripper/test-1.pddl"));
    auto grounding_context = ygg::ExecutionContext::create(1);
    auto ground_task = lifted_task->instantiate_ground_task(*grounding_context).task;
    ASSERT_NE(ground_task, nullptr);

    for (const auto mode : { p::StateRepositoryMode::HASH_DISTRIBUTED, p::StateRepositoryMode::SHARED })
    {
        expect_parallel_lazy_gbfs(ground_task, mode);
        expect_parallel_lazy_gbfs(lifted_task, mode);
    }
}

TEST(TyrPlanningPlanReconstructionTest, ParallelAStarSupportsRepositoryAndFLayerModes)
{
    const auto root = std::filesystem::path(BENCHMARKS_DIR);
    auto lifted_task =
        p::Task<LiftedTag>::create(make_test_parser(root / "classical/tests/gripper/domain.pddl").parse_task(root / "classical/tests/gripper/test-1.pddl"));
    auto grounding_context = ygg::ExecutionContext::create(1);
    auto ground_task = lifted_task->instantiate_ground_task(*grounding_context).task;
    ASSERT_NE(ground_task, nullptr);

    for (const auto state_repository_mode : { p::StateRepositoryMode::HASH_DISTRIBUTED, p::StateRepositoryMode::SHARED })
    {
        for (const auto parallel_search_mode : { p::astar_eager::ParallelSearchMode::SYNCHRONOUS, p::astar_eager::ParallelSearchMode::ASYNCHRONOUS })
        {
            expect_parallel_astar(ground_task, state_repository_mode, parallel_search_mode);
            expect_parallel_astar(lifted_task, state_repository_mode, parallel_search_mode);
        }
    }
}

TEST(TyrPlanningPlanReconstructionTest, ParallelBrFSSupportsRepositoryModes)
{
    const auto root = std::filesystem::path(BENCHMARKS_DIR);
    auto lifted_task =
        p::Task<LiftedTag>::create(make_test_parser(root / "classical/tests/gripper/domain.pddl").parse_task(root / "classical/tests/gripper/test-1.pddl"));
    auto grounding_context = ygg::ExecutionContext::create(1);
    auto ground_task = lifted_task->instantiate_ground_task(*grounding_context).task;
    ASSERT_NE(ground_task, nullptr);

    for (const auto mode : { p::StateRepositoryMode::HASH_DISTRIBUTED, p::StateRepositoryMode::SHARED })
    {
        expect_parallel_brfs(ground_task, mode);
        expect_parallel_brfs(lifted_task, mode);
    }
}

TEST(TyrPlanningPlanReconstructionTest, BrFSEmitsWorkerAndGlobalLayerEvents)
{
    const auto root = std::filesystem::path(BENCHMARKS_DIR);
    auto lifted_task =
        p::Task<LiftedTag>::create(make_test_parser(root / "classical/tests/gripper/domain.pddl").parse_task(root / "classical/tests/gripper/test-1.pddl"));
    auto grounding_context = ygg::ExecutionContext::create(1);
    auto ground_task = lifted_task->instantiate_ground_task(*grounding_context).task;
    ASSERT_NE(ground_task, nullptr);

    expect_brfs_events(ground_task, p::StateRepositoryMode::HASH_DISTRIBUTED, 1);
    for (const auto mode : { p::StateRepositoryMode::HASH_DISTRIBUTED, p::StateRepositoryMode::SHARED })
        expect_brfs_events(ground_task, mode, 2);
}

TEST(TyrPlanningPlanReconstructionTest, ParallelBrFSSupportsIW)
{
    const auto root = std::filesystem::path(BENCHMARKS_DIR);
    auto lifted_task =
        p::Task<LiftedTag>::create(make_test_parser(root / "classical/tests/gripper/domain.pddl").parse_task(root / "classical/tests/gripper/test-1.pddl"));
    auto grounding_context = ygg::ExecutionContext::create(1);
    auto ground_task = lifted_task->instantiate_ground_task(*grounding_context).task;
    ASSERT_NE(ground_task, nullptr);

    for (const auto mode : { p::StateRepositoryMode::HASH_DISTRIBUTED, p::StateRepositoryMode::SHARED })
    {
        expect_parallel_iw(ground_task, mode);
        expect_parallel_iw(lifted_task, mode);
    }
}

TEST(TyrPlanningPlanReconstructionTest, SIWSequentiallyOrchestratesParallelIWSubsearches)
{
    const auto root = std::filesystem::path(BENCHMARKS_DIR);
    auto lifted_task =
        p::Task<LiftedTag>::create(make_test_parser(root / "classical/tests/blocks_4/domain.pddl").parse_task(root / "classical/tests/blocks_4/test-1.pddl"));
    auto grounding_context = ygg::ExecutionContext::create(1);
    auto ground_task = lifted_task->instantiate_ground_task(*grounding_context).task;
    ASSERT_NE(ground_task, nullptr);

    expect_parallel_siw_outer_orchestration(ground_task);
}

TEST(TyrPlanningPlanReconstructionTest, LazyGBFSWorkerEventsMatchResultStatistics)
{
    const auto root = std::filesystem::path(BENCHMARKS_DIR);
    auto lifted_task =
        p::Task<LiftedTag>::create(make_test_parser(root / "classical/tests/gripper/domain.pddl").parse_task(root / "classical/tests/gripper/test-1.pddl"));
    auto grounding_context = ygg::ExecutionContext::create(1);
    auto ground_task = lifted_task->instantiate_ground_task(*grounding_context).task;
    ASSERT_NE(ground_task, nullptr);

    expect_lazy_gbfs_worker_events(ground_task, p::StateRepositoryMode::SHARED, 1);
    for (const auto mode : { p::StateRepositoryMode::HASH_DISTRIBUTED, p::StateRepositoryMode::SHARED })
        expect_lazy_gbfs_worker_events(ground_task, mode, 2);
}

TEST(TyrPlanningPlanReconstructionTest, ParallelSearchDetectsGlobalExhaustion)
{
    const auto benchmark_root = std::filesystem::path(BENCHMARKS_DIR);
    const auto fixture_root = std::filesystem::path(ROOT_DIR) / "tests/fixtures/planning/algorithms";
    auto lifted_task = p::Task<LiftedTag>::create(
        make_test_parser(benchmark_root / "classical/tests/gripper/domain.pddl").parse_task(fixture_root / "parallel_gripper_unsolvable.pddl"));
    auto grounding_context = ygg::ExecutionContext::create(1);
    auto grounding_options = p::GroundTaskInstantiationOptions {};
    grounding_options.disable_invariant_synthesis = true;
    auto ground_task = lifted_task->instantiate_ground_task(*grounding_context, grounding_options).task;
    ASSERT_NE(ground_task, nullptr);

    for (const auto mode : { p::StateRepositoryMode::HASH_DISTRIBUTED, p::StateRepositoryMode::SHARED })
    {
        expect_parallel_lazy_gbfs_exhaustion(ground_task, mode);
        expect_parallel_lazy_gbfs_exhaustion(lifted_task, mode);
        expect_parallel_brfs_exhaustion(ground_task, mode);
        expect_parallel_brfs_exhaustion(lifted_task, mode);
    }
}

TEST(TyrPlanningPlanReconstructionTest, ParallelAStarDoesNotStopAtTheFirstGoal)
{
    const auto benchmark_root = std::filesystem::path(BENCHMARKS_DIR);
    const auto fixture_root = std::filesystem::path(ROOT_DIR) / "tests/fixtures/planning/algorithms";
    auto lifted_task = p::Task<LiftedTag>::create(
        make_test_parser(benchmark_root / "classical/tests/transport/domain.pddl").parse_task(fixture_root / "parallel_astar_weighted.pddl"));
    auto grounding_context = ygg::ExecutionContext::create(1);
    auto ground_task = lifted_task->instantiate_ground_task(*grounding_context).task;
    ASSERT_NE(ground_task, nullptr);

    for (const auto state_repository_mode : { p::StateRepositoryMode::HASH_DISTRIBUTED, p::StateRepositoryMode::SHARED })
    {
        for (const auto mode : { p::astar_eager::ParallelSearchMode::SYNCHRONOUS, p::astar_eager::ParallelSearchMode::ASYNCHRONOUS })
        {
            expect_parallel_astar_keeps_searching_after_first_goal(ground_task, state_repository_mode, mode);
            expect_parallel_astar_keeps_searching_after_first_goal(lifted_task, state_repository_mode, mode);
        }
    }
}

TEST(TyrPlanningPlanReconstructionTest, ParallelAStarSelectsFLayerCoordination)
{
    const auto benchmark_root = std::filesystem::path(BENCHMARKS_DIR);
    const auto fixture_root = std::filesystem::path(ROOT_DIR) / "tests/fixtures/planning/algorithms";
    auto lifted_task = p::Task<LiftedTag>::create(
        make_test_parser(benchmark_root / "classical/tests/transport/domain.pddl").parse_task(fixture_root / "parallel_astar_weighted.pddl"));
    auto grounding_context = ygg::ExecutionContext::create(1);
    auto ground_task = lifted_task->instantiate_ground_task(*grounding_context).task;
    ASSERT_NE(ground_task, nullptr);

    for (const auto state_repository_mode : { p::StateRepositoryMode::HASH_DISTRIBUTED, p::StateRepositoryMode::SHARED })
    {
        expect_parallel_astar_coordinates_f_layers(ground_task, state_repository_mode, p::astar_eager::ParallelSearchMode::SYNCHRONOUS, false);
        expect_parallel_astar_coordinates_f_layers(lifted_task, state_repository_mode, p::astar_eager::ParallelSearchMode::SYNCHRONOUS, false);
        expect_parallel_astar_coordinates_f_layers(ground_task, state_repository_mode, p::astar_eager::ParallelSearchMode::ASYNCHRONOUS, true);
        expect_parallel_astar_coordinates_f_layers(lifted_task, state_repository_mode, p::astar_eager::ParallelSearchMode::ASYNCHRONOUS, true);
    }
    expect_synchronous_parallel_astar_stops_all_workers(ground_task);
}

}
