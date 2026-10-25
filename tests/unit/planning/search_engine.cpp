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

#include "planning/algorithms/search_engine/astar_eager.hpp"
#include "planning/algorithms/search_engine/gbfs_lazy.hpp"
#include "planning/algorithms/search_engine/parallel.hpp"
#include "planning/parser.hpp"
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/planning/algorithms/astar_eager.hpp"
#include "tyr/planning/algorithms/astar_eager/event_handler.hpp"
#include "tyr/planning/algorithms/brfs.hpp"
#include "tyr/planning/algorithms/brfs/event_handler.hpp"
#include "tyr/planning/algorithms/gbfs_lazy.hpp"
#include "tyr/planning/algorithms/gbfs_lazy/event_handler.hpp"
#include "tyr/planning/algorithms/iw.hpp"
#include "tyr/planning/algorithms/iw/event_handler.hpp"
#include "tyr/planning/algorithms/openlists/alternating.hpp"
#include "tyr/planning/algorithms/openlists/interface.hpp"
#include "tyr/planning/algorithms/openlists/priority_queue.hpp"
#include "tyr/planning/algorithms/statistics.hpp"
#include "tyr/planning/algorithms/strategies/goal.hpp"
#include "tyr/planning/algorithms/strategies/pruning.hpp"
#include "tyr/planning/algorithms/utils.hpp"
#include "tyr/planning/applicability_decl.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/factory.hpp"
#include "tyr/planning/ground/task.hpp"
#include "tyr/planning/heuristic.hpp"
#include "tyr/planning/heuristics/blind.hpp"
#include "tyr/planning/lifted/task.hpp"
#include "tyr/planning/node.hpp"
#include "tyr/planning/plan.hpp"
#include "tyr/planning/state_routing/dist_hash.hpp"
#include "tyr/planning/state_routing/dist_hash_concepts.hpp"

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <gtest/gtest.h>
#include <limits>
#include <memory>
#include <mutex>
#include <optional>
#include <semaphore>
#include <stdexcept>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>
#include <yggdrasil/execution/onetbb.hpp>

namespace tyr::tests
{
namespace
{

using GroundSearchPolicy = tyr::planning::detail::EagerAStarPolicy<GroundTag, tyr::planning::ParallelSearch>;
using GroundRoutingPolicy = tyr::planning::detail::SharedStatePolicy<GroundTag>;
using GroundExecutionPolicy = tyr::planning::detail::ParallelExecutionPolicy<GroundTag, GroundSearchPolicy, GroundRoutingPolicy>;
using LiftedSearchPolicy = tyr::planning::detail::EagerAStarPolicy<LiftedTag, tyr::planning::ParallelSearch>;
using LiftedRoutingPolicy = tyr::planning::detail::SharedStatePolicy<LiftedTag>;
using LiftedExecutionPolicy = tyr::planning::detail::ParallelExecutionPolicy<LiftedTag, LiftedSearchPolicy, LiftedRoutingPolicy>;

static_assert(tyr::planning::detail::SearchPolicyConcept<GroundSearchPolicy, GroundTag>);
static_assert(tyr::planning::detail::StateRoutingPolicyConcept<GroundRoutingPolicy, GroundTag>);
static_assert(tyr::planning::detail::ExecutionPolicyConcept<GroundExecutionPolicy, GroundTag, GroundSearchPolicy>);
static_assert(tyr::planning::detail::SearchPolicyConcept<LiftedSearchPolicy, LiftedTag>);
static_assert(tyr::planning::detail::StateRoutingPolicyConcept<LiftedRoutingPolicy, LiftedTag>);
static_assert(tyr::planning::detail::ExecutionPolicyConcept<LiftedExecutionPolicy, LiftedTag, LiftedSearchPolicy>);
static_assert(!tyr::planning::detail::SearchPolicyConcept<GroundSearchPolicy, LiftedTag>);
static_assert(!tyr::planning::detail::StateRoutingPolicyConcept<GroundRoutingPolicy, LiftedTag>);
static_assert(!tyr::planning::detail::ExecutionPolicyConcept<GroundExecutionPolicy, LiftedTag, LiftedSearchPolicy>);
static_assert(!tyr::planning::detail::SearchPolicyConcept<int, GroundTag>);
static_assert(!tyr::planning::detail::StateRoutingPolicyConcept<int, GroundTag>);
static_assert(!tyr::planning::detail::ExecutionPolicyConcept<int, GroundTag, GroundSearchPolicy>);

struct QueueEntryWithoutKeyType
{
    using ItemType = int;

    int key;
    int item;

    [[nodiscard]] int get_key() const { return key; }
    [[nodiscard]] int get_item() const { return item; }
};

static_assert(tyr::planning::IsPriorityQueueEntry<QueueEntryWithoutKeyType>);
static_assert(tyr::planning::IsOpenList<tyr::planning::PriorityQueue<QueueEntryWithoutKeyType>>);

/// QueueEntryWithoutKeyType, but its item is not of ItemType.
struct QueueEntryWithWrongItem
{
    using ItemType = int;

    int key;

    [[nodiscard]] int get_key() const { return key; }
    [[nodiscard]] std::string_view get_item() const { return {}; }
};

struct QueueEntryWithDifferentItem : QueueEntryWithWrongItem
{
    using ItemType = std::string_view;
};

struct NonDefaultItem
{
    int value;

    explicit NonDefaultItem(int value) : value(value) {}
    NonDefaultItem(const NonDefaultItem&) = default;
    NonDefaultItem(NonDefaultItem&&) = default;
    NonDefaultItem& operator=(const NonDefaultItem&) = delete;
    NonDefaultItem& operator=(NonDefaultItem&&) = delete;
    ~NonDefaultItem() = default;
};

struct QueueEntryWithNonDefaultItem
{
    using ItemType = NonDefaultItem;

    int key;
    int item;

    [[nodiscard]] int get_key() const { return key; }
    [[nodiscard]] ItemType get_item() const { return ItemType(item); }
};

template<typename... Queues>
concept CanAlternate = requires { typename tyr::planning::AlternatingOpenList<Queues...>; };

static_assert(!tyr::planning::IsPriorityQueueEntry<QueueEntryWithWrongItem>);
static_assert(!CanAlternate<>);
static_assert(!CanAlternate<tyr::planning::PriorityQueue<QueueEntryWithoutKeyType>, tyr::planning::PriorityQueue<QueueEntryWithDifferentItem>>);
static_assert(CanAlternate<tyr::planning::PriorityQueue<QueueEntryWithNonDefaultItem>, tyr::planning::PriorityQueue<QueueEntryWithNonDefaultItem>>);

struct TaskPair
{
    tyr::planning::TaskPtr<LiftedTag> lifted;
    tyr::planning::TaskPtr<GroundTag> ground;
};

TaskPair make_task_pair(tyr::planning::TaskPtr<LiftedTag> lifted)
{
    auto execution_context = ygg::ExecutionContext::create(1);
    auto ground = lifted->instantiate_ground_task(*execution_context).task;
    if (!ground)
        throw std::runtime_error("Failed to instantiate parallel-search edge-case task.");
    return { std::move(lifted), std::move(ground) };
}

TaskPair parse_tasks(const std::filesystem::path& domain, const std::filesystem::path& problem)
{
    const auto fixture_root = std::filesystem::path(ROOT_DIR) / "tests/fixtures/planning/algorithms";
    return make_task_pair(tyr::planning::Task<LiftedTag>::create(make_test_parser(fixture_root / domain).parse_task(fixture_root / problem)));
}

TaskPair parse_tasks(const std::filesystem::path& problem) { return parse_tasks("parallel_search_edge_cases_domain.pddl", problem); }

TaskPair parse_transport_tasks(const std::filesystem::path& problem)
{
    const auto fixture_root = std::filesystem::path(ROOT_DIR) / "tests/fixtures/planning/algorithms";
    const auto benchmark_root = std::filesystem::path(BENCHMARKS_DIR);
    return make_task_pair(
        tyr::planning::Task<LiftedTag>::create(make_test_parser(benchmark_root / "classical/tests/transport/domain.pddl").parse_task(fixture_root / problem)));
}

TaskPair parse_pruned_improvement_tasks() { return parse_transport_tasks("parallel_astar_pruned_improvement.pddl"); }

template<TaskKind Kind>
struct SearchContext
{
    ygg::ExecutionContextPtr execution_context;
    tyr::planning::StateRepositoryPtr<Kind> repository;
    tyr::planning::AxiomEvaluatorPtr<Kind> axiom_evaluator;
    tyr::planning::SuccessorGeneratorPtr<Kind> successor_generator;
};

template<TaskKind Kind>
SearchContext<Kind> make_search_context(const tyr::planning::TaskPtr<Kind>& task, bool concurrent = false, size_t num_datalog_threads = 1)
{
    auto execution_context = ygg::ExecutionContext::create(num_datalog_threads);
    auto axiom_evaluator = tyr::planning::AxiomEvaluatorFactory<Kind>().create(task, execution_context);
    auto repository_factory = tyr::planning::StateRepositoryFactory<Kind> {};
    auto repository = concurrent ? repository_factory.create_concurrent(task) : repository_factory.create(task);
    auto successor_generator = tyr::planning::SuccessorGeneratorFactory<Kind>().create(task, execution_context);
    return { std::move(execution_context), std::move(repository), std::move(axiom_evaluator), std::move(successor_generator) };
}

template<TaskKind Kind>
class GoalAsDeadEndHeuristic final : public tyr::planning::Heuristic<Kind>
{
public:
    using tyr::planning::Heuristic<Kind>::evaluate;

    explicit GoalAsDeadEndHeuristic(tyr::planning::TaskPtr<Kind> task) : m_task(std::move(task)), m_goal(m_task->get_task().get_goal()) {}

    void set_goal(formalism::planning::ConjunctiveConditionView<GroundTag> goal) override { m_goal = goal; }

