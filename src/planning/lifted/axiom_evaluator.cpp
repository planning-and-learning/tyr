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

#include "tyr/datalog/fact_sets.hpp"
#include "tyr/datalog/formatter.hpp"
#include "tyr/datalog/lifted/bottom_up.hpp"
#include "tyr/datalog/lifted/contexts/program.hpp"
#include "tyr/datalog/lifted/policies/annotation.hpp"
#include "tyr/datalog/lifted/workspaces/program.hpp"
#include "tyr/datalog/policies/termination.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/merge_datalog.hpp"
#include "tyr/formalism/planning/merge_planning.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/planning/views.hpp"
#include "tyr/planning/lifted/programs/axiom.hpp"
#include "tyr/planning/lifted/state_builder.hpp"
#include "tyr/planning/lifted/task.hpp"
#include "tyr/planning/task_utils.hpp"

#include <algorithm>
#include <atomic>
#include <cista/containers/hash_storage.h>
#include <fmt/ostream.h>
#include <gtl/phmap.hpp>
#include <iostream>
#include <memory>
#include <utility>
#include <vector>
#include <yggdrasil/containers/vector.hpp>
#include <yggdrasil/execution/onetbb.hpp>
#include <yggdrasil/formatting/formatter.hpp>
#include <yggdrasil/semantics/comparators.hpp>
#include <yggdrasil/semantics/equal_to.hpp>
#include <yggdrasil/semantics/hash.hpp>

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

    for (const auto binding : derived_bindings)
    {
        const auto ground_atom =
            fp::merge_atom_d2p<f::FluentTag, f::DerivedTag>(binding, axiom_program.get_translation_context().d2p.fluent_to_derived_predicate, merge_context)
                .first.get_index();

        state_builder.set(ground_atom);
    }
}
}

struct AxiomEvaluator<LiftedTag>::Impl
{
    using Program = AxiomEvaluatorProgram<LiftedTag>;

    struct Definition
    {
        explicit Definition(TaskPtr<LiftedTag> task_) : task(std::move(task_)), program(task->get_task()) {}

        TaskPtr<LiftedTag> task;
        Program program;
    };

    struct Evaluator
    {
        Evaluator(const Definition& definition, ygg::ExecutionContextPtr execution_context_) :
            execution_context(std::move(execution_context_)),
            workspace(definition.program.get_datalog_program()),
            derived_bindings()
        {
        }

        ygg::ExecutionContextPtr execution_context;
        datalog::ProgramWorkspace<LiftedTag> workspace;
        std::vector<::tyr::formalism::datalog::PredicateBindingView<::tyr::formalism::FluentTag>> derived_bindings;
    };

    Impl(ygg::uint_t index_, TaskPtr<LiftedTag> task, ygg::ExecutionContextPtr execution_context, std::shared_ptr<std::atomic<ygg::uint_t>> next_index_) :
        index(index_),
        next_index(std::move(next_index_)),
        definition(std::make_shared<Definition>(std::move(task))),
        evaluator(*definition, std::move(execution_context))
    {
    }

    Impl(ygg::uint_t index_,
         std::shared_ptr<const Definition> definition_,
         ygg::ExecutionContextPtr execution_context,
         std::shared_ptr<std::atomic<ygg::uint_t>> next_index_) :
        index(index_),
        next_index(std::move(next_index_)),
        definition(std::move(definition_)),
        evaluator(*definition, std::move(execution_context))
    {
    }

    ygg::uint_t index;
    std::shared_ptr<std::atomic<ygg::uint_t>> next_index;
    std::shared_ptr<const Definition> definition;
    Evaluator evaluator;
};

AxiomEvaluator<LiftedTag>::~AxiomEvaluator() = default;

