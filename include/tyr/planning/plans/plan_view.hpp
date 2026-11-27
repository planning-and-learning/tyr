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
#ifndef TYR_PLANNING_PLANS_PLAN_VIEW_HPP_
#define TYR_PLANNING_PLANS_PLAN_VIEW_HPP_

#include "tyr/declarations.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/nodes/labeled_node_view.hpp"
#include "tyr/planning/nodes/node_view.hpp"
#include "tyr/planning/plans/plan_builder.hpp"
#include "tyr/planning/plans/plan_data.hpp"

#include <cstddef>
#include <ranges>
#include <urd/elements/data.hpp>
#include <urd/elements/flat.hpp>
#include <urd/elements/view.hpp>
#include <yggdrasil/core/config.hpp>

/// A plan: its start node and the labeled nodes that follow it; its length, its cost (its last node's metric) and the
/// node it ends in.
template<::tyr::TaskKind Kind, typename V>
struct urd::view_mixin<::tyr::planning::Plan<Kind>, V>
{
private:
    [[nodiscard]] const V& self() const noexcept { return static_cast<const V&>(*this); }

public:
    [[nodiscard]] auto get_start_node() const noexcept { return self().template member<&urd::record_of<V>::start_node>(); }

    [[nodiscard]] auto get_labeled_succ_nodes() const noexcept { return self().template member<&urd::record_of<V>::labeled_succ_nodes>(); }

    [[nodiscard]] std::size_t get_length() const noexcept { return std::ranges::size(get_labeled_succ_nodes()); }

    [[nodiscard]] bool empty() const noexcept { return get_length() == 0; }

    /// The node it ends in: its last labeled node's, or its start node.
    [[nodiscard]] auto get_goal_node() const noexcept
    {
        const auto steps = get_labeled_succ_nodes();
        return std::ranges::empty(steps) ? get_start_node() : (*std::ranges::prev(std::ranges::end(steps))).get_node();
    }

    /// Its last node's metric, 0 where it has no step.
    [[nodiscard]] ygg::float_t get_cost() const noexcept { return empty() ? ygg::float_t { 0 } : get_goal_node().get_metric(); }

    /// Its record: its nodes' records. A plan's builder's only.
    [[nodiscard]] urd::data<::tyr::planning::Plan<Kind>> pack() const
        requires urd::is_builder<urd::record_of<V>>
    {
        auto steps = urd::flat_vector<urd::data<::tyr::planning::LabeledNode<Kind>>> {};
        for (const auto& step : get_labeled_succ_nodes())
            steps.push_back(step.pack());
        return urd::data<::tyr::planning::Plan<Kind>>(get_start_node().pack(), std::move(steps));
    }

    static constexpr auto functions() noexcept
    {
        constexpr auto read = urd::Functions<urd::function<"get_length", &view_mixin::get_length>,
                                             urd::function<"empty", &view_mixin::empty>,
                                             urd::function<"get_goal_node", &view_mixin::get_goal_node>,
                                             urd::function<"get_cost", &view_mixin::get_cost>> {};
        if constexpr (urd::is_builder<urd::record_of<V>>)
            return urd::Functions<urd::function<"pack", &view_mixin::pack>> {} + read;
        else
            return read;
    }
};

#endif
