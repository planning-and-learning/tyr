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

#ifndef TYR_ANALYSIS_STRATIFICATION_HPP_
#define TYR_ANALYSIS_STRATIFICATION_HPP_

#include "tyr/declarations.hpp"
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/declarations.hpp"

#include <urd/elements/index.hpp>
#include <urd/kinds.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
// for Program (ptr only), Rule

#include <urd/elements/flat.hpp>
#include <yggdrasil/formatting/cista_formatters.hpp>

namespace tyr::analysis
{

/// The rules of a stratum: those whose heads it holds.
template<TaskKind Kind>
using RuleStratum = urd::flat_vector<urd::index<formalism::Rule<Kind>>>;

/// A program's rules by stratum, lowest first: a rule's negative literals read relations of lower strata only.
template<TaskKind Kind>
struct RuleStrata
{
    urd::Store<urd::sequence<RuleStratum<Kind>>> data;
};

/// The least strata of the program's rules, over its relations (predicates and fluent functions); throws if a relation
/// depends negatively on itself.
template<TaskKind Kind>
RuleStrata<Kind> compute_rule_stratification(formalism::ProgramView<Kind> program);
}

#endif
