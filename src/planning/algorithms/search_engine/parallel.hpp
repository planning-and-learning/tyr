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

#include "tyr/planning/algorithms/utils.hpp"
#include "tyr/planning/state_routing/dist_hash.hpp"
#include "tyr/planning/state_routing/state_transfer.hpp"
#include "tyr/planning/worker_state_index.hpp"

#include <atomic>
#include <cassert>
#include <chrono>
#include <concepts>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <exception>
#include <limits>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>
#include <yggdrasil/execution/onetbb.hpp>

namespace tyr::planning::detail
{

template<TaskKind Kind, typename Metadata, DistHashKind HashKind>
class ParallelExecutionPolicy
{
    using BuilderPtr = ygg::SharedObjectPoolPtr<ygg::Builder<State<Kind>>, true>;
    using TransferredState = typename StateTransferPool<Kind>::TransferredState;

    struct Message
    {
        Node<Kind> source;
        TransferredState target;
        PendingActionResult action_result;
        ::tyr::formalism::planning::ActionBindingView action;
        Metadata metadata;
    };

public:
    using Search = ParallelSearch;
    static constexpr bool parallel = true;

    template<DistHashKind WorkerHashKind, typename WorkerMetadata>
    struct WorkerState
    {
        static_assert(std::same_as<WorkerHashKind, HashKind>);
        static_assert(std::same_as<WorkerMetadata, Metadata>);

        explicit WorkerState(uint64_t) {}

        std::mutex mutex;
        std::condition_variable condition;
        std::deque<Message> messages;
    };

    explicit ParallelExecutionPolicy(uint64_t seed) : m_dist_hash(seed) {}

    template<typename SearchPolicy>
    static typename SearchPolicy::EventHandlerPtr make_event_handler(const typename SearchPolicy::Options& options)
    {
        return options.event_handler;
    }

    template<typename Options>
    static void validate(const Options& options)
    {
        if (options.num_search_workers <= 1)
            throw std::invalid_argument("gbfs_lazy::find_solution(...): num_search_workers must be greater than one for parallel search.");
        if (options.num_search_workers > std::numeric_limits<ygg::uint_t>::max())
            throw std::invalid_argument("gbfs_lazy::find_solution(...): num_search_workers exceeds the worker index range.");
        if (options.pruning_strategy)
            throw std::invalid_argument("gbfs_lazy::find_solution(...): pruning strategies are not supported by parallel search.");
        if (options.goal_strategy)
            throw std::invalid_argument("gbfs_lazy::find_solution(...): custom goal strategies are not supported by parallel search.");
    }

    template<typename Options>
    static size_t num_workers(const Options& options) noexcept
    {
        return options.num_search_workers;
    }

    static ygg::ExecutionContextPtr create_execution_context() { return ygg::ExecutionContext::create(1); }

    void initialize_best_h(ygg::float_t value) noexcept { m_best_h_value.store(value, std::memory_order_relaxed); }

    template<typename EventHandlerPtr, typename Callback>
    bool improve_best_h(ygg::float_t value, EventHandlerPtr& event_handler, Callback&& callback)
    {
        if (value >= m_best_h_value.load(std::memory_order_relaxed))
            return false;

        const auto lock = std::lock_guard(m_best_h_mutex);
        if (value >= m_best_h_value.load(std::memory_order_relaxed))
            return false;

        m_best_h_value.store(value, std::memory_order_relaxed);
        call_event(event_handler, std::forward<Callback>(callback));
        return true;
    }

    template<typename EventHandlerPtr, typename Callback>
    void call_event(EventHandlerPtr& event_handler, Callback&& callback)
    {
        if (!event_handler)
            return;
        const auto lock = std::lock_guard(m_event_mutex);
        std::forward<Callback>(callback)(*event_handler);
    }

