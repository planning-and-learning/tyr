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

#ifndef TYR_SRC_PLANNING_ALGORITHMS_SEARCH_ENGINE_ASTAR_EAGER_HPP_
#define TYR_SRC_PLANNING_ALGORITHMS_SEARCH_ENGINE_ASTAR_EAGER_HPP_

#include "engine.hpp"
#include "parent_state.hpp"
#include "tyr/planning/algorithms/astar_eager.hpp"
#include "tyr/planning/algorithms/astar_eager/event_handler.hpp"
#include "tyr/planning/algorithms/openlists/priority_queue.hpp"

#include <cmath>
#include <limits>
#include <stdexcept>
#include <tuple>
#include <utility>

namespace tyr::planning::detail
{

template<TaskKind Kind, SearchKind Search>
struct AStarModePolicy;

template<TaskKind Kind>
struct AStarModePolicy<Kind, SequentialSearch>
{
    static constexpr bool terminate_on_goal = true;
    static constexpr bool supports_priority_layer_synchronization = false;

    static constexpr bool should_discard(ygg::float_t, ygg::float_t) noexcept { return false; }

    template<typename EmitEvent>
    static void finish_f_layer(ygg::float_t entry_f_value, ygg::float_t& current_f_value, EmitEvent&& emit_event)
    {
        if (entry_f_value <= current_f_value)
            return;

        std::forward<EmitEvent>(emit_event)([&](auto& handler) { handler.on_finish_f_layer(current_f_value); });
        current_f_value = entry_f_value;
    }

    static constexpr bool synchronize_priority_layers(const astar_eager::Options<Kind>&) noexcept { return false; }
};

template<TaskKind Kind>
struct AStarModePolicy<Kind, ParallelSearch>
{
    static constexpr bool terminate_on_goal = false;
    static constexpr bool supports_priority_layer_synchronization = true;

    static constexpr bool should_discard(ygg::float_t entry_f_value, ygg::float_t incumbent_cost) noexcept
    {
        // Work credits keep every open entry and in-flight state alive, so exhausting entries below the incumbent proves the global lower bound.
        return entry_f_value >= incumbent_cost;
    }

    template<typename EmitEvent>
    static constexpr void finish_f_layer(ygg::float_t, ygg::float_t&, EmitEvent&&) noexcept
    {
    }

    static bool synchronize_priority_layers(const astar_eager::Options<Kind>& options) noexcept
    {
        return options.parallel_search_mode == astar_eager::ParallelSearchMode::SYNCHRONOUS;
    }
};

template<TaskKind Kind, SearchKind Search>
class EagerAStarPolicy
{
public:
    using ModePolicy = AStarModePolicy<Kind, Search>;
    using ParentPolicy = ParentStatePolicy<Kind, Search>;
    using TaskTag = Kind;
    using SearchTag = Search;
    using Options = astar_eager::Options<Kind>;
    using EventHandlerPtr = astar_eager::EventHandlerPtr<Kind>;
    using WorkerEventHandlerPtr = astar_eager::WorkerEventHandlerPtr<Kind>;
    using ParentState = typename ParentPolicy::Type;
    static constexpr bool terminate_on_goal = ModePolicy::terminate_on_goal;
    static constexpr bool supports_priority_layer_synchronization = ModePolicy::supports_priority_layer_synchronization;
    static constexpr bool emits_priority_layer_events = false;

    struct SearchNode
    {
        ygg::float_t g_value;
        ParentState parent_state;
        SearchNodeStatus status;
    };

    struct SuccessorMetadata
    {
        WorkerStateIndex<Kind> parent;
        ygg::float_t source_g_value;
    };

    struct QueueEntry
    {
        using KeyType = std::tuple<ygg::float_t, SearchNodeStatus, ygg::uint_t>;
        using ItemType = std::tuple<ygg::float_t, ygg::Index<State<Kind>>>;

        ygg::float_t f_value;
        ygg::Index<State<Kind>> state;
        SearchNodeStatus status;
        ygg::uint_t step;

        KeyType get_key() const { return std::make_tuple(f_value, status, step); }
        ItemType get_item() const { return std::make_tuple(f_value, state); }
    };

    struct PoppedEntry
    {
        ygg::float_t f_value;
        ygg::Index<State<Kind>> state;
    };

