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

#ifndef TYR_SRC_PLANNING_ALGORITHMS_SEARCH_ENGINE_BRFS_HPP_
#define TYR_SRC_PLANNING_ALGORITHMS_SEARCH_ENGINE_BRFS_HPP_

#include "engine.hpp"
#include "parent_state.hpp"
#include "tyr/planning/algorithms/brfs.hpp"
#include "tyr/planning/algorithms/brfs/event_handler.hpp"

#include <cstddef>
#include <deque>
#include <limits>
#include <utility>

namespace tyr::planning::detail
{

template<SearchKind Search>
struct BreadthFirstModePolicy;

template<>
struct BreadthFirstModePolicy<SequentialSearch>
{
    static constexpr bool supports_priority_layer_synchronization = false;

    template<typename FinishPriorityLayer>
    static void finish_layer(ygg::uint_t depth, ygg::uint_t& current_layer, FinishPriorityLayer&& finish_priority_layer)
    {
        if (depth <= current_layer)
            return;

        std::forward<FinishPriorityLayer>(finish_priority_layer)(static_cast<ygg::float_t>(current_layer));
        current_layer = depth;
    }

    template<TaskKind Kind>
    static constexpr bool synchronize_priority_layers(const brfs::Options<Kind>&) noexcept
    {
        return false;
    }
};

template<>
struct BreadthFirstModePolicy<ParallelSearch>
{
    static constexpr bool supports_priority_layer_synchronization = true;

    template<typename FinishPriorityLayer>
    static constexpr void finish_layer(ygg::uint_t, ygg::uint_t&, FinishPriorityLayer&&) noexcept
    {
    }

    template<TaskKind Kind>
    static constexpr bool synchronize_priority_layers(const brfs::Options<Kind>&) noexcept
    {
        return true;
    }
};

template<TaskKind Kind, SearchKind Search>
class BreadthFirstPolicy
{
public:
    using ModePolicy = BreadthFirstModePolicy<Search>;
    using ParentPolicy = ParentStatePolicy<Kind, Search>;
    using TaskTag = Kind;
    using SearchTag = Search;
    using Options = brfs::Options<Kind>;
    using EventHandlerPtr = brfs::EventHandlerPtr<Kind>;
    using WorkerEventHandlerPtr = brfs::WorkerEventHandlerPtr<Kind>;
    using ParentState = typename ParentPolicy::Type;

    static constexpr bool terminate_on_goal = true;
    static constexpr bool supports_priority_layer_synchronization = ModePolicy::supports_priority_layer_synchronization;
    static constexpr bool emits_priority_layer_events = true;
    struct SearchNode
    {
        ygg::uint_t g_value;
        ParentState parent_state;
        SearchNodeStatus status;
    };

    struct SuccessorMetadata
    {
        WorkerStateIndex<Kind> parent;
        ygg::float_t source_g_value;
    };

    struct PoppedEntry
    {
        ygg::uint_t depth;
        ygg::Index<State<Kind>> state;
    };

    BreadthFirstPolicy(Heuristic<Kind>&, const Options&) {}

    SearchNode& initialize_start(ygg::Index<State<Kind>> state, ygg::float_t, ygg::float_t)
    {
        auto& search_node = get_search_node(state);
        search_node.g_value = 0;
        search_node.status = SearchNodeStatus::OPEN;
        return search_node;
    }

    static constexpr ygg::float_t get_start_priority() noexcept { return 0; }

    void open_start(ygg::Index<State<Kind>> state, const SearchNode& search_node) { m_openlist.push_back(PoppedEntry { search_node.g_value, state }); }

    bool empty() const noexcept { return m_openlist.empty(); }

    ygg::float_t get_min_priority() const noexcept
    {
        return m_openlist.empty() ? std::numeric_limits<ygg::float_t>::infinity() : static_cast<ygg::float_t>(m_openlist.front().depth);
    }

    size_t get_num_open_entries() const noexcept { return m_openlist.size(); }

    PoppedEntry pop()
    {
        const auto entry = m_openlist.front();
        m_openlist.pop_front();
        return entry;
    }

    static constexpr bool should_discard(const PoppedEntry&, ygg::float_t) noexcept { return false; }

    SearchNode& get_search_node(ygg::Index<State<Kind>> state)
    {
        static const auto default_node = SearchNode { std::numeric_limits<ygg::uint_t>::max(), ParentPolicy::no_parent(), SearchNodeStatus::NEW };
        return get_or_create_search_node(state, m_search_nodes, default_node);
    }

