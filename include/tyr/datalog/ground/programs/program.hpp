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

#ifndef TYR_DATALOG_GROUND_PROGRAMS_PROGRAM_HPP_
#define TYR_DATALOG_GROUND_PROGRAMS_PROGRAM_HPP_

#include "tyr/datalog/ground/workspaces/program.hpp"
#include "tyr/datalog/programs/program.hpp"
#include "tyr/formalism/datalog/repository.hpp"

namespace tyr::datalog
{

template<>
class Program<GroundTag>
{
public:
    Program(::tyr::formalism::datalog::ProgramView<GroundTag> program,
            ::tyr::formalism::datalog::RepositoryPtr program_repository,
            ::tyr::formalism::datalog::RepositoryFactoryPtr repository_factory);

    auto get_program() const noexcept { return m_const_program_workspace.program; }
    auto& get_program_repository() noexcept { return *m_program_repository; }
    const auto& get_program_repository() const noexcept { return *m_program_repository; }
    auto& get_repository_factory() noexcept { return *m_repository_factory; }
    const auto& get_const_program_workspace() const noexcept { return m_const_program_workspace; }

private:
    ::tyr::formalism::datalog::RepositoryPtr m_program_repository;
    ::tyr::formalism::datalog::RepositoryFactoryPtr m_repository_factory;
    ConstProgramWorkspace<GroundTag> m_const_program_workspace;
};

template<AnnotationPolicyConcept<GroundTag> AP, TerminationPolicyConcept<GroundTag> TP, RuleCostPolicyConcept<GroundTag> CP>
ProgramWorkspace<GroundTag, AP, TP, CP>::ProgramWorkspace(Program<GroundTag>& program, AP annotation_policy, TP tp, CP cost_policy) :
    ProgramWorkspace(program.get_const_program_workspace(), std::move(annotation_policy), std::move(tp), std::move(cost_policy))
{
}

}

#endif
