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

#include "tyr/datalog/workspaces/rule.hpp"
#include "tyr/formalism/datalog/repository.hpp"

#include <concepts>
#include <optional>
#include <type_traits>
#include <variant>
#include <vector>
#include <yggdrasil/core/types.hpp>

namespace tyr::datalog
{

template<::tyr::formalism::RelationKind R>
struct RuleState
{
    ygg::uint_t unsatisfied_count = 0;
    bool fired = false;
    std::optional<Cost> queued_cost;
    std::vector<bool> numeric_constraint_satisfied;
    [[no_unique_address]] std::conditional_t<std::same_as<R, ::tyr::formalism::PredicateTag>, std::optional<Cost>, std::monostate> pending_cost;
};

template<::tyr::formalism::RelationKind R>
struct RuleWorkspace<GroundTag, R>
{
    std::vector<RuleState<R>> states;

    explicit RuleWorkspace(::tyr::formalism::datalog::ProgramView<GroundTag>) {}

    void clear() { states.clear(); }
};

}

#endif