    ygg::float_t evaluate(const ygg::Builder<tyr::planning::State<Kind>>& state) override
    {
        const auto context = tyr::planning::StateContext<Kind>(*m_task, state, 0);
        return tyr::planning::is_dynamically_applicable(m_goal, context) ? std::numeric_limits<ygg::float_t>::infinity() : 0;
    }

    [[nodiscard]] tyr::planning::HeuristicPtr<Kind> make_worker(ygg::ExecutionContextPtr) const override
    {
        auto worker = std::make_shared<GoalAsDeadEndHeuristic>(m_task);
        worker->set_goal(m_goal);
        return worker;
    }

private:
    tyr::planning::TaskPtr<Kind> m_task;
    formalism::planning::ConjunctiveConditionView<GroundTag> m_goal;
};

template<TaskKind Kind>
class NullWorkerHeuristic final : public tyr::planning::Heuristic<Kind>
{
public:
    using tyr::planning::Heuristic<Kind>::evaluate;

    void set_goal(formalism::planning::ConjunctiveConditionView<GroundTag>) override {}
    ygg::float_t evaluate(const ygg::Builder<tyr::planning::State<Kind>>&) override { return 0; }
    [[nodiscard]] tyr::planning::HeuristicPtr<Kind> make_worker(ygg::ExecutionContextPtr) const override { return nullptr; }
};

template<TaskKind Kind>
class WorkerContextRecordingHeuristic final : public tyr::planning::Heuristic<Kind>
{
public:
    using tyr::planning::Heuristic<Kind>::evaluate;

    explicit WorkerContextRecordingHeuristic(std::vector<size_t>& worker_threads) : m_worker_threads(&worker_threads) {}

    void set_goal(formalism::planning::ConjunctiveConditionView<GroundTag>) override {}
    ygg::float_t evaluate(const ygg::Builder<tyr::planning::State<Kind>>&) override { return 0; }

    [[nodiscard]] tyr::planning::HeuristicPtr<Kind> make_worker(ygg::ExecutionContextPtr execution_context) const override
    {
        m_worker_threads->push_back(execution_context->get_num_threads());
        return std::make_shared<WorkerContextRecordingHeuristic>(*m_worker_threads);
    }

private:
    std::vector<size_t>* m_worker_threads;
};

template<TaskKind Kind>
bool is_at_location(const ygg::Builder<tyr::planning::State<Kind>>& state, const formalism::planning::Repository& repository, std::string_view location)
{
    return std::ranges::any_of(state.get_fluent_facts_view(repository),
                               [&](const auto fact)
                               {
                                   const auto atom = fact.get_atom();
                                   return atom && atom->get_predicate().get_name().str() == "at" && atom->get_objects().size() == 2
                                          && atom->get_objects().back().get_name().str() == location;
                               });
}

struct AStarHeuristicGate
{
    std::counting_semaphore<8> target_started { 0 };
    std::counting_semaphore<8> target_continue { 0 };
    std::atomic_size_t num_evaluations { 0 };
    std::atomic_size_t num_target_evaluations { 0 };
    std::atomic_bool transition_seen { false };
};

template<TaskKind Kind>
class GatedLocationHeuristic final : public tyr::planning::Heuristic<Kind>
{
public:
    using tyr::planning::Heuristic<Kind>::evaluate;

    GatedLocationHeuristic(tyr::planning::TaskPtr<Kind> task,
                           std::shared_ptr<AStarHeuristicGate> gate,
                           std::string_view target_location,
                           std::optional<std::string_view> wait_location = std::nullopt) :
        m_task(std::move(task)),
        m_gate(std::move(gate)),
        m_target_location(target_location),
        m_wait_location(wait_location)
    {
    }

    void set_goal(formalism::planning::ConjunctiveConditionView<GroundTag>) override {}

    ygg::float_t evaluate(const ygg::Builder<tyr::planning::State<Kind>>& state) override
    {
        m_gate->num_evaluations.fetch_add(1, std::memory_order_relaxed);
        if (m_wait_location && is_at_location(state, *m_task->get_repository(), *m_wait_location)
            && !m_gate->target_started.try_acquire_for(std::chrono::seconds(5)))
            throw std::runtime_error("Timed out waiting for the target heuristic evaluation.");

        if (is_at_location(state, *m_task->get_repository(), m_target_location))
        {
            m_gate->num_target_evaluations.fetch_add(1, std::memory_order_relaxed);
            m_gate->target_started.release();
            if (!m_gate->target_continue.try_acquire_for(std::chrono::seconds(5)))
                throw std::runtime_error("Timed out waiting for the competing transition.");
        }
        return 0;
    }

    [[nodiscard]] tyr::planning::HeuristicPtr<Kind> make_worker(ygg::ExecutionContextPtr) const override
    { return std::make_shared<GatedLocationHeuristic>(m_task, m_gate, m_target_location, m_wait_location); }

private:
    tyr::planning::TaskPtr<Kind> m_task;
    std::shared_ptr<AStarHeuristicGate> m_gate;
    std::string_view m_target_location;
    std::optional<std::string_view> m_wait_location;
};

template<TaskKind Kind>
class LocationThrowingHeuristic final : public tyr::planning::Heuristic<Kind>
{
public:
    using tyr::planning::Heuristic<Kind>::evaluate;

    LocationThrowingHeuristic(tyr::planning::TaskPtr<Kind> task, std::string_view location) : m_task(std::move(task)), m_location(location) {}

    void set_goal(formalism::planning::ConjunctiveConditionView<GroundTag>) override {}

    ygg::float_t evaluate(const ygg::Builder<tyr::planning::State<Kind>>& state) override
    {
        if (is_at_location(state, *m_task->get_repository(), m_location))
            throw std::runtime_error("Successor heuristic evaluation failed.");
        return 0;
    }

    [[nodiscard]] tyr::planning::HeuristicPtr<Kind> make_worker(ygg::ExecutionContextPtr) const override
    { return std::make_shared<LocationThrowingHeuristic>(m_task, m_location); }

private:
    tyr::planning::TaskPtr<Kind> m_task;
    std::string_view m_location;
};

template<TaskKind Kind>
class ReleasingAStarWorkerEventHandler final : public tyr::planning::astar_eager::WorkerEventHandler<Kind>
{
public:
    ReleasingAStarWorkerEventHandler(std::shared_ptr<AStarHeuristicGate> gate, tyr::planning::TransitionOutcome expected) :
        m_gate(std::move(gate)),
        m_expected(expected)
    {
    }

    void on_generate_transition(const tyr::planning::Node<Kind>&, const tyr::planning::LabeledNode<Kind>&, tyr::planning::TransitionOutcome outcome) override
    {
        if (outcome == m_expected && !m_gate->transition_seen.exchange(true, std::memory_order_relaxed))
            m_gate->target_continue.release();
    }

private:
    std::shared_ptr<AStarHeuristicGate> m_gate;
    tyr::planning::TransitionOutcome m_expected;
};

template<TaskKind Kind>
class ReleasingAStarEventHandler final : public tyr::planning::astar_eager::EventHandler<Kind>
{
public:
    ReleasingAStarEventHandler(std::shared_ptr<AStarHeuristicGate> gate, tyr::planning::TransitionOutcome expected) :
        m_gate(std::move(gate)),
        m_expected(expected)
    {
    }

    void on_start_search(const tyr::planning::Node<Kind>&, ygg::float_t) override {}
    void on_end_search(tyr::planning::SearchStatus, const tyr::planning::Statistics&) override {}
    void on_solved(const tyr::planning::Plan<Kind>&) override {}

    tyr::planning::astar_eager::WorkerEventHandlerPtr<Kind> make_worker(ygg::Index<tyr::planning::Worker>) override
    { return std::make_unique<ReleasingAStarWorkerEventHandler<Kind>>(m_gate, m_expected); }

private:
    std::shared_ptr<AStarHeuristicGate> m_gate;
    tyr::planning::TransitionOutcome m_expected;
};

template<TaskKind Kind>
class RecordingAStarLayerEventHandler final : public tyr::planning::astar_eager::EventHandler<Kind>
{
public:
    std::vector<std::pair<ygg::float_t, tyr::planning::Statistics>> snapshots;

    void on_start_search(const tyr::planning::Node<Kind>&, ygg::float_t) override {}
    void on_finish_f_layer(ygg::float_t f_value, const tyr::planning::Statistics& statistics) override { snapshots.emplace_back(f_value, statistics); }
    void on_end_search(tyr::planning::SearchStatus, const tyr::planning::Statistics&) override {}
    void on_solved(const tyr::planning::Plan<Kind>&) override {}
};

template<TaskKind Kind>
class ThrowingHeuristic final : public tyr::planning::Heuristic<Kind>
{
public:
    using tyr::planning::Heuristic<Kind>::evaluate;

    void set_goal(formalism::planning::ConjunctiveConditionView<GroundTag>) override {}
    ygg::float_t evaluate(const ygg::Builder<tyr::planning::State<Kind>>&) override { throw std::runtime_error("Unexpected heuristic evaluation."); }
    [[nodiscard]] tyr::planning::HeuristicPtr<Kind> make_worker(ygg::ExecutionContextPtr) const override { return std::make_shared<ThrowingHeuristic>(); }
};

template<TaskKind Kind>
class GoalAsThrowingHeuristic final : public tyr::planning::Heuristic<Kind>
{
public:
    using tyr::planning::Heuristic<Kind>::evaluate;

    explicit GoalAsThrowingHeuristic(tyr::planning::TaskPtr<Kind> task) : m_task(std::move(task)), m_goal(m_task->get_task().get_goal()) {}

    void set_goal(formalism::planning::ConjunctiveConditionView<GroundTag> goal) override { m_goal = goal; }

    ygg::float_t evaluate(const ygg::Builder<tyr::planning::State<Kind>>& state) override
    {
        const auto context = tyr::planning::StateContext<Kind>(*m_task, state, 0);
        if (tyr::planning::is_dynamically_applicable(m_goal, context))
            throw std::runtime_error("Successor heuristic evaluation failed.");
        return 0;
    }

