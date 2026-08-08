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

#ifndef TYR_SRC_PLANNING_ALGORITHMS_SEARCH_ENGINE_GBFS_LAZY_HPP_
#define TYR_SRC_PLANNING_ALGORITHMS_SEARCH_ENGINE_GBFS_LAZY_HPP_

#include "engine.hpp"
#include "parent_state.hpp"
#include "tyr/planning/algorithms/gbfs_lazy.hpp"
#include "tyr/planning/algorithms/gbfs_lazy/event_handler.hpp"
#include "tyr/planning/algorithms/openlists/alternating.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <tuple>
#include <utility>

namespace tyr::planning::detail
{

constexpr size_t preferred_boost_share(size_t boost, size_t num_workers, size_t worker)
{
    assert(num_workers > 0);
    assert(worker < num_workers);
    return boost / num_workers + (worker < boost % num_workers);
}

static_assert(preferred_boost_share(1000, 4, 0) == 250);
static_assert(preferred_boost_share(2, 4, 0) == 1);
static_assert(preferred_boost_share(2, 4, 2) == 0);

template<TaskKind Kind, SearchKind Search>
class LazyGBFSPolicy
{
public:
    using ParentPolicy = ParentStatePolicy<Kind, Search>;
    using TaskTag = Kind;
    using SearchTag = Search;
    using Options = gbfs_lazy::Options<Kind>;
    using EventHandlerPtr = gbfs_lazy::EventHandlerPtr<Kind>;
    using WorkerEventHandlerPtr = gbfs_lazy::WorkerEventHandlerPtr<Kind>;
    using ParentState = typename ParentPolicy::Type;
    static constexpr bool terminate_on_goal = true;
    static constexpr bool supports_priority_layer_synchronization = false;
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
        ygg::float_t inherited_h_value;
        bool preferred;
    };

    struct QueueEntry
    {
        using KeyType = std::tuple<ygg::float_t, ygg::float_t, ygg::uint_t>;
        using ItemType = ygg::Index<State<Kind>>;

        ygg::float_t g_value;
        ygg::float_t h_value;
        ygg::Index<State<Kind>> state;
        ygg::uint_t step;

        KeyType get_key() const { return std::make_tuple(h_value, g_value, step); }
        ItemType get_item() const { return state; }
    };

    struct PoppedEntry
    {
        ygg::Index<State<Kind>> state;
    };

    LazyGBFSPolicy(Heuristic<Kind>& heuristic, const Options& options) :
        m_heuristic(heuristic),
        m_options(options),
        m_openlist(m_preferred_openlist, m_standard_openlist, std::array<size_t, 2> { 1, 1 })
    {
    }

    SearchNode& initialize_start(ygg::Index<State<Kind>> state, ygg::float_t g_value, ygg::float_t h_value)
    {
        m_start_h_value = h_value;
        auto& search_node = get_search_node(state);
        search_node.status = h_value == std::numeric_limits<ygg::float_t>::infinity() ? SearchNodeStatus::DEAD_END : SearchNodeStatus::OPEN;
        search_node.g_value = g_value;
        return search_node;
    }

    ygg::float_t get_start_priority() const noexcept { return m_start_h_value; }

    void open_start(ygg::Index<State<Kind>> state, const SearchNode& search_node)
    {
        m_standard_openlist.insert(QueueEntry { search_node.g_value, m_start_h_value, state, m_step++ });
    }

    bool empty() const { return m_openlist.empty(); }

    PoppedEntry pop()
    {
        auto& weights = m_openlist.get_weights();
        weights[0] += m_pending_preferred_boost.exchange(0, std::memory_order_relaxed);
        const auto state = m_openlist.top();
        m_openlist.pop();
        weights[0] = std::max(weights[0] - 1, size_t { 1 });
        return PoppedEntry { state };
    }

    // Best-h progress is detected while another worker may own this queue, so defer the local weight update without nesting worker locks.
    void queue_preferred_boost(size_t boost) noexcept { m_pending_preferred_boost.fetch_add(boost, std::memory_order_relaxed); }

    bool should_discard(const PoppedEntry&, ygg::float_t) const noexcept { return false; }

    SearchNode& get_search_node(ygg::Index<State<Kind>> state)
    {
        static const auto default_node = SearchNode { std::numeric_limits<ygg::float_t>::infinity(), no_parent(), SearchNodeStatus::NEW };
        return get_or_create_search_node(state, m_search_nodes, default_node);
    }

    template<typename EvaluateUnlocked, typename ImproveBestH, typename EmitEvent, typename FinishPriorityLayer>
    ExpansionResult prepare_expansion(const PoppedEntry&,
                                      const Node<Kind>& node,
                                      SearchNode& search_node,
                                      Statistics& statistics,
                                      EvaluateUnlocked&& evaluate_unlocked,
                                      ImproveBestH&& improve_best_h,
                                      EmitEvent&& emit_event,
                                      FinishPriorityLayer&&)
    {
        statistics.increment_num_expanded();
        std::forward<EmitEvent>(emit_event)([&](auto& handler) { handler.on_expand_node(node); });

        m_state_h_value = std::forward<EvaluateUnlocked>(evaluate_unlocked)(
            [&] { return ygg::FloatTolerance<ygg::float_t>::canonicalize(m_heuristic.evaluate(node.get_state())); });
        if (std::isnan(m_state_h_value))
            throw std::runtime_error("GBFS heuristic value is NaN.");
        if (m_state_h_value == std::numeric_limits<ygg::float_t>::infinity())
        {
            statistics.increment_num_deadends();
            search_node.status = SearchNodeStatus::DEAD_END;
            return ExpansionResult::SKIP;
        }

        static_cast<void>(std::forward<ImproveBestH>(improve_best_h)(m_state_h_value, [&](auto& handler) { handler.on_new_best_h_value(m_state_h_value); }));

        m_preferred_actions = m_options.use_preferred_actions ? &m_heuristic.get_preferred_actions() : nullptr;
        search_node.status = SearchNodeStatus::CLOSED;
        return ExpansionResult::EXPAND;
    }

