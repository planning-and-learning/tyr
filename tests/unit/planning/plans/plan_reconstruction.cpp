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
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/parser.hpp"
#include "tyr/planning/algorithms/astar_eager.hpp"
#include "tyr/planning/algorithms/astar_eager/event_handler.hpp"
#include "tyr/planning/algorithms/brfs.hpp"
#include "tyr/planning/algorithms/brfs/event_handler.hpp"
#include "tyr/planning/algorithms/gbfs_lazy.hpp"
#include "tyr/planning/algorithms/gbfs_lazy/event_handler.hpp"
#include "tyr/planning/algorithms/iw.hpp"
#include "tyr/planning/algorithms/iw/event_handler.hpp"
#include "tyr/planning/algorithms/siw.hpp"
#include "tyr/planning/algorithms/siw/event_handler.hpp"
#include "tyr/planning/algorithms/statistics.hpp"
#include "tyr/planning/algorithms/strategies/goal.hpp"
#include "tyr/planning/algorithms/strategies/pruning.hpp"
#include "tyr/planning/algorithms/utils.hpp"
#include "tyr/planning/axioms/axiom_evaluator.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/successors/ground/successor_generator.hpp"
#include "tyr/planning/tasks/ground/task.hpp"
#include "tyr/planning/heuristics/blind.hpp"
#include "tyr/planning/nodes/labeled_node_view.hpp"
#include "tyr/planning/successors/lifted/successor_generator.hpp"
#include "tyr/planning/tasks/lifted/task.hpp"
#include "tyr/planning/tasks/lifted/task_grounder_decl.hpp"
#include "tyr/planning/nodes/node_concepts.hpp"
#include "tyr/planning/nodes/node_view.hpp"
#include "tyr/planning/plans/plan_view.hpp"
#include "tyr/planning/algorithms/search_space/parallel.hpp"
#include "tyr/planning/algorithms/search_space/sequential.hpp"
#include "tyr/planning/states/state_delta.hpp"
#include "tyr/planning/states/state_repository.hpp"
#include "tyr/planning/algorithms/state_routing/dist_hash.hpp"
#include "tyr/planning/algorithms/state_routing/dist_hash_concepts.hpp"
#include "tyr/planning/successors/successors.hpp"
#include "tyr/planning/states/worker_state_index.hpp"

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
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
#include <urd/repr/sequences/segmented.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <vector>
#include <yggdrasil/execution/onetbb.hpp>

namespace tyr::tests
{
namespace
{

enum class RepositoryMode : std::uint8_t
{
    HASH_DISTRIBUTED,
    SHARED,
};

template<TaskKind Kind>
tyr::planning::StateRepositoryPtr<Kind> make_repository(const tyr::planning::TaskPtr<Kind>& task, RepositoryMode mode)
{ return tyr::planning::StateRepository<Kind>::create(task, mode == RepositoryMode::SHARED); }

template<TaskKind Kind>
struct SequentialSearchNode
{
    ygg::float_t g_value;
    urd::index<tyr::planning::State<Kind>> parent_state;
};

template<TaskKind Kind>
struct ParallelSearchNode
{
    ygg::float_t g_value;
    tyr::planning::WorkerStateIndex<Kind> parent_state;
};

template<TaskKind Kind>
class CountingGBFSWorkerEventHandler final : public tyr::planning::gbfs_lazy::WorkerEventHandler<Kind>
{
public:
    explicit CountingGBFSWorkerEventHandler(tyr::planning::Statistics& statistics) : m_statistics(&statistics) {}

    void on_expand_node(const tyr::planning::NodeBuilderView<Kind>&) override { m_statistics->increment_num_expanded(); }

    void on_generate_transition(const tyr::planning::NodeBuilderView<Kind>&,
                                tyr::planning::ActionBindingView<Kind>,
                                const tyr::planning::NodeBuilderView<Kind>&,
                                tyr::planning::TransitionOutcome outcome) override
    {
        if (outcome == tyr::planning::TransitionOutcome::OPENED || outcome == tyr::planning::TransitionOutcome::GOAL)
            m_statistics->increment_num_generated_successors();
        else if (outcome == tyr::planning::TransitionOutcome::PRUNED)
            m_statistics->increment_num_pruned();
    }

private:
    tyr::planning::Statistics* m_statistics;
};

template<TaskKind Kind>
class UnsupportedParallelPruningStrategy final : public tyr::planning::PruningStrategy<Kind>
{
};

template<TaskKind Kind>
class UnsupportedParallelGoalStrategy final : public tyr::planning::GoalStrategy<Kind>
{
public:
    bool is_static_goal_satisfied(const tyr::planning::Task<Kind>&) override { return true; }

    bool is_dynamic_goal_satisfied(const tyr::planning::StateBuilderView<Kind>&, const tyr::planning::StateBuilderView<Kind>&) override { return false; }
};

template<TaskKind Kind>
class CountingGBFSEventHandler final : public tyr::planning::gbfs_lazy::EventHandler<Kind>
{
public:
    explicit CountingGBFSEventHandler(const tyr::planning::StateRepository<Kind>* caller_repository_) : caller_repository(caller_repository_) {}

    void on_start_search(const tyr::planning::NodeBuilderView<Kind>&, ygg::float_t) override {}

    void on_new_best_h_value(ygg::float_t) override {}

    void on_end_search(tyr::planning::SearchStatus status, const tyr::planning::Statistics& statistics) override
    {
        end_status = status;
        end_statistics = statistics;
    }

    void on_solved(const tyr::planning::PlanBuilderView<Kind>& plan) override
    {
        solved_plan_length = plan.get_length();
        plan_materialized = &plan.get_start_node().context() == caller_repository;
        for (const auto& successor : plan.get_labeled_succ_nodes())
            plan_materialized &= &successor.get_node().context() == caller_repository;
    }

    tyr::planning::gbfs_lazy::WorkerEventHandlerPtr<Kind> make_worker(urd::index<tyr::planning::Worker> index) override
    {
        ++num_workers_created;
        return std::make_unique<CountingGBFSWorkerEventHandler<Kind>>(worker_statistics.at(static_cast<ygg::uint_t>(index.value)));
    }

    [[nodiscard]] tyr::planning::Statistics totals() const
    {
        auto result = tyr::planning::Statistics {};
        for (const auto& statistics : worker_statistics)
            result.add(statistics);
        return result;
    }

    std::array<tyr::planning::Statistics, 2> worker_statistics;
    size_t num_workers_created = 0;
    bool plan_materialized = false;
    size_t solved_plan_length = 0;
    std::optional<tyr::planning::SearchStatus> end_status;
    std::optional<tyr::planning::Statistics> end_statistics;
    const tyr::planning::StateRepository<Kind>* caller_repository;
};

template<TaskKind Kind>
class CountingBrFSWorkerEventHandler final : public tyr::planning::brfs::WorkerEventHandler<Kind>
{
public:
    explicit CountingBrFSWorkerEventHandler(tyr::planning::Statistics& statistics) : m_statistics(&statistics) {}

    void on_expand_node(const tyr::planning::NodeBuilderView<Kind>&) override { m_statistics->increment_num_expanded(); }

    void on_generate_transition(const tyr::planning::NodeBuilderView<Kind>&,
                                tyr::planning::ActionBindingView<Kind>,
                                const tyr::planning::NodeBuilderView<Kind>&,
                                tyr::planning::TransitionOutcome outcome) override
    {
        if (outcome == tyr::planning::TransitionOutcome::OPENED || outcome == tyr::planning::TransitionOutcome::GOAL)
            m_statistics->increment_num_generated_successors();
        else if (outcome == tyr::planning::TransitionOutcome::PRUNED)
            m_statistics->increment_num_pruned();
    }

private:
    tyr::planning::Statistics* m_statistics;
};

template<TaskKind Kind>
class CountingBrFSEventHandler final : public tyr::planning::brfs::EventHandler<Kind>
{
public:
    void on_start_search(const tyr::planning::NodeBuilderView<Kind>&) override { ++num_starts; }

    void on_finish_layer(ygg::uint_t layer, const tyr::planning::Statistics& statistics) override
    {
        finished_layers.push_back(layer);
        layer_statistics.push_back(statistics);
    }

    void on_end_search(tyr::planning::SearchStatus status, const tyr::planning::Statistics& statistics) override
    {
        end_status = status;
        end_statistics = statistics;
    }

    void on_solved(const tyr::planning::PlanBuilderView<Kind>& plan) override { solved_plan_length = plan.get_length(); }

    tyr::planning::brfs::WorkerEventHandlerPtr<Kind> make_worker(urd::index<tyr::planning::Worker> index) override
    {
        ++num_workers_created;
        return std::make_unique<CountingBrFSWorkerEventHandler<Kind>>(worker_statistics.at(static_cast<ygg::uint_t>(index.value)));
    }

    std::array<tyr::planning::Statistics, 2> worker_statistics;
    urd::Store<urd::sequence<ygg::uint_t>> finished_layers;
    urd::Store<urd::sequence<tyr::planning::Statistics>> layer_statistics;
    size_t num_starts = 0;
    size_t num_workers_created = 0;
    size_t solved_plan_length = 0;
    std::optional<tyr::planning::SearchStatus> end_status;
    std::optional<tyr::planning::Statistics> end_statistics;
};

template<TaskKind Kind>
class OwnerAccessGuard
{
public:
    static constexpr size_t num_workers = 4;

    OwnerAccessGuard(RepositoryMode mode, uint64_t seed) : m_mode(mode), m_hash(seed) {}

    void access(urd::index<tyr::planning::Worker> worker, const std::type_identity_t<tyr::planning::StateBuilderView<Kind>>& state)
    {
        auto& lane = m_lanes.at(static_cast<ygg::uint_t>(worker.value));
        if (lane.active.test_and_set(std::memory_order_acquire))
            lane.reentered.store(true, std::memory_order_relaxed);

        if (owner(state) != worker)
            lane.wrong_owner.store(true, std::memory_order_relaxed);
        observe_repository(lane, state);
        lane.num_accesses.fetch_add(1, std::memory_order_relaxed);
        std::this_thread::yield();
        lane.active.clear(std::memory_order_release);
    }

