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

#ifndef TYR_DATALOG_LIFTED_KCKP_WORKSPACES_PROGRAM_HPP_
#define TYR_DATALOG_LIFTED_KCKP_WORKSPACES_PROGRAM_HPP_

#include "tyr/analysis/program_analysis.hpp"
#include "tyr/analysis/variable_domains.hpp"
#include "tyr/datalog/declarations.hpp"
#include "tyr/datalog/fact_sets.hpp"
#include "tyr/datalog/lifted/kckp/assignment_sets.hpp"
#include "tyr/datalog/lifted/kckp/workspaces/rule.hpp"
#include "tyr/declarations.hpp"
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/declarations.hpp"

#include <memory>
#include <optional>
#include <urd/kinds.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>

namespace tyr::datalog
{

/// The workspaces of the rules of relation kind R, by rule index (none for a rule without one).
using RuleWorkspaces = urd::Store<urd::sequence<std::unique_ptr<RuleWorkspace<LiftedTag>>>>;

using ConstRuleWorkspaces = urd::Store<urd::sequence<std::optional<ConstRuleWorkspace<LiftedTag>>>>;

/// The KCKP engine's part of a program that every evaluation shares: the static facts' assignment sets and each
/// rule's const workspace (by rule index; none for rules the program does not evaluate).
struct ConstKCKPWorkspace
{
    TaggedAssignmentSets<formalism::StaticTag> assignment_sets;
    ConstRuleWorkspaces rules;

    ConstKCKPWorkspace(formalism::ProgramView<LiftedTag> program,
                       formalism::Repository& program_repository,
                       analysis::ProgramAnalysis& analysis,
                       const TaggedFactSets<formalism::StaticTag>& static_fact_sets);
};

/// The KCKP engine's part of an evaluation: the assignment sets of the facts of each mutable kind (KCKP's index over the
/// facts, rebuilt from them when an evaluation starts and kept in step as facts are committed) and each rule's workspace.
struct KCKPWorkspace
{
    MutableAssignmentSets assignment_sets;
    RuleWorkspaces rules;

    KCKPWorkspace(const ConstKCKPWorkspace& const_workspace,
                  formalism::ProgramView<LiftedTag> program,
                  const analysis::ProgramVariableDomains& domains,
                  const MutableFactSets& fact_sets);

    /// Forgets the rules' evaluation state.
    void clear();
};

/// Prints the aggregated statistics of the rules and of their workers.
void print_rule_statistics(const RuleWorkspaces& rules);

}

#endif