    void start(std::optional<std::chrono::steady_clock::duration> max_time)
    {
        m_status.store(SearchStatus::IN_PROGRESS, std::memory_order_relaxed);
        m_work.store(1, std::memory_order_relaxed);
        m_num_states.store(1, std::memory_order_relaxed);
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
    bool set_goal(Engine& engine, WorkerStateIndex<Kind> goal)
    {
        {
            const auto lock = std::lock_guard(m_terminal_mutex);
            if (!running())
                return false;
            m_goal = goal;
            m_status.store(SearchStatus::SOLVED, std::memory_order_release);
        }
        notify_workers(engine);
        return true;
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

        auto target = m_transfer_pool.import_state(*worker.successor_generator.get_state_repository(), std::move(message->target));
        auto node = worker.successor_generator.finalize_successor_state(std::move(target), message->action_result);
        return typename Engine::IncomingSuccessor { std::move(message->source),
                                                    RoutedSuccessor<Kind, Metadata> { LabeledNode<Kind> { message->action, std::move(node) },
                                                                                      std::move(message->metadata) } };
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
        const auto target_owner = m_dist_hash.owner(*target, engine.num_workers());
        if (target_owner == sender.index)
        {
            retain_successor();
            auto node = sender.successor_generator.finalize_successor_state(std::move(target), action_result);
            return
                typename Engine::IncomingSuccessor { source,
                                                     RoutedSuccessor<Kind, Metadata> { LabeledNode<Kind> { action, std::move(node) }, std::move(metadata) } };
        }

        auto message = Message { source, m_transfer_pool.export_state(std::move(target)), action_result, action, std::move(metadata) };
        auto& inbox = engine.get_worker(target_owner).execution;
        {
            const auto lock = std::lock_guard(inbox.mutex);
            inbox.messages.push_back(std::move(message));
            // The receiver cannot observe the message before its credit is published.
            m_work.fetch_add(1, std::memory_order_release);
        }
        inbox.condition.notify_one();
        return std::nullopt;
    }

    template<typename Engine, typename WorkerData>
    void wait_for_work(Engine& engine, WorkerData& worker)
    {
        auto lock = std::unique_lock(worker.execution.mutex);
        const auto ready = [&] { return !running() || !worker.execution.messages.empty(); };
        if (m_deadline)
        {
            if (!worker.execution.condition.wait_until(lock, *m_deadline, ready) && running())
                set_terminal(engine, SearchStatus::OUT_OF_TIME);
        }
        else
        {
            worker.execution.condition.wait(lock, ready);
        }
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

    template<typename Engine>
    ygg::Index<Worker> owner(Engine& engine, const ygg::Builder<State<Kind>>& state) const noexcept
    {
        return m_dist_hash.owner(state, engine.num_workers());
    }

private:
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
            if (!running())
                return;
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
            set_terminal(engine, SearchStatus::EXHAUSTED);
    }

    template<typename Engine>
    void notify_workers(Engine& engine)
    {
        for (size_t i = 0; i < engine.num_workers(); ++i)
            engine.get_worker(ygg::Index<Worker>(static_cast<ygg::uint_t>(i))).execution.condition.notify_all();
    }

    DistHash<Kind, HashKind> m_dist_hash;
    std::optional<std::chrono::steady_clock::time_point> m_deadline;
    StateTransferPool<Kind> m_transfer_pool;
    mutable std::mutex m_event_mutex;
    mutable std::mutex m_best_h_mutex;
    mutable std::mutex m_terminal_mutex;
    std::atomic<SearchStatus> m_status { SearchStatus::IN_PROGRESS };
    std::atomic<size_t> m_work { 0 };
    std::atomic<ygg::uint_t> m_num_states { 0 };
    std::atomic<ygg::float_t> m_best_h_value { std::numeric_limits<ygg::float_t>::infinity() };
    std::optional<WorkerStateIndex<Kind>> m_goal;
    std::exception_ptr m_exception;
    std::vector<std::jthread> m_threads;
};

}

#endif
