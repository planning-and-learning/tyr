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

#ifndef TYR_FORMALISM_PLANNING_PLANNING_TASK_HPP_
#define TYR_FORMALISM_PLANNING_PLANNING_TASK_HPP_

#include "tyr/analysis/declarations.hpp"
#include "tyr/analysis/domains_view.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/fdr_context.hpp"
#include "tyr/formalism/planning/planning_domain.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/planning/task_view.hpp"

#include <filesystem>
#include <optional>
#include <type_traits>
#include <variant>

namespace tyr::formalism::planning
{

template<TaskKind T>
class PlanningTask
{
public:
    PlanningTask(TaskView<T> task,
                 FDRContextPtr fdr_context,
                 std::shared_ptr<Repository> repository,
                 PlanningDomain domain,
                 std::optional<std::filesystem::path> path = std::nullopt);

    [[nodiscard]] const auto& get_repository() const noexcept { return m_repository; }
    [[nodiscard]] auto get_task() const noexcept { return m_task; }
    [[nodiscard]] const auto& get_fdr_context() const noexcept { return m_fdr_context; }
    [[nodiscard]] const auto& get_domain() const noexcept { return m_domain; }
    [[nodiscard]] const auto& get_path() const noexcept { return m_path; }
    [[nodiscard]] const auto& get_variable_domains() const noexcept
        requires std::same_as<T, LiftedTag>
    { return m_variable_domains.domains; }
    [[nodiscard]] const auto& get_variable_domains_view() const noexcept
        requires std::same_as<T, LiftedTag>
    { return m_variable_domains.view; }

    /// The variable domains of a lifted task; a ground task has none.
    struct VariableDomains
    {
        analysis::TaskVariableDomains domains;
        analysis::TaskVariableDomainsView view;
    };

private:
    std::shared_ptr<Repository> m_repository;
    TaskView<T> m_task;
    FDRContextPtr m_fdr_context;
    PlanningDomain m_domain;
    std::optional<std::filesystem::path> m_path;
    [[no_unique_address]] std::conditional_t<std::same_as<T, LiftedTag>, VariableDomains, std::monostate> m_variable_domains;
};

}

#endif
