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

#ifndef TYR_PLANNING_SUCCESSORS_SUCCESSOR_DATA_HPP_
#define TYR_PLANNING_SUCCESSORS_SUCCESSOR_DATA_HPP_

#include "tyr/declarations.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/successors/distribution_data.hpp"
#include "tyr/planning/successors/outcome_node_data.hpp"
#include "tyr/planning/successors/transition_data.hpp"

#include <urd/elements/data.hpp>
#include <urd/elements/flat.hpp>
#include <urd/elements/index.hpp>
#include <utility>

namespace urd
{
/// A transition from a node where nature picks among its outcomes directly (an adversary in the nondeterministic
/// environment, chance in the probabilistic one): its label and its outcome nodes, each stored in the state repository
/// (their indices). Equal outcomes (same state and metric) are merged; chance's probabilities sum to 1. A transition
/// whose outcomes all fail is not applicable: it has no outcomes.
template<::tyr::planning::OutcomeEnvironment E, ::tyr::TaskKind Kind>
struct data<::tyr::planning::Successor<E, Kind>>
{
    urd::key<::tyr::planning::Successor<E, Kind>> index;
    urd::index<::tyr::planning::Transition> label;
    urd::flat_vector<urd::index<::tyr::planning::OutcomeNode<E, Kind>>> outcomes;

    data() = default;

    data(urd::index<::tyr::planning::Transition> label_, urd::flat_vector<urd::index<::tyr::planning::OutcomeNode<E, Kind>>> outcomes_) :
        index(),
        label(label_),
        outcomes(std::move(outcomes_))
    {
    }

    data(const data& other) = delete;
    data& operator=(const data& other) = delete;
    data(data&& other) = default;
    data& operator=(data&& other) = default;
    ~data() = default;

    static constexpr auto fields() noexcept { return urd::Fields<urd::field<"label", &data::label>, urd::field<"outcomes", &data::outcomes>> {}; }

    static constexpr auto cista_members() noexcept { return urd::cista_members_of<data>(); }

    [[nodiscard]] urd::key<::tyr::planning::Successor<E, Kind>> key() const noexcept { return index; }
};

/// A transition from a node in the general environment: its label and the distributions over outcome nodes an adversary
/// picks from (a oneof: one per outcome; chance: the distribution it draws from; a deterministic action: one of one
/// outcome), each stored in the state repository (their indices). Equal outcomes of a distribution (same state and
/// metric) are merged. A transition whose outcomes all fail is not applicable: it has no distributions.
template<::tyr::TaskKind Kind>
struct data<::tyr::planning::Successor<::tyr::planning::GeneralTag, Kind>>
{
    urd::key<::tyr::planning::Successor<::tyr::planning::GeneralTag, Kind>> index;
    urd::index<::tyr::planning::Transition> label;
    urd::flat_vector<urd::index<::tyr::planning::Distribution<Kind>>> distributions;

    data() = default;

    data(urd::index<::tyr::planning::Transition> label_, urd::flat_vector<urd::index<::tyr::planning::Distribution<Kind>>> distributions_) :
        index(),
        label(label_),
        distributions(std::move(distributions_))
    {
    }

    data(const data& other) = delete;
    data& operator=(const data& other) = delete;
    data(data&& other) = default;
    data& operator=(data&& other) = default;
    ~data() = default;

    static constexpr auto fields() noexcept { return urd::Fields<urd::field<"label", &data::label>, urd::field<"distributions", &data::distributions>> {}; }

    static constexpr auto cista_members() noexcept { return urd::cista_members_of<data>(); }

    [[nodiscard]] urd::key<::tyr::planning::Successor<::tyr::planning::GeneralTag, Kind>> key() const noexcept { return index; }
};
}

#endif
