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

#ifndef TYR_DATALOG_RULE_MAPPING_HPP_
#define TYR_DATALOG_RULE_MAPPING_HPP_

#include "tyr/formalism/datalog/declarations.hpp"

#include <urd/elements/equal_to.hpp>
#include <urd/elements/hash.hpp>
#include <urd/repr/maps/std_map.hpp>
#include <urd/repr/maps/unordered_flat_map.hpp>
#include <urd/repr/values/unordered_flat_set.hpp>
#include <urd/store.hpp>

namespace tyr::datalog
{

/// The rule of a program transformation's output that replaces each rule of its source program, rewritten or copied; source
/// rules index the source program's repository.
using RuleMapping = urd::Store<urd::map<urd::index<formalism::datalog::Rule<LiftedTag>>, urd::index<formalism::datalog::Rule<LiftedTag>>>>;

}

#endif
