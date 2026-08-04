/*
 * Copyright (C) 2025-2026 Dominik Drexler
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

#ifndef TYR_PLANNING_ALGORITHMS_BRFS_EVENT_HANDLER_HPP_
#define TYR_PLANNING_ALGORITHMS_BRFS_EVENT_HANDLER_HPP_

#include "tyr/formalism/planning/ground_action_view.hpp"
#include "tyr/planning/algorithms/statistics.hpp"
#include "tyr/planning/algorithms/utils.hpp"
#include "tyr/planning/declarations.hpp"

#include <cstddef>

namespace tyr::planning::brfs
{

template<TaskKind Kind>
class EventHandler
{
public:
    using StatisticsType = tyr::planning::Statistics;

    virtual ~EventHandler() = default;

    virtual void on_expand_node(const Node<Kind>& node) = 0;

    virtual void on_expand_goal_node(const Node<Kind>& node) = 0;

    virtual void on_generate_node(const Node<Kind>& source_node, const LabeledNode<Kind>& labeled_succ_node) = 0;

    virtual void on_prune_node(const Node<Kind>& node) = 0;

    virtual void on_prune_node(const Node<Kind>& source_node, const LabeledNode<Kind>& labeled_succ_node) = 0;

    virtual void on_start_search(const Node<Kind>& node) = 0;

    virtual void on_finish_layer(ygg::uint_t layer, const tyr::planning::Statistics& statistics) = 0;

    virtual void on_end_search(tyr::planning::SearchStatus status, const tyr::planning::Statistics& statistics) = 0;

    virtual void on_solved(const Plan<Kind>& plan) = 0;
};

template<typename Derived, TaskKind Kind>
class EventHandlerBase : public EventHandler<Kind>
{
protected:
    tyr::planning::ProgressStatistics m_progress_statistics;
    size_t m_verbosity;

private:
    EventHandlerBase() = default;
    friend Derived;

    constexpr const auto& self() const { return static_cast<const Derived&>(*this); }
    constexpr auto& self() { return static_cast<Derived&>(*this); }

    bool verbosity(size_t level) const { return m_verbosity >= level; }

public:
    explicit EventHandlerBase(size_t verbosity = 0) : m_verbosity(verbosity) {}

    void on_expand_node(const Node<Kind>& node) override
    {
        if (verbosity(2))
            self().on_expand_node_impl(node);
    }

    void on_expand_goal_node(const Node<Kind>& node) override
    {
        if (verbosity(2))
            self().on_expand_goal_node_impl(node);
    }

    void on_generate_node(const Node<Kind>& source_node, const LabeledNode<Kind>& labeled_succ_node) override
    {
        static_cast<void>(source_node);

        if (verbosity(2))
            self().on_generate_node_impl(labeled_succ_node);
    }

    void on_prune_node(const Node<Kind>& node) override
    {
        if (verbosity(2))
            self().on_prune_node_impl(node);
    }

    void on_prune_node(const Node<Kind>& source_node, const LabeledNode<Kind>& labeled_succ_node) override
    {
        static_cast<void>(source_node);

        if (verbosity(2))
            self().on_prune_node_impl(labeled_succ_node.node);
    }

    void on_start_search(const Node<Kind>& node) override
    {
        m_progress_statistics.clear();

        if (verbosity(1))
            self().on_start_search_impl(node);
    }

    void on_finish_layer(ygg::uint_t layer, const tyr::planning::Statistics& statistics) override
    {
        m_progress_statistics.add_snapshot(statistics);

        if (verbosity(1))
            self().on_finish_layer_impl(layer, statistics);
    }

    void on_end_search(tyr::planning::SearchStatus status, const tyr::planning::Statistics& statistics) override
    {
        if (verbosity(1))
            self().on_end_search_impl(status, statistics);
    }

    void on_solved(const Plan<Kind>& plan) override
    {
        if (verbosity(1))
            self().on_solved_impl(plan);
    }

    const tyr::planning::ProgressStatistics& get_progress_statistics() const { return m_progress_statistics; }
};

template<TaskKind Kind>
class DefaultEventHandler : public EventHandlerBase<DefaultEventHandler<Kind>, Kind>
{
private:
    friend class EventHandlerBase<DefaultEventHandler<Kind>, Kind>;

    void on_expand_node_impl(const Node<Kind>& node) const;

    void on_expand_goal_node_impl(const Node<Kind>& node) const;

    void on_generate_node_impl(const LabeledNode<Kind>& labeled_succ_node) const;

    void on_prune_node_impl(const Node<Kind>& node) const;

    void on_start_search_impl(const Node<Kind>& node) const;

    void on_finish_layer_impl(ygg::uint_t layer, const tyr::planning::Statistics& statistics) const;

    void on_end_search_impl(tyr::planning::SearchStatus status, const tyr::planning::Statistics& statistics) const;

    void on_solved_impl(const Plan<Kind>& plan) const;

public:
    DefaultEventHandler(size_t verbosity = 0);

    static DefaultEventHandlerPtr<Kind> create(size_t verbosity = 0);
};

}

#endif