    static constexpr bool check_timeout_after_generation = true;
    static constexpr bool check_timeout_per_successor = true;

    EagerAStarPolicy(Heuristic<Kind>&, const Options&) {}

    SearchNode& initialize_start(ygg::Index<State<Kind>> state, ygg::float_t g_value, ygg::float_t h_value)
    {
        m_f_value = ygg::FloatTolerance<ygg::float_t>::canonicalize(g_value + h_value);
        if (std::isnan(m_f_value))
            throw std::runtime_error("A* start f-value is NaN.");
        auto& search_node = get_search_node(state);
        search_node.status = h_value == std::numeric_limits<ygg::float_t>::infinity() ? SearchNodeStatus::DEAD_END : SearchNodeStatus::OPEN;
        search_node.g_value = g_value;
        return search_node;
    }

    ygg::float_t get_start_priority() const noexcept { return m_f_value; }

    void open_start(ygg::Index<State<Kind>> state, const SearchNode& search_node)
    {
        m_openlist.insert(QueueEntry { m_f_value, state, search_node.status, m_step++ });
    }

    bool empty() const { return m_openlist.empty(); }

    ygg::float_t get_min_priority() const noexcept
    {
        return m_openlist.empty() ? std::numeric_limits<ygg::float_t>::infinity() : m_openlist.top_entry().f_value;
    }

    size_t get_num_open_entries() const noexcept { return m_openlist.size(); }

    PoppedEntry pop()
    {
        const auto entry = m_openlist.top_entry();
        m_openlist.pop();
        return PoppedEntry { entry.f_value, entry.state };
    }

    bool should_discard(const PoppedEntry& entry, ygg::float_t incumbent_cost) const noexcept
    {
        return ModePolicy::should_discard(entry.f_value, incumbent_cost);
    }

    SearchNode& get_search_node(ygg::Index<State<Kind>> state)
    {
        static const auto default_node = SearchNode { std::numeric_limits<ygg::float_t>::infinity(), no_parent(), SearchNodeStatus::NEW };
        return get_or_create_search_node(state, m_search_nodes, default_node);
    }

    template<typename ImproveBestH, typename EmitEvent, typename FinishPriorityLayer>
    ExpansionResult prepare_expansion(const PoppedEntry& entry,
                                      const Node<Kind>& node,
                                      SearchNode& search_node,
                                      Statistics& statistics,
                                      ImproveBestH&&,
                                      EmitEvent&& emit_event,
                                      FinishPriorityLayer&&)
    {
        ModePolicy::finish_f_layer(entry.f_value, m_f_value, emit_event);
        // A remote lower-f entry may arrive later, so a parallel worker cannot declare an f-layer finished.

        if (search_node.status == SearchNodeStatus::GOAL)
            return ExpansionResult::GOAL;

        statistics.increment_num_expanded();
        std::forward<EmitEvent>(emit_event)([&](auto& handler) { handler.on_expand_node(node); });
        search_node.status = SearchNodeStatus::CLOSED;
        return ExpansionResult::EXPAND;
    }

    SuccessorMetadata make_successor_metadata(ygg::Index<Worker> worker,
                                              ygg::Index<State<Kind>> state,
                                              const SearchNode& search_node,
                                              ::tyr::formalism::planning::ActionBindingView) const
    {
        return SuccessorMetadata { WorkerStateIndex<Kind> { worker, state }, search_node.g_value };
    }

    static void set_parent(SearchNode& search_node, WorkerStateIndex<Kind> parent) noexcept { search_node.parent_state = ParentPolicy::make_parent(parent); }

    void open_successor(ygg::Index<State<Kind>> state, ygg::float_t g_value, ygg::float_t h_value, SearchNodeStatus status, bool)
    {
        const auto f_value = ygg::FloatTolerance<ygg::float_t>::canonicalize(g_value + h_value);
        if (std::isnan(f_value))
            throw std::runtime_error("A* successor f-value is NaN.");
        m_openlist.insert(QueueEntry { f_value, state, status, m_step++ });
    }

