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
#ifndef TYR_PLANNING_NODES_NODE_VIEW_HPP_
#define TYR_PLANNING_NODES_NODE_VIEW_HPP_

#include "tyr/declarations.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/nodes/node_builder.hpp"
#include "tyr/planning/nodes/node_data.hpp"
#include "tyr/planning/states/state_view.hpp"

#include <concepts>
#include <type_traits>
#include <urd/elements/view.hpp>

/// A search node: its state (a packed node reads it as its StateView; a node's builder as the view of the form its
/// child holds: urd::built_view takes the built one out) and its metric.
template<::tyr::TaskKind Kind, typename V>
struct urd::view_mixin<::tyr::planning::Node<Kind>, V>
{
private:
    [[nodiscard]] const V& self() const noexcept { return static_cast<const V&>(*this); }

public:
    [[nodiscard]] auto get_state() const noexcept { return self().template member<&urd::record_of<V>::state>(); }

    [[nodiscard]] auto get_metric() const noexcept { return self().template member<&urd::record_of<V>::metric>(); }

    /// Its record: its state's key (an interned state's; a built one's index, a registered state's; none for a state being
    /// built or a record not interned) and its metric. A node's builder's only (a packed node is its record).
    [[nodiscard]] urd::data<::tyr::planning::Node<Kind>> pack() const noexcept
        requires urd::is_builder<urd::record_of<V>>
    {
        const auto state = urd::visit_view(get_state(),
                                           [](const auto& view) -> urd::key<::tyr::planning::State<Kind>>
                                           {
                                               using View = std::remove_cvref_t<decltype(view)>;
                                               if constexpr (std::same_as<View, ::tyr::planning::StateView<Kind>>)
                                                   return view.index();
                                               else if constexpr (urd::is_builder<urd::record_of<View>>)
                                                   return view.pack().index();
                                               else
                                                   return {};
                                           });
        return urd::data<::tyr::planning::Node<Kind>>(state, get_metric());
    }

    static constexpr auto functions() noexcept
    {
        if constexpr (urd::is_builder<urd::record_of<V>>)
            return urd::Functions<urd::function<"pack", &view_mixin::pack>> {};
        else
            return urd::Functions<> {};
    }
};

#endif
