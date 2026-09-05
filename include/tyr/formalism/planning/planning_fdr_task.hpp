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

#ifndef TYR_FORMALISM_PLANNING_PLANNING_FDR_TASK_HPP_
#define TYR_FORMALISM_PLANNING_PLANNING_FDR_TASK_HPP_

#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/fdr_context.hpp"
#include "tyr/formalism/planning/fdr_task_view.hpp"
#include "tyr/formalism/planning/planning_domain.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <filesystem>
#include <optional>

namespace tyr::formalism::planning
{

class PlanningFDRTask
{
public:
    PlanningFDRTask(FDRTaskView task,
                    FDRContextPtr fdr_context,
                    std::shared_ptr<Repository> repository,
                    PlanningDomain domain,
                    std::optional<std::filesystem::path> path = std::nullopt) :
        m_repository(std::move(repository)),
        m_task(task),
        m_fdr_context(std::move(fdr_context)),
        m_domain(std::move(domain)),
        m_path(std::move(path))
    {
        if (&m_task.get_context() != m_repository.get())
            throw std::invalid_argument("Task context does not match the given Repository.");
    }

    const auto& get_repository() const noexcept { return m_repository; }
    auto get_task() const noexcept { return m_task; }
    const auto& get_fdr_context() const noexcept { return m_fdr_context; }
    const auto& get_domain() const noexcept { return m_domain; }
    const auto& get_path() const noexcept { return m_path; }

private:
    std::shared_ptr<Repository> m_repository;
    FDRTaskView m_task;
    FDRContextPtr m_fdr_context;
    PlanningDomain m_domain;
    std::optional<std::filesystem::path> m_path;
};

}

#endif
