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

#ifndef TYR_SRC_PLANNING_ALGORITHMS_SEARCH_ENGINE_ENGINE_HPP_
#define TYR_SRC_PLANNING_ALGORITHMS_SEARCH_ENGINE_ENGINE_HPP_

#include "../repository_statistics.hpp"
#include "concepts.hpp"
#include "tyr/planning/algorithms/statistics.hpp"
#include "tyr/planning/algorithms/strategies/goal.hpp"
#include "tyr/planning/algorithms/strategies/pruning.hpp"
#include "tyr/planning/algorithms/utils.hpp"
#include "tyr/planning/ground/state_repository.hpp"
#include "tyr/planning/ground/state_view.hpp"
#include "tyr/planning/ground/successor_generator.hpp"
#include "tyr/planning/ground/task.hpp"
#include "tyr/planning/heuristic.hpp"
#include "tyr/planning/lifted/state_repository.hpp"
#include "tyr/planning/lifted/state_view.hpp"
#include "tyr/planning/lifted/successor_generator.hpp"
#include "tyr/planning/lifted/task.hpp"
#include "tyr/planning/node.hpp"
#include "tyr/planning/state_repository.hpp"
#include "tyr/planning/worker_state_index.hpp"

#include <cassert>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <memory>
#include <optional>
#include <random>
#include <stdexcept>
#include <utility>
#include <vector>
#include <yggdrasil/containers/segmented_vector.hpp>

namespace tyr::planning::detail
{

enum class ExpansionResult : uint8_t
{
    SKIP,
    EXPAND,
    GOAL,
};

template<TaskKind Kind, typename SearchNode>
SearchNode& get_or_create_search_node(ygg::Index<State<Kind>> state_index, ygg::SegmentedVector<SearchNode>& search_nodes, const SearchNode& default_node)
{
    while (ygg::uint_t(state_index) >= search_nodes.size())
        search_nodes.push_back(default_node);

    return search_nodes[ygg::uint_t(state_index)];
}

template<TaskKind Kind, SearchPolicyConcept<Kind> SearchPolicy, ExecutionPolicyConcept<Kind, SearchPolicy> ExecutionPolicy>
class SearchEngine
{
    friend SearchPolicy;
    friend ExecutionPolicy;

public:
    using SearchTag = typename SearchPolicy::SearchTag;
    using Options = typename SearchPolicy::Options;
    using SearchNode = typename SearchPolicy::SearchNode;
    using WorkerExecutionState = typename ExecutionPolicy::WorkerState;

    struct RoutedSuccessor
    {
        LabeledNode<Kind> labeled_node;
        typename SearchPolicy::SuccessorMetadata metadata;
    };

    struct WorkerData
    {
        WorkerData(ygg::Index<Worker> index_,
                   SuccessorGenerator<Kind>& successor_generator_,
                   Heuristic<Kind>& heuristic_,
                   const Options& options,
                   PruningStrategyPtr<Kind> pruning_strategy_,
                   GoalStrategyPtr<Kind> goal_strategy_,
                   typename SearchPolicy::WorkerEventHandlerPtr event_handler_) :
            index(index_),
            successor_generator(successor_generator_),
            heuristic(heuristic_),
            search(heuristic, options),
            execution(options.random_seed),
            rng(options.random_seed + ygg::uint_t(index)),
            pruning_strategy(std::move(pruning_strategy_)),
            goal_strategy(std::move(goal_strategy_)),
            event_handler(std::move(event_handler_))
        {
        }

