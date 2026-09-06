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

#ifndef TYR_DATALOG_LIFTED_PROGRAM_HPP_
#define TYR_DATALOG_LIFTED_PROGRAM_HPP_

#include "tyr/analysis/listeners.hpp"
#include "tyr/analysis/program_analysis.hpp"
#include "tyr/analysis/stratification.hpp"
#include "tyr/datalog/lifted/workspaces/program.hpp"
#include "tyr/formalism/datalog/program_index.hpp"
#include "tyr/formalism/datalog/repository.hpp"

namespace tyr::datalog
{

template<>
class Program<LiftedTag>
{
public:
    Program(formalism::datalog::ProgramView<LiftedTag> program,
            formalism::datalog::RepositoryPtr program_repository,
            formalism::datalog::RepositoryFactoryPtr repository_factory);

    auto get_program() const noexcept { return m_program; }
    const auto& get_program_repository() const noexcept { return *m_program_repository; }
    auto& get_repository_factory() noexcept { return *m_repository_factory; }
    auto& get_repository_factory() const noexcept { return *m_repository_factory; }
    const auto& get_domains() const noexcept { return m_analysis.domains; }
    const auto& get_strata() const noexcept { return m_strata; }
    const auto& get_listeners() const noexcept { return m_listeners; }
    const auto& get_const_program_workspace() const noexcept { return m_const_program_workspace; }

private:
    friend struct ConstProgramWorkspace<LiftedTag>;

    formalism::datalog::ProgramView<LiftedTag> m_program;
    formalism::datalog::RepositoryPtr m_program_repository;
    formalism::datalog::RepositoryFactoryPtr m_repository_factory;
    analysis::ProgramAnalysis m_analysis;
    analysis::RuleStrata m_strata;
    analysis::ListenerStrata m_listeners;
    ConstProgramWorkspace<LiftedTag> m_const_program_workspace;
};

}

#endif
