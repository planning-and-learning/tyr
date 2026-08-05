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

#ifndef TYR_SRC_PLANNING_ALGORITHMS_SEARCH_ENGINE_PARALLEL_HPP_
#define TYR_SRC_PLANNING_ALGORITHMS_SEARCH_ENGINE_PARALLEL_HPP_

#include "../repository_statistics.hpp"
#include "concepts.hpp"
#include "tyr/planning/algorithms/strategies/goal.hpp"
#include "tyr/planning/algorithms/strategies/pruning.hpp"
#include "tyr/planning/algorithms/utils.hpp"
#include "tyr/planning/search_space/parallel.hpp"
#include "tyr/planning/state_routing/dist_hash.hpp"
#include "tyr/planning/state_routing/state_transfer.hpp"
#include "tyr/planning/successor_generator.hpp"
#include "tyr/planning/worker_state_index.hpp"

#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <exception>
#include <limits>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>
#include <yggdrasil/core/portable_shuffle.hpp>

namespace tyr::planning::detail
{

template<TaskKind Kind, DistHashKind HashKind>
class HashDistributedStatePolicy
{
    using BuilderPtr = ygg::SharedObjectPoolPtr<ygg::Builder<State<Kind>>, true>;
    using TransferredState = typename StateTransferPool<Kind>::TransferredState;

public:
    using TaskTag = Kind;

    struct MessageTarget
    {
        TransferredState state;
        PendingActionResult action_result;
    };

    struct PreparedTarget
    {
        ygg::Index<Worker> owner;
        BuilderPtr state;
        PendingActionResult action_result;
    };

    explicit HashDistributedStatePolicy(uint64_t seed) : m_dist_hash(seed) {}

    static std::vector<SuccessorGeneratorPtr<Kind>> make_successor_generators(SuccessorGenerator<Kind>& successor_generator,
                                                                              std::span<const ygg::ExecutionContextPtr> execution_contexts)
    {
        auto result = std::vector<SuccessorGeneratorPtr<Kind>> {};
        result.reserve(execution_contexts.size());
        for (const auto& execution_context : execution_contexts)
            result.push_back(successor_generator.make_worker(execution_context));
        return result;
    }

    template<typename Engine>
    std::pair<ygg::Index<Worker>, StateView<Kind>> prepare_start_state(Engine& engine, const StateView<Kind>& start_state)
    {
        const auto owner = m_dist_hash.owner(start_state.get_state_builder(), engine.num_workers());
        auto& repository = *engine.get_worker(owner).successor_generator.get_state_repository();
        auto builder = repository.get_state_builder();
        builder->assign_unextended_part(start_state.get_state_builder());
        return { owner, repository.register_state(std::move(builder)) };
    }

    template<typename WorkerData>
    PreparedTarget prepare_target(WorkerData&, BuilderPtr target, PendingActionResult action_result, size_t num_workers)
    {
        return PreparedTarget { m_dist_hash.owner(*target, num_workers), std::move(target), action_result };
    }

    template<typename WorkerData>
    static Node<Kind> take_local_target(WorkerData& worker, PreparedTarget&& target)
    {
        return worker.successor_generator.finalize_successor_state(std::move(target.state), target.action_result);
    }

    MessageTarget make_message_target(PreparedTarget&& target)
    {
        return MessageTarget { m_transfer_pool.export_state(std::move(target.state)), target.action_result };
    }

    template<typename WorkerData>
    Node<Kind> receive_target(WorkerData& worker, MessageTarget target)
    {
        auto builder = m_transfer_pool.import_state(*worker.successor_generator.get_state_repository(), std::move(target.state));
        return worker.successor_generator.finalize_successor_state(std::move(builder), target.action_result);
    }

    static size_t search_node_divisor(size_t) noexcept { return 1; }

    static ygg::Index<State<Kind>> search_node_index(ygg::Index<State<Kind>> state, ygg::Index<Worker>, size_t) noexcept { return state; }