AxiomEvaluator<LiftedTag>::AxiomEvaluator(ygg::uint_t index,
                                          TaskPtr<LiftedTag> task,
                                          ygg::ExecutionContextPtr execution_context,
                                          std::shared_ptr<std::atomic<ygg::uint_t>> next_index) :
    m_impl(std::make_unique<Impl>(index, std::move(task), std::move(execution_context), std::move(next_index)))
{
}

AxiomEvaluator<LiftedTag>::AxiomEvaluator(std::unique_ptr<Impl> impl) : m_impl(std::move(impl)) {}

AxiomEvaluatorPtr<LiftedTag> AxiomEvaluator<LiftedTag>::make_worker(ygg::ExecutionContextPtr execution_context) const
{
    return AxiomEvaluatorPtr<LiftedTag>(new AxiomEvaluator<LiftedTag>(std::make_unique<Impl>(m_impl->next_index->fetch_add(1, std::memory_order_relaxed),
                                                                                             m_impl->definition,
                                                                                             std::move(execution_context),
                                                                                             m_impl->next_index)));
}

void AxiomEvaluator<LiftedTag>::compute_extended_state(ygg::Builder<State<LiftedTag>>& state_builder)
{
    auto& evaluator = m_impl->evaluator;
    evaluator.derived_bindings.clear();
    evaluator.workspace.reset_evaluation();

    auto merge_datalog_context = fp::MergeDatalogContext { evaluator.workspace.datalog_builder, evaluator.workspace.workspace_repository };
    const auto& program = m_impl->definition->program;

    insert_unextended_state(state_builder,
                            *m_impl->definition->task->get_repository(),
                            program.get_translation_context().p2d,
                            merge_datalog_context,
                            evaluator.workspace.facts.fact_sets,
                            evaluator.workspace.facts.assignment_sets);

    auto ctx = d::ProgramExecutionContext(evaluator.workspace);

    evaluator.execution_context->arena().execute([&] { d::solve_bottom_up(ctx); });

    auto merge_planning_context = fp::MergePlanningContext { evaluator.workspace.planning_builder, *m_impl->definition->task->get_repository() };

    read_derived_atoms_from_datalog_program(program, state_builder, merge_planning_context, evaluator.workspace.facts.fact_sets, evaluator.derived_bindings);
}

void AxiomEvaluator<LiftedTag>::print_summary(size_t verbosity) const
{
    if (verbosity < 1)
        return;

    std::cout << "[Axiom evaluator] Summary" << std::endl;
    fmt::print(std::cout, "{}\n", m_impl->evaluator.workspace.statistics);
    auto axiom_evaluator_rule_statistics = std::vector<datalog::RuleStatistics> {};
    for (const auto& ws_rule : m_impl->evaluator.workspace.template get_rules<f::PredicateTag>())
        axiom_evaluator_rule_statistics.push_back(ws_rule->common.statistics);
    fmt::print(std::cout, "{}\n", datalog::compute_aggregated_rule_statistics(axiom_evaluator_rule_statistics));
    auto axiom_evaluator_rule_worker_statistics = std::vector<datalog::RuleWorkerStatistics> {};
    for (const auto& ws_rule : m_impl->evaluator.workspace.template get_rules<f::PredicateTag>())
        for (const auto& worker : ws_rule->worker)
            axiom_evaluator_rule_worker_statistics.push_back(worker.solve.statistics);
    fmt::print(std::cout, "{}\n", datalog::compute_aggregated_rule_worker_statistics(axiom_evaluator_rule_worker_statistics));
}

const AxiomEvaluatorProgram<LiftedTag>& AxiomEvaluator<LiftedTag>::get_axiom_program() const noexcept { return m_impl->definition->program; }

const ygg::ExecutionContextPtr& AxiomEvaluator<LiftedTag>::get_execution_context() const noexcept { return m_impl->evaluator.execution_context; }

ygg::uint_t AxiomEvaluator<LiftedTag>::get_index() const noexcept { return m_impl->index; }

static_assert(AxiomEvaluatorConcept<AxiomEvaluator<LiftedTag>, LiftedTag>);

}
