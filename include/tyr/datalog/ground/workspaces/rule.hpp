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

#ifndef TYR_DATALOG_GROUND_WORKSPACES_RULE_HPP_
#define TYR_DATALOG_GROUND_WORKSPACES_RULE_HPP_

#include "tyr/datalog/declarations.hpp"
#include <urd/repr/sequences/sequence.hpp>
#include <urd/repr/values/dense_set.hpp>
#include <urd/store.hpp>
#include "tyr/datalog/policies/aggregation.hpp"
#include "tyr/formalism/datalog/repository.hpp"

#include <optional>
#include <yggdrasil/core/config.hpp>

namespace tyr::datalog
{

struct RuleState
{
    ygg::uint_t unsatisfied_count = 0;
    std::optional<Cost> queued_cost;
    /// The positions of the rule's numeric constraints that hold.
    urd::Store<urd::set<ygg::uint_t>, urd::dense> numeric_constraint_satisfied;
};

/// The states of the ground rules of relation kind R, by rule index (an ygg::Family member).
template<typename R>
using RuleStates = urd::Store<urd::sequence<RuleState>>;

}

#endif