    template<typename Engine>
    static void snapshot_worker_state_statistics(Engine& engine)
    {
        for (size_t i = 0; i < engine.num_workers(); ++i)
        {
            auto& worker = engine.get_worker(ygg::Index<Worker>(static_cast<ygg::uint_t>(i)));
            detail::snapshot_state_repository_statistics(*worker.successor_generator.get_state_repository(), worker.statistics);
        }
    }

private:
    DistHash<Kind, HashKind> m_dist_hash;
    StateTransferPool<Kind> m_transfer_pool;
};

template<TaskKind Kind>
class SharedStatePolicy
{
    using BuilderPtr = ygg::SharedObjectPoolPtr<ygg::Builder<State<Kind>>, true>;

public:
    using TaskTag = Kind;

    struct MessageTarget
    {
        ygg::Index<State<Kind>> state;
        ygg::float_t metric;
    };

    struct PreparedTarget
    {
        ygg::Index<Worker> owner;
        Node<Kind> node;
    };

    explicit SharedStatePolicy(uint64_t) noexcept {}

    static auto make_successor_generators(SuccessorGenerator<Kind>& successor_generator, std::span<const ygg::ExecutionContextPtr> execution_contexts)
    {
        return successor_generator.make_shared_workers(execution_contexts);
    }

    template<typename Engine>
    static std::pair<ygg::Index<Worker>, StateView<Kind>> prepare_start_state(Engine& engine, const StateView<Kind>& start_state)
    {
        auto& repository = *engine.get_worker(ygg::Index<Worker>(0)).successor_generator.get_state_repository();
        auto builder = repository.get_state_builder();
        builder->assign_unextended_part(start_state.get_state_builder());
        auto registered = repository.register_state(std::move(builder));
        const auto start_owner = SharedStatePolicy::owner(registered.get_index(), engine.num_workers());
        return { start_owner, std::move(registered) };
    }

    static ygg::Index<Worker> owner(ygg::Index<State<Kind>> state, size_t num_workers) noexcept
    {
        assert(num_workers > 0);
        return ygg::Index<Worker>(static_cast<ygg::uint_t>(ygg::uint_t(state) % num_workers));
    }

    template<typename WorkerData>
    static PreparedTarget prepare_target(WorkerData& worker, BuilderPtr target, PendingActionResult action_result, size_t num_workers)
    {
        auto node = worker.successor_generator.finalize_successor_state(std::move(target), action_result);
        return PreparedTarget { owner(node.get_state().get_index(), num_workers), std::move(node) };
    }

    template<typename WorkerData>
    static Node<Kind> take_local_target(WorkerData&, PreparedTarget&& target)
    {
        return std::move(target.node);
    }

    static MessageTarget make_message_target(PreparedTarget&& target)
    {
        return MessageTarget { target.node.get_state().get_index(), target.node.get_metric() };
    }

    template<typename WorkerData>
    static Node<Kind> receive_target(WorkerData& worker, MessageTarget target)
    {
        auto state = worker.successor_generator.get_state_repository()->get_registered_state(target.state);
        return Node<Kind>(std::move(state), target.metric);
    }

    static size_t search_node_divisor(size_t num_workers) noexcept { return num_workers; }

    static ygg::Index<State<Kind>> search_node_index(ygg::Index<State<Kind>> state, ygg::Index<Worker> worker, size_t num_workers) noexcept
    {
        assert(owner(state, num_workers) == worker);
        return ygg::Index<State<Kind>>(static_cast<ygg::uint_t>(ygg::uint_t(state) / num_workers));
    }

    static uint64_t owned_state_count(uint64_t num_states, ygg::Index<Worker> worker, size_t num_workers) noexcept
    {
        const auto worker_index = uint64_t(ygg::uint_t(worker));
        const auto worker_count = uint64_t(num_workers);
        return num_states <= worker_index ? uint64_t { 0 } : 1 + (num_states - 1 - worker_index) / worker_count;
    }

    template<typename Engine>
    static void snapshot_worker_state_statistics(Engine& engine)
    {
        const auto& repository = *engine.get_worker(ygg::Index<Worker>(0)).successor_generator.get_state_repository();
        const auto num_states = repository.num_states();
        const auto memory_usage = repository.memory_usage();
        for (size_t i = 0; i < engine.num_workers(); ++i)
        {
            auto& worker = engine.get_worker(ygg::Index<Worker>(static_cast<ygg::uint_t>(i)));
            worker.statistics.set_num_registered_states(owned_state_count(num_states, worker.index, engine.num_workers()));
            worker.statistics.set_state_storage_memory_usage(i == 0 ? memory_usage : 0);
        }
    }
};

template<typename SearchPolicy, bool = SearchPolicy::supports_priority_layer_synchronization>
class ParallelLayerCoordinationPolicy;

template<typename SearchPolicy>
class ParallelLayerCoordinationPolicy<SearchPolicy, false>
{
public:
    void start(ygg::float_t, const typename SearchPolicy::Options&) noexcept {}

