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

#include "tyr/datalog/lifted/kckp/workspaces/rule.hpp"

#include "tyr/datalog/lifted/kckp/assignment_sets.hpp"
#include "tyr/formalism/datalog/grounder.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/datalog/rule_view.hpp"

namespace tyr::datalog
{
/**
 * ConstRuleWorkspace<LiftedTag>
 */

ConstRuleWorkspace<LiftedTag>::ConstRuleWorkspace(formalism::datalog::RuleView<LiftedTag> rule_,
                                                  formalism::datalog::Repository& repository,
                                                  kckp::Graph compatibility_graph) :
    rule(rule_),
    nullary_condition(create_ground_nullary_conjunctive_condition(get_rule().get_body(), repository).first),
    unary_overapproximation_condition(create_overapproximation_conjunctive_condition(1, get_rule().get_body(), repository).first),
    binary_overapproximation_condition(create_overapproximation_conjunctive_condition(2, get_rule().get_body(), repository).first),
    conflicting_overapproximation_condition(
        create_overapproximation_conflicting_conjunctive_condition(get_rule().get_arity() == 1 ? 1 : 2, get_rule().get_body(), repository).first),
    hyperedge_condition(create_hyperedge_conjunctive_condition(conflicting_overapproximation_condition, repository).first),
    static_consistency_graph(unary_overapproximation_condition, binary_overapproximation_condition, hyperedge_condition, std::move(compatibility_graph))
{
}

RuleWorkspace<LiftedTag>::Common::Common(const StaticConsistencyGraph& static_consistency_graph_) :
    static_consistency_graph(&static_consistency_graph_),
    kckp(static_consistency_graph_.get_graph(), static_consistency_graph_.get_partitioned_adjacency_layout()),
    statistics()
{
}

void RuleWorkspace<LiftedTag>::Common::clear() noexcept { kckp.reset(); }

void RuleWorkspace<LiftedTag>::Common::initialize_iteration(const AssignmentSets& assignment_sets)
{
    kckp.update([&](auto& delta_graph, auto& full_graph)
                { static_consistency_graph->initialize_dynamic_consistency_graphs(assignment_sets, kckp.get_graph_layout(), delta_graph, full_graph); });
}

RuleWorkspace<LiftedTag>::Iteration::Iteration(const ConstRuleWorkspace<LiftedTag>&, const Common& common) : kckp_workspace(common.kckp.get_graph_layout()) {}

void RuleWorkspace<LiftedTag>::Iteration::clear() noexcept { head_updates.clear(); }

RuleWorkspace<LiftedTag>::Solve::Solve() :
#ifndef NDEBUG
    seen_bindings(),
#endif
    applicability_cache(),
    statistics()
{
}

void RuleWorkspace<LiftedTag>::Solve::clear() noexcept
{
#ifndef NDEBUG
    seen_bindings.clear();
#endif
    applicability_cache.clear();
}

RuleWorkspace<LiftedTag>::Worker::Worker(const ConstRuleWorkspace<LiftedTag>& cws, const Common& common) : builder(), binding(), iteration(cws, common), solve()
{
}

void RuleWorkspace<LiftedTag>::Worker::clear() noexcept
{
    iteration.clear();
    solve.clear();
}

RuleWorkspace<LiftedTag>::RuleWorkspace(const ConstRuleWorkspace<LiftedTag>& cws_) : common(cws_.get_static_consistency_graph()), worker()
{
    worker.emplace_back(cws_, common);

#if defined(TYR_ENABLE_INNER_PARALLELISM) && defined(TYR_ENABLE_SEMI_NAIVE)
    // Only propositional heads use partitionable delta KCKP; numeric effects require full KCKP enumeration.
    if (supports_inner_parallelism() && cws_.get_rule().get_arity() > 2)
        worker.emplace_back(cws_, common);
#endif
}

void RuleWorkspace<LiftedTag>::clear() noexcept
{
    common.clear();
    for (auto& w : worker)
        w.clear();
}

}
