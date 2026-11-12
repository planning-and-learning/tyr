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

#include "tyr/datalog/lifted/kckp/workspaces/program.hpp"

#include "tyr/datalog/formatter.hpp"

#include <fmt/ostream.h>
#include <iostream>

namespace tyr::datalog
{
namespace
{
void initialize_rule_workspaces(const ConstKCKPWorkspace& const_workspace, RuleWorkspaces& workspaces)
{
    const auto& const_workspaces = const_workspace.rules;
    workspaces.reserve(const_workspaces.size());
    for (const auto& workspace : const_workspaces)
        workspaces.emplace_back(workspace ? std::make_unique<RuleWorkspace<LiftedTag>>(*workspace) : nullptr);
}

void initialize_const_rule_workspaces(formalism::datalog::ProgramView<LiftedTag> program,
                                      formalism::datalog::Repository& program_repository,
                                      analysis::RuleCompatibilityGraphMap& graphs,
                                      ConstRuleWorkspaces& workspaces)
{
    const auto rules = program.get_rules();
    workspaces.resize(rules.size());
    for (const auto rule : rules)
        workspaces[static_cast<ygg::uint_t>(rule.index().value)].emplace(rule, program_repository, std::move(graphs.at(rule.index())));
}
}

ConstKCKPWorkspace::ConstKCKPWorkspace(formalism::datalog::ProgramView<LiftedTag> program,
                                       formalism::datalog::Repository& program_repository,
                                       analysis::ProgramAnalysis& analysis,
                                       const TaggedFactSets<formalism::StaticTag>& static_fact_sets) :
    assignment_sets(program.get_predicates<formalism::StaticTag>(),
                    program.get_functions<formalism::StaticTag>(),
                    analysis.domains.static_predicate_domains,
                    analysis.domains.static_function_domains,
                    program.get_objects().size(),
                    static_fact_sets),
    rules()
{ initialize_const_rule_workspaces(program, program_repository, analysis.compatibility_graphs, rules); }

KCKPWorkspace::KCKPWorkspace(const ConstKCKPWorkspace& const_workspace,
                             formalism::datalog::ProgramView<LiftedTag> program,
                             const analysis::ProgramVariableDomains& domains,
                             const TaggedFactSets<formalism::FluentTag>& fact_sets) :
    assignment_sets(program.get_predicates<formalism::FluentTag>(),
                    program.get_functions<formalism::FluentTag>(),
                    domains.fluent_predicate_domains,
                    domains.fluent_function_domains,
                    program.get_objects().size(),
                    fact_sets),
    rules()
{ initialize_rule_workspaces(const_workspace, rules); }

void KCKPWorkspace::clear()
{
    for (auto& rule : rules)
        if (rule)
            rule->clear();
}

void print_rule_statistics(const RuleWorkspaces& rules)
{
    auto rule_statistics = urd::Store<urd::sequence<RuleStatistics>> {};
    auto rule_worker_statistics = urd::Store<urd::sequence<RuleWorkerStatistics>> {};
    for (const auto& ws_rule : rules)
    {
        if (!ws_rule)
            continue;
        rule_statistics.push_back(ws_rule->common.statistics);
        for (const auto& worker : ws_rule->worker)
            rule_worker_statistics.push_back(worker.solve.statistics);
    }
    fmt::print(std::cout, "{}\n", compute_aggregated_rule_statistics(rule_statistics));
    fmt::print(std::cout, "{}\n", compute_aggregated_rule_worker_statistics(rule_worker_statistics));
}

}