        WorkerData(ygg::Index<Worker> index_,
                   ygg::ExecutionContextPtr execution_context_,
                   SuccessorGeneratorPtr<Kind> successor_generator_,
                   HeuristicPtr<Kind> heuristic_,
                   size_t num_state_owners_,
                   const Options& options,
                   PruningStrategyPtr<Kind> pruning_strategy_,
                   GoalStrategyPtr<Kind> goal_strategy_,
                   typename SearchPolicy::WorkerEventHandlerPtr event_handler_) :
            index(index_),
            num_state_owners(num_state_owners_),
            execution_context(std::move(execution_context_)),
            owned_successor_generator(std::move(successor_generator_)),
            owned_heuristic(std::move(heuristic_)),
            successor_generator(*owned_successor_generator),
            heuristic(*owned_heuristic),
            search(heuristic, options),
            execution(options.random_seed),
            rng(options.random_seed + ygg::uint_t(index)),
            pruning_strategy(std::move(pruning_strategy_)),
            goal_strategy(std::move(goal_strategy_)),
            event_handler(std::move(event_handler_))
        {
            assert(num_state_owners > 0);
        }

        ygg::Index<State<Kind>> get_search_node_index(ygg::Index<State<Kind>> state) const noexcept
        {
            return ExecutionPolicy::search_node_index(state, index, num_state_owners);
        }

        SearchNode& initialize_start(ygg::Index<State<Kind>> state, ygg::float_t g_value, ygg::float_t h_value)
        {
            return search.initialize_start(get_search_node_index(state), g_value, h_value);
        }

        SearchNode& get_search_node(ygg::Index<State<Kind>> state) { return search.get_search_node(get_search_node_index(state)); }

        ygg::Index<Worker> index;
        size_t num_state_owners { 1 };
        ygg::ExecutionContextPtr execution_context;
        SuccessorGeneratorPtr<Kind> owned_successor_generator;
        HeuristicPtr<Kind> owned_heuristic;
        SuccessorGenerator<Kind>& successor_generator;
        Heuristic<Kind>& heuristic;
        SearchPolicy search;
        WorkerExecutionState execution;
        std::mt19937_64 rng;
        std::vector<::tyr::formalism::planning::ActionBindingView> applicable_actions;
        PruningStrategyPtr<Kind> pruning_strategy;
        GoalStrategyPtr<Kind> goal_strategy;
        Statistics statistics;
        typename SearchPolicy::WorkerEventHandlerPtr event_handler;
    };

    using Workers = WorkerPolicy<SearchTag, Kind, SearchPolicy, ExecutionPolicy, WorkerData>;

    static_assert(WorkerPolicyConcept<Workers, WorkerData, Kind, SearchPolicy>);

    static SearchResult<Kind> find_solution(Task<Kind>& task, SuccessorGenerator<Kind>& successor_generator, Heuristic<Kind>& heuristic, const Options& options)
    {
        ExecutionPolicy::validate(options);
        return SearchEngine(task, successor_generator, heuristic, options).run();
    }

    size_t num_workers() const noexcept { return m_workers.size(); }

    WorkerData& get_worker(ygg::Index<Worker> index) noexcept { return m_workers.get(index); }

    const WorkerData& get_worker(ygg::Index<Worker> index) const noexcept { return m_workers.get(index); }

    void on_finish_priority_layer(ygg::float_t priority)
    {
        if (!m_event_handler || !SearchPolicy::emits_priority_layer_events)
            return;

        auto statistics = Statistics {};
        for_each_worker([&](const auto& worker) { statistics.add(worker.statistics); });
        statistics.set_search_start_time_point(m_search_start_time_point);
        statistics.set_search_end_time_point(std::chrono::steady_clock::now());
        call_root_event([&](auto& handler) { SearchPolicy::on_finish_priority_layer(handler, priority, statistics); });
    }

    void worker_loop(WorkerData& worker)
    {
        try
        {
            worker_loop_impl(worker);
        }
        catch (...)
        {
            worker.statistics.set_search_end_time_point(std::chrono::steady_clock::now());
            throw;
        }
        worker.statistics.set_search_end_time_point(std::chrono::steady_clock::now());
    }

private:
    void worker_loop_impl(WorkerData& worker)
    {
        while (m_execution.begin_iteration(*this, worker))
        {
            if (m_execution.timed_out())
            {
                m_execution.set_terminal(*this, SearchStatus::OUT_OF_TIME);
                m_execution.notify_if_stopped(*this);
                return;
            }

            if (expand_one(worker))
                continue;

            const auto idle_start = std::chrono::steady_clock::now();
            m_execution.wait_for_work(*this, worker);
            worker.statistics.add_idle_time(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - idle_start));
            if (!m_execution.running())
                return;
        }
    }

