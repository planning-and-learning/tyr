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
#include "tyr/analysis/domains_view.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/fdr_context.hpp"
#include "tyr/formalism/planning/planning_domain.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/planning/task_view.hpp"

namespace tyr::formalism::planning
{
template<TaskKind T>
struct PlanningTask<T>::VariableDomains
{
    analysis::TaskVariableDomains domains;
    analysis::TaskVariableDomainsView view;
};

namespace
{
std::monostate make_variable_domains(TaskView<GroundTag>, const Repository&) { return {}; }

std::shared_ptr<const PlanningTask<LiftedTag>::VariableDomains> make_variable_domains(TaskView<LiftedTag> task, const Repository& repository)
{
    using VariableDomains = PlanningTask<LiftedTag>::VariableDomains;
    auto domains = analysis::compute_variable_domains(task);
    auto view = analysis::compute_variable_domain_views(domains, repository);
    return std::make_shared<const VariableDomains>(VariableDomains { std::move(domains), std::move(view) });
}
}

template<TaskKind T>
PlanningTask<T>::PlanningTask(TaskView<T> task,
                              std::shared_ptr<const FDRContext> fdr_context,
                              std::shared_ptr<const Repository> repository,
                              PlanningDomain domain,
                              std::optional<std::filesystem::path> path) :
    m_repository(std::move(repository)),
    m_task(task),
    m_fdr_context(std::move(fdr_context)),
    m_domain(std::move(domain)),
    m_path(std::move(path)),
    m_variable_domains(make_variable_domains(m_task, *m_repository))
{
    if (&m_task.context() != m_repository.get())
        throw std::invalid_argument("Task context does not match the given Repository.");
}

template<TaskKind T>
const analysis::TaskVariableDomains& PlanningTask<T>::get_variable_domains() const noexcept
    requires std::same_as<T, LiftedTag>
{ return m_variable_domains->domains; }

template<TaskKind T>
const analysis::TaskVariableDomainsView& PlanningTask<T>::get_variable_domains_view() const noexcept
    requires std::same_as<T, LiftedTag>
{ return m_variable_domains->view; }

template class PlanningTask<LiftedTag>;
template class PlanningTask<GroundTag>;

}