    [[nodiscard]] tyr::planning::HeuristicPtr<Kind> make_worker(ygg::ExecutionContextPtr) const override
    {
        auto worker = std::make_shared<GoalAsThrowingHeuristic>(m_task);
        worker->set_goal(m_goal);
        return worker;
    }

private:
    tyr::planning::TaskPtr<Kind> m_task;
    formalism::planning::ConjunctiveConditionView<GroundTag> m_goal;
};

template<TaskKind Kind>
class InfiniteHeuristic final : public tyr::planning::Heuristic<Kind>
{
public:
    using tyr::planning::Heuristic<Kind>::evaluate;

    void set_goal(formalism::planning::ConjunctiveConditionView<GroundTag>) override {}
    ygg::float_t evaluate(const ygg::Builder<tyr::planning::State<Kind>>&) override { return std::numeric_limits<ygg::float_t>::infinity(); }
    [[nodiscard]] tyr::planning::HeuristicPtr<Kind> make_worker(ygg::ExecutionContextPtr) const override { return std::make_shared<InfiniteHeuristic>(); }
};

template<TaskKind Kind>
class DuplicatePruningStrategy final : public tyr::planning::PruningStrategy<Kind>
{
public:
    [[nodiscard]] tyr::planning::PruningStrategyPtr<Kind> make_worker(ygg::Index<tyr::planning::Worker>) const override
    { return std::make_shared<DuplicatePruningStrategy>(); }

    bool should_prune_successor_state(const tyr::planning::StateView<Kind>&, const tyr::planning::StateView<Kind>&, bool is_new) override { return !is_new; }
};

template<TaskKind Kind>
class AlwaysPruningStrategy final : public tyr::planning::PruningStrategy<Kind>
{
public:
    [[nodiscard]] tyr::planning::PruningStrategyPtr<Kind> make_worker(ygg::Index<tyr::planning::Worker>) const override
    { return std::make_shared<AlwaysPruningStrategy>(); }
    bool should_prune_state(const tyr::planning::StateView<Kind>&) override { return true; }
};

template<TaskKind Kind>
class AlwaysSuccessorPruningStrategy final : public tyr::planning::PruningStrategy<Kind>
{
public:
    [[nodiscard]] tyr::planning::PruningStrategyPtr<Kind> make_worker(ygg::Index<tyr::planning::Worker>) const override
    { return std::make_shared<AlwaysSuccessorPruningStrategy>(); }
    bool should_prune_successor_state(const tyr::planning::StateView<Kind>&, const tyr::planning::StateView<Kind>&, bool) override { return true; }
};

template<TaskKind Kind>
class AlwaysGoalStrategy final : public tyr::planning::GoalStrategy<Kind>
{
public:
    [[nodiscard]] tyr::planning::GoalStrategyPtr<Kind> make_worker(ygg::Index<tyr::planning::Worker>) const override
    { return std::make_shared<AlwaysGoalStrategy>(); }
    bool is_static_goal_satisfied(const tyr::planning::Task<Kind>&) override { return true; }
    bool is_dynamic_goal_satisfied(const tyr::planning::StateView<Kind>&, const ygg::Builder<tyr::planning::State<Kind>>&) override { return true; }
};

template<TaskKind Kind>
class StaticallyUnsatisfiedGoalStrategy final : public tyr::planning::GoalStrategy<Kind>
{
public:
    [[nodiscard]] tyr::planning::GoalStrategyPtr<Kind> make_worker(ygg::Index<tyr::planning::Worker>) const override
    { return std::make_shared<StaticallyUnsatisfiedGoalStrategy>(); }
    bool is_static_goal_satisfied(const tyr::planning::Task<Kind>&) override { return false; }
    bool is_dynamic_goal_satisfied(const tyr::planning::StateView<Kind>&, const ygg::Builder<tyr::planning::State<Kind>>&) override { return false; }
};

template<TaskKind Kind>
class NonRootGoalStrategy final : public tyr::planning::GoalStrategy<Kind>
{
public:
    [[nodiscard]] tyr::planning::GoalStrategyPtr<Kind> make_worker(ygg::Index<tyr::planning::Worker>) const override
    { return std::make_shared<NonRootGoalStrategy>(); }
    bool is_static_goal_satisfied(const tyr::planning::Task<Kind>&) override { return true; }
    bool is_dynamic_goal_satisfied(const tyr::planning::StateView<Kind>& seed_state, const ygg::Builder<tyr::planning::State<Kind>>& state) override
    { return &seed_state.get_state_builder() != &state; }
};

template<TaskKind Kind>
class SeedRecordingGoalStrategy final : public tyr::planning::GoalStrategy<Kind>
{
    using SeedIdentity = decltype(std::declval<tyr::planning::StateView<Kind>>().identifying_members());

    struct Observations
    {
        explicit Observations(SeedIdentity expected_seed_) : expected_seed(std::move(expected_seed_)) {}

        SeedIdentity expected_seed;
        std::optional<SeedIdentity> root_seed;
        std::optional<SeedIdentity> successor_seed;
        size_t num_root_checks { 0 };
        size_t num_successor_checks { 0 };
    };

public:
    SeedRecordingGoalStrategy(const tyr::planning::Task<Kind>& task, SeedIdentity expected_seed) :
        m_goal(task.get_task().get_goal()),
        m_observations(std::make_shared<Observations>(std::move(expected_seed)))
    {
    }

    [[nodiscard]] tyr::planning::GoalStrategyPtr<Kind> make_worker(ygg::Index<tyr::planning::Worker>) const override
    { return tyr::planning::GoalStrategyPtr<Kind>(new SeedRecordingGoalStrategy(m_goal, m_observations)); }

    bool is_static_goal_satisfied(const tyr::planning::Task<Kind>& task) override
    { return tyr::planning::is_statically_applicable(m_goal, task.get_static_state()); }

    bool is_dynamic_goal_satisfied(const tyr::planning::StateView<Kind>& seed_state, const ygg::Builder<tyr::planning::State<Kind>>& state) override
    {
        const auto context = tyr::planning::StateContext<Kind>(*seed_state.get_state_repository()->get_task(), state, 0);
        const auto is_goal = tyr::planning::is_dynamically_applicable(m_goal, context);
        if (is_goal)
        {
            m_observations->successor_seed = seed_state.identifying_members();
            ++m_observations->num_successor_checks;
        }
        else
        {
            m_observations->root_seed = seed_state.identifying_members();
            ++m_observations->num_root_checks;
        }
        return is_goal;
    }

    [[nodiscard]] bool observed_expected_seeds() const
    {
        return m_observations->root_seed && *m_observations->root_seed == m_observations->expected_seed && m_observations->successor_seed
               && *m_observations->successor_seed == m_observations->expected_seed;
    }

    [[nodiscard]] size_t get_num_root_checks() const noexcept { return m_observations->num_root_checks; }
    [[nodiscard]] size_t get_num_successor_checks() const noexcept { return m_observations->num_successor_checks; }

private:
    SeedRecordingGoalStrategy(formalism::planning::ConjunctiveConditionView<GroundTag> goal, std::shared_ptr<Observations> observations) :
        m_goal(goal),
        m_observations(std::move(observations))
    {
    }

