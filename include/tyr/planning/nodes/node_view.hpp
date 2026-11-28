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

/// A search node: its state (a packed node reads it as its StateView; a node's builder as its state builder's view)
/// and its metric.
template<::tyr::TaskKind Kind, typename V>
struct urd::view_mixin<::tyr::planning::Node<Kind>, V>
{
private:
    [[nodiscard]] const V& self() const noexcept { return static_cast<const V&>(*this); }

public:
    [[nodiscard]] auto state() const noexcept { return self().template member<&urd::record_of<V>::state>(); }

    [[nodiscard]] auto metric() const noexcept { return self().template member<&urd::record_of<V>::metric>(); }

    /// Its record (urd::pack): its state's key (a registered state's index; none for a state being built) and its
    /// metric. A node's builder's only (a packed node is its record).
    [[nodiscard]] urd::data<::tyr::planning::Node<Kind>> pack() const
        requires urd::Builder<urd::record_of<V>>
    { return urd::pack(self()); }

    static constexpr auto functions() noexcept
    {
        if constexpr (urd::Builder<urd::record_of<V>>)
            return urd::Functions<urd::function<"pack", &view_mixin::pack>> {};
        else
            return urd::Functions<> {};
    }
};

#endif
