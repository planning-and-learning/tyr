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

#ifndef TYR_ANALYSIS_DOMAINS_HPP_
#define TYR_ANALYSIS_DOMAINS_HPP_

#include "tyr/analysis/declarations.hpp"
#include "tyr/analysis/domains_view.hpp"
#include "tyr/analysis/program_analysis.hpp"
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/planning/declarations.hpp"

#include <utility>
#include <vector>
#include <yggdrasil/core/types.hpp>
#include <yggdrasil/semantics/equal_to.hpp>
#include <yggdrasil/semantics/hash.hpp>

namespace tyr::analysis
{
ProgramVariableDomains compute_variable_domains(formalism::datalog::ProgramView<LiftedTag> program);

ProgramAnalysis analyze_program(formalism::datalog::ProgramView<LiftedTag> program);

TaskVariableDomains compute_variable_domains(formalism::planning::TaskView<LiftedTag> task);

ProgramVariableDomainsView compute_variable_domain_views(const ProgramVariableDomains& domains, const formalism::datalog::Repository& repository);

TaskVariableDomainsView compute_variable_domain_views(const TaskVariableDomains& domains, const formalism::planning::Repository& repository);
}

#endif