    void access_transition(urd::index<tyr::planning::Worker> worker,
                           const std::type_identity_t<tyr::planning::StateBuilderView<Kind>>& source,
                           const std::type_identity_t<tyr::planning::StateBuilderView<Kind>>& target)
    {
        const auto source_owner = owner(source);
        observe_repository(m_lanes.at(static_cast<ygg::uint_t>(source_owner.value)), source);
        if (source_owner != worker)
        {
            m_num_remote_transitions.fetch_add(1, std::memory_order_relaxed);
            if (m_mode == RepositoryMode::SHARED && !source.context().shares_storage_with(target.context()))
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
        std::atomic<const tyr::planning::StateRepository<Kind>*> repository { nullptr };
        std::atomic<uint64_t> num_accesses { 0 };
    };

    [[nodiscard]] urd::index<tyr::planning::Worker> owner(const std::type_identity_t<tyr::planning::StateBuilderView<Kind>>& state) const
    {
        if (m_mode == RepositoryMode::SHARED)
            return urd::index<tyr::planning::Worker>(static_cast<ygg::uint_t>(static_cast<ygg::uint_t>(state.pack().index().value) % num_workers));
        return m_hash.owner(state, num_workers);
    }

    void observe_repository(Lane& lane, const std::type_identity_t<tyr::planning::StateBuilderView<Kind>>& state) const
    {
        if (m_mode == RepositoryMode::SHARED)
            return;

        auto* expected_repository = static_cast<const tyr::planning::StateRepository<Kind>*>(nullptr);
        const auto* repository = &state.context();
        if (!lane.repository.compare_exchange_strong(expected_repository, repository, std::memory_order_relaxed)
            && !expected_repository->shares_storage_with(*repository))
            lane.wrong_repository.store(true, std::memory_order_relaxed);
    }

    RepositoryMode m_mode;
    tyr::planning::DistHash<Kind, tyr::planning::RandomDistHashTag> m_hash;
    std::array<Lane, num_workers> m_lanes;
    std::atomic<uint64_t> m_num_remote_transitions { 0 };
    std::atomic<uint64_t> m_num_duplicates { 0 };
    std::atomic<bool> m_wrong_shared_remote_repository { false };
};

template<TaskKind Kind>
class GuardedBrFSWorkerEventHandler final : public tyr::planning::brfs::WorkerEventHandler<Kind>
{
public:
    GuardedBrFSWorkerEventHandler(urd::index<tyr::planning::Worker> worker, std::shared_ptr<OwnerAccessGuard<Kind>> guard) :
        m_worker(worker),
        m_guard(std::move(guard))
    {
    }

    void on_expand_node(const std::type_identity_t<tyr::planning::NodeBuilderView<Kind>>& node) override { m_guard->access(m_worker, tyr::tests::built_state(node)); }

    void on_generate_transition(const std::type_identity_t<tyr::planning::NodeBuilderView<Kind>>& source,
                                tyr::planning::ActionBindingView<Kind>,
                                const std::type_identity_t<tyr::planning::NodeBuilderView<Kind>>& target,
                                tyr::planning::TransitionOutcome outcome) override
    {
        m_guard->access_transition(m_worker, tyr::tests::built_state(source), tyr::tests::built_state(target));
        if (outcome == tyr::planning::TransitionOutcome::DUPLICATE)
            m_guard->count_duplicate();
    }

private:
    urd::index<tyr::planning::Worker> m_worker;
    std::shared_ptr<OwnerAccessGuard<Kind>> m_guard;
};

template<TaskKind Kind>
class GuardedBrFSEventHandler final : public tyr::planning::brfs::EventHandler<Kind>
{
public:
    explicit GuardedBrFSEventHandler(std::shared_ptr<OwnerAccessGuard<Kind>> guard) : m_guard(std::move(guard)) {}

    void on_start_search(const tyr::planning::NodeBuilderView<Kind>&) override {}

    void on_finish_layer(ygg::uint_t, const tyr::planning::Statistics&) override {}

    void on_end_search(tyr::planning::SearchStatus, const tyr::planning::Statistics&) override {}

    void on_solved(const tyr::planning::PlanBuilderView<Kind>&) override {}

    tyr::planning::brfs::WorkerEventHandlerPtr<Kind> make_worker(urd::index<tyr::planning::Worker> worker) override
    { return std::make_unique<GuardedBrFSWorkerEventHandler<Kind>>(worker, m_guard); }

private:
    std::shared_ptr<OwnerAccessGuard<Kind>> m_guard;
};

template<TaskKind Kind>
class GuardedPruningStrategy final : public tyr::planning::PruningStrategy<Kind>
{
public:
    explicit GuardedPruningStrategy(std::shared_ptr<OwnerAccessGuard<Kind>> guard) : m_guard(std::move(guard)) {}

    GuardedPruningStrategy(urd::index<tyr::planning::Worker> worker, std::shared_ptr<OwnerAccessGuard<Kind>> guard) :
        m_worker(worker),
        m_guard(std::move(guard))
    {
    }

    [[nodiscard]] tyr::planning::PruningStrategyPtr<Kind> make_worker(urd::index<tyr::planning::Worker> worker) const override
    { return std::make_shared<GuardedPruningStrategy>(worker, m_guard); }

    bool should_prune_state(const std::type_identity_t<tyr::planning::StateBuilderView<Kind>>& state) override
    {
        m_guard->access(m_worker, state);
        return false;
    }

    bool should_prune_successor_state(const std::type_identity_t<tyr::planning::StateBuilderView<Kind>>& source,
                                      const std::type_identity_t<tyr::planning::StateBuilderView<Kind>>& state,
                                      bool) override
    {
        m_guard->access_transition(m_worker, source, state);
        return false;
    }

private:
    urd::index<tyr::planning::Worker> m_worker { 0 };
    std::shared_ptr<OwnerAccessGuard<Kind>> m_guard;
};

struct AStarGoalGate
{
    std::binary_semaphore cheap_path { 0 };
    std::atomic<bool> expensive_goal_seen { false };
    std::atomic<uint64_t> num_expanded_goals { 0 };
};

template<TaskKind Kind>
class GatedAStarWorkerEventHandler final : public tyr::planning::astar_eager::WorkerEventHandler<Kind>
{
public:
    explicit GatedAStarWorkerEventHandler(AStarGoalGate& gate) : m_gate(&gate) {}

    void on_expand_node(const std::type_identity_t<tyr::planning::NodeBuilderView<Kind>>& node) override
    {
        if (node.get_metric() == 1 && !m_gate->cheap_path.try_acquire_for(std::chrono::seconds(5)))
            throw std::runtime_error("Timed out waiting for the expensive goal.");
    }

    void on_generate_transition(const tyr::planning::NodeBuilderView<Kind>&,
                                tyr::planning::ActionBindingView<Kind>,
                                const std::type_identity_t<tyr::planning::NodeBuilderView<Kind>>& successor,
                                tyr::planning::TransitionOutcome outcome) override
    {
        if (outcome == tyr::planning::TransitionOutcome::GOAL && successor.get_metric() == 100)
        {
            m_gate->expensive_goal_seen.store(true, std::memory_order_relaxed);
            m_gate->cheap_path.release();
        }
    }

    void on_expand_goal_node(const tyr::planning::NodeBuilderView<Kind>&) override { m_gate->num_expanded_goals.fetch_add(1, std::memory_order_relaxed); }

private:
    AStarGoalGate* m_gate;
};

template<TaskKind Kind>
class GatedAStarEventHandler final : public tyr::planning::astar_eager::EventHandler<Kind>
{
public:
    void on_start_search(const tyr::planning::NodeBuilderView<Kind>&, ygg::float_t) override {}

    void on_end_search(tyr::planning::SearchStatus, const tyr::planning::Statistics&) override {}

    void on_solved(const tyr::planning::PlanBuilderView<Kind>&) override {}

    tyr::planning::astar_eager::WorkerEventHandlerPtr<Kind> make_worker(urd::index<tyr::planning::Worker>) override
    { return std::make_unique<GatedAStarWorkerEventHandler<Kind>>(gate); }

    AStarGoalGate gate;
};

struct AStarLayerGate
{
    std::binary_semaphore lower_started { 0 };
    std::binary_semaphore higher_started { 0 };
    std::atomic<bool> higher_expanded { false };
};

template<TaskKind Kind>
class LayeredAStarWorkerEventHandler final : public tyr::planning::astar_eager::WorkerEventHandler<Kind>
{
public:
    explicit LayeredAStarWorkerEventHandler(AStarLayerGate& gate) : m_gate(&gate) {}

    void on_expand_node(const std::type_identity_t<tyr::planning::NodeBuilderView<Kind>>& node) override
    {
        if (node.get_metric() == 1)
        {
            m_gate->lower_started.release();
            static_cast<void>(m_gate->higher_started.try_acquire_for(std::chrono::milliseconds(100)));
        }
        else if (node.get_metric() == 100)
        {
            if (!m_gate->lower_started.try_acquire_for(std::chrono::seconds(5)))
                throw std::runtime_error("Timed out waiting for the lower f-layer.");
            m_gate->higher_expanded.store(true, std::memory_order_relaxed);
            m_gate->higher_started.release();
        }
    }

private:
    AStarLayerGate* m_gate;
};

template<TaskKind Kind>
class LayeredAStarEventHandler final : public tyr::planning::astar_eager::EventHandler<Kind>
{
public:
    void on_start_search(const tyr::planning::NodeBuilderView<Kind>&, ygg::float_t) override {}

    void on_finish_f_layer(ygg::float_t f_value, const tyr::planning::Statistics&) override { finished_f_layers.push_back(f_value); }

    void on_end_search(tyr::planning::SearchStatus, const tyr::planning::Statistics&) override {}

    void on_solved(const tyr::planning::PlanBuilderView<Kind>&) override {}

    tyr::planning::astar_eager::WorkerEventHandlerPtr<Kind> make_worker(urd::index<tyr::planning::Worker>) override
    { return std::make_unique<LayeredAStarWorkerEventHandler<Kind>>(gate); }

