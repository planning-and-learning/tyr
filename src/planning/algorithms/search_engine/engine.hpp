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
#include "tyr/planning/search_space/parallel.hpp"
#include "tyr/planning/search_space/sequential.hpp"
#include "tyr/planning/state_routing/dist_hash.hpp"
#include "tyr/planning/state_routing/single_worker.hpp"
#include "tyr/planning/worker_state_index.hpp"

#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <limits>
#include <memory>
#include <optional>
#include <random>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>
#include <yggdrasil/containers/segmented_vector.hpp>
#include <yggdrasil/core/portable_shuffle.hpp>

namespace tyr::planning::detail
{

enum class ExpansionResult : uint8_t
{
    SKIP,
    EXPAND,
    GOAL,
};

enum class AcceptanceResult : uint8_t
{
    QUEUED,
    DISCARDED,
    TERMINAL,
};

template<TaskKind Kind, typename SearchNode>
SearchNode& get_or_create_search_node(ygg::Index<State<Kind>> state_index, ygg::SegmentedVector<SearchNode>& search_nodes, const SearchNode& default_node)
{
    while (ygg::uint_t(state_index) >= search_nodes.size())
        search_nodes.push_back(default_node);

    return search_nodes[ygg::uint_t(state_index)];
}

template<TaskKind Kind, typename SearchPolicy, typename ExecutionPolicy, DistHashKind HashKind>
class SearchEngine
{
public:
    using Options = typename SearchPolicy::Options;
    using SearchNode = typename SearchPolicy::SearchNode;
    using WorkerExecutionState = typename ExecutionPolicy::template WorkerState<HashKind, typename SearchPolicy::SuccessorMetadata>;

    struct IncomingSuccessor
    {
        Node<Kind> source;
        RoutedSuccessor<Kind, typename SearchPolicy::SuccessorMetadata> routed;
    };

    struct WorkerData
    {
        WorkerData(ygg::Index<Worker> index_,
                   SuccessorGenerator<Kind>& successor_generator_,
                   Heuristic<Kind>& heuristic_,
                   const Options& options,
                   PruningStrategyPtr<Kind> pruning_strategy_,
                   GoalStrategyPtr<Kind> goal_strategy_) :
            index(index_),
            successor_generator(successor_generator_),
            heuristic(heuristic_),
            search(heuristic, options),
            execution(options.random_seed),
            rng(options.random_seed + ygg::uint_t(index)),
            pruning_strategy(std::move(pruning_strategy_)),
            goal_strategy(std::move(goal_strategy_))
        {
        }

        WorkerData(ygg::Index<Worker> index_,
                   ygg::ExecutionContextPtr execution_context_,
                   SuccessorGeneratorPtr<Kind> successor_generator_,
                   HeuristicPtr<Kind> heuristic_,
                   const Options& options,
                   PruningStrategyPtr<Kind> pruning_strategy_,
                   GoalStrategyPtr<Kind> goal_strategy_) :
            index(index_),
            execution_context(std::move(execution_context_)),
            owned_successor_generator(std::move(successor_generator_)),
            owned_heuristic(std::move(heuristic_)),
            successor_generator(*owned_successor_generator),
            heuristic(*owned_heuristic),
            search(heuristic, options),
            execution(options.random_seed),
            rng(options.random_seed + ygg::uint_t(index)),
            pruning_strategy(std::move(pruning_strategy_)),
            goal_strategy(std::move(goal_strategy_))
        {
        }

        ygg::Index<Worker> index;
        ygg::ExecutionContextPtr execution_context;
        SuccessorGeneratorPtr<Kind> owned_successor_generator;
        HeuristicPtr<Kind> owned_heuristic;
        SuccessorGenerator<Kind>& successor_generator;
        Heuristic<Kind>& heuristic;
        SearchPolicy search;
        WorkerExecutionState execution;
        std::mt19937_64 rng;
        std::vector<::tyr::formalism::planning::ActionBindingView> applicable_actions;
        std::vector<RoutedSuccessor<Kind, typename SearchPolicy::SuccessorMetadata>> routed_successors;
        PruningStrategyPtr<Kind> pruning_strategy;
        GoalStrategyPtr<Kind> goal_strategy;
    };

    using WorkerStorage = std::conditional_t<ExecutionPolicy::parallel, std::vector<std::unique_ptr<WorkerData>>, WorkerData>;

