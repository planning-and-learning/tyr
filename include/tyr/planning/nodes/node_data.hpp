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
#ifndef TYR_PLANNING_NODES_NODE_DATA_HPP_
#define TYR_PLANNING_NODES_NODE_DATA_HPP_

#include "tyr/declarations.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/states/state_data.hpp"

#include <type_traits>
#include <urd/elements/data.hpp>
#include <urd/elements/index.hpp>
#include <yggdrasil/core/config.hpp>

namespace urd
{
/// A search node, packed: its state (registered: its key) and its metric (what reaching it cost, or what a search orders
/// it by). A value: no store holds it.
template<::tyr::TaskKind Kind>
struct data<::tyr::planning::Node<Kind>>
{
    urd::key<::tyr::planning::State<Kind>> state;
    ygg::float_t metric = 0;

    data() = default;

    data(urd::key<::tyr::planning::State<Kind>> state_, ygg::float_t metric_) : state(state_), metric(metric_) {}

    static constexpr bool copyable() noexcept
    {
        static_assert(std::is_trivially_copyable_v<data>, "a copyable record is trivially copyable");
        return true;
    }

    static constexpr auto fields() noexcept { return urd::Fields<urd::field<"state", &data::state>, urd::field<"metric", &data::metric>> {}; }

    static constexpr auto cista_members() noexcept { return urd::cista_members_of<data>(); }
};
}

#endif
