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

#include "tyr/planning/tasks/ground/task.hpp"

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
#include "tyr/formalism/planning/fdr_context.hpp"
#include "tyr/formalism/planning/fdr_fact_view.hpp"
#include "tyr/formalism/planning/fdr_variable_view.hpp"
#include "tyr/formalism/planning/invariant_view.hpp"
#include "tyr/formalism/planning/metric_view.hpp"
#include "tyr/formalism/planning/task_view.hpp"
#include "tyr/formalism/predicate_view.hpp"
#include "tyr/formalism/repository.hpp"
#include "tyr/formalism/unary_operator_view.hpp"
#include "tyr/formalism/variable_view.hpp"
#include "tyr/planning/tasks/lifted/task.hpp"
#include "tyr/planning/tasks/lifted/task_grounder.hpp"

#include <memory>
#include <utility>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/formalism/row_view.hpp>

namespace tyr::planning
{
StaticState::StaticState(formalism::AtomListView<GroundTag, formalism::StaticTag> atoms,
                         formalism::FunctionTermValueListView<GroundTag, formalism::StaticTag> fterm_values)
{
    for (const auto atom : atoms)
        m_atoms.insert(atom.index());

    for (const auto fterm_value : fterm_values)
        m_numeric_variables.insert_or_assign(fterm_value.fterm().index(), fterm_value.value());
}

template<TaskKind Kind>
StaticState::StaticState(formalism::TaskView<Kind> task) :
    StaticState(task.template atoms<formalism::StaticTag>(), task.template fterm_values<formalism::StaticTag>())
{
}

template StaticState::StaticState(formalism::TaskView<GroundTag> task);
template StaticState::StaticState(formalism::TaskView<LiftedTag> task);

namespace
{
/// A writable layer over a formalism task's frozen repository. The layers do not own their parents: the Task does,
/// through m_task, which it declares first and so destroys last.
template<TaskKind Kind>
formalism::RepositoryPtr layer_over(const formalism::PlanningTask<Kind>& task)
{ return std::make_shared<formalism::Repository>(task.get_repository().get()); }
}

Task<LiftedTag>::Task(formalism::PlanningTask<LiftedTag> task) :
    m_task(std::move(task)),
    m_repository(layer_over(m_task)),
    m_fdr_context(std::make_shared<formalism::FDRContext>(m_task.get_fdr_context().get(), m_repository)),
    m_static_state(m_task.get_task())
{
}

TaskPtr<LiftedTag> Task<LiftedTag>::create(formalism::PlanningTask<LiftedTag> task) { return std::make_shared<Task<LiftedTag>>(std::move(task)); }

GroundTaskInstantiationResult Task<LiftedTag>::instantiate_ground_task(ygg::ExecutionContext& execution_context, const GroundTaskInstantiationOptions& options)
{ return tyr::planning::instantiate_ground_task(*this, execution_context, options); }

// Defined here, not in the header: reading the task's records instantiates the repository's reads, which every source
// including the header would compile again.
bool Task<LiftedTag>::has_axioms() const noexcept { return !get_task().axioms().empty() || !get_domain().get_domain().axioms().empty(); }

Task<GroundTag>::Task(formalism::PlanningTask<GroundTag> task) :
    m_task(std::move(task)),
    m_repository(layer_over(m_task)),
    m_fdr_context(std::make_shared<formalism::FDRContext>(m_task.get_fdr_context().get(), m_repository)),
    m_static_state(m_task.get_task())
{
}

bool Task<GroundTag>::has_axioms() const noexcept { return !get_task().ground_axioms().empty(); }

}