    bool proves_goal(ygg::float_t) const noexcept { return false; }

    template<typename ExecutionPolicy, typename WorkerData>
    static bool can_expand(const ExecutionPolicy&, const WorkerData& worker) noexcept
    {
        return !worker.search.empty();
    }

    template<typename ExecutionPolicy, typename Engine, typename WorkerData>
    static void wait_for_work(ExecutionPolicy& execution, Engine& engine, WorkerData& worker)
    {
        execution.wait_for_messages(engine, worker);
    }
};

template<typename SearchPolicy>
class ParallelLayerCoordinationPolicy<SearchPolicy, true>
{
public:
    void start(ygg::float_t start_priority, const typename SearchPolicy::Options& options) noexcept
    {
        m_synchronize_priority_layers = SearchPolicy::synchronize_priority_layers(options);
        m_active_priority.store(start_priority, std::memory_order_relaxed);
        m_layer_generation.store(0, std::memory_order_relaxed);
        m_num_waiting_workers = 0;
    }

    bool proves_goal(ygg::float_t cost) const noexcept { return m_synchronize_priority_layers && cost <= m_active_priority.load(std::memory_order_relaxed); }

    template<typename ExecutionPolicy, typename WorkerData>
    bool can_expand(const ExecutionPolicy& execution, const WorkerData& worker) const noexcept
    {
        if (!m_synchronize_priority_layers)
            return !worker.search.empty();

        const auto min_priority = worker.search.get_min_priority();
        const auto incumbent = execution.incumbent_cost();
        return min_priority <= m_active_priority.load(std::memory_order_acquire)
               && (incumbent == std::numeric_limits<ygg::float_t>::infinity() || min_priority < incumbent);
    }

    template<typename ExecutionPolicy, typename Engine, typename WorkerData>
    void wait_for_work(ExecutionPolicy& execution, Engine& engine, WorkerData& worker)
    {
        if (m_synchronize_priority_layers)
            wait_for_priority_layer(execution, engine, worker);
        else
            execution.wait_for_messages(engine, worker);
    }

private:
    template<typename ExecutionPolicy, typename Engine, typename WorkerData>
    void wait_for_priority_layer(ExecutionPolicy& execution, Engine& engine, WorkerData& worker)
    {
        auto generation = size_t { 0 };
        auto terminal_status = std::optional<SearchStatus> {};
        auto finished_priority = std::optional<ygg::float_t> {};
        auto release_workers = false;
        {
            auto lock = std::unique_lock(m_layer_mutex);
            if (!execution.running())
                return;

            generation = m_layer_generation.load(std::memory_order_relaxed);
            ++m_num_waiting_workers;
            assert(m_num_waiting_workers <= engine.num_workers());
            if (m_num_waiting_workers == engine.num_workers())
            {
                m_num_waiting_workers = 0;
                auto next_priority = std::numeric_limits<ygg::float_t>::infinity();
                auto num_open_entries = size_t { 0 };
                for (size_t i = 0; i < engine.num_workers(); ++i)
                {
                    const auto& search = engine.get_worker(ygg::Index<Worker>(static_cast<ygg::uint_t>(i))).search;
                    next_priority = std::min(next_priority, search.get_min_priority());
                    num_open_entries += search.get_num_open_entries();
                }

                const auto work = execution.m_work.load(std::memory_order_acquire);
                if (work < num_open_entries)
                    throw std::logic_error("Parallel search work credits are inconsistent with the open lists.");

                if (work == num_open_entries)
                {
                    const auto incumbent = execution.incumbent_cost();
                    if (incumbent != std::numeric_limits<ygg::float_t>::infinity() && next_priority >= incumbent)
                        terminal_status = SearchStatus::SOLVED;
                    else if (num_open_entries == 0)
                        terminal_status = SearchStatus::EXHAUSTED;
                    else
                    {
                        finished_priority = m_active_priority.load(std::memory_order_relaxed);
                        m_active_priority.store(next_priority, std::memory_order_release);
                    }
                }

                release_workers = true;
            }
        }

        if (release_workers)
        {
            if (terminal_status)
                execution.set_terminal(engine, *terminal_status);
            else
            {
                if (finished_priority)
                    engine.on_finish_priority_layer(*finished_priority);
                m_layer_generation.fetch_add(1, std::memory_order_release);
                execution.notify_workers(engine);
            }
            return;
        }

        auto lock = std::unique_lock(worker.execution.mutex);
        const auto ready = [&] { return !execution.running() || m_layer_generation.load(std::memory_order_acquire) != generation; };
        if (execution.m_deadline)
        {
            if (!worker.execution.condition.wait_until(lock, *execution.m_deadline, ready) && execution.running())
            {
                lock.unlock();
                execution.set_terminal(engine, SearchStatus::OUT_OF_TIME);
            }
        }
        else
        {
            worker.execution.condition.wait(lock, ready);
        }
    }

