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
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <limits>
#include <optional>
#include <stdexcept>
#include <utility>

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
    bool begin_iteration(Engine&, WorkerData& worker)
    {
        if (!running())
            return false;
        if (!worker.search.empty())
            return true;
        set_terminal(SearchStatus::EXHAUSTED);
        return false;
    }

    void start(std::optional<std::chrono::steady_clock::time_point> deadline, ygg::float_t, const typename SearchPolicy::Options&)
    {
        m_deadline = deadline;
        m_num_states = 1;
        m_status = SearchStatus::IN_PROGRESS;
    }

    bool running() const noexcept { return m_status == SearchStatus::IN_PROGRESS; }
    bool timed_out() const { return m_deadline && std::chrono::steady_clock::now() >= *m_deadline; }

    void set_terminal(SearchStatus status)
    {
        if (running())
            m_status = status;
    }

    bool consider_goal(WorkerStateIndex<Kind> goal, ygg::float_t cost, [[maybe_unused]] bool terminate)
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
    static bool can_expand_locked(Engine&, const Worker& worker) noexcept
    {
        return !worker.search.empty();
    }

    template<typename Worker, typename Callback>
    static decltype(auto) with_worker_lock(Worker&, Callback&& callback)
    {
        return std::forward<Callback>(callback)([](auto&& evaluate) { return std::forward<decltype(evaluate)>(evaluate)(); });
    }

    template<typename Engine>
    static constexpr void notify_if_stopped(Engine&) noexcept
    {
    }

    template<typename Engine, typename Worker>
    void wait_for_work(Engine&, Worker&)
    {
        set_terminal(SearchStatus::EXHAUSTED);
    }

    template<typename Engine, typename WorkerData>
    AcceptanceResult route(Engine& engine,
                           WorkerData& worker,
                           const Node<Kind>& source,
                           ygg::SharedObjectPoolPtr<ygg::Builder<State<Kind>>, true> target,
                           PendingActionResult action_result,
                           ::tyr::formalism::planning::ActionBindingView action,
                           typename SearchPolicy::SuccessorMetadata metadata)
    {
        const auto metric = engine.complete_successor_state(worker, *target, action_result);
        auto node = Node<Kind>(worker.state_repository.register_extended_state(std::move(target)), metric);
        const auto g_value = compute_successor_g_value(metadata.source_g_value, node.get_metric(), engine.m_options.cost_mode);
        if (!std::isfinite(g_value))
            throw std::runtime_error("find_solution(...): successor path cost is not finite.");

        worker.statistics.increment_num_generated_successors(false);
        const auto routed = typename Engine::RoutedSuccessor { LabeledNode<Kind> { action, std::move(node) }, std::move(metadata), g_value, false };
        return engine.accept_successor(worker, source, routed, [&](const StateView<Kind>& state) { return worker.heuristic.evaluate(state); });
    }

    template<typename Engine, typename WorkerData, typename EmitTransition>
    static std::optional<AcceptanceResult> accept_generated_goal(Engine&,
                                                                 WorkerData&,
                                                                 typename SearchPolicy::SearchNode&,
                                                                 const typename Engine::RoutedSuccessor&,
                                                                 const StateView<Kind>&,
                                                                 ygg::float_t,
                                                                 EmitTransition&&)
    {
        return std::nullopt;
    }

    template<typename Engine, typename WorkerData>
    static bool is_generated_goal(Engine& engine, WorkerData& worker, const typename Engine::RoutedSuccessor&, const StateView<Kind>& state)
    {
        return worker.goal_strategy->is_dynamic_goal_satisfied(engine.m_start_node.get_state(), state);
    }

    template<typename Engine>
    static void snapshot_worker_state_statistics(Engine& engine)
    {
        auto& worker = engine.get_worker(ygg::Index<Worker>(0));
        detail::snapshot_state_repository_statistics(worker.state_repository, worker.statistics);
    }

    bool reserve_state(ygg::uint_t max_num_states) noexcept
    {
        if (max_num_states == std::numeric_limits<ygg::uint_t>::max())
            return true;
        if (m_num_states >= max_num_states)
            return false;
        ++m_num_states;
        return true;
    }
    template<typename Engine>
    void finish_expansion(Engine&) noexcept
    {
    }

private:
    ygg::float_t m_best_h_value { 0 };
    ygg::float_t m_incumbent_cost { std::numeric_limits<ygg::float_t>::infinity() };
    SearchStatus m_status { SearchStatus::IN_PROGRESS };
    ygg::uint_t m_num_states { 0 };
    std::optional<WorkerStateIndex<Kind>> m_goal;
    std::optional<std::chrono::steady_clock::time_point> m_deadline;
};

template<TaskKind Kind, SearchPolicyConcept<Kind> SearchPolicy, ExecutionPolicyConcept<Kind, SearchPolicy> ExecutionPolicy, typename WorkerData>
class WorkerPolicy<SequentialSearch, Kind, SearchPolicy, ExecutionPolicy, WorkerData>
{
public:
    WorkerPolicy(Task<Kind>& task,
                 StateRepository<Kind>& state_repository,
                 AxiomEvaluator<Kind>& axiom_evaluator,
                 SuccessorGenerator<Kind>& successor_generator,
                 Heuristic<Kind>& heuristic,
                 const typename SearchPolicy::Options& options,
                 const typename SearchPolicy::EventHandlerPtr& event_handler) :
        m_worker(ygg::Index<Worker>(0),
                 state_repository,
                 axiom_evaluator,
                 successor_generator,
                 heuristic,
                 1,
                 options,
                 options.pruning_strategy ? options.pruning_strategy : PruningStrategy<Kind>::create(),
                 options.goal_strategy ? options.goal_strategy : ConjunctiveGoalStrategy<Kind>::create(task),
                 SearchPolicy::make_worker_event_handler(event_handler, ygg::Index<Worker>(0)))
    {
    }

    size_t size() const noexcept { return 1; }

    WorkerData& get([[maybe_unused]] ygg::Index<Worker> index) noexcept
    {
        assert(index == ygg::Index<Worker>(0));
        return m_worker;
    }

    template<typename Callback>
    void for_each(Callback&& callback)
    {
        callback(m_worker);
    }

    std::pair<Plan<Kind>, Node<Kind>> reconstruct_solution(WorkerStateIndex<Kind> goal, const typename SearchPolicy::Options& options)
    {
        auto& worker = get(goal.worker);
        const auto state = worker.state_repository.get_registered_state(goal.state);
        const auto& search_node = worker.get_search_node(goal.state);
        auto node = Node<Kind>(state, search_node.g_value);
        auto plan = PlanReconstructionPolicy<SequentialSearch>::extract_total_ordered_plan(search_node,
                                                                                           node,
                                                                                           worker.search.get_search_nodes(),
                                                                                           worker.state_repository,
                                                                                           worker.axiom_evaluator,
                                                                                           worker.successor_generator,
                                                                                           options.cost_mode);
        return { std::move(plan), std::move(node) };
    }

private:
    WorkerData m_worker;
};

}

#endif
