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
#include "tyr/formalism/atom_view.hpp"
#include "tyr/formalism/binary_operator_view.hpp"
#include "tyr/formalism/conjunctive_condition_view.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/function_term_value_view.hpp"
#include "tyr/formalism/function_term_view.hpp"
#include "tyr/formalism/function_view.hpp"
#include "tyr/formalism/literal_view.hpp"
#include "tyr/formalism/multi_operator_view.hpp"
#include "tyr/formalism/numeric_effect_view.hpp"
#include "tyr/formalism/object_view.hpp"
#include "tyr/formalism/parameter_index.hpp"
#include "tyr/formalism/planning/action_view.hpp"
#include "tyr/formalism/planning/axiom_view.hpp"
#include "tyr/formalism/planning/conditional_effect_view.hpp"
#include "tyr/formalism/planning/conjunctive_effect_view.hpp"
#include "tyr/formalism/planning/domain_view.hpp"
#include "tyr/formalism/planning/fdr_fact_view.hpp"
#include "tyr/formalism/planning/fdr_variable_view.hpp"
#include "tyr/formalism/planning/invariant_view.hpp"
#include "tyr/formalism/planning/metric_view.hpp"
#include "tyr/formalism/planning/task_view.hpp"
#include "tyr/formalism/predicate_view.hpp"
#include "tyr/formalism/repository.hpp"
#include "tyr/formalism/unary_operator_view.hpp"
#include "tyr/formalism/variable_view.hpp"
#include "tyr/planning/programs/axiom.hpp"
#include "tyr/planning/tasks/task_utils.hpp"
#include "tyr/datalog/ground/contexts/program.hpp"
#include "tyr/datalog/ground/workspaces/program.hpp"
#include "tyr/datalog/solver.hpp"
#include "tyr/planning/successors/applicability.hpp"
#include "tyr/planning/successors/ground/match_tree/match_tree.hpp"
#include "tyr/planning/tasks/ground/task.hpp"
#include "tyr/planning/states/state_builder.hpp"
#include "tyr/planning/states/state_repository.hpp"
#include "yggdrasil/formalism/row_view.hpp"

#include <atomic>
#include <memory>
#include <urd/elements/less.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <utility>
#include <vector>
#include <yggdrasil/core/config.hpp>

namespace tyr::planning
{

namespace detail
{
template<>
struct AxiomDefinition<GroundTag>
{
    explicit AxiomDefinition(TaskPtr<GroundTag> task_) : task(std::move(task_)), program()
    {
        if (task->has_axioms())
            program = std::make_unique<AxiomEvaluatorProgram<GroundTag>>(task->get_formalism_task());
    }

    TaskPtr<GroundTag> task;
    std::unique_ptr<AxiomEvaluatorProgram<GroundTag>> program;
};

template<>
struct AxiomWorkspace<GroundTag>
{
    AxiomWorkspace(const AxiomDefinition<GroundTag>& definition, ygg::ExecutionContextPtr execution_context_) :
        execution_context(std::move(execution_context_)),
        workspace(definition.program ? std::make_unique<datalog::ProgramWorkspace<GroundTag>>(definition.program->get_datalog_program()) : nullptr)
    {
    }

    ygg::ExecutionContextPtr execution_context;
    std::unique_ptr<datalog::ProgramWorkspace<GroundTag>> workspace;
};
}

template<>
void AxiomEvaluator<GroundTag>::compute_extended_state(urd::builder<State<GroundTag>, StateRepository<GroundTag>>& state_builder,
                                                       const StateRepository<GroundTag>& repository)
{
    auto& evaluator = m_impl->evaluator;
    if (!evaluator.workspace)
        return;

    auto& workspace = *evaluator.workspace;
    const auto& task_repository = *m_impl->definition->task->get_repository();
    workspace.facts.reset();
    insert_unextended_state(StateContext<GroundTag> { repository, state_builder, ygg::float_t { 0 } }, task_repository, workspace);

    auto ctx = datalog::ProgramExecutionContext(workspace);
    datalog::execute_model(ctx, *evaluator.execution_context);

    read_derived_atoms_from_fact_set(state_builder, task_repository, workspace);
}

template class AxiomEvaluator<GroundTag>;

}