    SearchEngine(Task<Kind>& task, SuccessorGenerator<Kind>& successor_generator, Heuristic<Kind>& heuristic, const Options& options) :
        m_task(task),
        m_caller_successor_generator(successor_generator),
        m_options(options),
        m_event_handler(options.event_handler),
        m_start_node(make_start_node(task, successor_generator, options)),
        m_execution(options.random_seed),
        m_workers(task, successor_generator, heuristic, options, m_event_handler)
    {
    }

    SearchResult<Kind> run()
    {
        auto [start_owner, start_state] = prepare_start_state();
        auto& start_worker = get_worker(start_owner);
        const auto start_state_index = start_state.get_index();
        const auto start_g_value = ygg::FloatTolerance<ygg::float_t>::canonicalize(m_start_node.get_metric());
        if (std::isnan(start_g_value))
            throw std::runtime_error("find_solution(...): start node metric value is NaN.");

        const auto search_start = std::chrono::steady_clock::now();
        m_search_start_time_point = search_start;
        m_result.statistics.set_search_start_time_point(search_start);
        for_each_worker(
            [&](auto& worker)
            {
                worker.statistics.set_search_start_time_point(search_start);
                worker.statistics.set_search_end_time_point(search_start);
            });

        if (!start_worker.goal_strategy->is_static_goal_satisfied(m_task))
        {
            finalize(SearchStatus::UNSOLVABLE);
            return std::move(m_result);
        }

        if (start_worker.goal_strategy->is_dynamic_goal_satisfied(start_state, start_state))
        {
            m_result.plan = Plan(m_start_node, LabeledNodeList<Kind> {});
            m_result.goal_node = m_start_node;
            finalize(SearchStatus::SOLVED);
            call_root_event([&](auto& handler) { handler.on_solved(*m_result.plan); });
            return std::move(m_result);
        }

        if (!ExecutionPolicy::has_start_state_capacity(m_options.max_num_states))
        {
            finalize(SearchStatus::OUT_OF_STATES);
            return std::move(m_result);
        }

        if (start_worker.pruning_strategy->should_prune_state(start_state))
        {
            finalize(SearchStatus::EXHAUSTED);
            return std::move(m_result);
        }

        const auto start_h_value = ygg::FloatTolerance<ygg::float_t>::canonicalize(start_worker.heuristic.evaluate(start_state));
        if (std::isnan(start_h_value))
            throw std::runtime_error("find_solution(...): start heuristic value is NaN.");
        m_execution.initialize_best_h(start_h_value);
        auto& start_search_node = start_worker.initialize_start(start_state_index, start_g_value, start_h_value);
        call_root_event([&](auto& handler) { SearchPolicy::on_start_search(handler, m_start_node, start_worker.search.get_start_priority()); });

        if (start_search_node.status == SearchNodeStatus::DEAD_END)
        {
            start_worker.statistics.increment_num_deadends();
            finalize(SearchStatus::UNSOLVABLE);
            return std::move(m_result);
        }

        start_worker.search.open_start(start_state_index, start_search_node);
        m_execution.start(m_options.max_time, start_worker.search.get_start_priority(), m_options);
        try
        {
            m_execution.invoke(*this);
            snapshot_statistics();
            return finish_search();
        }
        catch (...)
        {
            finalize(SearchStatus::FAILED);
            throw;
        }
    }

    std::pair<ygg::Index<Worker>, StateView<Kind>> prepare_start_state() { return m_execution.prepare_start_state(*this, m_start_node.get_state()); }