    SuccessorMetadata make_successor_metadata(ygg::Index<Worker> worker,
                                              ygg::Index<State<Kind>> state,
                                              const SearchNode& search_node,
                                              ::tyr::formalism::planning::ActionBindingView action) const
    {
        const auto preferred = m_preferred_actions && m_preferred_actions->contains(action);
        return SuccessorMetadata { WorkerStateIndex<Kind> { worker, state }, search_node.g_value, m_state_h_value, preferred };
    }

    static constexpr void prepare_routed_successor(Heuristic<Kind>&, const ygg::Builder<State<Kind>>&, bool, SuccessorMetadata&) noexcept {}

    static void set_parent(SearchNode& search_node, WorkerStateIndex<Kind> parent) noexcept { search_node.parent_state = ParentPolicy::make_parent(parent); }

    void open_successor(ygg::Index<State<Kind>> state, ygg::float_t g_value, ygg::float_t h_value, SearchNodeStatus, bool preferred)
    {
        if (preferred)
            m_preferred_openlist.insert(QueueEntry { g_value, h_value, state, m_step++ });
        else
            m_standard_openlist.insert(QueueEntry { g_value, h_value, state, m_step++ });
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
        if (!is_new)
        {
            emit_transition(TransitionOutcome::DUPLICATE);
            return AcceptanceResult::DISCARDED;
        }

        const auto& source_state = source_node.get_state();
        const auto& successor_state = successor_node.get_state();
        const auto g_value = successor_node.get_metric();
        successor_search_node.status = SearchNodeStatus::OPEN;
        set_parent(successor_search_node, routed_successor.metadata.parent);
        successor_search_node.g_value = g_value;
        if (engine.m_execution.is_generated_goal(engine, worker, routed_successor, successor_state))
        {
            worker.statistics.increment_num_accepted_successors();
            successor_search_node.status = SearchNodeStatus::GOAL;
            emit_transition(TransitionOutcome::GOAL);
            engine.solve(worker, successor_node);
            return AcceptanceResult::TERMINAL;
        }

        if (worker.pruning_strategy->should_prune_successor_state(source_state, successor_state, is_new))
        {
            successor_search_node.status = SearchNodeStatus::CLOSED;
            worker.statistics.increment_num_pruned();
            emit_transition(TransitionOutcome::PRUNED);
            return AcceptanceResult::DISCARDED;
        }

        worker.statistics.increment_num_accepted_successors();
        open_successor(successor_state.get_index(),
                       g_value,
                       routed_successor.metadata.inherited_h_value,
                       successor_search_node.status,
                       routed_successor.metadata.preferred);
        emit_transition(TransitionOutcome::OPENED);
        return AcceptanceResult::QUEUED;
    }

    const ygg::SegmentedVector<SearchNode>& get_search_nodes() const noexcept { return m_search_nodes; }

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

    template<typename Engine>
    static void boost_preferred_queues(Engine& engine)
    {
        if (!engine.m_options.use_preferred_actions)
            return;

        const auto num_workers = engine.num_workers();
        for (size_t i = 0; i < num_workers; ++i)
        {
            const auto boost = preferred_boost_share(engine.m_options.boost_preferred_queue, num_workers, i);
            engine.get_worker(ygg::Index<Worker>(static_cast<ygg::uint_t>(i))).search.queue_preferred_boost(boost);
        }
    }

private:
    using Queue = PriorityQueue<QueueEntry>;

    static constexpr ParentState no_parent() noexcept { return ParentPolicy::no_parent(); }

    Heuristic<Kind>& m_heuristic;
    const Options& m_options;
    ygg::uint_t m_step { 0 };
    ygg::float_t m_start_h_value { 0 };
    ygg::float_t m_state_h_value { 0 };
    const ygg::UnorderedSet<::tyr::formalism::planning::ActionBindingView>* m_preferred_actions { nullptr };
    ygg::SegmentedVector<SearchNode> m_search_nodes;
    Queue m_preferred_openlist;
    Queue m_standard_openlist;
    AlternatingOpenList<Queue, Queue> m_openlist;
    std::atomic<size_t> m_pending_preferred_boost { 0 };
};

static_assert(sizeof(LazyGBFSPolicy<LiftedTag, SequentialSearch>::SearchNode) == 16);
static_assert(sizeof(LazyGBFSPolicy<GroundTag, SequentialSearch>::SearchNode) == 16);
static_assert(sizeof(LazyGBFSPolicy<LiftedTag, SequentialSearch>::QueueEntry) == 24);
static_assert(sizeof(LazyGBFSPolicy<GroundTag, SequentialSearch>::QueueEntry) == 24);

}

#endif
