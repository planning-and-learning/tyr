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

#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/declarations.hpp"

#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
// for Program (ptr only), Rule

#include <urd/elements/flat.hpp>
#include <vector>
#include <yggdrasil/core/family.hpp>
#include <yggdrasil/formatting/cista_formatters.hpp>

namespace tyr::analysis
{

template<typename R>
using TypedRuleStratum = urd::flat_vector<urd::index<formalism::datalog::Rule<LiftedTag, R>>>;

using RuleStratum = ygg::Family<TypedRuleStratum, formalism::PredicateTag, formalism::FunctionTag>;

struct RuleStrata
{
    urd::Store<urd::sequence<RuleStratum>> data;
};

RuleStrata compute_rule_stratification(formalism::datalog::ProgramView<LiftedTag> program);
}

#endif