    std::mutex m_layer_mutex;
    std::atomic<ygg::float_t> m_active_priority { std::numeric_limits<ygg::float_t>::infinity() };
    std::atomic<size_t> m_layer_generation { 0 };
    size_t m_num_waiting_workers { 0 };
    bool m_synchronize_priority_layers { false };
};

template<TaskKind Kind, SearchPolicyConcept<Kind> SearchPolicy, StateRoutingPolicyConcept<Kind> StatePolicy>
    requires std::same_as<typename SearchPolicy::SearchTag, ParallelSearch>
class ParallelExecutionPolicy
{
    using BuilderPtr = ygg::SharedObjectPoolPtr<ygg::Builder<State<Kind>>, true>;
    using CoordinationPolicy = ParallelLayerCoordinationPolicy<SearchPolicy>;
    using Metadata = typename SearchPolicy::SuccessorMetadata;

    template<typename, bool>
    friend class ParallelLayerCoordinationPolicy;

    struct Message
    {
        Node<Kind> source;
        typename StatePolicy::MessageTarget target;
        ::tyr::formalism::planning::ActionBindingView action;
        Metadata metadata;
    };

public:
    using TaskTag = Kind;
    using SearchTag = ParallelSearch;

    struct WorkerState
    {
        explicit WorkerState(uint64_t) {}

        std::mutex mutex;
        std::condition_variable condition;
        std::deque<Message> messages;
    };

    explicit ParallelExecutionPolicy(uint64_t seed) : m_state_policy(seed) {}

    template<typename Options>
    static void validate(const Options& options)
    {
        if (options.num_search_workers <= 1)
            throw std::invalid_argument("Parallel search requires num_search_workers to be greater than one.");
        if (options.num_search_workers > std::numeric_limits<ygg::uint_t>::max())
            throw std::invalid_argument("Parallel search worker count exceeds the worker index range.");
    }

    template<typename Options>
    static size_t num_workers(const Options& options) noexcept
    {
        return options.num_search_workers;
    }

    static ygg::ExecutionContextPtr create_execution_context() { return ygg::ExecutionContext::create(1); }

    static auto make_successor_generators(SuccessorGenerator<Kind>& successor_generator, std::span<const ygg::ExecutionContextPtr> execution_contexts)
    {
        return StatePolicy::make_successor_generators(successor_generator, execution_contexts);
    }

    template<typename Options>
    static size_t search_node_divisor(const Options& options) noexcept
    {
        return StatePolicy::search_node_divisor(num_workers(options));
    }

    static ygg::Index<State<Kind>> search_node_index(ygg::Index<State<Kind>> state, ygg::Index<Worker> worker, size_t num_workers) noexcept
    {
        return StatePolicy::search_node_index(state, worker, num_workers);
    }

    static constexpr bool has_start_state_capacity(ygg::uint_t max_num_states) noexcept { return max_num_states > 0; }

    template<typename Engine>
    auto prepare_start_state(Engine& engine, const StateView<Kind>& start_state)
    {
        return m_state_policy.prepare_start_state(engine, start_state);
    }

    void initialize_best_h(ygg::float_t value) noexcept { m_best_h_value.store(value, std::memory_order_relaxed); }

