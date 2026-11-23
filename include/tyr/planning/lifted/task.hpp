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

#ifndef TYR_PLANNING_LIFTED_TASK_HPP_
#define TYR_PLANNING_LIFTED_TASK_HPP_

#include "tyr/declarations.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/planning_task.hpp"
#include "tyr/formalism/planning/task_view.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/lifted/task_grounder_decl.hpp"
#include "tyr/planning/static_state.hpp"
#include "tyr/planning/task.hpp"

#include <type_traits>
#include <yggdrasil/execution/declarations.hpp>

namespace tyr::planning
{

template<>
class Task<LiftedTag>
{
public:
    explicit Task(formalism::PlanningTask<LiftedTag> task);

    static TaskPtr<LiftedTag> create(formalism::PlanningTask<LiftedTag> task);

    GroundTaskInstantiationResult instantiate_ground_task(ygg::ExecutionContext& execution_context,
                                                          const GroundTaskInstantiationOptions& options = GroundTaskInstantiationOptions());

    /**
     * Getters
     */

    [[nodiscard]] const auto& get_formalism_task() const noexcept { return m_task; }

    [[nodiscard]] const auto& get_domain() const noexcept { return m_task.get_domain(); }

    [[nodiscard]] auto get_task() const noexcept { return m_task.get_task(); }

    /// This task's own layers over the formalism task's frozen repository and FDR context: what its search grounds
    /// and registers goes here.
    [[nodiscard]] const auto& get_fdr_context() const noexcept { return m_fdr_context; }

    [[nodiscard]] const auto& get_repository() const noexcept { return m_repository; }

    [[nodiscard]] const StaticState& get_static_state() const noexcept { return m_static_state; }

    [[nodiscard]] bool has_axioms() const noexcept;

private:
    formalism::PlanningTask<LiftedTag> m_task;
    formalism::RepositoryPtr m_repository;
    formalism::FDRContextPtr m_fdr_context;

    StaticState m_static_state;
};

/// A lifted task is serialized as the formalism task it was made from (pyyggdrasil.serialization).
template<class Archive>
void describe_fields(Archive& ar, std::type_identity<Task<LiftedTag>> /*type*/)
{
    ar.field("formalism_task", [](const auto& value) -> decltype(auto) { return (value.get_formalism_task()); });
}

}

#endif