    static SearchResult<Kind> find_solution(Task<Kind>& task, SuccessorGenerator<Kind>& successor_generator, Heuristic<Kind>& heuristic, const Options& options)
    {
        ExecutionPolicy::validate(options);
        return SearchEngine(task, successor_generator, heuristic, options).run();
    }

    size_t num_workers() const noexcept
    {
        if constexpr (ExecutionPolicy::parallel)
            return m_workers.size();
        else
            return 1;
    }

    WorkerData& get_worker(ygg::Index<Worker> index) noexcept
    {
        if constexpr (ExecutionPolicy::parallel)
        {
            const auto value = static_cast<size_t>(ygg::uint_t(index));
            assert(value < m_workers.size());
            return *m_workers[value];
        }
        else
        {
            assert(index == ygg::Index<Worker>(0));
            return m_workers;
        }
    }

    void worker_loop(WorkerData& worker)
    {
        while (m_execution.begin_iteration(*this, worker))
        {
            if (m_execution.timed_out())
            {
                m_execution.set_terminal(*this, SearchStatus::OUT_OF_TIME);
                return;
            }

            // Register queued states before generating more work from the local open list.
            while (auto incoming = m_execution.receive_one(*this, worker))
            {
                const auto result = accept_successor(worker, incoming->source, incoming->routed);
                handle_acceptance(result);
                if (result == AcceptanceResult::TERMINAL || !m_execution.running())
                    return;
            }

            if (!worker.search.empty())
            {
                expand_one(worker);
                continue;
            }

            m_execution.wait_for_work(*this, worker);
            if (!m_execution.running())
                return;
        }
    }

private:
    SearchEngine(Task<Kind>& task, SuccessorGenerator<Kind>& successor_generator, Heuristic<Kind>& heuristic, const Options& options) :
        m_task(task),
        m_caller_successor_generator(successor_generator),
        m_options(options),
        m_event_handler(ExecutionPolicy::template make_event_handler<SearchPolicy>(options)),
        m_start_node(options.start_node ? *options.start_node : successor_generator.get_initial_node()),
        m_execution(options.random_seed),
        m_workers(make_workers(task, successor_generator, heuristic, options))
    {
    }

    static WorkerStorage make_workers(Task<Kind>& task, SuccessorGenerator<Kind>& successor_generator, Heuristic<Kind>& heuristic, const Options& options)
    {
        if constexpr (ExecutionPolicy::parallel)
        {
            auto workers = std::vector<std::unique_ptr<WorkerData>> {};
            workers.reserve(ExecutionPolicy::num_workers(options));
            for (size_t i = 0; i < ExecutionPolicy::num_workers(options); ++i)
            {
                const auto index = ygg::Index<Worker>(static_cast<ygg::uint_t>(i));
                auto execution_context = ExecutionPolicy::create_execution_context();
                auto worker_successor_generator = successor_generator.make_worker(execution_context);
                auto worker_heuristic = heuristic.make_worker(execution_context);
                workers.push_back(std::make_unique<WorkerData>(index,
                                                               std::move(execution_context),
                                                               std::move(worker_successor_generator),
                                                               std::move(worker_heuristic),
                                                               options,
                                                               PruningStrategy<Kind>::create(),
                                                               ConjunctiveGoalStrategy<Kind>::create(task)));
            }
            return workers;
        }
        else
        {
            return WorkerData(ygg::Index<Worker>(0),
                              successor_generator,
                              heuristic,
                              options,
                              options.pruning_strategy ? options.pruning_strategy : PruningStrategy<Kind>::create(),
                              options.goal_strategy ? options.goal_strategy : ConjunctiveGoalStrategy<Kind>::create(task));
        }
    }