    template<typename Callback>
    bool improve_best_h(ygg::float_t value, Callback&& callback)
    {
        if (value >= m_best_h_value.load(std::memory_order_relaxed))
            return false;

        const auto lock = std::lock_guard(m_best_h_mutex);
        if (value >= m_best_h_value.load(std::memory_order_relaxed))
            return false;

        m_best_h_value.store(value, std::memory_order_relaxed);
        std::forward<Callback>(callback)();
        return true;
    }

    void start(std::optional<std::chrono::steady_clock::duration> max_time, ygg::float_t start_priority, const typename SearchPolicy::Options& options)
    {
        m_status.store(SearchStatus::IN_PROGRESS, std::memory_order_relaxed);
        m_work.store(1, std::memory_order_relaxed);
        m_num_states.store(1, std::memory_order_relaxed);
        m_coordination.start(start_priority, options);
        if (max_time)
            m_deadline = std::chrono::steady_clock::now() + *max_time;
    }

    template<typename Engine>
    void invoke(Engine& engine)
    {
        try
        {
            m_threads.reserve(engine.num_workers());
            for (size_t i = 0; i < engine.num_workers(); ++i)
            {
                m_threads.emplace_back(
                    [this, &engine, i]
                    {
                        try
                        {
                            engine.worker_loop(engine.get_worker(ygg::Index<Worker>(static_cast<ygg::uint_t>(i))));
                        }
                        catch (...)
                        {
                            record_exception(engine, std::current_exception());
                        }
                    });
            }
        }
        catch (...)
        {
            record_exception(engine, std::current_exception());
        }
        join();
    }

    template<typename Engine, typename WorkerData>
    bool begin_iteration(Engine&, WorkerData&) const noexcept
    {
        return running();
    }

    bool running() const noexcept { return m_status.load(std::memory_order_acquire) == SearchStatus::IN_PROGRESS; }
    bool timed_out() const noexcept { return m_deadline && std::chrono::steady_clock::now() >= *m_deadline; }
    SearchStatus status() const noexcept { return m_status.load(std::memory_order_acquire); }

    std::optional<WorkerStateIndex<Kind>> goal() const
    {
        const auto lock = std::lock_guard(m_terminal_mutex);
        return m_goal;
    }

    std::exception_ptr exception() const
    {
        const auto lock = std::lock_guard(m_terminal_mutex);
        return m_exception;
    }

    template<typename Engine>
    void set_terminal(Engine& engine, SearchStatus status)
    {
        {
            const auto lock = std::lock_guard(m_terminal_mutex);
            if (!running())
                return;
            m_status.store(status, std::memory_order_release);
        }
        notify_workers(engine);
    }

    template<typename Engine>
    bool consider_goal(Engine& engine, WorkerStateIndex<Kind> goal, ygg::float_t cost, bool terminate)
    {
        auto notify = false;
        {
            const auto lock = std::lock_guard(m_terminal_mutex);
            if (!running() || cost >= m_incumbent_cost.load(std::memory_order_relaxed))
                return false;
            m_goal = goal;
            m_incumbent_cost.store(cost, std::memory_order_relaxed);
            if (terminate || m_coordination.proves_goal(cost))
            {
                m_status.store(SearchStatus::SOLVED, std::memory_order_release);
                notify = true;
            }
        }
        if (notify)
            notify_workers(engine);
        return true;
    }

    ygg::float_t incumbent_cost() const noexcept { return m_incumbent_cost.load(std::memory_order_relaxed); }

    template<typename Engine, typename WorkerData>
    bool can_expand(Engine&, const WorkerData& worker) const noexcept
    {
        return m_coordination.can_expand(*this, worker);
    }

    template<typename Engine, typename WorkerData>
    auto receive_one(Engine&, WorkerData& worker) -> std::optional<typename Engine::IncomingSuccessor>
    {
        auto message = std::optional<Message> {};
        {
            const auto lock = std::lock_guard(worker.execution.mutex);
            if (worker.execution.messages.empty())
                return std::nullopt;
            message.emplace(std::move(worker.execution.messages.front()));
            worker.execution.messages.pop_front();
        }

        auto node = m_state_policy.receive_target(worker, std::move(message->target));
        return typename Engine::IncomingSuccessor {
            std::move(message->source),
            typename Engine::RoutedSuccessor { LabeledNode<Kind> { message->action, std::move(node) }, std::move(message->metadata) },
        };
    }

