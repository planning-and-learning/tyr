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

#include "tyr/planning/ground/task.hpp"
#include "tyr/planning/lifted/task.hpp"

#include "tyr/formalism/planning/views.hpp"
#include "tyr/planning/lifted/task_grounder.hpp"

#include <utility>
#include <yggdrasil/containers/dynamic_bitset.hpp>
#include <yggdrasil/containers/vector.hpp>

namespace tyr::planning
{
StaticState::StaticState(formalism::planning::AtomListView<GroundTag, formalism::StaticTag> atoms,
                         formalism::planning::FunctionTermValueListView<GroundTag, formalism::StaticTag> fterm_values)
{
    for (const auto atom : atoms)
        ygg::set(ygg::uint_t(atom.get_index()), true, m_atoms);

    for (const auto fterm_value : fterm_values)
        ygg::set(ygg::uint_t(fterm_value.get_fterm().get_index()),
                 fterm_value.get_value(),
                 m_numeric_variables,
                 std::numeric_limits<ygg::float_t>::quiet_NaN());
}

template<TaskKind Kind>
StaticState::StaticState(formalism::planning::TaskView<Kind> task) :
    StaticState(task.template get_atoms<formalism::StaticTag>(), task.template get_fterm_values<formalism::StaticTag>())
{
}

template StaticState::StaticState(formalism::planning::TaskView<GroundTag> task);
template StaticState::StaticState(formalism::planning::TaskView<LiftedTag> task);

Task<LiftedTag>::Task(formalism::planning::PlanningTask<LiftedTag> task) : m_task(std::move(task)), m_static_state(m_task.get_task()) {}

TaskPtr<LiftedTag> Task<LiftedTag>::create(formalism::planning::PlanningTask<LiftedTag> task) { return std::make_shared<Task<LiftedTag>>(std::move(task)); }

GroundTaskInstantiationResult Task<LiftedTag>::instantiate_ground_task(ygg::ExecutionContext& execution_context, const GroundTaskInstantiationOptions& options)
{
    return tyr::planning::instantiate_ground_task(*this, execution_context, options);
}

Task<GroundTag>::Task(formalism::planning::PlanningTask<GroundTag> task) : m_task(std::move(task)), m_static_state(m_task.get_task()) {}

}