    bool expand_one(WorkerData& worker)
    {
        struct PreparedExpansion
        {
            typename SearchPolicy::PoppedEntry entry;
            Node<Kind> node;
            SearchNode search_node;
        };

        auto prepared = std::optional<PreparedExpansion> {};
        auto claimed = false;
        m_execution.with_worker_lock(
            worker,
            [&]
            {
                if (!m_execution.can_expand_locked(*this, worker))
                    return;

                claimed = true;
                const auto entry = worker.search.pop();
                if (worker.search.should_discard(entry, m_execution.incumbent_cost()))
                    return;

                auto& state_repository = *worker.successor_generator.get_state_repository();
                auto state = state_repository.get_registered_state(entry.state);
                auto& search_node = worker.get_search_node(entry.state);
                auto node = Node<Kind>(std::move(state), search_node.g_value);

                if (search_node.status == SearchNodeStatus::CLOSED || search_node.status == SearchNodeStatus::DEAD_END)
                    return;

                const auto expansion_result = worker.search.prepare_expansion(
                    entry,
                    node,
                    search_node,
                    worker.statistics,
                    [&](ygg::float_t h_value, auto&& callback)
                    { return m_execution.improve_best_h(h_value, [&] { call_root_event(std::forward<decltype(callback)>(callback)); }); },
                    [&](auto&& callback) { call_worker_event(worker, std::forward<decltype(callback)>(callback)); },
                    [&](ygg::float_t priority) { on_finish_priority_layer(priority); });
                if (expansion_result == ExpansionResult::GOAL)
                {
                    solve(worker, search_node, node);
                    return;
                }
                if (expansion_result == ExpansionResult::EXPAND)
                    prepared.emplace(PreparedExpansion { entry, std::move(node), search_node });
            });

        if (!claimed)
            return false;

        m_execution.notify_if_stopped(*this);
        if (prepared && m_execution.running())
        {
            auto& state_repository = *worker.successor_generator.get_state_repository();
            worker.successor_generator.get_applicable_action_bindings(prepared->node, worker.applicable_actions);
            m_execution.expand_successors(*this, worker, prepared->node, prepared->entry, prepared->search_node, state_repository);
        }
        m_execution.finish_expansion(*this);
        return true;
    }

    AcceptanceResult accept_successor(WorkerData& worker, const Node<Kind>& source_node, const RoutedSuccessor& routed_successor)
    {
        const auto& labeled_successor = routed_successor.labeled_node;
        const auto& successor_node = labeled_successor.node;
        const auto& successor_state = successor_node.get_state();
        auto& successor_search_node = worker.get_search_node(successor_state.get_index());

        if (std::isnan(successor_node.get_metric()))
            throw std::runtime_error("find_solution(...): successor metric value is NaN.");

        const auto is_new = successor_search_node.status == SearchNodeStatus::NEW;
        if (is_new && !m_execution.reserve_state(m_options.max_num_states))
        {
            m_execution.set_terminal(*this, SearchStatus::OUT_OF_STATES);
            return AcceptanceResult::TERMINAL;
        }

        const auto g_value = compute_successor_g_value(routed_successor.metadata.source_g_value, successor_node.get_metric(), m_options.cost_mode);
        if (std::isnan(g_value))
            throw std::runtime_error("find_solution(...): successor path cost is NaN.");
        const auto normalized_node = Node<Kind>(successor_state, g_value);
        const auto emit_transition = [&](TransitionOutcome outcome)
        {
            call_worker_event(worker,
                              [&](auto& handler)
                              { handler.on_generate_transition(source_node, LabeledNode<Kind> { labeled_successor.label, normalized_node }, outcome); });
        };
        return worker.search.accept_successor(*this, worker, source_node, normalized_node, routed_successor, successor_search_node, is_new, emit_transition);
    }

    void solve(WorkerData& worker, const SearchNode&, const Node<Kind>& node)
    {
        const auto goal = WorkerStateIndex<Kind> { worker.index, node.get_state().get_index() };
        if (m_execution.consider_goal(*this, goal, node.get_metric(), SearchPolicy::terminate_on_goal))
            call_worker_event(worker, [&](auto& handler) { handler.on_expand_goal_node(node); });
    }

