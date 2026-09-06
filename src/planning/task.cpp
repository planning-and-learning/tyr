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

#include "tyr/formalism/planning/fdr_context.hpp"
#include "tyr/formalism/planning/formatter.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/planning/views.hpp"
#include "tyr/planning/lifted/task_grounder.hpp"
#include "tyr/planning/task_utils.hpp"

#include <tuple>
#include <utility>
#include <yggdrasil/containers/dynamic_bitset.hpp>
#include <yggdrasil/containers/vector.hpp>
#include <yggdrasil/semantics/comparators.hpp>

namespace f = tyr::formalism;

namespace tyr::planning
{
Task<LiftedTag>::Task(formalism::planning::PlanningTask task) : m_task(std::move(task)), m_static_atoms_bitset(), m_static_numeric_variables()
{
    for (const auto atom : get_task().template get_atoms<f::StaticTag>())
        ygg::set(ygg::uint_t(atom.get_index()), true, m_static_atoms_bitset);

    for (const auto fterm_value : get_task().template get_fterm_values<f::StaticTag>())
        ygg::set(ygg::uint_t(fterm_value.get_fterm().get_index()),
                 fterm_value.get_value(),
                 m_static_numeric_variables,
                 std::numeric_limits<ygg::float_t>::quiet_NaN());
}

TaskPtr<LiftedTag> Task<LiftedTag>::create(formalism::planning::PlanningTask task) { return std::make_shared<Task<LiftedTag>>(std::move(task)); }

GroundTaskInstantiationResult Task<LiftedTag>::instantiate_ground_task(ygg::ExecutionContext& execution_context, const GroundTaskInstantiationOptions& options)
{
    return tyr::planning::instantiate_ground_task(*this, execution_context, options);
}

Task<GroundTag>::Task(formalism::planning::PlanningFDRTask task) : m_task(std::move(task)), m_static_atoms_bitset(), m_static_numeric_variables()
{
    for (const auto atom : get_task().template get_atoms<f::StaticTag>())
        ygg::set(ygg::uint_t(atom.get_index()), true, m_static_atoms_bitset);

    for (const auto fterm_value : get_task().template get_fterm_values<f::StaticTag>())
        ygg::set(ygg::uint_t(fterm_value.get_fterm().get_index()),
                 fterm_value.get_value(),
                 m_static_numeric_variables,
                 std::numeric_limits<ygg::float_t>::quiet_NaN());
}

template<f::FactKind F>
size_t Task<GroundTag>::get_num_atoms() const noexcept
{
    return get_task().template get_atoms<F>().size();
}

template size_t Task<GroundTag>::get_num_atoms<f::FluentTag>() const noexcept;
template size_t Task<GroundTag>::get_num_atoms<f::DerivedTag>() const noexcept;

size_t Task<GroundTag>::get_num_actions() const noexcept { return get_task().get_ground_actions().size(); }

size_t Task<GroundTag>::get_num_axioms() const noexcept { return get_task().get_ground_axioms().size(); }

}
