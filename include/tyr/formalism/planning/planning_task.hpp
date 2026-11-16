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

#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/fdr_context.hpp"
#include "tyr/formalism/planning/planning_domain.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/planning/task_view.hpp"

#include <filesystem>
#include <memory>
#include <optional>
#include <type_traits>
#include <variant>

namespace tyr::analysis
{
struct TaskVariableDomains;
struct TaskVariableDomainsView;
}

namespace tyr::formalism::planning
{

template<TaskKind T>
class PlanningTask
{
public:
    /// The task as parsed or grounded, frozen: its repository and FDR context are shared and never written again (a
    /// planning Task writes into its own layers over them).
    PlanningTask(TaskView<T> task,
                 std::shared_ptr<const FDRContext> fdr_context,
                 std::shared_ptr<const Repository> repository,
                 PlanningDomain domain,
                 std::optional<std::filesystem::path> path = std::nullopt);

    [[nodiscard]] const auto& get_repository() const noexcept { return m_repository; }
    [[nodiscard]] auto get_task() const noexcept { return m_task; }
    [[nodiscard]] const auto& get_fdr_context() const noexcept { return m_fdr_context; }
    [[nodiscard]] const auto& get_domain() const noexcept { return m_domain; }
    [[nodiscard]] const auto& get_path() const noexcept { return m_path; }
    [[nodiscard]] const analysis::TaskVariableDomains& get_variable_domains() const noexcept
        requires std::same_as<T, LiftedTag>;
    [[nodiscard]] const analysis::TaskVariableDomainsView& get_variable_domains_view() const noexcept
        requires std::same_as<T, LiftedTag>;

    /// The variable domains of a lifted task, and their views; a ground task has none. Defined in planning_task.cpp, so
    /// that the sources including this header complete none of their maps (brokkr's template rules, rule 6).
    struct VariableDomains;

private:
    std::shared_ptr<const Repository> m_repository;
    TaskView<T> m_task;
    std::shared_ptr<const FDRContext> m_fdr_context;
    PlanningDomain m_domain;
    std::optional<std::filesystem::path> m_path;
    [[no_unique_address]] std::conditional_t<std::same_as<T, LiftedTag>, std::shared_ptr<const VariableDomains>, std::monostate> m_variable_domains;
};

}

#endif