    template<typename Engine, typename WorkerData>
    auto route(Engine& engine,
               WorkerData& sender,
               const Node<Kind>& source,
               BuilderPtr target,
               PendingActionResult action_result,
               ::tyr::formalism::planning::ActionBindingView action,
               Metadata metadata) -> std::optional<typename Engine::IncomingSuccessor>
    {
        auto prepared = m_state_policy.prepare_target(sender, std::move(target), action_result, engine.num_workers());
        if (prepared.owner == sender.index)
        {
            retain_successor();
            auto node = m_state_policy.take_local_target(sender, std::move(prepared));
            return typename Engine::IncomingSuccessor {
                source,
                typename Engine::RoutedSuccessor { LabeledNode<Kind> { action, std::move(node) }, std::move(metadata) },
            };
        }

        const auto target_owner = prepared.owner;
        enqueue(engine, target_owner, Message { source, m_state_policy.make_message_target(std::move(prepared)), action, std::move(metadata) });
        return std::nullopt;
    }

    template<typename Engine, typename WorkerData>
    void expand_successors(Engine& engine,
                           WorkerData& worker,
                           const Node<Kind>& node,
                           const typename SearchPolicy::PoppedEntry& entry,
                           typename SearchPolicy::SearchNode& search_node,
                           StateRepository<Kind>& state_repository)
    {
        if (engine.m_options.shuffle_labeled_succ_nodes)
            ygg::portable_shuffle(worker.applicable_actions.begin(), worker.applicable_actions.end(), worker.rng);

        for (const auto action : worker.applicable_actions)
        {
            if (!running())
                break;

            if (SearchPolicy::check_timeout_per_successor && timed_out())
            {
                set_terminal(engine, SearchStatus::OUT_OF_TIME);
                break;
            }

            auto successor_state = state_repository.get_state_builder();
            const auto action_result = worker.successor_generator.generate_successor_state(node, action, *successor_state);
            auto metadata = worker.search.make_successor_metadata(worker.index, entry.state, search_node, action);
            if (auto incoming = route(engine, worker, node, std::move(successor_state), action_result, action, std::move(metadata)))
            {
                const auto result = engine.accept_successor(worker, incoming->source, incoming->routed);
                engine.handle_acceptance(result);
                if (result == AcceptanceResult::TERMINAL || !running())
                    break;
            }
        }

        if (SearchPolicy::check_timeout_after_generation && running() && timed_out())
            set_terminal(engine, SearchStatus::OUT_OF_TIME);
    }

    template<typename Engine, typename WorkerData, typename EmitTransition>
    std::optional<AcceptanceResult> accept_generated_goal(Engine& engine,
                                                          WorkerData& worker,
                                                          typename SearchPolicy::SearchNode& search_node,
                                                          const StateView<Kind>& state,
                                                          ygg::float_t g_value,
                                                          EmitTransition&& emit_transition)
    {
        if (!worker.goal_strategy->is_dynamic_goal_satisfied(engine.m_start_node.get_state(), state))
            return std::nullopt;

        search_node.status = SearchNodeStatus::GOAL;
        static_cast<void>(consider_goal(engine, WorkerStateIndex<Kind> { worker.index, state.get_index() }, g_value, SearchPolicy::terminate_on_goal));
        std::forward<EmitTransition>(emit_transition)(TransitionOutcome::GOAL);
        return running() ? AcceptanceResult::DISCARDED : AcceptanceResult::TERMINAL;
    }

    template<typename Engine, typename WorkerData>
    static constexpr bool is_queued_goal(Engine&, WorkerData&, const StateView<Kind>&) noexcept
    {
        return false;
    }

    template<typename Engine>
    static void snapshot_worker_state_statistics(Engine& engine)
    {
        StatePolicy::snapshot_worker_state_statistics(engine);
    }

    template<typename Engine, typename WorkerData>
    void wait_for_work(Engine& engine, WorkerData& worker)
    {
        m_coordination.wait_for_work(*this, engine, worker);
    }

