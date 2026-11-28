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
#ifndef TYR_PLANNING_NODES_LABELED_NODE_VIEW_HPP_
#define TYR_PLANNING_NODES_LABELED_NODE_VIEW_HPP_

#include "tyr/declarations.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/nodes/labeled_node_builder.hpp"
#include "tyr/planning/nodes/labeled_node_data.hpp"
#include "tyr/planning/nodes/node_view.hpp"

#include <urd/elements/children.hpp>
#include <urd/elements/view.hpp>

/// A search node and the action binding that reached it: its label (an action row view) and its node.
template<::tyr::TaskKind Kind, typename V>
struct urd::view_mixin<::tyr::planning::LabeledNode<Kind>, V>
{
private:
    [[nodiscard]] const V& self() const noexcept { return static_cast<const V&>(*this); }

public:
    [[nodiscard]] auto label() const noexcept { return self().template member<&urd::record_of<V>::label>(); }

    [[nodiscard]] auto node() const noexcept { return self().template member<&urd::record_of<V>::node>(); }

    /// Its record (urd::pack): its label's key and its node's record. A labeled node's builder's only, its label interned
    /// (std::invalid_argument otherwise: a borrowed binding is interned first, intern_label).
    [[nodiscard]] urd::data<::tyr::planning::LabeledNode<Kind>> pack() const
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
