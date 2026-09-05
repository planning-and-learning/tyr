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

#include "tyr/formalism/planning/planning_task.hpp"

#include "tyr/analysis/domains.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/fdr_context.hpp"
#include "tyr/formalism/planning/planning_domain.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/planning/task_view.hpp"

namespace tyr::formalism::planning
{
PlanningTask::PlanningTask(TaskView task,
                           FDRContextPtr fdr_context,
                           std::shared_ptr<Repository> repository,
                           PlanningDomain domain,
                           std::optional<std::filesystem::path> path) :
    m_repository(std::move(repository)),
    m_task(task),
    m_fdr_context(std::move(fdr_context)),
    m_domain(std::move(domain)),
    m_path(std::move(path)),
    m_variable_domains(analysis::compute_variable_domains(m_task)),
    m_variable_domains_view(analysis::compute_variable_domain_views(m_variable_domains, *m_repository))
{
    if (&m_task.get_context() != m_repository.get())
        throw std::invalid_argument("Task context does not match the given Repository.");
}

}
