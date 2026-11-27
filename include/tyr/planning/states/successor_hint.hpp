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

#ifndef TYR_PLANNING_STATES_SUCCESSOR_HINT_HPP_
#define TYR_PLANNING_STATES_SUCCESSOR_HINT_HPP_

#include "tyr/declarations.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/states/state_data.hpp"
#include "tyr/planning/states/state_delta.hpp"

#include <concepts>
#include <urd/elements/index.hpp>

namespace tyr::planning
{

/// A successor state's parent and its exact changes from it: a hint to the state repository's insert, by which each pool
/// of a state's storages stores the successor's as a difference to the parent's, where it can (urd's insert_built with
/// hints). The parent is registered in the same storage.
template<TaskKind Kind>
struct SuccessorHint
{
    /// The parent's record: its storages' keys.
    const urd::data<State<Kind>>* parent;
    /// The parent, read: its derived atoms, which a successor's are compared with.
    const StateBuilderView<Kind>* parent_state;
    /// The exact changes from the parent: its fluent facts' and numeric values'.
    const StateDelta* delta;

    /// The parent's key of its storage S.
    template<typename S>
    [[nodiscard]] urd::key<S> parent_key() const noexcept
    {
        if constexpr (std::same_as<S, FactStorage<Kind>>)
            return parent->fact_storage;
        else if constexpr (std::same_as<S, AtomStorage<Kind>>)
            return parent->atom_storage;
        else if constexpr (std::same_as<S, NumericStorage<Kind>>)
            return parent->numeric_storage;
        else
        {
            static_assert(std::same_as<S, TemporalStorage<Kind>>, "SuccessorHint::parent_key: a state's storage");
            return parent->temporal_storage;
        }
    }
};

}

#endif
