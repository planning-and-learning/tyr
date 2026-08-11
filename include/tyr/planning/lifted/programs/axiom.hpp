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

#ifndef TYR_PLANNING_LIFTED_PROGRAMS_AXIOM_HPP_
#define TYR_PLANNING_LIFTED_PROGRAMS_AXIOM_HPP_

#include "tyr/datalog/lifted/program.hpp"
#include "tyr/formalism/planning/views.hpp"
#include "tyr/planning/lifted/programs/translation_context.hpp"
#include "tyr/planning/programs/axiom.hpp"

namespace tyr::planning
{

template<>
class AxiomEvaluatorProgram<LiftedTag>
{
public:
    explicit AxiomEvaluatorProgram(::tyr::formalism::planning::TaskView task);

    const TranslationContext<LiftedTag>& get_translation_context() const noexcept;
    datalog::Program<LiftedTag>& get_datalog_program() noexcept;
    const datalog::Program<LiftedTag>& get_datalog_program() const noexcept;
    const datalog::ConstProgramWorkspace<LiftedTag>& get_const_program_workspace() const noexcept;

private:
    TranslationContext<LiftedTag> m_translation_context;
    datalog::Program<LiftedTag> m_datalog_program;
};

}

#endif