    SearchResult<Kind> run()
    {
        const auto start_owner = m_execution.owner(*this, m_start_node.get_state().get_state_builder());
        auto& start_worker = get_worker(start_owner);
        auto start_state = prepare_start_state(start_worker);
        const auto start_state_index = start_state.get_index();
        const auto start_g_value = ygg::FloatTolerance<ygg::float_t>::canonicalize(m_start_node.get_metric());
        const auto start_h_value = ygg::FloatTolerance<ygg::float_t>::canonicalize(start_worker.heuristic.evaluate(start_state));
        m_execution.initialize_best_h(start_h_value);
        auto& start_search_node = start_worker.search.initialize_start(start_state_index, start_g_value, start_h_value);

        call_event([&](auto& handler) { handler.on_start_search(m_start_node, start_worker.search.get_start_priority()); });

        if (!start_worker.goal_strategy->is_static_goal_satisfied(m_task))
        {
            finish(SearchStatus::UNSOLVABLE);
            return std::move(m_result);
        }

        if (start_worker.goal_strategy->is_dynamic_goal_satisfied(start_state, start_state))
        {
            m_result.plan = Plan(m_start_node, LabeledNodeList<Kind> {});
            m_result.goal_node = m_start_node;
            m_result.status = SearchStatus::SOLVED;
            call_event([&](auto& handler) { handler.on_end_search(m_result.status); });
            call_event([&](auto& handler) { handler.on_solved(*m_result.plan); });
            return std::move(m_result);
        }

        if (std::isnan(m_start_node.get_metric()))
        {
            call_event([](auto& handler) { handler.on_end_search(SearchStatus::FAILED); });
            throw std::runtime_error("find_solution(...): start node metric value is NaN.");
        }

        if (start_search_node.status == SearchNodeStatus::DEAD_END)
        {
            finish(SearchStatus::UNSOLVABLE);
            return std::move(m_result);
        }

        if (start_worker.pruning_strategy->should_prune_state(start_state))
        {
            finish(SearchStatus::EXHAUSTED);
            return std::move(m_result);
        }

        if constexpr (ExecutionPolicy::parallel)
        {
            if (m_options.max_num_states == 0)
            {
                finish(SearchStatus::OUT_OF_STATES);
                return std::move(m_result);
            }
        }

        start_worker.search.open_start(start_state_index, start_search_node);
        m_execution.start(m_options.max_time);
        m_execution.invoke(*this);
        return finish_search();
    }

    StateView<Kind> prepare_start_state(WorkerData& worker)
    {
        if constexpr (!ExecutionPolicy::parallel)
        {
            return m_start_node.get_state();
        }
        else
        {
            auto builder = worker.successor_generator.get_state_repository()->get_state_builder();
            builder->assign_unextended_part(m_start_node.get_state().get_state_builder());
            return worker.successor_generator.get_state_repository()->register_state(std::move(builder));
        }
    }

    void expand_one(WorkerData& worker)
    {
        const auto entry = worker.search.pop();
        auto& state_repository = *worker.successor_generator.get_state_repository();
        const auto state = state_repository.get_registered_state(entry.state);
        auto& search_node = worker.search.get_search_node(entry.state);
        auto node = Node<Kind>(state, search_node.g_value);

        if (search_node.status == SearchNodeStatus::CLOSED || search_node.status == SearchNodeStatus::DEAD_END)
        {
            m_execution.finish_expansion(*this);
            return;
        }

        const auto expansion_result = worker.search.prepare_expansion(
            entry,
            node,
            search_node,
            [&](ygg::float_t h_value, auto&& callback)
            { return m_execution.improve_best_h(h_value, m_event_handler, std::forward<decltype(callback)>(callback)); },
            [&](auto&& callback) { call_event(std::forward<decltype(callback)>(callback)); });
        if (expansion_result == ExpansionResult::SKIP)
        {
            m_execution.finish_expansion(*this);
            return;
        }

        if (expansion_result == ExpansionResult::GOAL)
        {
            solve(worker, search_node, node);
            m_execution.finish_expansion(*this);
            return;
        }

        worker.successor_generator.get_applicable_action_bindings(node, worker.applicable_actions);
        if constexpr (ExecutionPolicy::parallel)
        {
            if (m_options.shuffle_labeled_succ_nodes)
                ygg::portable_shuffle(worker.applicable_actions.begin(), worker.applicable_actions.end(), worker.rng);

            for (const auto action : worker.applicable_actions)
            {
                if (!m_execution.running())
                    break;

                auto successor_state = state_repository.get_state_builder();
                const auto action_result = worker.successor_generator.generate_successor_state(node, action, *successor_state);
                auto metadata = worker.search.make_successor_metadata(worker.index, entry.state, search_node, action);
                if (auto incoming = m_execution.route(*this, worker, node, std::move(successor_state), action_result, action, std::move(metadata)))
                {
                    const auto result = accept_successor(worker, incoming->source, incoming->routed);
                    handle_acceptance(result);
                    if (result == AcceptanceResult::TERMINAL || !m_execution.running())
                        break;
                }
            }
        }
        else
        {
            worker.routed_successors.clear();
            worker.routed_successors.reserve(worker.applicable_actions.size());
            for (const auto action : worker.applicable_actions)
            {
                auto successor_state = state_repository.get_state_builder();
                const auto action_result = worker.successor_generator.generate_successor_state(node, action, *successor_state);
                worker.execution.router.send(std::move(successor_state),
                                             action_result,
                                             action,
                                             worker.search.make_successor_metadata(worker.index, entry.state, search_node, action));
                worker.routed_successors.push_back(worker.execution.router.receive(worker.successor_generator));
            }

            if constexpr (SearchPolicy::check_timeout_after_generation)
            {
                if (m_execution.timed_out())
                {
                    m_execution.set_terminal(*this, SearchStatus::OUT_OF_TIME);
                    return;
                }
            }

            if (m_options.shuffle_labeled_succ_nodes)
                ygg::portable_shuffle(worker.routed_successors.begin(), worker.routed_successors.end(), worker.rng);

            for (const auto& routed_successor : worker.routed_successors)
            {
                if constexpr (SearchPolicy::check_timeout_per_successor)
                {
                    if (m_execution.timed_out())
                    {
                        m_execution.set_terminal(*this, SearchStatus::OUT_OF_TIME);
                        return;
                    }
                }

                const auto result = accept_successor(worker, node, routed_successor);
                handle_acceptance(result);
                if (result == AcceptanceResult::TERMINAL)
                    return;
            }
        }

        m_execution.finish_expansion(*this);
    }