    void finalize(SearchStatus status)
    {
        if (m_finalized)
            return;

        m_execution.set_terminal(*this, status);
        m_result.status = status;
        snapshot_statistics();
        m_finalized = true;
        call_root_event([&](auto& handler) { handler.on_end_search(status, m_result.statistics); });
    }

    void snapshot_statistics()
    {
        if (m_statistics_snapshotted)
            return;

        const auto search_end = std::chrono::steady_clock::now();
        auto num_registered_states = uint64_t { 0 };
        auto state_storage_memory_usage = size_t { 0 };
        m_execution.snapshot_worker_state_statistics(*this);

        m_result.worker_statistics.clear();
        m_result.worker_statistics.reserve(num_workers());
        for_each_worker(
            [&](auto& worker)
            {
                num_registered_states += worker.statistics.get_num_registered_states();
                state_storage_memory_usage += worker.statistics.get_state_storage_memory_usage();
                m_result.statistics.add(worker.statistics);
                m_result.worker_statistics.push_back(worker.statistics);
            });
        m_result.statistics.set_num_registered_states(num_registered_states);
        m_result.statistics.set_state_storage_memory_usage(state_storage_memory_usage);
        snapshot_task_repository_statistics(*m_task.get_repository(), m_result.statistics);
        m_result.statistics.set_search_end_time_point(search_end);
        m_statistics_snapshotted = true;
    }

    template<typename Callback>
    void for_each_worker(Callback&& callback)
    {
        m_workers.for_each(std::forward<Callback>(callback));
    }

    SearchResult<Kind> finish_search()
    {
        if (const auto exception = m_execution.exception())
        {
            finalize(SearchStatus::FAILED);
            std::rethrow_exception(exception);
        }

        m_result.status = m_execution.status();
        if (m_result.status == SearchStatus::SOLVED)
        {
            const auto goal = m_execution.goal();
            assert(goal);
            auto [plan, goal_node] = m_workers.reconstruct_solution(*goal, m_caller_successor_generator, m_options);
            m_result.plan = std::move(plan);
            m_result.goal_node = std::move(goal_node);
        }

        finalize(m_result.status);
        if (m_result.plan)
            call_root_event([&](auto& handler) { handler.on_solved(*m_result.plan); });
        return std::move(m_result);
    }

    template<typename Callback>
    void call_root_event(Callback&& callback)
    {
        if (m_event_handler)
            std::forward<Callback>(callback)(*m_event_handler);
    }

    template<typename Callback>
    static void call_worker_event(WorkerData& worker, Callback&& callback)
    {
        if (worker.event_handler)
            std::forward<Callback>(callback)(*worker.event_handler);
    }

    static Node<Kind> make_start_node(Task<Kind>& task, SuccessorGenerator<Kind>& successor_generator, const Options& options)
    {
        auto& target_repository = *successor_generator.get_state_repository();
        if (target_repository.get_task().get() != &task)
            throw std::invalid_argument("find_solution(...): successor generator belongs to a different task.");

        const auto start_node = options.start_node ? *options.start_node : successor_generator.get_initial_node();
        if (start_node.get_state().get_state_repository()->get_task().get() != &task)
            throw std::invalid_argument("find_solution(...): start node belongs to a different task.");

        return Node<Kind>(materialize_state(start_node.get_state(), target_repository), start_node.get_metric());
    }

    Task<Kind>& m_task;
    SuccessorGenerator<Kind>& m_caller_successor_generator;
    const Options& m_options;
    typename SearchPolicy::EventHandlerPtr m_event_handler;
    std::chrono::steady_clock::time_point m_search_start_time_point;
    Node<Kind> m_start_node;
    ExecutionPolicy m_execution;
    Workers m_workers;
    SearchResult<Kind> m_result;
    bool m_statistics_snapshotted { false };
    bool m_finalized { false };
};

}

#endif
