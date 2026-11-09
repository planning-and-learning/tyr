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
#include "tyr/datalog/declarations.hpp"
#include "tyr/datalog/fact_sets.hpp"
#include "tyr/datalog/lifted/kckp/assignment_sets.hpp"
#include "tyr/datalog/lifted/kckp/workspaces/rule.hpp"
#include "tyr/formalism/datalog/repository.hpp"

#include <memory>
#include <optional>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <urd/utils/family.hpp>

namespace tyr::datalog
{

/// The workspaces of the rules of relation kind R, by rule index (none for a rule without one).
template<typename R>
using RuleWorkspaces = urd::Store<urd::sequence<std::unique_ptr<RuleWorkspace<LiftedTag, R>>>>;

template<typename R>
using ConstRuleWorkspaces = urd::Store<urd::sequence<std::optional<ConstRuleWorkspace<LiftedTag, R>>>>;

/// The KCKP engine's part of a program that every evaluation shares: the static facts' assignment sets and each
/// rule's const workspace (by rule index; none for rules the program does not evaluate).
struct ConstKCKPWorkspace
{
    TaggedAssignmentSets<formalism::StaticTag> assignment_sets;
    urd::utils::Family<ConstRuleWorkspaces, formalism::PredicateTag, formalism::FunctionTag> rules;

    ConstKCKPWorkspace(formalism::datalog::ProgramView<LiftedTag> program,
                       formalism::datalog::Repository& program_repository,
                       analysis::ProgramAnalysis& analysis,
                       const TaggedFactSets<formalism::StaticTag>& static_fact_sets);
};

/// The KCKP engine's part of an evaluation: the fluent facts' assignment sets (KCKP's index over the facts, rebuilt
/// from them when an evaluation starts and kept in step as facts are committed) and each rule's workspace.
struct KCKPWorkspace
{
    TaggedAssignmentSets<formalism::FluentTag> assignment_sets;
    urd::utils::Family<RuleWorkspaces, formalism::PredicateTag, formalism::FunctionTag> rules;

    KCKPWorkspace(const ConstKCKPWorkspace& const_workspace,
                  formalism::datalog::ProgramView<LiftedTag> program,
                  const analysis::ProgramVariableDomains& domains,
                  const TaggedFactSets<formalism::FluentTag>& fact_sets);

    /// Forgets the rules' evaluation state.
    void clear();
};

/// Prints the aggregated statistics of the rules and of their workers.
void print_rule_statistics(const urd::utils::Family<RuleWorkspaces, formalism::PredicateTag, formalism::FunctionTag>& rules);

}

#endif