    void handle_acceptance(AcceptanceResult result)
    {
        if (result == AcceptanceResult::DISCARDED)
            m_execution.release_successor(*this);
    }

    AcceptanceResult
    accept_successor(WorkerData& worker, const Node<Kind>& source_node, const RoutedSuccessor<Kind, typename SearchPolicy::SuccessorMetadata>& routed_successor)
    {
        const auto& source_state = source_node.get_state();
        const auto& labeled_successor = routed_successor.labeled_node;
        const auto& successor_node = labeled_successor.node;
        const auto& successor_state = successor_node.get_state();
        auto& successor_search_node = worker.search.get_search_node(successor_state.get_index());

        assert(!std::isnan(successor_node.get_metric()));

        const auto is_new = successor_search_node.status == SearchNodeStatus::NEW;
        if (is_new && !m_execution.reserve_state(worker.search.get_search_nodes().size(), m_options.max_num_states))
        {
            m_execution.set_terminal(*this, SearchStatus::OUT_OF_STATES);
            return AcceptanceResult::TERMINAL;
        }

        if constexpr (SearchPolicy::eager)
        {
            if (worker.pruning_strategy->should_prune_successor_state(source_state, successor_state, is_new))
            {
                successor_search_node.status = SearchNodeStatus::CLOSED;
                call_event([&](auto& handler) { handler.on_prune_node(source_node, labeled_successor); });
                return AcceptanceResult::DISCARDED;
            }

            const auto g_value = compute_successor_g_value(routed_successor.metadata.source_g_value, successor_node.get_metric(), m_options.cost_mode);
            const auto normalized_node = Node<Kind>(successor_state, g_value);
            const auto normalized_successor = LabeledNode<Kind> { labeled_successor.label, normalized_node };

            if (g_value < successor_search_node.g_value)
            {
                call_event([&](auto& handler) { handler.on_generate_node(source_node, normalized_successor); });
                worker.search.set_parent(successor_search_node, routed_successor.metadata.parent);
                successor_search_node.g_value = g_value;

                const auto h_value = ygg::FloatTolerance<ygg::float_t>::canonicalize(worker.heuristic.evaluate(successor_state));
                if (h_value == std::numeric_limits<ygg::float_t>::infinity())
                {
                    successor_search_node.status = SearchNodeStatus::DEAD_END;
                    return AcceptanceResult::DISCARDED;
                }

                successor_search_node.status = worker.goal_strategy->is_dynamic_goal_satisfied(m_start_node.get_state(), successor_state) ?
                                                   SearchNodeStatus::GOAL :
                                                   SearchNodeStatus::OPEN;
                call_event([&](auto& handler) { handler.on_generate_node_relaxed(source_node, normalized_successor); });
                worker.search.open_successor(successor_state.get_index(), g_value, h_value, successor_search_node.status, false);
                return AcceptanceResult::QUEUED;
            }

            call_event([&](auto& handler) { handler.on_generate_node_not_relaxed(source_node, normalized_successor); });
            return AcceptanceResult::DISCARDED;
        }
        else
        {
            if (!is_new)
                return AcceptanceResult::DISCARDED;

            const auto g_value = compute_successor_g_value(routed_successor.metadata.source_g_value, successor_node.get_metric(), m_options.cost_mode);
            const auto normalized_node = Node<Kind>(successor_state, g_value);
            const auto normalized_successor = LabeledNode<Kind> { labeled_successor.label, normalized_node };

            successor_search_node.status = SearchNodeStatus::OPEN;
            worker.search.set_parent(successor_search_node, routed_successor.metadata.parent);
            successor_search_node.g_value = g_value;
            successor_search_node.preferred = routed_successor.metadata.preferred;

            if (worker.goal_strategy->is_dynamic_goal_satisfied(m_start_node.get_state(), successor_state))
            {
                successor_search_node.status = SearchNodeStatus::GOAL;
                solve(worker, successor_search_node, normalized_node);
                return AcceptanceResult::TERMINAL;
            }

            if (worker.pruning_strategy->should_prune_successor_state(source_state, successor_state, is_new))
            {
                successor_search_node.status = SearchNodeStatus::CLOSED;
                call_event([&](auto& handler) { handler.on_prune_node(source_node, normalized_successor); });
                return AcceptanceResult::DISCARDED;
            }

            call_event([&](auto& handler) { handler.on_generate_node(source_node, normalized_successor); });
            worker.search.open_successor(successor_state.get_index(),
                                         g_value,
                                         routed_successor.metadata.inherited_h_value,
                                         successor_search_node.status,
                                         routed_successor.metadata.preferred);
            return AcceptanceResult::QUEUED;
        }
    }

