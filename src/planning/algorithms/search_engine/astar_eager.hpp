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
#include "tyr/planning/algorithms/astar_eager.hpp"
#include "tyr/planning/algorithms/astar_eager/event_handler.hpp"
#include "tyr/planning/algorithms/openlists/priority_queue.hpp"

#include <limits>
#include <tuple>
#include <utility>

namespace tyr::planning::detail
{

template<TaskKind Kind>
class EagerAStarPolicy
{
public:
    using Options = astar_eager::Options<Kind>;
    using EventHandlerPtr = astar_eager::EventHandlerPtr<Kind>;
    static constexpr bool eager = true;

    struct SearchNode
    {
        ygg::float_t g_value;
        ygg::Index<State<Kind>> parent_state;
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

    EagerAStarPolicy(Heuristic<Kind>& heuristic, const Options& options) : m_heuristic(heuristic), m_options(options) {}

    static EventHandlerPtr create_default_event_handler() { return astar_eager::DefaultEventHandler<Kind>::create(0); }

    SearchNode& initialize_start(ygg::Index<State<Kind>> state, ygg::float_t g_value, ygg::float_t h_value)
    {
        m_f_value = ygg::FloatTolerance<ygg::float_t>::canonicalize(g_value + h_value);
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

    PoppedEntry pop()
    {
        const auto entry = m_openlist.top_entry();
        m_openlist.pop();
        return PoppedEntry { entry.f_value, entry.state };
    }

    SearchNode& get_search_node(ygg::Index<State<Kind>> state)
    {
        static const auto default_node = SearchNode { std::numeric_limits<ygg::float_t>::infinity(), ygg::Index<State<Kind>>::max(), SearchNodeStatus::NEW };
        return get_or_create_search_node(state, m_search_nodes, default_node);
    }

    template<typename ImproveBestH, typename EmitEvent>
    ExpansionResult prepare_expansion(const PoppedEntry& entry, const Node<Kind>& node, SearchNode& search_node, ImproveBestH&&, EmitEvent&& emit_event)
    {
        if (entry.f_value > m_f_value)
        {
            std::forward<EmitEvent>(emit_event)([&](auto& handler) { handler.on_finish_f_layer(m_f_value); });
            m_f_value = entry.f_value;
        }

        if (search_node.status == SearchNodeStatus::GOAL)
            return ExpansionResult::GOAL;

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

    static void set_parent(SearchNode& search_node, WorkerStateIndex<Kind> parent) noexcept { search_node.parent_state = parent.state; }

    void open_successor(ygg::Index<State<Kind>> state, ygg::float_t g_value, ygg::float_t h_value, SearchNodeStatus status, bool)
    {
        const auto f_value = ygg::FloatTolerance<ygg::float_t>::canonicalize(g_value + h_value);
        m_openlist.insert(QueueEntry { f_value, state, status, m_step++ });
    }

    const ygg::SegmentedVector<SearchNode>& get_search_nodes() const noexcept { return m_search_nodes; }

private:
    Heuristic<Kind>& m_heuristic;
    const Options& m_options;
    ygg::uint_t m_step { 0 };
    ygg::float_t m_f_value { 0 };
    ygg::SegmentedVector<SearchNode> m_search_nodes;
    PriorityQueue<QueueEntry> m_openlist;
};

static_assert(sizeof(EagerAStarPolicy<LiftedTag>::SearchNode) == 16);
static_assert(sizeof(EagerAStarPolicy<GroundTag>::SearchNode) == 16);
static_assert(sizeof(EagerAStarPolicy<LiftedTag>::QueueEntry) == 24);
static_assert(sizeof(EagerAStarPolicy<GroundTag>::QueueEntry) == 24);

}

#endif
