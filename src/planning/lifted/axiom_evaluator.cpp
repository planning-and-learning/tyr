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

#include "tyr/planning/lifted/axiom_evaluator.hpp"

#include "tyr/datalog/fact_sets.hpp"  // for FactSets, Pre...
#include "tyr/datalog/formatter.hpp"
#include "tyr/datalog/lifted/bottom_up.hpp"  // for solve_bottom_up
#include "tyr/datalog/lifted/contexts/program.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/merge_datalog.hpp"   // for MergeContext
#include "tyr/formalism/planning/merge_planning.hpp"  // for MergeContext
#include "tyr/formalism/planning/repository.hpp"      // for Repository
#include "tyr/formalism/planning/views.hpp"
#include "tyr/planning/lifted/state_builder.hpp"
#include "tyr/planning/lifted/task.hpp"  // for LiftedTag
#include "tyr/planning/lifted/task.hpp"
#include "tyr/planning/task_utils.hpp"  // for insert_fact_s...

#include <algorithm>
#include <cista/containers/hash_storage.h>  // for operator!=
#include <fmt/ostream.h>
#include <gtl/phmap.hpp>                    // for operator!=
#include <utility>                          // for pair
#include <yggdrasil/containers/vector.hpp>  // for ygg::View
#include <yggdrasil/formatting/formatter.hpp>
#include <yggdrasil/semantics/comparators.hpp>  // for operator!=
#include <yggdrasil/semantics/equal_to.hpp>     // for EqualTo
#include <yggdrasil/semantics/hash.hpp>         // for Hash

namespace d = tyr::datalog;
namespace f = tyr::formalism;
namespace fd = tyr::formalism::datalog;
namespace fp = tyr::formalism::planning;

namespace tyr::planning
{
namespace
{
void read_derived_atoms_from_datalog_program(const AxiomEvaluatorProgram<LiftedTag>& axiom_program,
                                             ygg::Builder<State<LiftedTag>>& state_builder,
                                             fp::MergePlanningContext& merge_context,
                                             d::TaggedFactSets<f::FluentTag>& fact_sets,
                                             std::vector<fd::PredicateBindingView<f::FluentTag>>& derived_bindings)
{
    for (const auto& set : fact_sets.predicate.get_sets())
        for (const auto& binding : set.get_bindings())
            if (axiom_program.get_translation_context().d2p.fluent_to_derived_predicate.contains(binding.get_relation()))
                derived_bindings.push_back(binding);

    std::sort(derived_bindings.begin(), derived_bindings.end(), d::canonical_binding_less<fd::PredicateBindingView<f::FluentTag>>);

    for (const auto binding : derived_bindings)
    {
        const auto ground_atom =
            fp::merge_atom_d2p<f::FluentTag, f::DerivedTag>(binding, axiom_program.get_translation_context().d2p.fluent_to_derived_predicate, merge_context)
                .first.get_index();

        state_builder.set(ground_atom);
    }
}
}

AxiomEvaluator<LiftedTag>::AxiomEvaluator(ygg::uint_t index, TaskPtr<LiftedTag> task, ygg::ExecutionContextPtr execution_context) :
    m_index(index),
    m_task(std::move(task)),
    m_execution_context(std::move(execution_context)),
    m_axiom_program(m_task->get_task()),
    m_workspace(m_axiom_program.get_datalog_program())
{
}

void AxiomEvaluator<LiftedTag>::compute_extended_state(ygg::Builder<State<LiftedTag>>& state_builder)
{
    m_derived_bindings.clear();
    m_workspace.reset_evaluation();

    auto merge_datalog_context = fp::MergeDatalogContext { m_workspace.datalog_builder, m_workspace.workspace_repository };
    const auto& program = m_axiom_program;

    insert_unextended_state(state_builder,
                            *m_task->get_repository(),
                            program.get_translation_context().p2d,
                            merge_datalog_context,
                            m_workspace.facts.fact_sets,
                            m_workspace.facts.assignment_sets);

    auto ctx = d::ProgramExecutionContext(m_workspace);

    m_execution_context->arena().execute([&] { d::solve_bottom_up(ctx); });

    auto merge_planning_context = fp::MergePlanningContext { m_workspace.planning_builder, *m_task->get_repository() };

    read_derived_atoms_from_datalog_program(program, state_builder, merge_planning_context, m_workspace.facts.fact_sets, m_derived_bindings);
}

void AxiomEvaluator<LiftedTag>::print_summary(size_t verbosity) const
{
    if (verbosity < 1)
        return;

    std::cout << "[Axiom evaluator] Summary" << std::endl;
    fmt::print(std::cout, "{}\n", m_workspace.statistics);
    auto axiom_evaluator_rule_statistics = std::vector<datalog::RuleStatistics> {};
    for (const auto& ws_rule : m_workspace.template get_rules<f::PredicateTag>())
        axiom_evaluator_rule_statistics.push_back(ws_rule->common.statistics);
    fmt::print(std::cout, "{}\n", datalog::compute_aggregated_rule_statistics(axiom_evaluator_rule_statistics));
    auto axiom_evaluator_rule_worker_statistics = std::vector<datalog::RuleWorkerStatistics> {};
    for (const auto& ws_rule : m_workspace.template get_rules<f::PredicateTag>())
        for (const auto& worker : ws_rule->worker)
            axiom_evaluator_rule_worker_statistics.push_back(worker.solve.statistics);
    fmt::print(std::cout, "{}\n", datalog::compute_aggregated_rule_worker_statistics(axiom_evaluator_rule_worker_statistics));
}

static_assert(AxiomEvaluatorConcept<AxiomEvaluator<LiftedTag>, LiftedTag>);

}
