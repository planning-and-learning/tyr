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

#ifndef TYR_DATALOG_WORKSPACES_RULE_HPP_
#define TYR_DATALOG_WORKSPACES_RULE_HPP_

#include "tyr/datalog/declarations.hpp"

namespace tyr::datalog
{

template<TaskKind Kind, ::tyr::formalism::RelationKind R>
struct RuleWorkspace;

template<TaskKind Kind, ::tyr::formalism::RelationKind R>
struct ConstRuleWorkspace;

}

#endif
