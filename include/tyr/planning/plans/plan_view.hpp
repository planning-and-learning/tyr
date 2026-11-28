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
#include <fmt/base.h>
#include <ranges>
#include <urd/elements/data.hpp>
#include <urd/elements/flat.hpp>
#include <urd/elements/view.hpp>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/formatting/without_format_spec.hpp>

/// A plan: its start node and the labeled nodes that follow it; its length, its cost (its last node's metric) and the
/// node it ends in.
template<::tyr::TaskKind Kind, typename V>
struct urd::view_mixin<::tyr::planning::Plan<::tyr::planning::DeterministicTag, Kind>, V>
{
private:
    [[nodiscard]] const V& self() const noexcept { return static_cast<const V&>(*this); }

public:
    [[nodiscard]] auto start_node() const noexcept { return self().template member<&urd::record_of<V>::start_node>(); }

    [[nodiscard]] auto labeled_succ_nodes() const noexcept { return self().template member<&urd::record_of<V>::labeled_succ_nodes>(); }

    [[nodiscard]] std::size_t length() const noexcept { return std::ranges::size(labeled_succ_nodes()); }

    [[nodiscard]] bool empty() const noexcept { return length() == 0; }

    /// The node it ends in: its last labeled node's, or its start node.
    [[nodiscard]] auto goal_node() const noexcept
    {
        const auto steps = labeled_succ_nodes();
        return std::ranges::empty(steps) ? start_node() : (*std::ranges::prev(std::ranges::end(steps))).node();
    }

    /// Its last node's metric, 0 where it has no step.
    [[nodiscard]] ygg::float_t cost() const noexcept { return empty() ? ygg::float_t { 0 } : goal_node().metric(); }

    /// Its record (urd::pack): its nodes' records. A plan's builder's only.
    [[nodiscard]] urd::data<::tyr::planning::Plan<::tyr::planning::DeterministicTag, Kind>> pack() const
        requires urd::Builder<urd::record_of<V>>
    { return urd::pack(self()); }

    static constexpr auto functions() noexcept
    {
        constexpr auto read = urd::Functions<urd::function<"length", &view_mixin::length>,
                                             urd::function<"empty", &view_mixin::empty>,
                                             urd::function<"goal_node", &view_mixin::goal_node>,
                                             urd::function<"cost", &view_mixin::cost>> {};
        if constexpr (urd::Builder<urd::record_of<V>>)
            return urd::Functions<urd::function<"pack", &view_mixin::pack>> {} + read;
        else
            return read;
    }
};

/// A plan, as its views (borrowed or owned, of its builder or its record) write it: its actions, one per line.
template<::tyr::TaskKind Kind, typename R>
struct fmt::formatter<urd::borrowed_view<tyr::planning::Plan<tyr::planning::DeterministicTag, Kind>, tyr::planning::StateRepository<Kind>, R>, char> : ygg::without_format_spec
{
    fmt::format_context::iterator format(const urd::borrowed_view<tyr::planning::Plan<tyr::planning::DeterministicTag, Kind>, tyr::planning::StateRepository<Kind>, R>& value,
                                         fmt::format_context& ctx) const;
};

template<::tyr::TaskKind Kind, typename R>
struct fmt::formatter<urd::owning_view<tyr::planning::Plan<tyr::planning::DeterministicTag, Kind>, tyr::planning::StateRepository<Kind>, R>, char> :
    fmt::formatter<urd::borrowed_view<tyr::planning::Plan<tyr::planning::DeterministicTag, Kind>, tyr::planning::StateRepository<Kind>, R>, char>
{
};

#endif
