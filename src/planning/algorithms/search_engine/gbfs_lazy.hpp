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
#include "tyr/planning/algorithms/gbfs_lazy.hpp"
#include "tyr/planning/algorithms/gbfs_lazy/event_handler.hpp"
#include "tyr/planning/algorithms/openlists/alternating.hpp"

#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <limits>
#include <tuple>
#include <type_traits>
#include <utility>

namespace tyr::planning::detail
{

template<TaskKind Kind, SearchKind Search>
class LazyGBFSPolicy
{
public:
    using Options = gbfs_lazy::Options<Kind>;
    using EventHandlerPtr = gbfs_lazy::EventHandlerPtr<Kind>;
    using ParentState = std::conditional_t<std::same_as<Search, SequentialSearch>, ygg::Index<State<Kind>>, WorkerStateIndex<Kind>>;
    static constexpr bool eager = false;

    struct SearchNode
    {
        ygg::float_t g_value;
        ParentState parent_state;
        SearchNodeStatus status;
        bool preferred;
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

    static constexpr bool check_timeout_after_generation = false;
    static constexpr bool check_timeout_per_successor = false;

    LazyGBFSPolicy(Heuristic<Kind>& heuristic, const Options& options) :
        m_heuristic(heuristic),
        m_options(options),
        m_openlist(m_preferred_openlist, m_standard_openlist, std::array<size_t, 2> { 1, 1 })
    {
    }

    static EventHandlerPtr create_default_event_handler() { return gbfs_lazy::DefaultEventHandler<Kind>::create(0); }

    SearchNode& initialize_start(ygg::Index<State<Kind>> state, ygg::float_t g_value, ygg::float_t h_value)
    {
        m_start_h_value = h_value;
        auto& search_node = get_search_node(state);
        search_node.status = h_value == std::numeric_limits<ygg::float_t>::infinity() ? SearchNodeStatus::DEAD_END : SearchNodeStatus::OPEN;
        search_node.g_value = g_value;
        search_node.preferred = false;
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
        const auto state = m_openlist.top();
        m_openlist.pop();
        auto& weights = m_openlist.get_weights();
        weights[0] = std::max(weights[0] - 1, size_t { 1 });
        return PoppedEntry { state };
    }

    SearchNode& get_search_node(ygg::Index<State<Kind>> state)
    {
        static const auto default_node = SearchNode { std::numeric_limits<ygg::float_t>::infinity(), no_parent(), SearchNodeStatus::NEW, false };
        return get_or_create_search_node(state, m_search_nodes, default_node);
    }

    template<typename ImproveBestH, typename EmitEvent>
    ExpansionResult
    prepare_expansion(const PoppedEntry&, const Node<Kind>& node, SearchNode& search_node, ImproveBestH&& improve_best_h, EmitEvent&& emit_event)
    {
        std::forward<EmitEvent>(emit_event)([&](auto& handler) { handler.on_expand_node(node); });

        m_state_h_value = ygg::FloatTolerance<ygg::float_t>::canonicalize(m_heuristic.evaluate(node.get_state()));
        if (m_state_h_value == std::numeric_limits<ygg::float_t>::infinity())
        {
            search_node.status = SearchNodeStatus::DEAD_END;
            return ExpansionResult::SKIP;
        }

        if (std::forward<ImproveBestH>(improve_best_h)(m_state_h_value, [&](auto& handler) { handler.on_new_best_h_value(m_state_h_value); }))
        {
            if (m_options.use_preferred_actions)
                m_openlist.get_weights()[0] += m_options.boost_preferred_queue;
        }

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

    static void set_parent(SearchNode& search_node, WorkerStateIndex<Kind> parent) noexcept
    {
        if constexpr (std::same_as<Search, SequentialSearch>)
            search_node.parent_state = parent.state;
        else
            search_node.parent_state = parent;
    }

    void open_successor(ygg::Index<State<Kind>> state, ygg::float_t g_value, ygg::float_t h_value, SearchNodeStatus, bool preferred)
    {
        if (preferred)
            m_preferred_openlist.insert(QueueEntry { g_value, h_value, state, m_step++ });
        else
            m_standard_openlist.insert(QueueEntry { g_value, h_value, state, m_step++ });
    }

    const ygg::SegmentedVector<SearchNode>& get_search_nodes() const noexcept { return m_search_nodes; }

private:
    using Queue = PriorityQueue<QueueEntry>;

    static constexpr ParentState no_parent() noexcept
    {
        if constexpr (std::same_as<Search, SequentialSearch>)
            return ygg::Index<State<Kind>>::max();
        else
            return WorkerStateIndex<Kind> { ygg::Index<Worker>::max(), ygg::Index<State<Kind>>::max() };
    }

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
};

static_assert(sizeof(LazyGBFSPolicy<LiftedTag, SequentialSearch>::SearchNode) == 16);
static_assert(sizeof(LazyGBFSPolicy<GroundTag, SequentialSearch>::SearchNode) == 16);
static_assert(sizeof(LazyGBFSPolicy<LiftedTag, SequentialSearch>::QueueEntry) == 24);
static_assert(sizeof(LazyGBFSPolicy<GroundTag, SequentialSearch>::QueueEntry) == 24);

}

#endif