    AStarLayerGate gate;
    urd::Store<urd::sequence<ygg::float_t>> finished_f_layers;
};

template<TaskKind Kind>
class ThrowingAStarWorkerEventHandler final : public tyr::planning::astar_eager::WorkerEventHandler<Kind>
{
public:
    void on_expand_node(const tyr::planning::NodeBuilderView<Kind>&) override { throw std::runtime_error("Worker event failure."); }
};

template<TaskKind Kind>
class ThrowingAStarEventHandler final : public tyr::planning::astar_eager::EventHandler<Kind>
{
public:
    void on_start_search(const tyr::planning::NodeBuilderView<Kind>&, ygg::float_t) override {}

    void on_end_search(tyr::planning::SearchStatus, const tyr::planning::Statistics&) override {}

    void on_solved(const tyr::planning::PlanBuilderView<Kind>&) override {}

    tyr::planning::astar_eager::WorkerEventHandlerPtr<Kind> make_worker(urd::index<tyr::planning::Worker>) override
    { return std::make_unique<ThrowingAStarWorkerEventHandler<Kind>>(); }
};

template<typename SearchNode>
SearchNode& get_or_create_search_node(ygg::uint_t index, urd::Store<urd::sequence<SearchNode>, urd::stable_refs>& search_nodes, const SearchNode& default_node)
{
    while (search_nodes.size() <= index)
        search_nodes.push_back(default_node);
    return search_nodes[index];
}

template<TaskKind Kind>
tyr::planning::SharedStateBuilderView<Kind> copy_state(const std::type_identity_t<tyr::planning::StateBuilderView<Kind>>& source,
                                                tyr::planning::StateRepository<Kind>& destination,
                                                tyr::planning::AxiomEvaluator<Kind>& axiom_evaluator)
{
    auto builder = ::tyr::planning::get_state_builder(destination);
    builder->assign_unextended_part(source);
    ::tyr::planning::register_state(destination, axiom_evaluator, *builder);
    return ::tyr::planning::share(destination, std::move(builder));
}

template<TaskKind Kind>
auto find_successor(const tyr::tests::OwnedLabeledNodeBuilderViewList<Kind>& successors,
                    std::string_view action,
                    std::optional<urd::index<tyr::planning::State<Kind>>> excluded_state = std::nullopt)
{
    return std::ranges::find_if(successors,
                                [&](const auto& successor)
                                {
                                    return urd::interned_view(successor.get_label()).get_relation().get_name().str() == action
                                           && (!excluded_state || tyr::tests::built_state(successor.get_node()).pack().index() != *excluded_state);
                                });
}

template<TaskKind Kind>
void expect_action_sequence_replay(const tyr::planning::TaskPtr<Kind>& task)
{
    // Plans borrow their repository: the caller keeps it alive.
    auto retained_repository = tyr::planning::StateRepositoryPtr<Kind> {};
    auto plan = std::optional<tyr::planning::OwnedPlanBuilderView<Kind>> {};
    {
        auto execution = ygg::ExecutionContext::create(1);
        auto axioms = std::make_shared<tyr::planning::AxiomEvaluator<Kind>>(task, execution);
        auto repository = tyr::planning::StateRepository<Kind>::create(task);
        auto generator = std::make_shared<tyr::planning::SuccessorGenerator<Kind>>(task, execution);
        retained_repository = repository;
        const auto initial = tyr::tests::owned_initial_node(*repository, *axioms).pack();
        const auto start = urd::data<tyr::planning::Node<Kind>>(initial.state, 11);
        const auto empty = tyr::planning::OwnedPlanBuilderView<Kind>(tyr::planning::replay_plan<Kind>(start, {}, *generator, *repository, *axioms), repository.get());
        EXPECT_TRUE(empty.empty());
        EXPECT_TRUE(urd::EqualTo<> {}(empty.get_start_node().pack(), start));
        EXPECT_EQ(empty.get_length(), 0);
        EXPECT_EQ(empty.get_cost(), 0);

        auto node = tyr::tests::owned_node(*repository, start);
        auto actions = urd::Store<urd::sequence<formalism::ActionRowView>> {};
        for (const auto name : { "first", "second" })
        {
            const auto applicable = tyr::tests::get_applicable_action_bindings(*generator, node, {});
            ASSERT_EQ(applicable.size(), 1);
            EXPECT_EQ(applicable.front().get_relation().get_name().str(), name);
            actions.push_back(applicable.front());
            node = tyr::tests::get_successor_node(*generator, node, actions.back(), *repository, *axioms);
        }
        const auto states_before_replay = ::tyr::planning::num_states(*repository);
        plan.emplace(tyr::planning::replay_plan<Kind>(start, actions, *generator, *repository, *axioms), repository.get());
        EXPECT_EQ(::tyr::planning::num_states(*repository), states_before_replay);
    }
    ASSERT_TRUE(plan);
    ASSERT_EQ(plan->get_length(), 2);
    EXPECT_EQ(plan->get_start_node().get_metric(), 11);
    EXPECT_EQ(urd::interned_view(plan->get_labeled_succ_nodes()[0].get_label()).get_relation().get_name().str(), "first");
    EXPECT_EQ(urd::interned_view(plan->get_labeled_succ_nodes()[1].get_label()).get_relation().get_name().str(), "second");
    EXPECT_EQ(plan->get_labeled_succ_nodes()[0].get_node().get_metric(), 14);
    EXPECT_EQ(plan->get_labeled_succ_nodes()[1].get_node().get_metric(), 21);
    EXPECT_EQ(plan->get_cost(), 21);
    {
        auto goal = tyr::planning::ConjunctiveGoalStrategy<Kind>(*task);
        EXPECT_TRUE(goal.is_dynamic_goal_satisfied(tyr::tests::built_state(plan->get_start_node()), tyr::tests::built_state(plan->get_goal_node())));
    }
}

template<TaskKind Kind>
void expect_sequential_reconstruction(const tyr::planning::TaskPtr<Kind>& task)
{
    auto execution_context = ygg::ExecutionContext::create(1);
    auto axiom_evaluator = std::make_shared<tyr::planning::AxiomEvaluator<Kind>>(task, execution_context);
    auto repository = tyr::planning::StateRepository<Kind>::create(task);
    auto generator = std::make_shared<tyr::planning::SuccessorGenerator<Kind>>(task, execution_context);

    const auto start = tyr::tests::owned_initial_node(*repository, *axiom_evaluator);
    const auto successors = tyr::tests::get_labeled_successor_nodes(*generator, start, {}, *repository, *axiom_evaluator);
    const auto selected = find_successor(successors, "pick");
    ASSERT_NE(selected, successors.end());

    const auto g_value = tyr::planning::compute_successor_g_value(start.get_metric(), selected->get_node().get_metric(), CostMode::GENERAL);
    const auto final_node = tyr::tests::node_of_state<Kind>(tyr::tests::built_state(selected->get_node()), g_value);
    const auto default_node = SequentialSearchNode<Kind> { std::numeric_limits<ygg::float_t>::infinity(), urd::index<tyr::planning::State<Kind>>() };
    auto search_nodes = urd::Store<urd::sequence<SequentialSearchNode<Kind>>, urd::stable_refs> {};
    get_or_create_search_node(static_cast<ygg::uint_t>(tyr::tests::built_state(start).pack().index().value), search_nodes, default_node) =
        SequentialSearchNode<Kind> { start.get_metric(), urd::index<tyr::planning::State<Kind>>() };
    auto& final_search_node = get_or_create_search_node(static_cast<ygg::uint_t>(tyr::tests::built_state(final_node).pack().index().value), search_nodes, default_node);
    final_search_node = SequentialSearchNode<Kind> { g_value, tyr::tests::built_state(start).pack().index() };

    const auto num_states = ::tyr::planning::num_states(*repository);
    const auto plan = tyr::planning::OwnedPlanBuilderView<Kind>(tyr::planning::PlanReconstructionPolicy<tyr::planning::SequentialSearch>::extract_total_ordered_plan(final_search_node,
                                                                                                                           final_node.pack(),
                                                                                                                           search_nodes,
                                                                                                                           *repository,
                                                                                                                           *axiom_evaluator,
                                                                                                                           *generator,
                                                                                                                           CostMode::GENERAL),
                                                     repository.get());

    ASSERT_EQ(plan.get_length(), 1);
    EXPECT_EQ(::tyr::planning::num_states(*repository), num_states);
    EXPECT_EQ(&plan.get_start_node().context(), repository.get());
    EXPECT_EQ(tyr::tests::built_state(plan.get_start_node()).pack().index(), tyr::tests::built_state(start).pack().index());
    EXPECT_EQ(&plan.get_labeled_succ_nodes().front().get_node().context(), repository.get());
    EXPECT_EQ(tyr::tests::built_state(plan.get_labeled_succ_nodes().front().get_node()).pack().index(), tyr::tests::built_state(final_node).pack().index());
}

template<TaskKind Kind>
void expect_parallel_reconstruction(const tyr::planning::TaskPtr<Kind>& task)
{
    auto execution_context = ygg::ExecutionContext::create(1);
    auto axiom_evaluator = std::make_shared<tyr::planning::AxiomEvaluator<Kind>>(task, execution_context);
    auto caller_repository = tyr::planning::StateRepository<Kind>::create(task);
    auto caller_generator = std::make_shared<tyr::planning::SuccessorGenerator<Kind>>(task, execution_context);
    auto plan = std::optional<tyr::planning::OwnedPlanBuilderView<Kind>> {};
    auto first_repository = std::weak_ptr<tyr::planning::StateRepository<Kind>> {};
    auto second_repository = std::weak_ptr<tyr::planning::StateRepository<Kind>> {};

    {
        auto first_context = ygg::ExecutionContext::create(1);
        auto second_context = ygg::ExecutionContext::create(1);
        auto first_generator = caller_generator->make_worker(first_context);
        auto second_generator = caller_generator->make_worker(second_context);
        auto first_axiom_evaluator = axiom_evaluator->make_worker(first_context);
        auto second_axiom_evaluator = axiom_evaluator->make_worker(second_context);
        const auto first_repository_owner = caller_repository->make_worker();
        const auto second_repository_owner = caller_repository->make_worker();
        first_repository = first_repository_owner;
        second_repository = second_repository_owner;

        const auto start = tyr::tests::owned_initial_node(*first_repository_owner, *first_axiom_evaluator);
        const auto start_successors = tyr::tests::get_labeled_successor_nodes(*first_generator, start, {}, *first_repository_owner, *first_axiom_evaluator);
        const auto selected_middle = find_successor(start_successors, "pick");
        ASSERT_NE(selected_middle, start_successors.end());
        const auto middle_g_value = tyr::planning::compute_successor_g_value(start.get_metric(), selected_middle->get_node().get_metric(), CostMode::GENERAL);
        const auto middle_state = copy_state(tyr::tests::built_state(selected_middle->get_node()), *second_repository_owner, *second_axiom_evaluator);
        const auto middle = tyr::tests::node_of_state<Kind>(middle_state, middle_g_value);

        const auto middle_successors =
            tyr::tests::get_labeled_successor_nodes(*second_generator, middle, {}, *second_repository_owner, *second_axiom_evaluator);
        const auto selected_final = find_successor(middle_successors, "move", std::optional { middle_state.pack().index() });
        ASSERT_NE(selected_final, middle_successors.end());
        const auto final_g_value = tyr::planning::compute_successor_g_value(middle_g_value, selected_final->get_node().get_metric(), CostMode::GENERAL);
        const auto final_state = copy_state(tyr::tests::built_state(selected_final->get_node()), *first_repository_owner, *first_axiom_evaluator);

        const auto root = tyr::planning::WorkerStateIndex<Kind> { urd::index<tyr::planning::Worker>(0), tyr::tests::built_state(start).pack().index() };
        const auto middle_index = tyr::planning::WorkerStateIndex<Kind> { urd::index<tyr::planning::Worker>(1), middle_state.pack().index() };
        const auto final = tyr::planning::WorkerStateIndex<Kind> { urd::index<tyr::planning::Worker>(0), final_state.pack().index() };
        const auto no_parent = tyr::planning::WorkerStateIndex<Kind> { urd::index<tyr::planning::Worker>(), urd::index<tyr::planning::State<Kind>>() };
        const auto default_node = ParallelSearchNode<Kind> { std::numeric_limits<ygg::float_t>::infinity(), no_parent };
        auto first_search_nodes = urd::Store<urd::sequence<ParallelSearchNode<Kind>>, urd::stable_refs> {};
        auto second_search_nodes = urd::Store<urd::sequence<ParallelSearchNode<Kind>>, urd::stable_refs> {};
        get_or_create_search_node(static_cast<ygg::uint_t>(root.state.value), first_search_nodes, default_node) =
            ParallelSearchNode<Kind> { start.get_metric(), no_parent };
        get_or_create_search_node(static_cast<ygg::uint_t>(final.state.value), first_search_nodes, default_node) =
            ParallelSearchNode<Kind> { final_g_value, middle_index };
        get_or_create_search_node(static_cast<ygg::uint_t>(middle_index.state.value), second_search_nodes, default_node) =
            ParallelSearchNode<Kind> { middle_g_value, root };

        const auto workers = urd::Store<urd::sequence<tyr::planning::WorkerSearchSpaceView<Kind, ParallelSearchNode<Kind>>>> {
            { *first_repository_owner, *first_axiom_evaluator, *first_generator, first_search_nodes },
            { *second_repository_owner, *second_axiom_evaluator, *second_generator, second_search_nodes },
        };
        EXPECT_EQ(::tyr::planning::num_states(*caller_repository), 0);
        plan.emplace(tyr::planning::PlanReconstructionPolicy<tyr::planning::ParallelSearch>::extract_total_ordered_plan(
                         final,
                         std::span<const tyr::planning::WorkerSearchSpaceView<Kind, ParallelSearchNode<Kind>>>(workers),
                         *caller_repository,
                         *axiom_evaluator,
                         CostMode::GENERAL),
                     caller_repository.get());
        EXPECT_EQ(::tyr::planning::num_states(*caller_repository), 3);
    }

    ASSERT_TRUE(plan.has_value());
    ASSERT_EQ(plan->get_length(), 2);
    EXPECT_TRUE(first_repository.expired());
    EXPECT_TRUE(second_repository.expired());
    EXPECT_EQ(&plan->get_start_node().context(), caller_repository.get());
    EXPECT_EQ(&plan->get_labeled_succ_nodes()[0].get_node().context(), caller_repository.get());
    EXPECT_EQ(&plan->get_labeled_succ_nodes()[1].get_node().context(), caller_repository.get());
    EXPECT_EQ(urd::interned_view(plan->get_labeled_succ_nodes()[0].get_label()).get_relation().get_name().str(), "pick");
    EXPECT_EQ(urd::interned_view(plan->get_labeled_succ_nodes()[1].get_label()).get_relation().get_name().str(), "move");
}

template<TaskKind Kind>
void expect_plan_reconstruction(const tyr::planning::TaskPtr<Kind>& task)
{
    expect_sequential_reconstruction(task);
    expect_parallel_reconstruction(task);
}

template<TaskKind Kind>
void expect_parallel_lazy_gbfs(const tyr::planning::TaskPtr<Kind>& task, RepositoryMode state_repository_mode)
{
    auto execution_context = ygg::ExecutionContext::create(1);
    auto axiom_evaluator = std::make_shared<tyr::planning::AxiomEvaluator<Kind>>(task, execution_context);
    auto repository = make_repository(task, state_repository_mode);
    auto generator = std::make_shared<tyr::planning::SuccessorGenerator<Kind>>(task, execution_context);
    auto heuristic = tyr::planning::BlindHeuristic<Kind>::create();

    auto options = tyr::planning::gbfs_lazy::Options<Kind> {};
    options.num_search_workers = 2;

    const auto result = tyr::planning::gbfs_lazy::find_solution(*task, *repository, *axiom_evaluator, *generator, *heuristic, options);

    ASSERT_EQ(result.status, tyr::planning::SearchStatus::SOLVED);
    ASSERT_TRUE(result.plan);
    ASSERT_TRUE(result.goal_node);
    ASSERT_FALSE(result.plan.value().empty());

    const auto expect_caller_state = [&](const auto& node)
    {
        const auto state = tyr::tests::built_state(node);
        EXPECT_EQ(&state.context(), repository.get());
        EXPECT_EQ(::tyr::planning::share(*repository, ::tyr::planning::get_registered_state(*repository, state.pack().index())).pack(), state.pack());
    };
    expect_caller_state(result.plan.value().get_start_node());
    for (const auto& successor : result.plan.value().get_labeled_succ_nodes())
        expect_caller_state(successor.get_node());
    EXPECT_EQ(&result.goal_node.value().get_state().context(), repository.get());

    auto goal = tyr::planning::ConjunctiveGoalStrategy<Kind>(*task);
    EXPECT_TRUE(goal.is_dynamic_goal_satisfied(tyr::tests::built_state(result.plan.value().get_start_node()), result.goal_node.value().get_state().unpack()));

    options.num_search_workers = 0;
    EXPECT_THROW(tyr::planning::gbfs_lazy::find_solution(*task, *repository, *axiom_evaluator, *generator, *heuristic, options), std::invalid_argument);

    if constexpr (std::numeric_limits<size_t>::max() > std::numeric_limits<ygg::uint_t>::max())
    {
        options.num_search_workers = static_cast<size_t>(std::numeric_limits<ygg::uint_t>::max()) + 1;
        EXPECT_THROW(tyr::planning::gbfs_lazy::find_solution(*task, *repository, *axiom_evaluator, *generator, *heuristic, options), std::invalid_argument);
    }

    options.num_search_workers = 2;
    options.pruning_strategy = tyr::planning::PruningStrategy<Kind>::create();
    EXPECT_EQ(tyr::planning::gbfs_lazy::find_solution(*task, *repository, *axiom_evaluator, *generator, *heuristic, options).status,
              tyr::planning::SearchStatus::SOLVED);

    options.pruning_strategy = nullptr;
    options.goal_strategy = tyr::planning::ConjunctiveGoalStrategy<Kind>::create(*task);
    EXPECT_EQ(tyr::planning::gbfs_lazy::find_solution(*task, *repository, *axiom_evaluator, *generator, *heuristic, options).status,
              tyr::planning::SearchStatus::SOLVED);

    options.goal_strategy = nullptr;
    options.pruning_strategy = std::make_shared<UnsupportedParallelPruningStrategy<Kind>>();
    EXPECT_THROW(tyr::planning::gbfs_lazy::find_solution(*task, *repository, *axiom_evaluator, *generator, *heuristic, options), std::invalid_argument);

    options.pruning_strategy = nullptr;
    options.goal_strategy = std::make_shared<UnsupportedParallelGoalStrategy<Kind>>();
    EXPECT_THROW(tyr::planning::gbfs_lazy::find_solution(*task, *repository, *axiom_evaluator, *generator, *heuristic, options), std::invalid_argument);

    options.goal_strategy = nullptr;
    options.search_budget.max_num_states = 0;
    EXPECT_EQ(tyr::planning::gbfs_lazy::find_solution(*task, *repository, *axiom_evaluator, *generator, *heuristic, options).status,
              tyr::planning::SearchStatus::OUT_OF_STATES);

    options.search_budget.max_num_states = 1;
    EXPECT_EQ(tyr::planning::gbfs_lazy::find_solution(*task, *repository, *axiom_evaluator, *generator, *heuristic, options).status,
              tyr::planning::SearchStatus::OUT_OF_STATES);

    options.search_budget.max_num_states = std::nullopt;
    options.search_budget.max_time = std::chrono::steady_clock::duration::zero();
    EXPECT_EQ(tyr::planning::gbfs_lazy::find_solution(*task, *repository, *axiom_evaluator, *generator, *heuristic, options).status,
              tyr::planning::SearchStatus::OUT_OF_TIME);
}

template<TaskKind Kind>
void expect_parallel_brfs(const tyr::planning::TaskPtr<Kind>& task, RepositoryMode state_repository_mode)
{
    auto execution_context = ygg::ExecutionContext::create(1);
    auto axiom_evaluator = std::make_shared<tyr::planning::AxiomEvaluator<Kind>>(task, execution_context);
    auto repository = make_repository(task, state_repository_mode);
    auto generator = std::make_shared<tyr::planning::SuccessorGenerator<Kind>>(task, execution_context);

    auto options = tyr::planning::brfs::Options<Kind> {};
    options.num_search_workers = 2;

    const auto result = tyr::planning::brfs::find_solution(*task, *repository, *axiom_evaluator, *generator, options);

    ASSERT_EQ(result.status, tyr::planning::SearchStatus::SOLVED);
    ASSERT_TRUE(result.plan);
    ASSERT_TRUE(result.goal_node);
    ASSERT_FALSE(result.plan.value().empty());
    EXPECT_EQ(result.plan.value().get_cost(), result.plan.value().get_length());
    EXPECT_EQ(&result.plan.value().get_start_node().context(), repository.get());
    for (const auto& successor : result.plan.value().get_labeled_succ_nodes())
        EXPECT_EQ(&successor.get_node().context(), repository.get());
    EXPECT_EQ(&result.goal_node.value().get_state().context(), repository.get());

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
    EXPECT_THROW(tyr::planning::brfs::find_solution(*task, *repository, *axiom_evaluator, *generator, options), std::invalid_argument);

    options.num_search_workers = 2;
    options.search_budget.max_num_states = 0;
    EXPECT_EQ(tyr::planning::brfs::find_solution(*task, *repository, *axiom_evaluator, *generator, options).status, tyr::planning::SearchStatus::OUT_OF_STATES);

    options.search_budget.max_num_states = 1;
    EXPECT_EQ(tyr::planning::brfs::find_solution(*task, *repository, *axiom_evaluator, *generator, options).status, tyr::planning::SearchStatus::OUT_OF_STATES);

    options.search_budget.max_num_states = std::nullopt;
    options.search_budget.max_time = std::chrono::steady_clock::duration::zero();
    EXPECT_EQ(tyr::planning::brfs::find_solution(*task, *repository, *axiom_evaluator, *generator, options).status, tyr::planning::SearchStatus::OUT_OF_TIME);
}

template<TaskKind Kind>
void expect_brfs_events(const tyr::planning::TaskPtr<Kind>& task, RepositoryMode state_repository_mode, size_t num_workers)
{
    auto execution_context = ygg::ExecutionContext::create(1);
    auto axiom_evaluator = std::make_shared<tyr::planning::AxiomEvaluator<Kind>>(task, execution_context);
    auto repository = make_repository(task, state_repository_mode);
    auto generator = std::make_shared<tyr::planning::SuccessorGenerator<Kind>>(task, execution_context);
    auto event_handler = std::make_shared<CountingBrFSEventHandler<Kind>>();

    auto options = tyr::planning::brfs::Options<Kind> {};
    options.event_handler = event_handler;
    options.num_search_workers = num_workers;

    const auto result = tyr::planning::brfs::find_solution(*task, *repository, *axiom_evaluator, *generator, options);

    ASSERT_EQ(result.status, tyr::planning::SearchStatus::SOLVED);
    ASSERT_TRUE(result.plan);
    EXPECT_EQ(event_handler->num_starts, 1);
    EXPECT_EQ(event_handler->num_workers_created, num_workers);
    EXPECT_EQ(event_handler->solved_plan_length, result.plan.value().get_length());
    ASSERT_TRUE(event_handler->end_status);
    EXPECT_EQ(event_handler->end_status.value(), result.status);
    ASSERT_TRUE(event_handler->end_statistics);
    EXPECT_EQ(event_handler->end_statistics.value().get_num_generated_successors(), result.statistics.get_num_generated_successors());
    EXPECT_EQ(event_handler->end_statistics.value().get_num_expanded(), result.statistics.get_num_expanded());

    auto worker_totals = tyr::planning::Statistics {};
    for (const auto& statistics : event_handler->worker_statistics)
        worker_totals.add(statistics);
    EXPECT_EQ(worker_totals.get_num_generated_successors(), result.statistics.get_num_generated_successors());
    EXPECT_EQ(worker_totals.get_num_expanded(), result.statistics.get_num_expanded());
    EXPECT_EQ(worker_totals.get_num_pruned(), result.statistics.get_num_pruned());

    ASSERT_EQ(event_handler->finished_layers.size(), event_handler->layer_statistics.size());
    ASSERT_FALSE(event_handler->finished_layers.empty());
    EXPECT_EQ(event_handler->finished_layers.front(), 0);
    EXPECT_TRUE(std::ranges::is_sorted(event_handler->finished_layers));
    for (size_t i = 1; i < event_handler->layer_statistics.size(); ++i)
    {
        EXPECT_EQ(event_handler->finished_layers[i], event_handler->finished_layers[i - 1] + 1);
        EXPECT_LE(event_handler->layer_statistics[i - 1].get_num_generated_successors(), event_handler->layer_statistics[i].get_num_generated_successors());
        EXPECT_LE(event_handler->layer_statistics[i - 1].get_num_expanded(), event_handler->layer_statistics[i].get_num_expanded());
    }
}

template<TaskKind Kind>
void expect_aggregated_worker_statistics(const tyr::planning::SearchResult<Kind>& result, size_t num_workers)
{
    ASSERT_EQ(result.worker_statistics.size(), num_workers);
    auto totals = tyr::planning::Statistics {};
    for (const auto& worker : result.worker_statistics)
    {
        totals.add(worker);
        EXPECT_EQ(worker.get_search_time(), result.statistics.get_search_time());
    }
    EXPECT_EQ(totals.get_num_generated_successors(), result.statistics.get_num_generated_successors());
    EXPECT_EQ(totals.get_num_expanded(), result.statistics.get_num_expanded());
    EXPECT_EQ(totals.get_num_deadends(), result.statistics.get_num_deadends());
    EXPECT_EQ(totals.get_num_pruned(), result.statistics.get_num_pruned());
    EXPECT_EQ(totals.get_num_generated_candidates(), result.statistics.get_num_generated_candidates());
    EXPECT_EQ(totals.get_num_transferred_candidates(), result.statistics.get_num_transferred_candidates());
    EXPECT_EQ(totals.get_idle_time(), result.statistics.get_idle_time());
}

template<TaskKind Kind>
void expect_parallel_iw(const tyr::planning::TaskPtr<Kind>& task, RepositoryMode state_repository_mode)
{
    auto execution_context = ygg::ExecutionContext::create(1);
    auto axiom_evaluator = std::make_shared<tyr::planning::AxiomEvaluator<Kind>>(task, execution_context);
    auto repository = make_repository(task, state_repository_mode);
    auto generator = std::make_shared<tyr::planning::SuccessorGenerator<Kind>>(task, execution_context);
    auto brfs_solver = tyr::planning::brfs::Solver<Kind> { task, repository, axiom_evaluator, generator };
    brfs_solver.options.num_search_workers = 2;

    const auto iw_result = tyr::planning::iw::find_solution(brfs_solver, 2);
    ASSERT_EQ(iw_result.status, tyr::planning::SearchStatus::SOLVED);
    ASSERT_TRUE(iw_result.plan);
    EXPECT_EQ(&iw_result.plan.value().get_start_node().context(), repository.get());
    expect_aggregated_worker_statistics(iw_result, 2);
}

template<TaskKind Kind>
void expect_parallel_siw_outer_orchestration(const tyr::planning::TaskPtr<Kind>& task)
{
    auto execution_context = ygg::ExecutionContext::create(1);
    auto axiom_evaluator = std::make_shared<tyr::planning::AxiomEvaluator<Kind>>(task, execution_context);
    auto repository = tyr::planning::StateRepository<Kind>::create(task, true);
    auto generator = std::make_shared<tyr::planning::SuccessorGenerator<Kind>>(task, execution_context);
    auto brfs_solver = tyr::planning::brfs::Solver<Kind> { task, repository, axiom_evaluator, generator };
    brfs_solver.options.num_search_workers = 2;

    auto iw_solver = tyr::planning::iw::Solver<Kind> { brfs_solver, 2 };
    auto event_handler = tyr::planning::siw::DefaultEventHandler<Kind>::create();
    auto options = tyr::planning::siw::Options<Kind> {};
    options.event_handler = event_handler;
    const auto siw_result = tyr::planning::siw::find_solution(iw_solver, options);
    ASSERT_EQ(siw_result.status, tyr::planning::SearchStatus::SOLVED);
    ASSERT_TRUE(siw_result.plan);
    EXPECT_GT(event_handler->get_statistics().get_num_solved_subsearches(), 1);
    EXPECT_EQ(&siw_result.plan.value().get_start_node().context(), repository.get());
    for (const auto& successor : siw_result.plan.value().get_labeled_succ_nodes())
        EXPECT_EQ(&successor.get_node().context(), repository.get());
    expect_aggregated_worker_statistics(siw_result, 2);
}

template<TaskKind Kind>
void expect_parallel_astar(const tyr::planning::TaskPtr<Kind>& task,
                           RepositoryMode state_repository_mode,
                           tyr::planning::astar_eager::ParallelSearchMode parallel_search_mode)
{
    auto execution_context = ygg::ExecutionContext::create(1);
    auto axiom_evaluator = std::make_shared<tyr::planning::AxiomEvaluator<Kind>>(task, execution_context);
    auto repository = make_repository(task, state_repository_mode);
    auto generator = std::make_shared<tyr::planning::SuccessorGenerator<Kind>>(task, execution_context);
    auto heuristic = tyr::planning::BlindHeuristic<Kind>::create();

    auto options = tyr::planning::astar_eager::Options<Kind> {};
    options.num_search_workers = 2;

    options.parallel_search_mode = parallel_search_mode;
    const auto result = tyr::planning::astar_eager::find_solution(*task, *repository, *axiom_evaluator, *generator, *heuristic, options);

    ASSERT_EQ(result.status, tyr::planning::SearchStatus::SOLVED);
    ASSERT_TRUE(result.plan);
    ASSERT_TRUE(result.goal_node);
    ASSERT_FALSE(result.plan.value().empty());
    EXPECT_EQ(&result.plan.value().get_start_node().context(), repository.get());
    for (const auto& successor : result.plan.value().get_labeled_succ_nodes())
        EXPECT_EQ(&successor.get_node().context(), repository.get());
    EXPECT_EQ(&result.goal_node.value().get_state().context(), repository.get());

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
    if (state_repository_mode == RepositoryMode::SHARED)
    {
        EXPECT_EQ(result.worker_statistics[0].get_num_registered_states(), (result.statistics.get_num_registered_states() + 1) / 2);
        EXPECT_EQ(result.worker_statistics[1].get_num_registered_states(), result.statistics.get_num_registered_states() / 2);
        EXPECT_EQ(result.worker_statistics[0].get_state_storage_memory_usage(), result.statistics.get_state_storage_memory_usage());
        EXPECT_EQ(result.worker_statistics[1].get_state_storage_memory_usage(), 0);
    }
}

template<TaskKind Kind>
void expect_lazy_gbfs_worker_events(const tyr::planning::TaskPtr<Kind>& task, RepositoryMode state_repository_mode, size_t num_search_workers)
{
    auto execution_context = ygg::ExecutionContext::create(1);
    auto axiom_evaluator = std::make_shared<tyr::planning::AxiomEvaluator<Kind>>(task, execution_context);
    auto repository = make_repository(task, state_repository_mode);
    auto generator = std::make_shared<tyr::planning::SuccessorGenerator<Kind>>(task, execution_context);
    auto heuristic = tyr::planning::BlindHeuristic<Kind>::create();
    auto event_handler = std::make_shared<CountingGBFSEventHandler<Kind>>(repository.get());

    auto options = tyr::planning::gbfs_lazy::Options<Kind> {};
    options.event_handler = event_handler;
    options.num_search_workers = num_search_workers;

    const auto result = tyr::planning::gbfs_lazy::find_solution(*task, *repository, *axiom_evaluator, *generator, *heuristic, options);

    ASSERT_EQ(result.status, tyr::planning::SearchStatus::SOLVED);
    ASSERT_TRUE(result.plan);
    EXPECT_EQ(event_handler->num_workers_created, num_search_workers);

    const auto totals = event_handler->totals();
    EXPECT_EQ(totals.get_num_generated_successors(), result.statistics.get_num_generated_successors());
    EXPECT_EQ(totals.get_num_expanded(), result.statistics.get_num_expanded());
    EXPECT_EQ(totals.get_num_pruned(), result.statistics.get_num_pruned());

    ASSERT_TRUE(event_handler->end_status);
    EXPECT_EQ(event_handler->end_status.value(), result.status);
    ASSERT_TRUE(event_handler->end_statistics);
    EXPECT_EQ(event_handler->end_statistics.value().get_num_generated_successors(), result.statistics.get_num_generated_successors());
    EXPECT_EQ(event_handler->end_statistics.value().get_num_expanded(), result.statistics.get_num_expanded());
    EXPECT_EQ(event_handler->end_statistics.value().get_num_deadends(), result.statistics.get_num_deadends());
    EXPECT_EQ(event_handler->end_statistics.value().get_num_pruned(), result.statistics.get_num_pruned());
    EXPECT_EQ(event_handler->end_statistics.value().get_search_time(), result.statistics.get_search_time());

    EXPECT_TRUE(event_handler->plan_materialized);
    EXPECT_EQ(event_handler->solved_plan_length, result.plan.value().get_length());

    ASSERT_EQ(result.worker_statistics.size(), num_search_workers);
    auto worker_totals = tyr::planning::Statistics {};
    auto num_registered_states = uint64_t { 0 };
    auto state_storage_memory_usage = size_t { 0 };
    for (const auto& statistics : result.worker_statistics)
    {
        worker_totals.add(statistics);
        num_registered_states += statistics.get_num_registered_states();
        state_storage_memory_usage += statistics.get_state_storage_memory_usage();
        EXPECT_LE(statistics.get_idle_time(), std::chrono::duration_cast<std::chrono::nanoseconds>(statistics.get_search_time()));
    }
    EXPECT_EQ(worker_totals.get_num_generated_successors(), result.statistics.get_num_generated_successors());
    EXPECT_EQ(worker_totals.get_num_expanded(), result.statistics.get_num_expanded());
    EXPECT_EQ(worker_totals.get_num_deadends(), result.statistics.get_num_deadends());
    EXPECT_EQ(worker_totals.get_num_pruned(), result.statistics.get_num_pruned());
    EXPECT_EQ(worker_totals.get_idle_time(), result.statistics.get_idle_time());
    EXPECT_EQ(num_registered_states, result.statistics.get_num_registered_states());
    EXPECT_EQ(state_storage_memory_usage, result.statistics.get_state_storage_memory_usage());
    if (num_search_workers > 1 && state_repository_mode == RepositoryMode::SHARED)
    {
        EXPECT_EQ(result.worker_statistics[0].get_num_registered_states(), (result.statistics.get_num_registered_states() + 1) / 2);
        EXPECT_EQ(result.worker_statistics[1].get_num_registered_states(), result.statistics.get_num_registered_states() / 2);
        EXPECT_EQ(result.worker_statistics[0].get_state_storage_memory_usage(), result.statistics.get_state_storage_memory_usage());
        EXPECT_EQ(result.worker_statistics[1].get_state_storage_memory_usage(), 0);
    }
}

template<TaskKind Kind>
std::optional<uint64_t> find_weighted_astar_seed(tyr::planning::SuccessorGenerator<Kind>& generator,
                                                 tyr::planning::StateRepository<Kind>& state_repository,
                                                 tyr::planning::AxiomEvaluator<Kind>& axiom_evaluator)
{
    const auto start = tyr::tests::owned_initial_node(state_repository, axiom_evaluator);
    auto direct = std::optional<decltype(::tyr::planning::get_state_builder(state_repository))> {};
    auto via = std::optional<decltype(::tyr::planning::get_state_builder(state_repository))> {};
    for (const auto action : tyr::tests::get_applicable_action_bindings(generator, start, {}))
    {
        auto state = ::tyr::planning::get_state_builder(state_repository);
        auto delta = tyr::planning::StateDelta {};
        const auto auxiliary_value = generator.generate_successor_state(start, action, *state, delta);
        if (auxiliary_value == 100)
            direct.emplace(std::move(state));
        else if (auxiliary_value == 1)
            via.emplace(std::move(state));
    }
    if (!direct || !via)
        return std::nullopt;

    for (uint64_t seed = 0; seed < 1024; ++seed)
    {
        const auto hash = tyr::planning::DistHash<Kind, tyr::planning::RandomDistHashTag>(seed);
        const auto start_owner = hash.owner(tyr::tests::built_state(start), 2);
        if (hash.owner(urd::resolve(**direct, state_repository), 2) == start_owner && hash.owner(urd::resolve(**via, state_repository), 2) != start_owner)
            return seed;
    }
    return std::nullopt;
}

template<TaskKind Kind>
void expect_parallel_astar_keeps_searching_after_first_goal(const tyr::planning::TaskPtr<Kind>& task,
                                                            RepositoryMode state_repository_mode,
                                                            tyr::planning::astar_eager::ParallelSearchMode mode)
{
    auto execution_context = ygg::ExecutionContext::create(1);
    auto axiom_evaluator = std::make_shared<tyr::planning::AxiomEvaluator<Kind>>(task, execution_context);
    auto repository = make_repository(task, state_repository_mode);
    auto generator = std::make_shared<tyr::planning::SuccessorGenerator<Kind>>(task, execution_context);
    auto heuristic = tyr::planning::BlindHeuristic<Kind>::create();
    const auto seed = state_repository_mode == RepositoryMode::HASH_DISTRIBUTED ? find_weighted_astar_seed(*generator, *repository, *axiom_evaluator) :
                                                                                  std::optional<uint64_t> { 0 };
    ASSERT_TRUE(seed);

    auto event_handler = std::make_shared<GatedAStarEventHandler<Kind>>();
    auto options = tyr::planning::astar_eager::Options<Kind> {};
    options.event_handler = event_handler;
    options.num_search_workers = 2;

    options.dist_hash_mode = tyr::planning::DistHashMode::RANDOM;
    options.parallel_search_mode = mode;
    options.random_seed = seed.value();
    const auto result = tyr::planning::astar_eager::find_solution(*task, *repository, *axiom_evaluator, *generator, *heuristic, options);

    ASSERT_EQ(result.status, tyr::planning::SearchStatus::SOLVED);
    ASSERT_TRUE(result.plan);
    ASSERT_TRUE(result.goal_node);
    EXPECT_TRUE(event_handler->gate.expensive_goal_seen.load(std::memory_order_relaxed));
    EXPECT_EQ(event_handler->gate.num_expanded_goals.load(std::memory_order_relaxed), 0);
    EXPECT_EQ(result.plan.value().get_cost(), 2);
    EXPECT_EQ(result.plan.value().get_length(), 2);
    ASSERT_EQ(result.worker_statistics.size(), 2);
    for (const auto& statistics : result.worker_statistics)
    {
        EXPECT_GT(statistics.get_num_registered_states(), 0);
    }
    if (state_repository_mode == RepositoryMode::HASH_DISTRIBUTED)
        EXPECT_GT(result.worker_statistics[1].get_state_storage_memory_usage(), 0);
    else
        EXPECT_EQ(result.worker_statistics[1].get_state_storage_memory_usage(), 0);
    EXPECT_EQ(&result.plan.value().get_start_node().context(), repository.get());
    for (const auto& successor : result.plan.value().get_labeled_succ_nodes())
        EXPECT_EQ(&successor.get_node().context(), repository.get());
    EXPECT_EQ(&result.goal_node.value().get_state().context(), repository.get());
}

template<TaskKind Kind>
void expect_parallel_astar_coordinates_f_layers(const tyr::planning::TaskPtr<Kind>& task,
                                                RepositoryMode state_repository_mode,
                                                tyr::planning::astar_eager::ParallelSearchMode mode,
                                                bool expect_higher_expansion)
{
    auto execution_context = ygg::ExecutionContext::create(1);
    auto axiom_evaluator = std::make_shared<tyr::planning::AxiomEvaluator<Kind>>(task, execution_context);
    auto repository = make_repository(task, state_repository_mode);
    auto generator = std::make_shared<tyr::planning::SuccessorGenerator<Kind>>(task, execution_context);
    auto heuristic = tyr::planning::BlindHeuristic<Kind>::create();
    const auto seed = state_repository_mode == RepositoryMode::HASH_DISTRIBUTED ? find_weighted_astar_seed(*generator, *repository, *axiom_evaluator) :
                                                                                  std::optional<uint64_t> { 0 };
    ASSERT_TRUE(seed);

    auto event_handler = std::make_shared<LayeredAStarEventHandler<Kind>>();
    auto options = tyr::planning::astar_eager::Options<Kind> {};
    options.event_handler = event_handler;
    options.goal_strategy = tyr::planning::ExhaustiveGoalStrategy<Kind>::create();
    options.num_search_workers = 2;

    options.dist_hash_mode = tyr::planning::DistHashMode::RANDOM;
    options.parallel_search_mode = mode;
    options.random_seed = seed.value();
    const auto result = tyr::planning::astar_eager::find_solution(*task, *repository, *axiom_evaluator, *generator, *heuristic, options);

    EXPECT_EQ(result.status, tyr::planning::SearchStatus::EXHAUSTED);
    EXPECT_EQ(event_handler->gate.higher_expanded.load(std::memory_order_relaxed), expect_higher_expansion);
    EXPECT_EQ(result.statistics.get_num_expanded(), expect_higher_expansion ? 4 : 3);
    if (mode == tyr::planning::astar_eager::ParallelSearchMode::SYNCHRONOUS)
        EXPECT_EQ(event_handler->finished_f_layers, (urd::Store<urd::sequence<ygg::float_t>> { 0, 1, 2 }));
    else
        EXPECT_TRUE(event_handler->finished_f_layers.empty());
}

template<TaskKind Kind>
void expect_synchronous_parallel_astar_stops_all_workers(const tyr::planning::TaskPtr<Kind>& task)
{
    auto execution_context = ygg::ExecutionContext::create(1);
    auto axiom_evaluator = std::make_shared<tyr::planning::AxiomEvaluator<Kind>>(task, execution_context);
    auto repository = tyr::planning::StateRepository<Kind>::create(task);
    auto generator = std::make_shared<tyr::planning::SuccessorGenerator<Kind>>(task, execution_context);
    auto heuristic = tyr::planning::BlindHeuristic<Kind>::create();
    const auto seed = find_weighted_astar_seed(*generator, *repository, *axiom_evaluator);
    ASSERT_TRUE(seed);
    auto options = tyr::planning::astar_eager::Options<Kind> {};
    options.event_handler = std::make_shared<LayeredAStarEventHandler<Kind>>();
    options.goal_strategy = tyr::planning::ExhaustiveGoalStrategy<Kind>::create();
    options.num_search_workers = 2;

    options.dist_hash_mode = tyr::planning::DistHashMode::RANDOM;
    options.parallel_search_mode = tyr::planning::astar_eager::ParallelSearchMode::SYNCHRONOUS;
    options.random_seed = seed.value();
    options.search_budget.max_time = std::chrono::milliseconds(50);
    EXPECT_EQ(tyr::planning::astar_eager::find_solution(*task, *repository, *axiom_evaluator, *generator, *heuristic, options).status,
              tyr::planning::SearchStatus::OUT_OF_TIME);

    options.search_budget.max_time = std::nullopt;
    options.event_handler = std::make_shared<ThrowingAStarEventHandler<Kind>>();
    EXPECT_THROW(tyr::planning::astar_eager::find_solution(*task, *repository, *axiom_evaluator, *generator, *heuristic, options), std::runtime_error);
}

template<TaskKind Kind>
void expect_parallel_lazy_gbfs_exhaustion(const tyr::planning::TaskPtr<Kind>& task, RepositoryMode state_repository_mode)
{
    auto execution_context = ygg::ExecutionContext::create(1);
    auto axiom_evaluator = std::make_shared<tyr::planning::AxiomEvaluator<Kind>>(task, execution_context);
    auto repository = make_repository(task, state_repository_mode);
    auto generator = std::make_shared<tyr::planning::SuccessorGenerator<Kind>>(task, execution_context);
    auto heuristic = tyr::planning::BlindHeuristic<Kind>::create();
    auto options = tyr::planning::gbfs_lazy::Options<Kind> {};
    options.num_search_workers = 2;

    const auto result = tyr::planning::gbfs_lazy::find_solution(*task, *repository, *axiom_evaluator, *generator, *heuristic, options);
    EXPECT_EQ(result.status, tyr::planning::SearchStatus::EXHAUSTED);
    EXPECT_GT(result.statistics.get_num_expanded(), 0);
    EXPECT_GT(result.statistics.get_num_generated_successors(), 0);
}

template<TaskKind Kind>
void expect_parallel_brfs_exhaustion(const tyr::planning::TaskPtr<Kind>& task, RepositoryMode state_repository_mode)
{
    constexpr auto kind_name = std::same_as<Kind, GroundTag> ? "ground" : "lifted";
    SCOPED_TRACE(kind_name);
    SCOPED_TRACE(state_repository_mode == RepositoryMode::HASH_DISTRIBUTED ? "hash-distributed" : "shared");

    auto sequential_execution_context = ygg::ExecutionContext::create(1);
    auto sequential_axiom_evaluator = std::make_shared<tyr::planning::AxiomEvaluator<Kind>>(task, sequential_execution_context);
    auto sequential_repository = tyr::planning::StateRepository<Kind>::create(task);
    auto sequential_generator = std::make_shared<tyr::planning::SuccessorGenerator<Kind>>(task, sequential_execution_context);
    auto sequential_options = tyr::planning::brfs::Options<Kind> {};
    sequential_options.goal_strategy = tyr::planning::ExhaustiveGoalStrategy<Kind>::create();
    const auto sequential_result =
        tyr::planning::brfs::find_solution(*task, *sequential_repository, *sequential_axiom_evaluator, *sequential_generator, sequential_options);

    auto execution_context = ygg::ExecutionContext::create(1);
    auto axiom_evaluator = std::make_shared<tyr::planning::AxiomEvaluator<Kind>>(task, execution_context);
    auto repository = make_repository(task, state_repository_mode);
    auto generator = std::make_shared<tyr::planning::SuccessorGenerator<Kind>>(task, execution_context);
    constexpr auto seed = uint64_t { 0 };
    auto guard = std::make_shared<OwnerAccessGuard<Kind>>(state_repository_mode, seed);
    auto options = tyr::planning::brfs::Options<Kind> {};
    options.event_handler = std::make_shared<GuardedBrFSEventHandler<Kind>>(guard);
    options.pruning_strategy = std::make_shared<GuardedPruningStrategy<Kind>>(guard);
    options.goal_strategy = tyr::planning::ExhaustiveGoalStrategy<Kind>::create();
    options.num_search_workers = OwnerAccessGuard<Kind>::num_workers;

    options.dist_hash_mode = tyr::planning::DistHashMode::RANDOM;
    options.random_seed = seed;

    const auto result = tyr::planning::brfs::find_solution(*task, *repository, *axiom_evaluator, *generator, options);
    guard->expect_valid();

    EXPECT_EQ(sequential_result.status, tyr::planning::SearchStatus::EXHAUSTED);
    EXPECT_EQ(result.status, tyr::planning::SearchStatus::EXHAUSTED);
    EXPECT_EQ(result.statistics.get_num_registered_states(), sequential_result.statistics.get_num_registered_states());
    EXPECT_EQ(result.statistics.get_num_expanded(), sequential_result.statistics.get_num_expanded());
    EXPECT_EQ(result.statistics.get_num_generated_successors(), sequential_result.statistics.get_num_generated_successors());
    EXPECT_EQ(result.statistics.get_num_pruned(), 0);
    EXPECT_EQ(result.worker_statistics.size(), OwnerAccessGuard<Kind>::num_workers);
    EXPECT_GT(sequential_result.statistics.get_num_generated_candidates(), 0);
    EXPECT_EQ(sequential_result.statistics.get_num_transferred_candidates(), 0);
    EXPECT_DOUBLE_EQ(sequential_result.statistics.get_communication_overhead(), 0.0);
    EXPECT_GT(result.statistics.get_num_generated_candidates(), 0);
    EXPECT_GT(result.statistics.get_num_transferred_candidates(), 0);
    EXPECT_LE(result.statistics.get_num_transferred_candidates(), result.statistics.get_num_generated_candidates());
    EXPECT_DOUBLE_EQ(result.statistics.get_communication_overhead(),
                     static_cast<double>(result.statistics.get_num_transferred_candidates())
                         / static_cast<double>(result.statistics.get_num_generated_candidates()));

    auto worker_totals = tyr::planning::Statistics {};
    for (const auto& statistics : result.worker_statistics)
        worker_totals.add(statistics);
    EXPECT_EQ(worker_totals.get_num_generated_candidates(), result.statistics.get_num_generated_candidates());
    EXPECT_EQ(worker_totals.get_num_transferred_candidates(), result.statistics.get_num_transferred_candidates());
}

}

TEST(TyrPlanningPlanReconstructionTest, ReplaysKnownActionsWithMetricsAndOwnedPackedStates)
{
    auto lifted = tyr::planning::Task<LiftedTag>::create(formalism::Parser(task_fixture("action-replay/domain.pddl"))
                                                             .parse_task(task_fixture("action-replay/action-replay-problem.pddl")));
    auto execution = ygg::ExecutionContext::create(1);
    const auto ground = lifted->instantiate_ground_task(*execution).task;
    ASSERT_TRUE(ground);
    expect_action_sequence_replay(ground);
    expect_action_sequence_replay(lifted);
}

TEST(TyrPlanningPlanReconstructionTest, ReconstructsGroundAndLiftedWorkerTrajectories)
{
    const auto root = std::filesystem::path(BENCHMARKS_DIR);
    auto lifted_task = tyr::planning::Task<LiftedTag>::create(
        make_test_parser(root / "classical/tests/gripper/domain.pddl").parse_task(root / "classical/tests/gripper/test-1.pddl"));
    auto grounding_context = ygg::ExecutionContext::create(1);
    auto ground_task = lifted_task->instantiate_ground_task(*grounding_context).task;
    ASSERT_NE(ground_task, nullptr);

    expect_plan_reconstruction(ground_task);
    expect_plan_reconstruction(lifted_task);
}

TEST(TyrPlanningPlanReconstructionTest, ParallelLazyGBFSMaterializesGroundAndLiftedPlansInCallerRepository)
{
    const auto root = std::filesystem::path(BENCHMARKS_DIR);
    auto lifted_task = tyr::planning::Task<LiftedTag>::create(
        make_test_parser(root / "classical/tests/gripper/domain.pddl").parse_task(root / "classical/tests/gripper/test-1.pddl"));
    auto grounding_context = ygg::ExecutionContext::create(1);
    auto ground_task = lifted_task->instantiate_ground_task(*grounding_context).task;
    ASSERT_NE(ground_task, nullptr);

    for (const auto mode : { RepositoryMode::HASH_DISTRIBUTED, RepositoryMode::SHARED })
    {
        expect_parallel_lazy_gbfs(ground_task, mode);
        expect_parallel_lazy_gbfs(lifted_task, mode);
    }
}

TEST(TyrPlanningPlanReconstructionTest, ParallelAStarSupportsRepositoryAndFLayerModes)
{
    const auto root = std::filesystem::path(BENCHMARKS_DIR);
    auto lifted_task = tyr::planning::Task<LiftedTag>::create(
        make_test_parser(root / "classical/tests/gripper/domain.pddl").parse_task(root / "classical/tests/gripper/test-1.pddl"));
    auto grounding_context = ygg::ExecutionContext::create(1);
    auto ground_task = lifted_task->instantiate_ground_task(*grounding_context).task;
    ASSERT_NE(ground_task, nullptr);

    for (const auto state_repository_mode : { RepositoryMode::HASH_DISTRIBUTED, RepositoryMode::SHARED })
    {
        for (const auto parallel_search_mode :
             { tyr::planning::astar_eager::ParallelSearchMode::SYNCHRONOUS, tyr::planning::astar_eager::ParallelSearchMode::ASYNCHRONOUS })
        {
            expect_parallel_astar(ground_task, state_repository_mode, parallel_search_mode);
            expect_parallel_astar(lifted_task, state_repository_mode, parallel_search_mode);
        }
    }
}

TEST(TyrPlanningPlanReconstructionTest, ParallelBrFSSupportsRepositoryModes)
{
    const auto root = std::filesystem::path(BENCHMARKS_DIR);
    auto lifted_task = tyr::planning::Task<LiftedTag>::create(
        make_test_parser(root / "classical/tests/gripper/domain.pddl").parse_task(root / "classical/tests/gripper/test-1.pddl"));
    auto grounding_context = ygg::ExecutionContext::create(1);
    auto ground_task = lifted_task->instantiate_ground_task(*grounding_context).task;
    ASSERT_NE(ground_task, nullptr);

    for (const auto mode : { RepositoryMode::HASH_DISTRIBUTED, RepositoryMode::SHARED })
    {
        expect_parallel_brfs(ground_task, mode);
        expect_parallel_brfs(lifted_task, mode);
    }
}

TEST(TyrPlanningPlanReconstructionTest, BrFSEmitsWorkerAndGlobalLayerEvents)
{
    const auto root = std::filesystem::path(BENCHMARKS_DIR);
    auto lifted_task = tyr::planning::Task<LiftedTag>::create(
        make_test_parser(root / "classical/tests/gripper/domain.pddl").parse_task(root / "classical/tests/gripper/test-1.pddl"));
    auto grounding_context = ygg::ExecutionContext::create(1);
    auto ground_task = lifted_task->instantiate_ground_task(*grounding_context).task;
    ASSERT_NE(ground_task, nullptr);

    expect_brfs_events(ground_task, RepositoryMode::HASH_DISTRIBUTED, 1);
    for (const auto mode : { RepositoryMode::HASH_DISTRIBUTED, RepositoryMode::SHARED })
        expect_brfs_events(ground_task, mode, 2);
}

TEST(TyrPlanningPlanReconstructionTest, ParallelBrFSSupportsIW)
{
    const auto root = std::filesystem::path(BENCHMARKS_DIR);
    auto lifted_task = tyr::planning::Task<LiftedTag>::create(
        make_test_parser(root / "classical/tests/gripper/domain.pddl").parse_task(root / "classical/tests/gripper/test-1.pddl"));
    auto grounding_context = ygg::ExecutionContext::create(1);
    auto ground_task = lifted_task->instantiate_ground_task(*grounding_context).task;
    ASSERT_NE(ground_task, nullptr);

    for (const auto mode : { RepositoryMode::HASH_DISTRIBUTED, RepositoryMode::SHARED })
    {
        expect_parallel_iw(ground_task, mode);
        expect_parallel_iw(lifted_task, mode);
    }
}

TEST(TyrPlanningPlanReconstructionTest, SIWSequentiallyOrchestratesParallelIWSubsearches)
{
    const auto root = std::filesystem::path(BENCHMARKS_DIR);
    auto lifted_task = tyr::planning::Task<LiftedTag>::create(
        make_test_parser(root / "classical/tests/blocks_4/domain.pddl").parse_task(root / "classical/tests/blocks_4/test-1.pddl"));
    auto grounding_context = ygg::ExecutionContext::create(1);
    auto ground_task = lifted_task->instantiate_ground_task(*grounding_context).task;
    ASSERT_NE(ground_task, nullptr);

    expect_parallel_siw_outer_orchestration(ground_task);
}

TEST(TyrPlanningPlanReconstructionTest, LazyGBFSWorkerEventsMatchResultStatistics)
{
    const auto root = std::filesystem::path(BENCHMARKS_DIR);
    auto lifted_task = tyr::planning::Task<LiftedTag>::create(
        make_test_parser(root / "classical/tests/gripper/domain.pddl").parse_task(root / "classical/tests/gripper/test-1.pddl"));
    auto grounding_context = ygg::ExecutionContext::create(1);
    auto ground_task = lifted_task->instantiate_ground_task(*grounding_context).task;
    ASSERT_NE(ground_task, nullptr);

    expect_lazy_gbfs_worker_events(ground_task, RepositoryMode::SHARED, 1);
    for (const auto mode : { RepositoryMode::HASH_DISTRIBUTED, RepositoryMode::SHARED })
        expect_lazy_gbfs_worker_events(ground_task, mode, 2);
}

TEST(TyrPlanningPlanReconstructionTest, ParallelSearchDetectsGlobalExhaustion)
{
    const auto benchmark_root = std::filesystem::path(BENCHMARKS_DIR);
    const auto fixture_root = std::filesystem::path(ROOT_DIR) / "tests/support/data/planning/algorithms";
    auto lifted_task = tyr::planning::Task<LiftedTag>::create(
        make_test_parser(benchmark_root / "classical/tests/gripper/domain.pddl").parse_task(fixture_root / "parallel_gripper_unsolvable.pddl"));
    auto grounding_context = ygg::ExecutionContext::create(1);
    auto grounding_options = tyr::planning::GroundTaskInstantiationOptions {};
    grounding_options.disable_invariant_synthesis = true;
    auto ground_task = lifted_task->instantiate_ground_task(*grounding_context, grounding_options).task;
    ASSERT_NE(ground_task, nullptr);

    for (const auto mode : { RepositoryMode::HASH_DISTRIBUTED, RepositoryMode::SHARED })
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
    const auto fixture_root = std::filesystem::path(ROOT_DIR) / "tests/support/data/planning/algorithms";
    auto lifted_task = tyr::planning::Task<LiftedTag>::create(
        make_test_parser(benchmark_root / "classical/tests/transport/domain.pddl").parse_task(fixture_root / "parallel_astar_weighted.pddl"));
    auto grounding_context = ygg::ExecutionContext::create(1);
    auto ground_task = lifted_task->instantiate_ground_task(*grounding_context).task;
    ASSERT_NE(ground_task, nullptr);

    for (const auto state_repository_mode : { RepositoryMode::HASH_DISTRIBUTED, RepositoryMode::SHARED })
    {
        for (const auto mode : { tyr::planning::astar_eager::ParallelSearchMode::SYNCHRONOUS, tyr::planning::astar_eager::ParallelSearchMode::ASYNCHRONOUS })
        {
            expect_parallel_astar_keeps_searching_after_first_goal(ground_task, state_repository_mode, mode);
            expect_parallel_astar_keeps_searching_after_first_goal(lifted_task, state_repository_mode, mode);
        }
    }
}

TEST(TyrPlanningPlanReconstructionTest, ParallelAStarSelectsFLayerCoordination)
{
    const auto benchmark_root = std::filesystem::path(BENCHMARKS_DIR);
    const auto fixture_root = std::filesystem::path(ROOT_DIR) / "tests/support/data/planning/algorithms";
    auto lifted_task = tyr::planning::Task<LiftedTag>::create(
        make_test_parser(benchmark_root / "classical/tests/transport/domain.pddl").parse_task(fixture_root / "parallel_astar_weighted.pddl"));
    auto grounding_context = ygg::ExecutionContext::create(1);
    auto ground_task = lifted_task->instantiate_ground_task(*grounding_context).task;
    ASSERT_NE(ground_task, nullptr);

    for (const auto state_repository_mode : { RepositoryMode::HASH_DISTRIBUTED, RepositoryMode::SHARED })
    {
        expect_parallel_astar_coordinates_f_layers(ground_task, state_repository_mode, tyr::planning::astar_eager::ParallelSearchMode::SYNCHRONOUS, false);
        expect_parallel_astar_coordinates_f_layers(lifted_task, state_repository_mode, tyr::planning::astar_eager::ParallelSearchMode::SYNCHRONOUS, false);
        expect_parallel_astar_coordinates_f_layers(ground_task, state_repository_mode, tyr::planning::astar_eager::ParallelSearchMode::ASYNCHRONOUS, true);
        expect_parallel_astar_coordinates_f_layers(lifted_task, state_repository_mode, tyr::planning::astar_eager::ParallelSearchMode::ASYNCHRONOUS, true);
    }
    expect_synchronous_parallel_astar_stops_all_workers(ground_task);
}

}
