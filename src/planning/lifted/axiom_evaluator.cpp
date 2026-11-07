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

#include "../axiom_evaluator_impl.hpp"
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>

#include "tyr/datalog/fact_sets.hpp"
#include "tyr/datalog/formatter.hpp"
#include "tyr/datalog/lifted/contexts/program.hpp"
#include "tyr/datalog/lifted/workspaces/program.hpp"
#include "tyr/datalog/policies/annotation.hpp"
#include "tyr/datalog/policies/termination.hpp"
#include "tyr/datalog/solver.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/planning/action_view.hpp"
#include "tyr/formalism/atom_view.hpp"
#include "tyr/formalism/planning/axiom_view.hpp"
#include "tyr/formalism/binary_operator_view.hpp"
#include "tyr/formalism/planning/conditional_effect_view.hpp"
#include "tyr/formalism/planning/conjunctive_condition_view.hpp"
#include "tyr/formalism/planning/conjunctive_effect_view.hpp"
#include "tyr/formalism/planning/domain_view.hpp"
#include "tyr/formalism/planning/fdr_fact_view.hpp"
#include "tyr/formalism/planning/fdr_variable_view.hpp"
#include "tyr/formalism/function_term_value_view.hpp"
#include "tyr/formalism/function_term_view.hpp"
#include "tyr/formalism/planning/invariant_view.hpp"
#include "tyr/formalism/literal_view.hpp"
#include "tyr/formalism/planning/metric_view.hpp"
#include "tyr/formalism/multi_operator_view.hpp"
#include "tyr/formalism/numeric_effect_view.hpp"
#include "tyr/formalism/planning/task_view.hpp"
#include "tyr/formalism/unary_operator_view.hpp"
#include "tyr/formalism/declarations.hpp"
#include "yggdrasil/formalism/binding_view.hpp"
#include "tyr/formalism/function_view.hpp"
#include "tyr/formalism/object_view.hpp"
#include "tyr/formalism/parameter_index.hpp"
#include "tyr/formalism/predicate_view.hpp"
#include "tyr/formalism/variable_view.hpp"
#include "tyr/planning/lifted/programs/axiom.hpp"
#include "tyr/planning/lifted/task.hpp"
#include "tyr/planning/state_builder.hpp"
#include "tyr/planning/task_utils.hpp"

#include <algorithm>
#include <atomic>
#include <cista/containers/hash_storage.h>
#include <fmt/ostream.h>
#include <gtl/phmap.hpp>
#include <iostream>
#include <memory>
#include <urd/elements/equal_to.hpp>
#include <urd/elements/hash.hpp>
#include <urd/elements/less.hpp>
#include <utility>
#include <vector>
#include <urd/elements/flat.hpp>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/execution/onetbb.hpp>
#include <yggdrasil/formatting/formatter.hpp>

namespace tyr::planning
{
namespace detail
{
template<>
struct AxiomDefinition<LiftedTag>
{
    explicit AxiomDefinition(TaskPtr<LiftedTag> task_) : task(std::move(task_)), program()
    {
        if (task->has_axioms())
            program = std::make_unique<AxiomEvaluatorProgram>(task->get_task());
    }

    TaskPtr<LiftedTag> task;
    std::unique_ptr<AxiomEvaluatorProgram> program;
};

template<>
struct AxiomWorkspace<LiftedTag>
{
    AxiomWorkspace(const AxiomDefinition<LiftedTag>& definition, ygg::ExecutionContextPtr execution_context_) :
        execution_context(std::move(execution_context_)),
        workspace(definition.program ? std::make_unique<datalog::ProgramWorkspace<LiftedTag>>(definition.program->get_datalog_program()) : nullptr),
        derived_bindings()
    {
    }

    ygg::ExecutionContextPtr execution_context;
    std::unique_ptr<datalog::ProgramWorkspace<LiftedTag>> workspace;
    urd::Store<urd::sequence<formalism::datalog::PredicateBindingView<formalism::FluentTag>>> derived_bindings;
};
}

template<>
void AxiomEvaluator<LiftedTag>::compute_extended_state(urd::builder<State<LiftedTag>>& state_builder)
{
    auto& evaluator = m_impl->evaluator;
    if (!evaluator.workspace)
        return;

    auto& workspace = *evaluator.workspace;
    workspace.reset_evaluation();

    const auto& program = *m_impl->definition->program;

    insert_unextended_state(state_builder, *m_impl->definition->task->get_repository(), program.get_translation_context().p2d, workspace);

    auto ctx = datalog::ProgramExecutionContext<LiftedTag>::from_reset_workspace(workspace);
    datalog::execute_model(ctx, *evaluator.execution_context);

    read_derived_atoms_from_fact_set(state_builder,
                                     *m_impl->definition->task->get_repository(),
                                     program.get_translation_context().d2p,
                                     workspace,
                                     evaluator.derived_bindings);
}

template<>
void AxiomEvaluator<LiftedTag>::print_summary(size_t verbosity) const
{
    if (verbosity < 1 || !m_impl->evaluator.workspace)
        return;

    const auto& workspace = *m_impl->evaluator.workspace;
    std::cout << "[Axiom evaluator] Summary" << '\n';
    fmt::print(std::cout, "{}\n", workspace.statistics);
    datalog::print_rule_statistics(workspace.rules);
}

template class AxiomEvaluator<LiftedTag>;

}