    template<typename Engine, typename WorkerData, typename EmitTransition>
    AcceptanceResult accept_successor(Engine& engine,
                                      WorkerData& worker,
                                      const Node<Kind>& source_node,
                                      const Node<Kind>& successor_node,
                                      const typename Engine::RoutedSuccessor& routed_successor,
                                      SearchNode& successor_search_node,
                                      bool is_new,
                                      EmitTransition&& emit_transition)
    {
        const auto& source_state = source_node.get_state();
        const auto& successor_state = successor_node.get_state();
        const auto g_value = successor_node.get_metric();

        if (g_value >= successor_search_node.g_value)
        {
            emit_transition(TransitionOutcome::DUPLICATE);
            return AcceptanceResult::DISCARDED;
        }

        if (worker.pruning_strategy->should_prune_successor_state(source_state, successor_state, is_new))
        {
            if (is_new)
                successor_search_node.status = SearchNodeStatus::CLOSED;
            worker.statistics.increment_num_pruned();
            emit_transition(TransitionOutcome::PRUNED);
            return AcceptanceResult::DISCARDED;
        }

        worker.statistics.increment_num_generated();
        set_parent(successor_search_node, routed_successor.metadata.parent);
        successor_search_node.g_value = g_value;

        if (auto result = engine.m_execution.accept_generated_goal(engine, worker, successor_search_node, successor_state, g_value, emit_transition))
            return *result;

        if (engine.m_execution.is_queued_goal(engine, worker, successor_state))
        {
            successor_search_node.status = SearchNodeStatus::GOAL;
            open_successor(successor_state.get_index(), g_value, 0, successor_search_node.status, false);
            emit_transition(TransitionOutcome::GOAL);
            return AcceptanceResult::QUEUED;
        }

        const auto h_value = ygg::FloatTolerance<ygg::float_t>::canonicalize(worker.heuristic.evaluate(successor_state));
        if (std::isnan(h_value))
            throw std::runtime_error("find_solution(...): successor heuristic value is NaN.");
        if (h_value == std::numeric_limits<ygg::float_t>::infinity())
        {
            successor_search_node.status = SearchNodeStatus::DEAD_END;
            emit_transition(TransitionOutcome::DEAD_END);
            return AcceptanceResult::DISCARDED;
        }

        successor_search_node.status = SearchNodeStatus::OPEN;
        open_successor(successor_state.get_index(), g_value, h_value, successor_search_node.status, false);

        emit_transition(is_new ? TransitionOutcome::OPENED : TransitionOutcome::RELAXED);
        return AcceptanceResult::QUEUED;
    }

    static bool synchronize_priority_layers(const Options& options) noexcept { return ModePolicy::synchronize_priority_layers(options); }

    static WorkerEventHandlerPtr make_worker_event_handler(const EventHandlerPtr& event_handler, ygg::Index<Worker> index)
    {
        return event_handler ? event_handler->make_worker(index) : nullptr;
    }

    template<typename Handler>
    static void on_start_search(Handler& handler, const Node<Kind>& node, ygg::float_t priority)
    {
        handler.on_start_search(node, priority);
    }

    template<typename Handler>
    static constexpr void on_finish_priority_layer(Handler&, ygg::float_t, const Statistics&) noexcept
    {
    }

    const ygg::SegmentedVector<SearchNode>& get_search_nodes() const noexcept { return m_search_nodes; }

private:
    static constexpr ParentState no_parent() noexcept { return ParentPolicy::no_parent(); }

    ygg::uint_t m_step { 0 };
    ygg::float_t m_f_value { 0 };
    ygg::SegmentedVector<SearchNode> m_search_nodes;
    PriorityQueue<QueueEntry> m_openlist;
};

static_assert(sizeof(EagerAStarPolicy<LiftedTag, SequentialSearch>::SearchNode) == 16);
static_assert(sizeof(EagerAStarPolicy<GroundTag, SequentialSearch>::SearchNode) == 16);
static_assert(sizeof(EagerAStarPolicy<LiftedTag, ParallelSearch>::SearchNode) == 24);
static_assert(sizeof(EagerAStarPolicy<GroundTag, ParallelSearch>::SearchNode) == 24);
static_assert(sizeof(EagerAStarPolicy<LiftedTag, SequentialSearch>::QueueEntry) == 24);
static_assert(sizeof(EagerAStarPolicy<GroundTag, SequentialSearch>::QueueEntry) == 24);

}

#endif