    formalism::planning::ConjunctiveConditionView<GroundTag> m_goal;
    std::shared_ptr<Observations> m_observations;
};

template<typename Callback>
void for_each_execution_mode(Callback callback)
{
    callback(size_t { 1 }, false);
    callback(size_t { 2 }, false);
    callback(size_t { 2 }, true);
}

template<TaskKind Kind>
void expect_astar_goal_precedes_heuristic(const tyr::planning::TaskPtr<Kind>& task)
{
    for_each_execution_mode(
        [&](size_t num_workers, bool concurrent)
        {
            auto context = make_search_context(task, concurrent);
            auto heuristic = GoalAsDeadEndHeuristic<Kind>(task);
            auto options = tyr::planning::astar_eager::Options<Kind> {};
            options.num_search_workers = num_workers;

            const auto result = tyr::planning::astar_eager::find_solution(*task,
                                                                          *context.repository,
                                                                          *context.axiom_evaluator,
                                                                          *context.successor_generator,
                                                                          heuristic,
                                                                          options);
            ASSERT_EQ(result.status, tyr::planning::SearchStatus::SOLVED);
            ASSERT_TRUE(result.plan);
            EXPECT_EQ(result.plan->get_length(), 1);
        });
}

template<TaskKind Kind>
void expect_sequential_astar_root_layer_statistics(const tyr::planning::TaskPtr<Kind>& task)
{
    auto context = make_search_context(task);
    auto heuristic = tyr::planning::BlindHeuristic<Kind>::create();
    auto event_handler = std::make_shared<RecordingAStarLayerEventHandler<Kind>>();
    auto options = tyr::planning::astar_eager::Options<Kind> {};
    options.event_handler = event_handler;

    const auto result =
        tyr::planning::astar_eager::find_solution(*task, *context.repository, *context.axiom_evaluator, *context.successor_generator, *heuristic, options);

    ASSERT_EQ(result.status, tyr::planning::SearchStatus::SOLVED);
    ASSERT_TRUE(result.plan);
    EXPECT_EQ(result.plan.value().get_length(), 3);
    ASSERT_EQ(event_handler->snapshots.size(), 3);

    const auto expanded = std::vector<uint64_t> { 1, 3, 4 };
    const auto generated_successors = std::vector<uint64_t> { 2, 3, 4 };
    const auto generated_candidates = std::vector<uint64_t> { 2, 4, 5 };
    for (size_t i = 0; i < event_handler->snapshots.size(); ++i)
    {
        const auto& [f_value, statistics] = event_handler->snapshots[i];
        EXPECT_EQ(f_value, i);
        EXPECT_EQ(statistics.get_num_expanded(), expanded[i]);
        EXPECT_EQ(statistics.get_num_generated_successors(), generated_successors[i]);
        EXPECT_EQ(statistics.get_num_generated_candidates(), generated_candidates[i]);
        EXPECT_EQ(statistics.get_num_deadends(), 0);
        EXPECT_EQ(statistics.get_num_pruned(), 0);
    }
}

template<TaskKind Kind>
void expect_astar_duplicate_pruning_preserves_open_state(const tyr::planning::TaskPtr<Kind>& task)
{
    for_each_execution_mode(
        [&](size_t num_workers, bool concurrent)
        {
            auto context = make_search_context(task, concurrent);
            auto heuristic = tyr::planning::BlindHeuristic<Kind>::create();
            auto options = tyr::planning::astar_eager::Options<Kind> {};
            options.num_search_workers = num_workers;

            options.pruning_strategy = std::make_shared<DuplicatePruningStrategy<Kind>>();

            const auto result = tyr::planning::astar_eager::find_solution(*task,
                                                                          *context.repository,
                                                                          *context.axiom_evaluator,
                                                                          *context.successor_generator,
                                                                          *heuristic,
                                                                          options);
            ASSERT_EQ(result.status, tyr::planning::SearchStatus::SOLVED);
            ASSERT_TRUE(result.plan);
            EXPECT_EQ(result.plan->get_length(), 3);
        });
}

template<TaskKind Kind>
void expect_astar_pruned_improvement_preserves_existing_state(const tyr::planning::TaskPtr<Kind>& task)
{
    for_each_execution_mode(
        [&](size_t num_workers, bool concurrent)
        {
            auto context = make_search_context(task, concurrent);
            auto heuristic = tyr::planning::BlindHeuristic<Kind>::create();
            auto options = tyr::planning::astar_eager::Options<Kind> {};
            options.num_search_workers = num_workers;

            options.pruning_strategy = std::make_shared<DuplicatePruningStrategy<Kind>>();

            const auto result = tyr::planning::astar_eager::find_solution(*task,
                                                                          *context.repository,
                                                                          *context.axiom_evaluator,
                                                                          *context.successor_generator,
                                                                          *heuristic,
                                                                          options);
            ASSERT_EQ(result.status, tyr::planning::SearchStatus::SOLVED);
            ASSERT_TRUE(result.plan);
            EXPECT_EQ(result.plan->get_cost(), 101);
        });
}

template<TaskKind Kind>
void expect_one_step_goal_result(const tyr::planning::SearchResult<Kind>& result)
{
    ASSERT_EQ(result.status, tyr::planning::SearchStatus::SOLVED);
    ASSERT_TRUE(result.plan);
    EXPECT_EQ(result.plan.value().get_length(), 1);
    EXPECT_EQ(result.statistics.get_num_generated_successors(), 1);
    EXPECT_EQ(result.statistics.get_num_pruned(), 0);
}

template<TaskKind Kind>
void expect_goals_bypass_successor_pruning(const tyr::planning::TaskPtr<Kind>& task)
{
    for_each_execution_mode(
        [&](size_t num_workers, bool concurrent)
        {
            {
                auto context = make_search_context(task, concurrent);
                auto heuristic = tyr::planning::BlindHeuristic<Kind>::create();
                auto options = tyr::planning::astar_eager::Options<Kind> {};
                options.num_search_workers = num_workers;

                options.pruning_strategy = std::make_shared<AlwaysSuccessorPruningStrategy<Kind>>();
                expect_one_step_goal_result(tyr::planning::astar_eager::find_solution(*task,
                                                                                      *context.repository,
                                                                                      *context.axiom_evaluator,
                                                                                      *context.successor_generator,
                                                                                      *heuristic,
                                                                                      options));
            }
            {
                auto context = make_search_context(task, concurrent);
                auto options = tyr::planning::brfs::Options<Kind> {};
                options.num_search_workers = num_workers;

                options.pruning_strategy = std::make_shared<AlwaysSuccessorPruningStrategy<Kind>>();
                expect_one_step_goal_result(
                    tyr::planning::brfs::find_solution(*task, *context.repository, *context.axiom_evaluator, *context.successor_generator, options));
            }
            {
                auto context = make_search_context(task, concurrent);
                auto heuristic = tyr::planning::BlindHeuristic<Kind>::create();
                auto options = tyr::planning::gbfs_lazy::Options<Kind> {};
                options.num_search_workers = num_workers;

                options.pruning_strategy = std::make_shared<AlwaysSuccessorPruningStrategy<Kind>>();
                expect_one_step_goal_result(tyr::planning::gbfs_lazy::find_solution(*task,
                                                                                    *context.repository,
                                                                                    *context.axiom_evaluator,
                                                                                    *context.successor_generator,
                                                                                    *heuristic,
                                                                                    options));
            }
        });
}

template<TaskKind Kind>
void expect_iw_goal_bypasses_novelty_pruning(const tyr::planning::TaskPtr<Kind>& task)
{
    for_each_execution_mode(
        [&](size_t num_workers, bool concurrent)
        {
            auto context = make_search_context(task, concurrent);
            auto brfs_options = tyr::planning::brfs::Options<Kind> {};
            brfs_options.num_search_workers = num_workers;
            auto solver =
                tyr::planning::brfs::Solver<Kind> { task, context.repository, context.axiom_evaluator, context.successor_generator, std::move(brfs_options) };
            auto event_handler = tyr::planning::iw::DefaultEventHandler<Kind>::create();
            auto options = tyr::planning::iw::Options<Kind> {};
            options.event_handler = event_handler;

            const auto result = tyr::planning::iw::find_solution(solver, 1, options);
            ASSERT_EQ(result.status, tyr::planning::SearchStatus::SOLVED);
            ASSERT_TRUE(result.plan);
            EXPECT_EQ(result.plan->get_length(), 3);
            const auto solution_arity = event_handler->get_statistics().get_solution_arity();
            ASSERT_TRUE(solution_arity);
            EXPECT_EQ(*solution_arity, 1);
        });
}

template<TaskKind Kind>
void expect_parallel_goal_workers_receive_caller_seed(const tyr::planning::TaskPtr<Kind>& task)
{
    for (const auto concurrent : { false, true })
    {
        auto caller = make_search_context(task, concurrent);
        auto foreign = make_search_context(task);
        const auto foreign_start = foreign.successor_generator->get_initial_node(*foreign.repository, *foreign.axiom_evaluator);
        const auto normalized_start = tyr::planning::materialize_state(foreign_start.get_state(), *caller.repository, *caller.axiom_evaluator);
        ASSERT_NE(normalized_start.identifying_members(), foreign_start.get_state().identifying_members());

        auto goal_strategy = std::make_shared<SeedRecordingGoalStrategy<Kind>>(*task, normalized_start.identifying_members());
        auto options = tyr::planning::brfs::Options<Kind> {};
        options.start_node = foreign_start;
        options.goal_strategy = goal_strategy;
        options.num_search_workers = 2;

        const auto result = tyr::planning::brfs::find_solution(*task, *caller.repository, *caller.axiom_evaluator, *caller.successor_generator, options);
        ASSERT_EQ(result.status, tyr::planning::SearchStatus::SOLVED);
        EXPECT_TRUE(goal_strategy->observed_expected_seeds());
        EXPECT_EQ(goal_strategy->get_num_root_checks(), 1);
        EXPECT_EQ(goal_strategy->get_num_successor_checks(), 1);
    }
}

template<TaskKind Kind>
void expect_root_inclusive_state_limits(const tyr::planning::TaskPtr<Kind>& task)
{
    for_each_execution_mode(
        [&](size_t num_workers, bool concurrent)
        {
            const auto solve = [&](std::optional<ygg::uint_t> limit, tyr::planning::GoalStrategyPtr<Kind> goal_strategy = nullptr)
            {
                auto context = make_search_context(task, concurrent);
                auto options = tyr::planning::brfs::Options<Kind> {};
                options.num_search_workers = num_workers;

                options.search_budget.max_num_states = limit;
                options.goal_strategy = std::move(goal_strategy);
                return tyr::planning::brfs::find_solution(*task, *context.repository, *context.axiom_evaluator, *context.successor_generator, options);
            };

            EXPECT_EQ(solve(0).status, tyr::planning::SearchStatus::OUT_OF_STATES);
            EXPECT_EQ(solve(1).status, tyr::planning::SearchStatus::OUT_OF_STATES);
            EXPECT_EQ(solve(2).status, tyr::planning::SearchStatus::SOLVED);
            EXPECT_EQ(solve(std::nullopt).status, tyr::planning::SearchStatus::SOLVED);
            EXPECT_EQ(solve(0, std::make_shared<AlwaysGoalStrategy<Kind>>()).status, tyr::planning::SearchStatus::SOLVED);
        });

    auto context = make_search_context(task);
    const auto initial = context.successor_generator->get_initial_node(*context.repository, *context.axiom_evaluator);
    auto successors = context.successor_generator->get_labeled_successor_nodes(initial, *context.repository, *context.axiom_evaluator);
    ASSERT_EQ(successors.size(), 1);
    auto options = tyr::planning::brfs::Options<Kind> {};
    options.search_budget.max_num_states = 2;
    EXPECT_EQ(tyr::planning::brfs::find_solution(*task, *context.repository, *context.axiom_evaluator, *context.successor_generator, options).status,
              tyr::planning::SearchStatus::SOLVED);
}

template<TaskKind Kind>
void expect_foreign_start_handling(const tyr::planning::TaskPtr<Kind>& task, const tyr::planning::TaskPtr<Kind>& other_task)
{
    for_each_execution_mode(
        [&](size_t num_workers, bool concurrent)
        {
            auto caller = make_search_context(task, concurrent);
            auto foreign = make_search_context(task);
            auto heuristic = tyr::planning::BlindHeuristic<Kind>::create();
            auto options = tyr::planning::astar_eager::Options<Kind> {};
            options.start_node =
                tyr::planning::Node<Kind>(foreign.successor_generator->get_initial_node(*foreign.repository, *foreign.axiom_evaluator).get_state(), 7);
            options.num_search_workers = num_workers;

            const auto result =
                tyr::planning::astar_eager::find_solution(*task, *caller.repository, *caller.axiom_evaluator, *caller.successor_generator, *heuristic, options);
            ASSERT_EQ(result.status, tyr::planning::SearchStatus::SOLVED);
            ASSERT_TRUE(result.plan);
            EXPECT_EQ(result.plan->get_start_node().get_state().get_state_repository(), caller.repository);
            EXPECT_EQ(result.plan->get_start_node().get_metric(), 7);

            options.goal_strategy = std::make_shared<AlwaysGoalStrategy<Kind>>();
            const auto immediate_result =
                tyr::planning::astar_eager::find_solution(*task, *caller.repository, *caller.axiom_evaluator, *caller.successor_generator, *heuristic, options);
            ASSERT_TRUE(immediate_result.plan);
            EXPECT_EQ(immediate_result.plan->get_start_node().get_state().get_state_repository(), caller.repository);
        });

    auto caller = make_search_context(task);
    auto foreign = make_search_context(other_task);
    auto heuristic = tyr::planning::BlindHeuristic<Kind>::create();
    auto options = tyr::planning::astar_eager::Options<Kind> {};
    const auto foreign_start = foreign.successor_generator->get_initial_node(*foreign.repository, *foreign.axiom_evaluator);
    EXPECT_THROW(tyr::planning::materialize_state(foreign_start.get_state(), *caller.repository, *caller.axiom_evaluator), std::invalid_argument);
    EXPECT_THROW(caller.successor_generator->get_initial_node(*foreign.repository, *foreign.axiom_evaluator), std::invalid_argument);
    EXPECT_THROW(caller.successor_generator->get_applicable_action_bindings(foreign_start), std::invalid_argument);
    options.start_node = foreign_start;
    EXPECT_THROW(
        tyr::planning::astar_eager::find_solution(*task, *caller.repository, *caller.axiom_evaluator, *caller.successor_generator, *heuristic, options),
        std::invalid_argument);
}

template<TaskKind Kind>
void expect_null_heuristic_worker_is_rejected(const tyr::planning::TaskPtr<Kind>& task)
{
    auto context = make_search_context(task);
    auto heuristic = NullWorkerHeuristic<Kind> {};
    auto options = tyr::planning::astar_eager::Options<Kind> {};
    options.num_search_workers = 2;
    EXPECT_THROW(
        tyr::planning::astar_eager::find_solution(*task, *context.repository, *context.axiom_evaluator, *context.successor_generator, heuristic, options),
        std::invalid_argument);
}

template<TaskKind Kind>
void expect_destination_lock_statistics(const tyr::planning::TaskPtr<Kind>& task)
{
    for (const auto concurrent : { false, true })
    {
        for (const auto collect : { false, true })
        {
            auto context = make_search_context(task, concurrent);
            auto options = tyr::planning::brfs::Options<Kind> {};
            options.num_search_workers = 2;

            options.collect_destination_lock_statistics = collect;

            const auto result = tyr::planning::brfs::find_solution(*task, *context.repository, *context.axiom_evaluator, *context.successor_generator, options);
            ASSERT_EQ(result.status, tyr::planning::SearchStatus::SOLVED);

            auto acquisitions = uint64_t { 0 };
            auto wait_time = std::chrono::nanoseconds { 0 };
            auto hold_time = std::chrono::nanoseconds { 0 };
            for (const auto& worker : result.worker_statistics)
            {
                acquisitions += worker.get_num_destination_lock_acquisitions();
                wait_time += worker.get_destination_lock_wait_time();
                hold_time += worker.get_destination_lock_hold_time();
            }

            EXPECT_EQ(acquisitions, result.statistics.get_num_destination_lock_acquisitions());
            EXPECT_EQ(wait_time, result.statistics.get_destination_lock_wait_time());
            EXPECT_EQ(hold_time, result.statistics.get_destination_lock_hold_time());
            if (collect)
                EXPECT_GT(acquisitions, 0);
            else
            {
                EXPECT_EQ(acquisitions, 0);
                EXPECT_EQ(wait_time, std::chrono::nanoseconds(0));
                EXPECT_EQ(hold_time, std::chrono::nanoseconds(0));
            }
        }
    }
}

template<TaskKind Kind>
void expect_terminal_roots_skip_heuristic(const tyr::planning::TaskPtr<Kind>& task)
{
    for_each_execution_mode(
        [&](size_t num_workers, bool concurrent)
        {
            const auto check_astar = [&](auto configure, tyr::planning::SearchStatus expected_status)
            {
                auto context = make_search_context(task, concurrent);
                auto heuristic = ThrowingHeuristic<Kind> {};
                auto options = tyr::planning::astar_eager::Options<Kind> {};
                options.num_search_workers = num_workers;

                configure(options);

                auto result = tyr::planning::SearchResult<Kind> {};
                ASSERT_NO_THROW(result = tyr::planning::astar_eager::find_solution(*task,
                                                                                   *context.repository,
                                                                                   *context.axiom_evaluator,
                                                                                   *context.successor_generator,
                                                                                   heuristic,
                                                                                   options));
                EXPECT_EQ(result.status, expected_status);
                EXPECT_EQ(result.statistics.get_num_registered_states(), 1);
            };
            const auto check_gbfs = [&](auto configure, tyr::planning::SearchStatus expected_status)
            {
                auto context = make_search_context(task, concurrent);
                auto heuristic = ThrowingHeuristic<Kind> {};
                auto options = tyr::planning::gbfs_lazy::Options<Kind> {};
                options.num_search_workers = num_workers;

                configure(options);

                auto result = tyr::planning::SearchResult<Kind> {};
                ASSERT_NO_THROW(result = tyr::planning::gbfs_lazy::find_solution(*task,
                                                                                 *context.repository,
                                                                                 *context.axiom_evaluator,
                                                                                 *context.successor_generator,
                                                                                 heuristic,
                                                                                 options));
                EXPECT_EQ(result.status, expected_status);
                EXPECT_EQ(result.statistics.get_num_registered_states(), 1);
            };

            const auto dynamic_goal = [](auto& options) { options.goal_strategy = std::make_shared<AlwaysGoalStrategy<Kind>>(); };
            const auto static_failure = [](auto& options) { options.goal_strategy = std::make_shared<StaticallyUnsatisfiedGoalStrategy<Kind>>(); };
            const auto no_capacity = [](auto& options) { options.search_budget.max_num_states = 0; };
            const auto root_pruned = [](auto& options) { options.pruning_strategy = std::make_shared<AlwaysPruningStrategy<Kind>>(); };

            check_astar(dynamic_goal, tyr::planning::SearchStatus::SOLVED);
            check_astar(static_failure, tyr::planning::SearchStatus::UNSOLVABLE);
            check_astar(no_capacity, tyr::planning::SearchStatus::OUT_OF_STATES);
            check_astar(root_pruned, tyr::planning::SearchStatus::EXHAUSTED);
            check_gbfs(dynamic_goal, tyr::planning::SearchStatus::SOLVED);
            check_gbfs(static_failure, tyr::planning::SearchStatus::UNSOLVABLE);
            check_gbfs(no_capacity, tyr::planning::SearchStatus::OUT_OF_STATES);
            check_gbfs(root_pruned, tyr::planning::SearchStatus::EXHAUSTED);
        });
}

template<TaskKind Kind>
void expect_non_finite_start_metrics_are_rejected(const tyr::planning::TaskPtr<Kind>& task)
{
    for_each_execution_mode(
        [&](size_t num_workers, bool concurrent)
        {
            for (const auto metric : { std::numeric_limits<ygg::float_t>::quiet_NaN(), std::numeric_limits<ygg::float_t>::infinity() })
            {
                {
                    auto context = make_search_context(task, concurrent);
                    auto heuristic = tyr::planning::BlindHeuristic<Kind>::create();
                    auto options = tyr::planning::astar_eager::Options<Kind> {};
                    options.start_node =
                        tyr::planning::Node<Kind>(context.successor_generator->get_initial_node(*context.repository, *context.axiom_evaluator).get_state(),
                                                  metric);
                    options.num_search_workers = num_workers;

                    options.goal_strategy = std::make_shared<AlwaysGoalStrategy<Kind>>();
                    EXPECT_THROW(tyr::planning::astar_eager::find_solution(*task,
                                                                           *context.repository,
                                                                           *context.axiom_evaluator,
                                                                           *context.successor_generator,
                                                                           *heuristic,
                                                                           options),
                                 std::runtime_error);
                }
                {
                    auto context = make_search_context(task, concurrent);
                    auto heuristic = tyr::planning::BlindHeuristic<Kind>::create();
                    auto options = tyr::planning::gbfs_lazy::Options<Kind> {};
                    options.start_node =
                        tyr::planning::Node<Kind>(context.successor_generator->get_initial_node(*context.repository, *context.axiom_evaluator).get_state(),
                                                  metric);
                    options.num_search_workers = num_workers;

                    options.goal_strategy = std::make_shared<AlwaysGoalStrategy<Kind>>();
                    EXPECT_THROW(tyr::planning::gbfs_lazy::find_solution(*task,
                                                                         *context.repository,
                                                                         *context.axiom_evaluator,
                                                                         *context.successor_generator,
                                                                         *heuristic,
                                                                         options),
                                 std::runtime_error);
                }
                {
                    auto context = make_search_context(task, concurrent);
                    auto options = tyr::planning::brfs::Options<Kind> {};
                    options.start_node =
                        tyr::planning::Node<Kind>(context.successor_generator->get_initial_node(*context.repository, *context.axiom_evaluator).get_state(),
                                                  metric);
                    options.num_search_workers = num_workers;

                    options.goal_strategy = std::make_shared<AlwaysGoalStrategy<Kind>>();
                    EXPECT_THROW(
                        tyr::planning::brfs::find_solution(*task, *context.repository, *context.axiom_evaluator, *context.successor_generator, options),
                        std::runtime_error);
                }
            }
        });
}

template<TaskKind Kind>
void expect_dead_end_statistics(const tyr::planning::TaskPtr<Kind>& task)
{
    const auto expect_one_dead_end = [](const tyr::planning::SearchResult<Kind>& result)
    {
        EXPECT_EQ(result.statistics.get_num_deadends(), 1);
        auto worker_total = uint64_t { 0 };
        for (const auto& statistics : result.worker_statistics)
            worker_total += statistics.get_num_deadends();
        EXPECT_EQ(worker_total, 1);
    };

    for_each_execution_mode(
        [&](size_t num_workers, bool concurrent)
        {
            {
                auto context = make_search_context(task, concurrent);
                auto heuristic = InfiniteHeuristic<Kind> {};
                auto options = tyr::planning::astar_eager::Options<Kind> {};
                options.num_search_workers = num_workers;

                options.goal_strategy = tyr::planning::ExhaustiveGoalStrategy<Kind>::create();
                const auto result = tyr::planning::astar_eager::find_solution(*task,
                                                                              *context.repository,
                                                                              *context.axiom_evaluator,
                                                                              *context.successor_generator,
                                                                              heuristic,
                                                                              options);
                EXPECT_EQ(result.status, tyr::planning::SearchStatus::UNSOLVABLE);
                expect_one_dead_end(result);
            }
            {
                auto context = make_search_context(task, concurrent);
                auto heuristic = InfiniteHeuristic<Kind> {};
                auto options = tyr::planning::gbfs_lazy::Options<Kind> {};
                options.num_search_workers = num_workers;

                options.goal_strategy = tyr::planning::ExhaustiveGoalStrategy<Kind>::create();
                const auto result = tyr::planning::gbfs_lazy::find_solution(*task,
                                                                            *context.repository,
                                                                            *context.axiom_evaluator,
                                                                            *context.successor_generator,
                                                                            heuristic,
                                                                            options);
                EXPECT_EQ(result.status, tyr::planning::SearchStatus::UNSOLVABLE);
                expect_one_dead_end(result);
            }
            {
                auto context = make_search_context(task, concurrent);
                auto heuristic = GoalAsDeadEndHeuristic<Kind>(task);
                auto options = tyr::planning::astar_eager::Options<Kind> {};
                options.num_search_workers = num_workers;

                options.goal_strategy = tyr::planning::ExhaustiveGoalStrategy<Kind>::create();
                const auto result = tyr::planning::astar_eager::find_solution(*task,
                                                                              *context.repository,
                                                                              *context.axiom_evaluator,
                                                                              *context.successor_generator,
                                                                              heuristic,
                                                                              options);
                EXPECT_EQ(result.status, tyr::planning::SearchStatus::EXHAUSTED);
                expect_one_dead_end(result);
            }
            {
                auto context = make_search_context(task, concurrent);
                auto heuristic = GoalAsDeadEndHeuristic<Kind>(task);
                auto options = tyr::planning::gbfs_lazy::Options<Kind> {};
                options.num_search_workers = num_workers;

                options.goal_strategy = tyr::planning::ExhaustiveGoalStrategy<Kind>::create();
                const auto result = tyr::planning::gbfs_lazy::find_solution(*task,
                                                                            *context.repository,
                                                                            *context.axiom_evaluator,
                                                                            *context.successor_generator,
                                                                            heuristic,
                                                                            options);
                EXPECT_EQ(result.status, tyr::planning::SearchStatus::EXHAUSTED);
                expect_one_dead_end(result);
            }
        });
}

template<TaskKind Kind>
void expect_sequential_expansion_stops_materializing_after_goal(const tyr::planning::TaskPtr<Kind>& task)
{
    auto context = make_search_context(task);
    auto options = tyr::planning::brfs::Options<Kind> {};
    options.goal_strategy = std::make_shared<NonRootGoalStrategy<Kind>>();

    const auto result = tyr::planning::brfs::find_solution(*task, *context.repository, *context.axiom_evaluator, *context.successor_generator, options);
    ASSERT_EQ(result.status, tyr::planning::SearchStatus::SOLVED);
    ASSERT_TRUE(result.plan);
    EXPECT_EQ(result.plan.value().get_length(), 1);
    EXPECT_EQ(result.statistics.get_num_registered_states(), 2);
}

template<TaskKind Kind>
void expect_parallel_lazy_heuristic_exceptions_propagate(const tyr::planning::TaskPtr<Kind>& task)
{
    for (const auto concurrent : { false, true })
    {
        auto context = make_search_context(task, concurrent);
        auto heuristic = GoalAsThrowingHeuristic<Kind>(task);
        auto options = tyr::planning::gbfs_lazy::Options<Kind> {};
        options.num_search_workers = 2;

        options.goal_strategy = tyr::planning::ExhaustiveGoalStrategy<Kind>::create();
        EXPECT_THROW(
            tyr::planning::gbfs_lazy::find_solution(*task, *context.repository, *context.axiom_evaluator, *context.successor_generator, heuristic, options),
            std::runtime_error);
    }
}

template<TaskKind Kind>
void expect_pending_astar_improvement_is_coalesced(const tyr::planning::TaskPtr<Kind>& task)
{
    auto probe = make_search_context(task);
    const auto start = probe.successor_generator->get_initial_node(*probe.repository, *probe.axiom_evaluator);
    auto expensive = std::optional<tyr::planning::Node<Kind>> {};
    auto cheap = std::optional<tyr::planning::Node<Kind>> {};
    for (const auto& successor : probe.successor_generator->get_labeled_successor_nodes(start, *probe.repository, *probe.axiom_evaluator))
    {
        if (is_at_location(successor.node.get_state().get_state_builder(), *task->get_repository(), "expensive"))
            expensive = successor.node;
        else if (is_at_location(successor.node.get_state().get_state_builder(), *task->get_repository(), "cheap"))
            cheap = successor.node;
    }
    ASSERT_TRUE(expensive);
    ASSERT_TRUE(cheap);
    const auto cheap_successors = probe.successor_generator->get_labeled_successor_nodes(cheap.value(), *probe.repository, *probe.axiom_evaluator);
    const auto cheap_wait = std::ranges::find_if(
        cheap_successors,
        [&](const auto& successor) { return is_at_location(successor.node.get_state().get_state_builder(), *task->get_repository(), "cheap-wait"); });
    ASSERT_NE(cheap_wait, cheap_successors.end());

    auto seed = std::optional<uint64_t> {};
    for (uint64_t candidate = 0; candidate < 1024 && !seed; ++candidate)
    {
        auto hash = tyr::planning::DistHash<Kind, tyr::planning::RandomDistHashTag>(candidate);
        hash.initialize(start.get_state());
        const auto expensive_owner = hash.owner(expensive.value().get_state().get_state_builder(), 3);
        const auto cheap_owner = hash.owner(cheap.value().get_state().get_state_builder(), 3);
        if (expensive_owner != cheap_owner && hash.owner(cheap_wait->node.get_state().get_state_builder(), 3) == cheap_owner)
            seed = candidate;
    }
    ASSERT_TRUE(seed);

    auto context = make_search_context(task);
    auto gate = std::make_shared<AStarHeuristicGate>();
    auto heuristic = GatedLocationHeuristic<Kind>(task, gate, "merge", "cheap-wait");
    auto options = tyr::planning::astar_eager::Options<Kind> {};
    options.event_handler = std::make_shared<ReleasingAStarEventHandler<Kind>>(gate, tyr::planning::TransitionOutcome::RELAXED);
    options.num_search_workers = 3;
    options.dist_hash_mode = tyr::planning::DistHashMode::RANDOM;
    options.parallel_search_mode = tyr::planning::astar_eager::ParallelSearchMode::ASYNCHRONOUS;
    options.random_seed = seed.value();

    const auto result =
        tyr::planning::astar_eager::find_solution(*task, *context.repository, *context.axiom_evaluator, *context.successor_generator, heuristic, options);

    ASSERT_EQ(result.status, tyr::planning::SearchStatus::SOLVED);
    ASSERT_TRUE(result.plan);
    EXPECT_EQ(result.plan.value().get_cost(), 4);
    EXPECT_EQ(result.plan.value().get_length(), 4);
    EXPECT_TRUE(gate->transition_seen.load(std::memory_order_relaxed));
    EXPECT_EQ(gate->num_target_evaluations.load(std::memory_order_relaxed), 1);
}

template<TaskKind Kind>
void expect_parallel_astar_heuristic_exceptions_propagate(const tyr::planning::TaskPtr<Kind>& task)
{
    for (const auto mode : { tyr::planning::astar_eager::ParallelSearchMode::SYNCHRONOUS, tyr::planning::astar_eager::ParallelSearchMode::ASYNCHRONOUS })
    {
        auto context = make_search_context(task, true);
        auto heuristic = LocationThrowingHeuristic<Kind>(task, "left");
        auto options = tyr::planning::astar_eager::Options<Kind> {};
        options.num_search_workers = 2;
        options.parallel_search_mode = mode;

        EXPECT_THROW(
            tyr::planning::astar_eager::find_solution(*task, *context.repository, *context.axiom_evaluator, *context.successor_generator, heuristic, options),
            std::runtime_error);
    }
}

TEST(TyrPlanningSearchEngineTest, PriorityQueueUsesKeyGetterWithoutMetadata)
{
    auto queue = tyr::planning::PriorityQueue<QueueEntryWithoutKeyType> {};
    queue.insert({ 3, 30 });
    queue.insert({ 1, 10 });
    queue.insert({ 2, 20 });

    for (const auto item : { 10, 20, 30 })
    {
        EXPECT_EQ(queue.top(), item);
        queue.pop();
    }
    EXPECT_TRUE(queue.empty());
}

TEST(TyrPlanningSearchEngineTest, AlternatingOpenListConstructsSelectedItemWithoutDefaultConstructionOrAssignment)
{
    using Queue = tyr::planning::PriorityQueue<QueueEntryWithNonDefaultItem>;
    auto first = Queue {};
    auto second = Queue {};
    auto alternating = tyr::planning::AlternatingOpenList<Queue, Queue>(first, second, std::array<std::size_t, 2> { 1, 1 });
    first.insert({ 1, 10 });
    second.insert({ 2, 20 });

    EXPECT_EQ(alternating.top().value, 10);
    alternating.pop();
    EXPECT_EQ(alternating.top().value, 20);
    alternating.pop();
    EXPECT_TRUE(alternating.empty());
}

TEST(TyrPlanningSearchEngineTest, AlternatingOpenListClearsEveryQueue)
{
    using Queue = tyr::planning::PriorityQueue<QueueEntryWithoutKeyType>;
    auto first = Queue {};
    auto second = Queue {};
    auto alternating = tyr::planning::AlternatingOpenList<Queue, Queue>(first, second, std::array<std::size_t, 2> { 1, 1 });
    first.insert({ 1, 10 });
    first.insert({ 2, 20 });
    second.insert({ 3, 30 });
    EXPECT_EQ(alternating.size(), 3);

    alternating.clear();
    EXPECT_TRUE(first.empty());
    EXPECT_TRUE(second.empty());
    EXPECT_TRUE(alternating.empty());
    EXPECT_EQ(alternating.size(), 0);

    first.insert({ 4, 40 });
    second.insert({ 5, 50 });
    EXPECT_EQ(alternating.top(), 40);
    alternating.pop();
    EXPECT_EQ(alternating.top(), 50);
    alternating.clear();
    EXPECT_TRUE(second.empty());
    EXPECT_EQ(alternating.size(), 0);
}

TEST(TyrPlanningSearchEngineTest, ChecksGoalsBeforeHeuristics)
{
    const auto tasks = parse_tasks("parallel_search_simple.pddl");
    expect_astar_goal_precedes_heuristic(tasks.ground);
    expect_astar_goal_precedes_heuristic(tasks.lifted);
}

TEST(TyrPlanningSearchEngineTest, SequentialAStarReportsRootLayerStatisticsWithoutWorkerHandler)
{
    const auto tasks = parse_tasks("parallel_search_diamond.pddl");
    expect_sequential_astar_root_layer_statistics(tasks.ground);
    expect_sequential_astar_root_layer_statistics(tasks.lifted);
}

TEST(TyrPlanningSearchEngineTest, PrunedDuplicatesDoNotCloseAcceptedAStarStates)
{
    const auto tasks = parse_tasks("parallel_search_diamond.pddl");
    expect_astar_duplicate_pruning_preserves_open_state(tasks.ground);
    expect_astar_duplicate_pruning_preserves_open_state(tasks.lifted);
}

TEST(TyrPlanningSearchEngineTest, DuplicateDetectionPrecedesParallelAStarHeuristicEvaluation)
{
    const auto fixture_root = std::filesystem::path(ROOT_DIR) / "tests/fixtures/planning/algorithms";
    const auto task = tyr::planning::Task<LiftedTag>::create(
        make_test_parser(fixture_root / "parallel_search_edge_cases_domain.pddl").parse_task(fixture_root / "parallel_search_diamond.pddl"));
    auto probe = make_search_context(task);
    const auto start = probe.successor_generator->get_initial_node(*probe.repository, *probe.axiom_evaluator);
    const auto successors = probe.successor_generator->get_labeled_successor_nodes(start, *probe.repository, *probe.axiom_evaluator);
    ASSERT_EQ(successors.size(), 2);

    auto seed = std::optional<uint64_t> {};
    for (uint64_t candidate = 0; candidate < 1024 && !seed; ++candidate)
    {
        auto hash = tyr::planning::DistHash<LiftedTag, tyr::planning::RandomDistHashTag>(candidate);
        hash.initialize(start.get_state());
        if (hash.owner(successors.front().node.get_state().get_state_builder(), 3) != hash.owner(successors.back().node.get_state().get_state_builder(), 3))
            seed = candidate;
    }
    ASSERT_TRUE(seed);

    for (const auto concurrent : { false, true })
    {
        auto context = make_search_context(task, concurrent);
        auto gate = std::make_shared<AStarHeuristicGate>();
        auto heuristic = GatedLocationHeuristic<LiftedTag>(task, gate, "merge");
        auto options = tyr::planning::astar_eager::Options<LiftedTag> {};
        options.event_handler = std::make_shared<ReleasingAStarEventHandler<LiftedTag>>(gate, tyr::planning::TransitionOutcome::DUPLICATE);
        options.num_search_workers = concurrent ? 2 : 3;
        options.collect_destination_lock_statistics = true;
        options.random_seed = seed.value();

        const auto result =
            tyr::planning::astar_eager::find_solution(*task, *context.repository, *context.axiom_evaluator, *context.successor_generator, heuristic, options);

        ASSERT_EQ(result.status, tyr::planning::SearchStatus::SOLVED);
        EXPECT_EQ(result.statistics.get_num_generated_candidates(), 5);
        EXPECT_EQ(result.statistics.get_num_destination_lock_acquisitions(), 8);
        EXPECT_TRUE(gate->transition_seen.load(std::memory_order_relaxed));
        EXPECT_EQ(gate->num_evaluations.load(std::memory_order_relaxed), 4);
        EXPECT_EQ(gate->num_target_evaluations.load(std::memory_order_relaxed), 1);
    }
}

TEST(TyrPlanningSearchEngineTest, ParallelAStarCoalescesBetterPathsWhileHeuristicIsPending)
{
    const auto tasks = parse_transport_tasks("parallel_astar_pending_improvement.pddl");
    expect_pending_astar_improvement_is_coalesced(tasks.ground);
    expect_pending_astar_improvement_is_coalesced(tasks.lifted);
}

TEST(TyrPlanningSearchEngineTest, ParallelAStarUnlockedHeuristicExceptionsPropagateWithoutDeadlock)
{
    const auto tasks = parse_tasks("parallel_search_diamond.pddl");
    expect_parallel_astar_heuristic_exceptions_propagate(tasks.ground);
    expect_parallel_astar_heuristic_exceptions_propagate(tasks.lifted);
}

TEST(TyrPlanningSearchEngineTest, PrunedImprovementsDoNotOverwriteAcceptedAStarStates)
{
    const auto tasks = parse_pruned_improvement_tasks();
    expect_astar_pruned_improvement_preserves_existing_state(tasks.ground);
    expect_astar_pruned_improvement_preserves_existing_state(tasks.lifted);
}

TEST(TyrPlanningSearchEngineTest, GoalsBypassSuccessorPruning)
{
    const auto tasks = parse_tasks("parallel_search_simple.pddl");
    expect_goals_bypass_successor_pruning(tasks.ground);
    expect_goals_bypass_successor_pruning(tasks.lifted);
}

TEST(TyrPlanningSearchEngineTest, IwGoalsBypassNoveltyPruning)
{
    const auto tasks = parse_tasks("iw_goal_before_novelty_domain.pddl", "iw_goal_before_novelty_problem.pddl");
    expect_iw_goal_bypasses_novelty_pruning(tasks.ground);
    expect_iw_goal_bypasses_novelty_pruning(tasks.lifted);
}

TEST(TyrPlanningSearchEngineTest, ParallelGoalWorkersReceiveCallerNormalizedSeed)
{
    const auto tasks = parse_tasks("parallel_search_simple.pddl");
    expect_parallel_goal_workers_receive_caller_seed(tasks.ground);
    expect_parallel_goal_workers_receive_caller_seed(tasks.lifted);
}

TEST(TyrPlanningSearchEngineTest, StateLimitsAreRootInclusiveAndSolveLocal)
{
    const auto tasks = parse_tasks("parallel_search_simple.pddl");
    expect_root_inclusive_state_limits(tasks.ground);
    expect_root_inclusive_state_limits(tasks.lifted);
}

TEST(TyrPlanningSearchEngineTest, MaterializesCompatibleForeignStartsAndRejectsDifferentTasks)
{
    const auto tasks = parse_tasks("parallel_search_simple.pddl");
    const auto other_tasks = parse_tasks("parallel_search_simple.pddl");
    expect_foreign_start_handling(tasks.ground, other_tasks.ground);
    expect_foreign_start_handling(tasks.lifted, other_tasks.lifted);
}

TEST(TyrPlanningSearchEngineTest, RejectsNullParallelHeuristicWorkers)
{
    const auto tasks = parse_tasks("parallel_search_simple.pddl");
    expect_null_heuristic_worker_is_rejected(tasks.ground);
    expect_null_heuristic_worker_is_rejected(tasks.lifted);
}

TEST(TyrPlanningSearchEngineTest, SecondarySearchWorkersInheritPrototypeInnerConcurrency)
{
    const auto task = parse_tasks("parallel_search_simple.pddl").lifted;
    auto context = make_search_context(task, false, 2);
    auto worker_threads = std::vector<size_t> {};
    auto heuristic = WorkerContextRecordingHeuristic<LiftedTag>(worker_threads);
    auto options = tyr::planning::gbfs_lazy::Options<LiftedTag> {};
    options.num_search_workers = 2;

    const auto result =
        tyr::planning::gbfs_lazy::find_solution(*task, *context.repository, *context.axiom_evaluator, *context.successor_generator, heuristic, options);

    EXPECT_EQ(result.status, tyr::planning::SearchStatus::SOLVED);
    EXPECT_EQ(worker_threads, (std::vector<size_t> { 2 }));
}

TEST(TyrPlanningSearchEngineTest, HashDistributedPolicyUsesOwnerRepositoriesWithAConcurrentCaller)
{
    using Policy = tyr::planning::detail::EagerAStarPolicy<GroundTag, tyr::planning::ParallelSearch>;
    using Distribution = tyr::planning::detail::HashDistributedStatePolicy<GroundTag, tyr::planning::RandomDistHashTag>;
    using Execution = tyr::planning::detail::ParallelExecutionPolicy<GroundTag, Policy, Distribution>;
    const auto task = parse_tasks("parallel_search_simple.pddl").ground;
    auto context = make_search_context(task, true);
    auto heuristic = tyr::planning::BlindHeuristic<GroundTag> {};
    auto options = tyr::planning::astar_eager::Options<GroundTag> {};
    options.num_search_workers = 2;
    ASSERT_TRUE(context.repository->is_concurrent());

    const auto result = tyr::planning::detail::SearchEngine<GroundTag, Policy, Execution>::find_solution(*task,
                                                                                                         *context.repository,
                                                                                                         *context.axiom_evaluator,
                                                                                                         *context.successor_generator,
                                                                                                         heuristic,
                                                                                                         options);

    ASSERT_EQ(result.status, tyr::planning::SearchStatus::SOLVED);
    ASSERT_TRUE(result.plan);
    EXPECT_EQ(result.plan.value().get_length(), 1);
}

TEST(TyrPlanningSearchEngineTest, QueuedPreferredBoostAffectsTheNextPop)
{
    auto heuristic = tyr::planning::BlindHeuristic<GroundTag> {};
    auto options = tyr::planning::gbfs_lazy::Options<GroundTag> {};
    auto policy = tyr::planning::detail::LazyGBFSPolicy<GroundTag, tyr::planning::SequentialSearch>(heuristic, options);
    const auto first_preferred = ygg::Index<tyr::planning::State<GroundTag>>(0);
    const auto second_preferred = ygg::Index<tyr::planning::State<GroundTag>>(1);
    const auto standard = ygg::Index<tyr::planning::State<GroundTag>>(2);

    policy.open_successor(first_preferred, 0, 0, tyr::planning::SearchNodeStatus::OPEN, true);
    policy.open_successor(second_preferred, 0, 0, tyr::planning::SearchNodeStatus::OPEN, true);
    policy.open_successor(standard, 0, 0, tyr::planning::SearchNodeStatus::OPEN, false);
    policy.queue_preferred_boost(2);

    EXPECT_EQ(policy.pop().state, first_preferred);
    EXPECT_EQ(policy.pop().state, second_preferred);
    EXPECT_EQ(policy.pop().state, standard);
}

TEST(TyrPlanningSearchEngineTest, ParallelAStarSnapshotExcludesTimedOutWaitersButCallbackDoesNot)
{
    using Policy = tyr::planning::detail::EagerAStarPolicy<GroundTag, tyr::planning::ParallelSearch>;
    using Execution = tyr::planning::detail::ParallelExecutionPolicy<GroundTag, Policy, tyr::planning::detail::SharedStatePolicy<GroundTag>>;
    struct Search
    {
        [[nodiscard]] bool empty() const { return false; }
        [[nodiscard]] ygg::float_t get_min_priority() const { return 1; }
        [[nodiscard]] size_t get_num_open_entries() const { return 1; }
    };
    struct Worker
    {
        ygg::Index<tyr::planning::Worker> index;
        Execution::WorkerState execution;
        Search search;
        tyr::planning::Statistics statistics;
    };
    struct Engine
    {
        std::array<Worker, 2> workers;
        std::binary_semaphore snapshot_started { 0 };
        std::binary_semaphore allow_snapshot { 0 };
        std::counting_semaphore<2> waiter_exited { 0 };
        size_t num_callbacks { 0 };

        [[nodiscard]] size_t num_workers() const { return workers.size(); }
        Worker& get_worker(ygg::Index<tyr::planning::Worker> index) { return workers[ygg::uint_t(index)]; }

        std::optional<tyr::planning::Statistics> collect_priority_layer_statistics()
        {
            snapshot_started.release();
            allow_snapshot.acquire();
            auto statistics = tyr::planning::Statistics {};
            for (const auto& worker : workers)
                statistics.add(worker.statistics);
            return statistics;
        }

        void on_finish_priority_layer(ygg::float_t priority, const tyr::planning::Statistics& statistics)
        {
            ++num_callbacks;
            EXPECT_EQ(priority, 0);
            EXPECT_TRUE(waiter_exited.try_acquire_for(std::chrono::seconds(2)));
            EXPECT_EQ(statistics.get_idle_time(), std::chrono::nanoseconds(0));
        }
    } engine;
    for (size_t i = 0; i < engine.num_workers(); ++i)
        engine.workers[i].index = ygg::Index<tyr::planning::Worker>(static_cast<ygg::uint_t>(i));

    auto execution = Execution(0);
    auto options = tyr::planning::astar_eager::Options<GroundTag> {};
    options.num_search_workers = engine.num_workers();
    options.parallel_search_mode = tyr::planning::astar_eager::ParallelSearchMode::SYNCHRONOUS;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(1);
    execution.start(deadline, 0, options);
    execution.retain_successor();

    auto threads = std::vector<std::jthread> {};
    for (auto& worker : engine.workers)
    {
        threads.emplace_back(
            [&engine, &execution, &worker]
            {
                execution.wait_for_work(engine, worker);
                worker.statistics.add_idle_time(std::chrono::nanoseconds(1));
                engine.waiter_exited.release();
            });
    }

    EXPECT_TRUE(engine.snapshot_started.try_acquire_for(std::chrono::seconds(2)));
    // A timeout must not let the peer update its idle time until collection has finished.
    EXPECT_FALSE(engine.waiter_exited.try_acquire_until(deadline + std::chrono::milliseconds(50)));
    engine.allow_snapshot.release();
    threads.clear();

    EXPECT_EQ(engine.num_callbacks, 1);
    EXPECT_EQ(execution.status(), tyr::planning::SearchStatus::OUT_OF_TIME);
}

TEST(TyrPlanningSearchEngineTest, LazyGbfsBestHCallbackReleasesTheWorkerLock)
{
    using Policy = tyr::planning::detail::LazyGBFSPolicy<GroundTag, tyr::planning::ParallelSearch>;
    using Execution = tyr::planning::detail::ParallelExecutionPolicy<GroundTag, Policy, tyr::planning::detail::SharedStatePolicy<GroundTag>>;
    struct Worker
    {
        Execution::WorkerState execution;
    } worker;
    struct Handler
    {
        std::mutex* worker_mutex;
        bool called { false };

        void on_new_best_h_value(ygg::float_t h_value)
        {
            called = true;
            EXPECT_EQ(h_value, 0);
            auto unlocked = false;
            auto receiver = std::thread(
                [&]
                {
                    auto lock = std::unique_lock(*worker_mutex, std::try_to_lock);
                    unlocked = lock.owns_lock();
                });
            receiver.join();
            EXPECT_TRUE(unlocked);
        }
    } handler { &worker.execution.mutex };

    const auto task = parse_tasks("parallel_search_simple.pddl").ground;
    auto context = make_search_context(task);
    const auto start = context.successor_generator->get_initial_node(*context.repository, *context.axiom_evaluator);
    auto heuristic = tyr::planning::BlindHeuristic<GroundTag> {};
    auto options = tyr::planning::gbfs_lazy::Options<GroundTag> {};
    auto policy = Policy(heuristic, options);
    auto execution = Execution(0);
    execution.initialize_best_h(1);
    auto& search_node = policy.initialize_start(start.get_state().get_index(), start.get_metric(), 1);
    auto statistics = tyr::planning::Statistics {};

    const auto result =
        execution.with_worker_lock(worker,
                                   [&](auto&& evaluate_unlocked)
                                   {
                                       return policy.prepare_expansion(
                                           Policy::PoppedEntry { start.get_state().get_index() },
                                           start,
                                           search_node,
                                           statistics,
                                           evaluate_unlocked,
                                           [&](ygg::float_t h_value, auto&& callback) { return execution.improve_best_h(h_value, [&] { callback(handler); }); },
                                           [](auto&&) {},
                                           [](auto) {});
                                   });

    EXPECT_TRUE(handler.called);
    EXPECT_EQ(result, tyr::planning::detail::ExpansionResult::EXPAND);
    EXPECT_EQ(search_node.status, tyr::planning::SearchNodeStatus::CLOSED);
    EXPECT_EQ(statistics.get_num_expanded(), 1);
}

TEST(TyrPlanningSearchEngineTest, DestinationLockStatisticsAreOptInAndAggregated)
{
    const auto tasks = parse_tasks("parallel_search_simple.pddl");
    expect_destination_lock_statistics(tasks.ground);
    expect_destination_lock_statistics(tasks.lifted);
}

TEST(TyrPlanningSearchEngineTest, TerminalRootsSkipHeuristicEvaluation)
{
    const auto tasks = parse_tasks("parallel_search_simple.pddl");
    expect_terminal_roots_skip_heuristic(tasks.ground);
    expect_terminal_roots_skip_heuristic(tasks.lifted);
}

TEST(TyrPlanningSearchEngineTest, RejectsNonFiniteStartMetricsBeforeRecognizingGoals)
{
    const auto tasks = parse_tasks("parallel_search_simple.pddl");
    expect_non_finite_start_metrics_are_rejected(tasks.ground);
    expect_non_finite_start_metrics_are_rejected(tasks.lifted);
}

TEST(TyrPlanningSearchEngineTest, CountsEachDeadEndOnce)
{
    const auto tasks = parse_tasks("parallel_search_simple.pddl");
    expect_dead_end_statistics(tasks.ground);
    expect_dead_end_statistics(tasks.lifted);
}

TEST(TyrPlanningSearchEngineTest, SequentialExpansionStopsMaterializingAfterGoal)
{
    const auto tasks = parse_tasks("parallel_search_diamond.pddl");
    expect_sequential_expansion_stops_materializing_after_goal(tasks.ground);
    expect_sequential_expansion_stops_materializing_after_goal(tasks.lifted);
}

TEST(TyrPlanningSearchEngineTest, ParallelLazyHeuristicExceptionsPropagateWithoutDeadlock)
{
    const auto tasks = parse_tasks("parallel_search_simple.pddl");
    expect_parallel_lazy_heuristic_exceptions_propagate(tasks.ground);
    expect_parallel_lazy_heuristic_exceptions_propagate(tasks.lifted);
}

}
}
