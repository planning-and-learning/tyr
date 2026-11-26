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

#ifndef TYR_PLANNING_OUTCOME_NODE_DATA_HPP_
#define TYR_PLANNING_OUTCOME_NODE_DATA_HPP_

#include "tyr/declarations.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/state_data.hpp"
#include <yggdrasil/core/config.hpp>

#include <type_traits>
#include <urd/elements/data.hpp>
#include <urd/elements/flat.hpp>
#include <urd/elements/index.hpp>
#include <utility>

namespace urd
{

/// One outcome of a transition: its successor state (registered: its key; being built: a pooled state builder), its
/// metric, the probability chance draws it with in its distribution (1 where no chance is involved), and whether it fails
/// (a running durative action's over-all condition does not hold in it: a plan reaching it is invalid).
template<::tyr::TaskKind Kind>
struct data<::tyr::planning::OutcomeNode<Kind>>
{
    urd::key<::tyr::planning::OutcomeNode<Kind>> index;
    urd::key<::tyr::planning::State<Kind>> state;
    ygg::float_t metric;
    ygg::float_t probability;
    bool failed;

    data() = default;

    data(urd::key<::tyr::planning::State<Kind>> state_, ygg::float_t metric_, ygg::float_t probability_, bool failed_) : index(), state(state_), metric(metric_), probability(probability_), failed(failed_) {}

    static constexpr bool copyable() noexcept
    {
        static_assert(std::is_trivially_copyable_v<data>, "a copyable record is trivially copyable");
        return true;
    }

    static constexpr auto fields() noexcept { return urd::Fields<urd::field<"state", &data::state>, urd::field<"metric", &data::metric>, urd::field<"probability", &data::probability>, urd::field<"failed", &data::failed>> {}; }

    static constexpr auto identifying_members() noexcept { return urd::members_of(fields()); }

    static constexpr auto cista_members() noexcept { return urd::Members<&data::index, &data::state, &data::metric, &data::probability, &data::failed> {}; }

    [[nodiscard]] urd::key<::tyr::planning::OutcomeNode<Kind>> key() const noexcept { return urd::key<::tyr::planning::OutcomeNode<Kind>>(index); }
};

}

#endif
