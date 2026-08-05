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

#include "../repository_statistics.hpp"
#include "concepts.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/planning/algorithms/strategies/goal.hpp"
#include "tyr/planning/algorithms/strategies/pruning.hpp"
#include "tyr/planning/algorithms/utils.hpp"
#include "tyr/planning/ground/state_builder.hpp"
#include "tyr/planning/lifted/state_builder.hpp"
#include "tyr/planning/search_space/sequential.hpp"
#include "tyr/planning/successor_generator.hpp"
#include "tyr/planning/worker_state_index.hpp"

#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <limits>
#include <optional>
#include <utility>
#include <yggdrasil/core/chrono.hpp>
#include <yggdrasil/core/portable_shuffle.hpp>

namespace tyr::planning::detail
{

template<TaskKind Kind, SearchPolicyConcept<Kind> SearchPolicy>
    requires std::same_as<typename SearchPolicy::SearchTag, SequentialSearch>
class SequentialExecutionPolicy
{
public:
    using TaskTag = Kind;
    using SearchTag = SequentialSearch;

    explicit SequentialExecutionPolicy(uint64_t) noexcept {}

    struct WorkerState
    {
        explicit WorkerState(uint64_t) noexcept {}
    };

    template<typename Options>
    static void validate(const Options&)
    {
    }

    template<typename Options>
    static size_t num_workers(const Options&) noexcept
    {
        return 1;
    }

    static ygg::Index<State<Kind>> search_node_index(ygg::Index<State<Kind>> state, ygg::Index<Worker>, size_t) noexcept { return state; }

    static constexpr bool has_start_state_capacity(ygg::uint_t) noexcept { return true; }

    template<typename Engine>
    static std::pair<ygg::Index<Worker>, StateView<Kind>> prepare_start_state(Engine&, const StateView<Kind>& start_state)
    {
        return { ygg::Index<Worker>(0), start_state };
    }

    void initialize_best_h(ygg::float_t value) noexcept { m_best_h_value = value; }

