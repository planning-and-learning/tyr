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

#ifndef TYR_PLANNING_GROUND_TASK_HPP_
#define TYR_PLANNING_GROUND_TASK_HPP_

#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/fdr_context.hpp"
#include "tyr/formalism/planning/planning_task.hpp"
#include "tyr/formalism/planning/views.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/static_state.hpp"
#include "tyr/planning/task.hpp"

namespace tyr::planning
{

template<>
class Task<GroundTag>
{
public:
    explicit Task(formalism::planning::PlanningTask<GroundTag> task);

    const auto& get_formalism_task() const noexcept { return m_task; }
    const auto& get_domain() const noexcept { return m_task.get_domain(); }
    auto get_task() const noexcept { return m_task.get_task(); }
    const auto& get_fdr_context() const noexcept { return m_task.get_fdr_context(); }
    const auto& get_repository() const noexcept { return m_task.get_repository(); }
    const StaticState& get_static_state() const noexcept { return m_static_state; }
    bool has_axioms() const noexcept { return !get_task().get_ground_axioms().empty(); }

private:
    formalism::planning::PlanningTask<GroundTag> m_task;

    StaticState m_static_state;
};

}

#endif
