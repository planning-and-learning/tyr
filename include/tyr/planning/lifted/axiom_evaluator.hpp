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

#ifndef TYR_PLANNING_LIFTED_AXIOM_EVALUATOR_HPP_
#define TYR_PLANNING_LIFTED_AXIOM_EVALUATOR_HPP_

#include "tyr/planning/lifted/state_builder.hpp"
//
#include "tyr/datalog/lifted/policies/annotation.hpp"
#include "tyr/datalog/lifted/workspaces/program.hpp"
#include "tyr/datalog/policies/termination.hpp"
#include "tyr/planning/axiom_evaluator.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/lifted/programs/axiom.hpp"

#include <memory>
#include <yggdrasil/execution/onetbb.hpp>

namespace tyr::planning
{

template<>
class AxiomEvaluator<LiftedTag>
{
    friend class AxiomEvaluatorFactory<LiftedTag>;

private:
    AxiomEvaluator(ygg::uint_t index, TaskPtr<LiftedTag> task, ygg::ExecutionContextPtr execution_context);

public:
    void compute_extended_state(ygg::Builder<State<LiftedTag>>& state_builder);

    const auto& get_axiom_program() const noexcept { return m_axiom_program; }
    const auto& get_workspace() const noexcept { return m_workspace; }
    const auto& get_execution_context() const noexcept { return m_execution_context; }
    auto get_index() const noexcept { return m_index; }

    void print_summary(size_t verbosity) const;

private:
    ygg::uint_t m_index;
    TaskPtr<LiftedTag> m_task;
    ygg::ExecutionContextPtr m_execution_context;
    AxiomEvaluatorProgram<LiftedTag> m_axiom_program;

    datalog::ProgramWorkspace<LiftedTag> m_workspace;
};

}

#endif