    bool reserve_state(size_t, ygg::uint_t max_num_states) noexcept
    {
        auto count = m_num_states.load(std::memory_order_relaxed);
        while (count < max_num_states)
        {
            if (m_num_states.compare_exchange_weak(count, count + 1, std::memory_order_relaxed))
                return true;
        }
        return false;
    }

    void retain_successor() noexcept { m_work.fetch_add(1, std::memory_order_relaxed); }

    template<typename Engine>
    void release_successor(Engine& engine)
    {
        release_work(engine);
    }

    template<typename Engine>
    void finish_expansion(Engine& engine)
    {
        release_work(engine);
    }

private:
    template<typename Engine>
    void enqueue(Engine& engine, ygg::Index<Worker> target_owner, Message message)
    {
        auto& inbox = engine.get_worker(target_owner).execution;
        {
            const auto lock = std::lock_guard(inbox.mutex);
            inbox.messages.push_back(std::move(message));
            // The receiver cannot observe the message before its credit is published.
            m_work.fetch_add(1, std::memory_order_release);
        }
        inbox.condition.notify_one();
    }

    template<typename Engine, typename WorkerData>
    void wait_for_messages(Engine& engine, WorkerData& worker)
    {
        auto lock = std::unique_lock(worker.execution.mutex);
        const auto ready = [&] { return !running() || !worker.execution.messages.empty(); };
        if (m_deadline)
        {
            if (!worker.execution.condition.wait_until(lock, *m_deadline, ready) && running())
            {
                lock.unlock();
                set_terminal(engine, SearchStatus::OUT_OF_TIME);
            }
        }
        else
        {
            worker.execution.condition.wait(lock, ready);
        }
    }

    void join()
    {
        for (auto& thread : m_threads)
        {
            if (thread.joinable())
                thread.join();
        }
    }

    template<typename Engine>
    void record_exception(Engine& engine, std::exception_ptr exception)
    {
        {
            const auto lock = std::lock_guard(m_terminal_mutex);
            if (!m_exception)
                m_exception = std::move(exception);
            m_status.store(SearchStatus::FAILED, std::memory_order_release);
        }
        notify_workers(engine);
    }

    template<typename Engine>
    void release_work(Engine& engine)
    {
        const auto old = m_work.fetch_sub(1, std::memory_order_acq_rel);
        assert(old > 0);
        if (old == 1)
            finish_search(engine);
    }

    template<typename Engine>
    void finish_search(Engine& engine)
    {
        {
            const auto lock = std::lock_guard(m_terminal_mutex);
            if (!running())
                return;
            m_status.store(m_goal ? SearchStatus::SOLVED : SearchStatus::EXHAUSTED, std::memory_order_release);
        }
        notify_workers(engine);
    }

    template<typename Engine>
    void notify_workers(Engine& engine)
    {
        for (size_t i = 0; i < engine.num_workers(); ++i)
        {
            auto& execution = engine.get_worker(ygg::Index<Worker>(static_cast<ygg::uint_t>(i))).execution;
            const auto lock = std::lock_guard(execution.mutex);
            execution.condition.notify_all();
        }
    }