    void solve(WorkerData& worker, const SearchNode&, const Node<Kind>& node)
    {
        const auto goal = WorkerStateIndex<Kind> { worker.index, node.get_state().get_index() };
        if (m_execution.set_goal(*this, goal))
            call_event([&](auto& handler) { handler.on_expand_goal_node(node); });
    }

    void finish(SearchStatus status)
    {
        m_execution.set_terminal(*this, status);
        m_result.status = status;
        call_event([&](auto& handler) { handler.on_end_search(status); });
    }

    SearchResult<Kind> finish_search()
    {
        if (const auto exception = m_execution.exception())
        {
            call_event([](auto& handler) { handler.on_end_search(SearchStatus::FAILED); });
            std::rethrow_exception(exception);
        }

        m_result.status = m_execution.status();
        if (m_result.status == SearchStatus::SOLVED)
        {
            const auto goal = m_execution.goal();
            assert(goal);

            if constexpr (ExecutionPolicy::parallel)
            {
                auto views = std::vector<WorkerSearchSpaceView<Kind, SearchNode>> {};
                views.reserve(num_workers());
                for (const auto& worker : m_workers)
                    views.push_back(WorkerSearchSpaceView<Kind, SearchNode> { worker->successor_generator, worker->search.get_search_nodes() });

                m_result.plan =
                    PlanReconstructionPolicy<ParallelSearch>::extract_total_ordered_plan(*goal,
                                                                                         std::span<const WorkerSearchSpaceView<Kind, SearchNode>>(views),
                                                                                         m_caller_successor_generator,
                                                                                         m_options.cost_mode);
                m_result.goal_node = m_result.plan->empty() ? m_result.plan->get_start_node() : m_result.plan->get_labeled_succ_nodes().back().node;
            }
            else
            {
                auto& worker = get_worker(goal->worker);
                const auto state = worker.successor_generator.get_state_repository()->get_registered_state(goal->state);
                const auto& search_node = worker.search.get_search_node(goal->state);
                const auto node = Node<Kind>(state, search_node.g_value);
                m_result.plan = PlanReconstructionPolicy<SequentialSearch>::extract_total_ordered_plan(search_node,
                                                                                                       node,
                                                                                                       worker.search.get_search_nodes(),
                                                                                                       worker.successor_generator,
                                                                                                       m_options.cost_mode);
                m_result.goal_node = node;
            }
        }

        call_event([&](auto& handler) { handler.on_end_search(m_result.status); });
        if (m_result.plan)
            call_event([&](auto& handler) { handler.on_solved(*m_result.plan); });
        return std::move(m_result);
    }

    template<typename Callback>
    void call_event(Callback&& callback)
    {
        m_execution.call_event(m_event_handler, std::forward<Callback>(callback));
    }

    Task<Kind>& m_task;
    SuccessorGenerator<Kind>& m_caller_successor_generator;
    const Options& m_options;
    typename SearchPolicy::EventHandlerPtr m_event_handler;
    Node<Kind> m_start_node;
    ExecutionPolicy m_execution;
    WorkerStorage m_workers;
    SearchResult<Kind> m_result;
};

}

#endif
