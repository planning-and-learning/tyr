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
#ifndef TYR_PLANNING_NODES_LABELED_NODE_DATA_HPP_
#define TYR_PLANNING_NODES_LABELED_NODE_DATA_HPP_

#include "tyr/declarations.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/nodes/node_data.hpp"

#include <type_traits>
#include <urd/elements/data.hpp>
#include <urd/elements/index.hpp>
#include <yggdrasil/formalism/declarations.hpp>

namespace urd
{
/// A search node and the action binding that reached it (its label: an interned row of its action), packed. A value.
template<::tyr::TaskKind Kind>
struct data<::tyr::planning::LabeledNode<Kind>>
{
    urd::key<::tyr::formalism::Action<::tyr::LiftedTag>, ygg::Row<::tyr::formalism::Action<::tyr::LiftedTag>>> label;
    urd::data<::tyr::planning::Node<Kind>> node;

    data() = default;

    data(urd::key<::tyr::formalism::Action<::tyr::LiftedTag>, ygg::Row<::tyr::formalism::Action<::tyr::LiftedTag>>> label_,
         urd::data<::tyr::planning::Node<Kind>> node_) :
        label(label_),
        node(node_)
    {
    }

    static constexpr bool copyable() noexcept
    {
        static_assert(std::is_trivially_copyable_v<data>, "a copyable record is trivially copyable");
        return true;
    }

    static constexpr auto fields() noexcept { return urd::Fields<urd::field<"label", &data::label>, urd::field<"node", &data::node>> {}; }

    static constexpr auto cista_members() noexcept { return urd::cista_members_of<data>(); }
};
}

#endif