    template<typename EvaluateUnlocked, typename ImproveBestH, typename EmitEvent, typename FinishPriorityLayer>
    ExpansionResult prepare_expansion(const PoppedEntry& entry,
                                      const Node<Kind>& node,
                                      SearchNode& search_node,
                                      Statistics& statistics,
                                      EvaluateUnlocked&&,
                                      ImproveBestH&&,
                                      EmitEvent&& emit_event,
                                      FinishPriorityLayer&& finish_priority_layer)
    {
        ModePolicy::finish_layer(entry.depth, m_current_layer, std::forward<FinishPriorityLayer>(finish_priority_layer));
        statistics.increment_num_expanded();
        std::forward<EmitEvent>(emit_event)([&](auto& handler) { handler.on_expand_node(node); });
        search_node.status = SearchNodeStatus::CLOSED;
        return ExpansionResult::EXPAND;
    }

    static SuccessorMetadata make_successor_metadata(ygg::Index<Worker> worker,
                                                     ygg::Index<State<Kind>> state,
                                                     const SearchNode& search_node,
                                                     ::tyr::formalism::planning::ActionBindingView)
    {
        return SuccessorMetadata { WorkerStateIndex<Kind> { worker, state }, static_cast<ygg::float_t>(search_node.g_value) };
    }

    static void set_parent(SearchNode& search_node, WorkerStateIndex<Kind> parent) noexcept { search_node.parent_state = ParentPolicy::make_parent(parent); }

    void open_successor(ygg::Index<State<Kind>> state, ygg::float_t g_value, ygg::float_t, SearchNodeStatus, bool)
    {
        m_openlist.push_back(PoppedEntry { static_cast<ygg::uint_t>(g_value), state });
    }

    template<typename Engine, typename WorkerData, typename EvaluateHeuristic, typename EmitTransition>
    AcceptanceResult accept_successor(Engine& engine,
                                      WorkerData& worker,
                                      const Node<Kind>& source_node,
                                      const Node<Kind>& successor_node,
                                      const typename Engine::RoutedSuccessor& routed_successor,
                                      SearchNode& successor_search_node,
                                      bool is_new,
                                      EvaluateHeuristic&&,
                                      EmitTransition&& emit_transition)
    {
        if (!is_new)
        {
            emit_transition(TransitionOutcome::DUPLICATE);
            return AcceptanceResult::DISCARDED;
        }

        const auto& source_state = source_node.get_state();
        const auto& successor_state = successor_node.get_state();
        const auto g_value = successor_node.get_metric();
        successor_search_node.status = SearchNodeStatus::OPEN;
        successor_search_node.g_value = static_cast<ygg::uint_t>(g_value);
        set_parent(successor_search_node, routed_successor.metadata.parent);

        const auto is_goal = engine.m_execution.is_generated_goal(engine, worker, routed_successor, successor_state);
        if (!is_goal && worker.pruning_strategy->should_prune_successor_state(source_state, successor_state, true))
        {
            successor_search_node.status = SearchNodeStatus::CLOSED;
            worker.statistics.increment_num_pruned();
            emit_transition(TransitionOutcome::PRUNED);
            return AcceptanceResult::DISCARDED;
        }

        worker.statistics.increment_num_generated_successors();
        if (is_goal)
        {
            successor_search_node.status = SearchNodeStatus::GOAL;
            emit_transition(TransitionOutcome::GOAL);
            engine.solve(worker, successor_node);
            return AcceptanceResult::TERMINAL;
        }

        open_successor(successor_state.get_index(), g_value, 0, SearchNodeStatus::OPEN, false);
        emit_transition(TransitionOutcome::OPENED);
        return AcceptanceResult::QUEUED;
    }

    static constexpr bool synchronize_priority_layers(const Options& options) noexcept { return ModePolicy::synchronize_priority_layers(options); }

    static WorkerEventHandlerPtr make_worker_event_handler(const EventHandlerPtr& event_handler, ygg::Index<Worker> index)
    {
        return event_handler ? event_handler->make_worker(index) : nullptr;
    }

    template<typename Handler>
    static void on_start_search(Handler& handler, const Node<Kind>& node, ygg::float_t)
    {
        handler.on_start_search(node);
    }

    template<typename Handler>
    static void on_finish_priority_layer(Handler& handler, ygg::float_t priority, const Statistics& statistics)
    {
        handler.on_finish_layer(static_cast<ygg::uint_t>(priority), statistics);
    }

    const ygg::SegmentedVector<SearchNode>& get_search_nodes() const noexcept { return m_search_nodes; }

private:
    ygg::uint_t m_current_layer { 0 };
    ygg::SegmentedVector<SearchNode> m_search_nodes;
    std::deque<PoppedEntry> m_openlist;
};

static_assert(sizeof(BreadthFirstPolicy<LiftedTag, SequentialSearch>::SearchNode) == 12);
static_assert(sizeof(BreadthFirstPolicy<GroundTag, SequentialSearch>::SearchNode) == 12);
static_assert(sizeof(BreadthFirstPolicy<LiftedTag, ParallelSearch>::SearchNode) == 16);
static_assert(sizeof(BreadthFirstPolicy<GroundTag, ParallelSearch>::SearchNode) == 16);

}

#endif