    template<typename Callback>
    bool improve_best_h(ygg::float_t value, Callback&& callback)
    {
        if (value >= m_best_h_value)
            return false;
        m_best_h_value = value;
        std::forward<Callback>(callback)();
        return true;
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

    void start(std::optional<std::chrono::steady_clock::duration> max_time, ygg::float_t, const typename SearchPolicy::Options&)
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
    bool consider_goal(Engine&, WorkerStateIndex<Kind> goal, ygg::float_t cost, bool terminate)
    {
        if (!running())
            return false;
        assert(terminate);
        m_goal = goal;
        m_incumbent_cost = cost;
        m_status = SearchStatus::SOLVED;
        return true;
    }

    ygg::float_t incumbent_cost() const noexcept { return m_incumbent_cost; }

    SearchStatus status() const noexcept { return m_status; }
    std::optional<WorkerStateIndex<Kind>> goal() const noexcept { return m_goal; }
    std::exception_ptr exception() const noexcept { return nullptr; }

    template<typename Engine, typename Worker>
    static bool can_expand(Engine&, const Worker& worker) noexcept
    {
        return !worker.search.empty();
    }

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

    template<typename Engine, typename WorkerData, typename Metadata>
    auto route(Engine&,
               WorkerData& worker,
               ygg::SharedObjectPoolPtr<ygg::Builder<State<Kind>>, true> target,
               PendingActionResult action_result,
               ::tyr::formalism::planning::ActionBindingView action,
               Metadata metadata) -> typename Engine::RoutedSuccessor
    {
        auto node = worker.successor_generator.finalize_successor_state(std::move(target), action_result);
        return typename Engine::RoutedSuccessor { LabeledNode<Kind> { action, std::move(node) }, std::move(metadata) };
    }

    template<typename Engine, typename WorkerData>
    void expand_successors(Engine& engine,
                           WorkerData& worker,
                           const Node<Kind>& node,
                           const typename SearchPolicy::PoppedEntry& entry,
                           typename SearchPolicy::SearchNode& search_node,
                           StateRepository<Kind>& state_repository)
    {
        worker.routed_successors.clear();
        worker.routed_successors.reserve(worker.applicable_actions.size());
        for (const auto action : worker.applicable_actions)
        {
            auto successor_state = state_repository.get_state_builder();
            const auto action_result = worker.successor_generator.generate_successor_state(node, action, *successor_state);
            worker.routed_successors.push_back(route(engine,
                                                     worker,
                                                     std::move(successor_state),
                                                     action_result,
                                                     action,
                                                     worker.search.make_successor_metadata(worker.index, entry.state, search_node, action)));
        }

        if (SearchPolicy::check_timeout_after_generation && timed_out())
        {
            set_terminal(engine, SearchStatus::OUT_OF_TIME);
            return;
        }

        if (engine.m_options.shuffle_labeled_succ_nodes)
            ygg::portable_shuffle(worker.routed_successors.begin(), worker.routed_successors.end(), worker.rng);

        for (const auto& routed_successor : worker.routed_successors)
        {
            if (SearchPolicy::check_timeout_per_successor && timed_out())
            {
                set_terminal(engine, SearchStatus::OUT_OF_TIME);
                return;
            }

            const auto result = engine.accept_successor(worker, node, routed_successor);
            engine.handle_acceptance(result);
            if (result == AcceptanceResult::TERMINAL)
                return;
        }
    }

    template<typename Engine, typename WorkerData, typename EmitTransition>
    static std::optional<AcceptanceResult>
    accept_generated_goal(Engine&, WorkerData&, typename SearchPolicy::SearchNode&, const StateView<Kind>&, ygg::float_t, EmitTransition&&)
    {
        return std::nullopt;
    }

    template<typename Engine, typename WorkerData>
    static bool is_queued_goal(Engine& engine, WorkerData& worker, const StateView<Kind>& state)
    {
        return worker.goal_strategy->is_dynamic_goal_satisfied(engine.m_start_node.get_state(), state);
    }

    template<typename Engine>
    static void snapshot_worker_state_statistics(Engine& engine)
    {
        auto& worker = engine.get_worker(ygg::Index<Worker>(0));
        detail::snapshot_state_repository_statistics(*worker.successor_generator.get_state_repository(), worker.statistics);
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

private:
    ygg::float_t m_best_h_value { 0 };
    ygg::float_t m_incumbent_cost { std::numeric_limits<ygg::float_t>::infinity() };
    SearchStatus m_status { SearchStatus::IN_PROGRESS };
    std::optional<WorkerStateIndex<Kind>> m_goal;
    std::optional<ygg::CountdownWatch> m_stopwatch;
};

template<TaskKind Kind, SearchPolicyConcept<Kind> SearchPolicy, ExecutionPolicyConcept<Kind, SearchPolicy> ExecutionPolicy, typename WorkerData>
class WorkerPolicy<SequentialSearch, Kind, SearchPolicy, ExecutionPolicy, WorkerData>
{
public:
    WorkerPolicy(Task<Kind>& task,
                 SuccessorGenerator<Kind>& successor_generator,
                 Heuristic<Kind>& heuristic,
                 const typename SearchPolicy::Options& options,
                 const typename SearchPolicy::EventHandlerPtr& event_handler) :
        m_worker(ygg::Index<Worker>(0),
                 successor_generator,
                 heuristic,
                 options,
                 options.pruning_strategy ? options.pruning_strategy : PruningStrategy<Kind>::create(),
                 options.goal_strategy ? options.goal_strategy : ConjunctiveGoalStrategy<Kind>::create(task),
                 event_handler ? event_handler->make_worker(ygg::Index<Worker>(0)) : nullptr)
    {
    }

    size_t size() const noexcept { return 1; }

    WorkerData& get(ygg::Index<Worker> index) noexcept
    {
        assert(index == ygg::Index<Worker>(0));
        return m_worker;
    }

    const WorkerData& get(ygg::Index<Worker> index) const noexcept
    {
        assert(index == ygg::Index<Worker>(0));
        return m_worker;
    }

    template<typename Callback>
    void for_each(Callback&& callback)
    {
        callback(m_worker);
    }

    template<typename Callback>
    void for_each(Callback&& callback) const
    {
        callback(m_worker);
    }

    std::pair<Plan<Kind>, Node<Kind>>
    reconstruct_solution(WorkerStateIndex<Kind> goal, SuccessorGenerator<Kind>&, const typename SearchPolicy::Options& options)
    {
        auto& worker = get(goal.worker);
        const auto state = worker.successor_generator.get_state_repository()->get_registered_state(goal.state);
        const auto& search_node = worker.get_search_node(goal.state);
        auto node = Node<Kind>(state, search_node.g_value);
        auto plan = PlanReconstructionPolicy<SequentialSearch>::extract_total_ordered_plan(search_node,
                                                                                           node,
                                                                                           worker.search.get_search_nodes(),
                                                                                           worker.successor_generator,
                                                                                           options.cost_mode);
        return { std::move(plan), std::move(node) };
    }

private:
    WorkerData m_worker;
};

}

#endif
