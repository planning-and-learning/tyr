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

#ifndef TYR_SRC_PLANNING_ALGORITHMS_SEARCH_ENGINE_SEQUENTIAL_HPP_
#define TYR_SRC_PLANNING_ALGORITHMS_SEARCH_ENGINE_SEQUENTIAL_HPP_

#include "tyr/planning/algorithms/utils.hpp"
#include "tyr/planning/state_routing/single_worker.hpp"
#include "tyr/planning/worker_state_index.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <optional>
#include <utility>
#include <yggdrasil/core/chrono.hpp>

namespace tyr::planning::detail
{

template<TaskKind Kind>
class SequentialExecutionPolicy
{
public:
    using Search = SequentialSearch;
    static constexpr bool parallel = false;

    explicit SequentialExecutionPolicy(uint64_t) noexcept {}

    template<DistHashKind HashKind, typename Metadata>
    struct WorkerState
    {
        explicit WorkerState(uint64_t seed) : router(seed) {}

        SingleWorkerStateRouter<Kind, HashKind, Metadata> router;
    };

    template<typename SearchPolicy>
    static typename SearchPolicy::EventHandlerPtr make_event_handler(const typename SearchPolicy::Options& options)
    {
        return options.event_handler ? options.event_handler : SearchPolicy::create_default_event_handler();
    }

    template<typename Options>
    static void validate(const Options&)
    {
    }

    template<typename Options>
    static size_t num_workers(const Options&) noexcept
    {
        return 1;
    }

    void initialize_best_h(ygg::float_t value) noexcept { m_best_h_value = value; }

    template<typename EventHandlerPtr, typename Callback>
    bool improve_best_h(ygg::float_t value, EventHandlerPtr& event_handler, Callback&& callback)
    {
        if (value >= m_best_h_value)
            return false;
        m_best_h_value = value;
        call_event(event_handler, std::forward<Callback>(callback));
        return true;
    }

    template<typename EventHandlerPtr, typename Callback>
    static void call_event(EventHandlerPtr& event_handler, Callback&& callback)
    {
        std::forward<Callback>(callback)(*event_handler);
    }

    template<typename Engine>
    static void invoke(Engine& engine)
    {
        engine.worker_loop(engine.get_worker(ygg::Index<Worker>(0)));
    }

    template<typename Engine, typename WorkerData>
    bool begin_iteration(Engine& engine, WorkerData& worker)
    {
        if (!running())
            return false;
        if (!worker.search.empty())
            return true;
        set_terminal(engine, SearchStatus::EXHAUSTED);
        return false;
    }

    void start(std::optional<std::chrono::steady_clock::duration> max_time)
    {
        if (max_time)
            m_stopwatch.emplace(*max_time);
        m_status = SearchStatus::IN_PROGRESS;
    }

    bool running() const noexcept { return m_status == SearchStatus::IN_PROGRESS; }
    bool timed_out() const { return m_stopwatch && m_stopwatch->has_finished(); }

    template<typename Engine>
    void set_terminal(Engine&, SearchStatus status)
    {
        if (running())
            m_status = status;
    }

    template<typename Engine>
    bool set_goal(Engine&, WorkerStateIndex<Kind> goal)
    {
        if (!running())
            return false;
        m_goal = goal;
        m_status = SearchStatus::SOLVED;
        return true;
    }

    SearchStatus status() const noexcept { return m_status; }
    std::optional<WorkerStateIndex<Kind>> goal() const noexcept { return m_goal; }
    std::exception_ptr exception() const noexcept { return nullptr; }

    template<typename Engine, typename Worker>
    void wait_for_work(Engine& engine, Worker&)
    {
        set_terminal(engine, SearchStatus::EXHAUSTED);
    }

    template<typename Engine, typename Worker>
    auto receive_one(Engine&, Worker&) -> std::optional<typename Engine::IncomingSuccessor>
    {
        return std::nullopt;
    }

    bool reserve_state(size_t current_size, ygg::uint_t max_num_states) const noexcept { return current_size < max_num_states; }
    void retain_successor() noexcept {}

    template<typename Engine>
    void release_successor(Engine&) noexcept
    {
    }

    template<typename Engine>
    void finish_expansion(Engine&) noexcept
    {
    }

    template<typename Engine>
    ygg::Index<Worker> owner(Engine&, const ygg::Builder<State<Kind>>&) const noexcept
    {
        return ygg::Index<Worker>(0);
    }

private:
    ygg::float_t m_best_h_value { 0 };
    SearchStatus m_status { SearchStatus::IN_PROGRESS };
    std::optional<WorkerStateIndex<Kind>> m_goal;
    std::optional<ygg::CountdownWatch> m_stopwatch;
};

}

#endif