    StatePolicy m_state_policy;
    [[no_unique_address]] CoordinationPolicy m_coordination;
    std::optional<std::chrono::steady_clock::time_point> m_deadline;
    mutable std::mutex m_best_h_mutex;
    mutable std::mutex m_terminal_mutex;
    std::atomic<SearchStatus> m_status { SearchStatus::IN_PROGRESS };
    std::atomic<size_t> m_work { 0 };
    std::atomic<ygg::uint_t> m_num_states { 0 };
    std::atomic<ygg::float_t> m_best_h_value { std::numeric_limits<ygg::float_t>::infinity() };
    std::atomic<ygg::float_t> m_incumbent_cost { std::numeric_limits<ygg::float_t>::infinity() };
    std::optional<WorkerStateIndex<Kind>> m_goal;
    std::exception_ptr m_exception;
    std::vector<std::jthread> m_threads;
};

template<TaskKind Kind, SearchPolicyConcept<Kind> SearchPolicy, ExecutionPolicyConcept<Kind, SearchPolicy> ExecutionPolicy, typename WorkerData>
class WorkerPolicy<ParallelSearch, Kind, SearchPolicy, ExecutionPolicy, WorkerData>
{
public:
    WorkerPolicy(Task<Kind>& task,
                 SuccessorGenerator<Kind>& successor_generator,
                 Heuristic<Kind>& heuristic,
                 const typename SearchPolicy::Options& options,
                 const typename SearchPolicy::EventHandlerPtr& event_handler)
    {
        const auto num_workers = ExecutionPolicy::num_workers(options);
        m_workers.reserve(num_workers);

        auto execution_contexts = std::vector<ygg::ExecutionContextPtr> {};
        execution_contexts.reserve(num_workers);
        for (size_t i = 0; i < num_workers; ++i)
            execution_contexts.push_back(ExecutionPolicy::create_execution_context());

        auto successor_generators =
            ExecutionPolicy::make_successor_generators(successor_generator, std::span<const ygg::ExecutionContextPtr>(execution_contexts));
        if (successor_generators.size() != num_workers)
            throw std::logic_error("Parallel search successor-generator worker count mismatch.");

        for (size_t i = 0; i < num_workers; ++i)
        {
            const auto index = ygg::Index<Worker>(static_cast<ygg::uint_t>(i));
            auto execution_context = std::move(execution_contexts[i]);
            auto worker_heuristic = heuristic.make_worker(execution_context);
            auto worker_pruning_strategy = options.pruning_strategy ? options.pruning_strategy->make_worker(index) : PruningStrategy<Kind>::create();
            if (!worker_pruning_strategy)
                throw std::invalid_argument("Parallel search pruning strategy does not support worker construction.");
            auto worker_goal_strategy = options.goal_strategy ? options.goal_strategy->make_worker(index) : ConjunctiveGoalStrategy<Kind>::create(task);
            if (!worker_goal_strategy)
                throw std::invalid_argument("Parallel search goal strategy does not support worker construction.");
            auto worker_event_handler = SearchPolicy::make_worker_event_handler(event_handler, index);
            m_workers.push_back(std::make_unique<WorkerData>(index,
                                                             std::move(execution_context),
                                                             std::move(successor_generators[i]),
                                                             std::move(worker_heuristic),
                                                             num_workers,
                                                             options,
                                                             std::move(worker_pruning_strategy),
                                                             std::move(worker_goal_strategy),
                                                             std::move(worker_event_handler)));
        }
    }

    size_t size() const noexcept { return m_workers.size(); }

    WorkerData& get(ygg::Index<Worker> index) noexcept
    {
        const auto value = static_cast<size_t>(ygg::uint_t(index));
        assert(value < m_workers.size());
        return *m_workers[value];
    }

    const WorkerData& get(ygg::Index<Worker> index) const noexcept
    {
        const auto value = static_cast<size_t>(ygg::uint_t(index));
        assert(value < m_workers.size());
        return *m_workers[value];
    }

    template<typename Callback>
    void for_each(Callback&& callback)
    {
        for (auto& worker : m_workers)
            callback(*worker);
    }

    template<typename Callback>
    void for_each(Callback&& callback) const
    {
        for (const auto& worker : m_workers)
            callback(*worker);
    }

    std::pair<Plan<Kind>, Node<Kind>>
    reconstruct_solution(WorkerStateIndex<Kind> goal, SuccessorGenerator<Kind>& caller_successor_generator, const typename SearchPolicy::Options& options)
    {
        auto views = std::vector<WorkerSearchSpaceView<Kind, typename SearchPolicy::SearchNode>> {};
        views.reserve(size());
        const auto state_index_divisor = ExecutionPolicy::search_node_divisor(options);
        for_each(
            [&](const auto& worker)
            {
                views.push_back(WorkerSearchSpaceView<Kind, typename SearchPolicy::SearchNode> {
                    worker.successor_generator,
                    worker.search.get_search_nodes(),
                    state_index_divisor,
                });
            });

        auto plan = PlanReconstructionPolicy<ParallelSearch>::extract_total_ordered_plan(
            goal,
            std::span<const WorkerSearchSpaceView<Kind, typename SearchPolicy::SearchNode>>(views),
            caller_successor_generator,
            options.cost_mode);
        auto goal_node = plan.empty() ? plan.get_start_node() : plan.get_labeled_succ_nodes().back().node;
        return { std::move(plan), std::move(goal_node) };
    }

private:
    std::vector<std::unique_ptr<WorkerData>> m_workers;
};

}

#endif
