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

#include "tyr/datalog/lifted/programs/program.hpp"

#include "tyr/analysis/domains.hpp"

namespace tyr::datalog
{

Program<LiftedTag>::Program(::tyr::formalism::datalog::ProgramView<LiftedTag> program,
                            ::tyr::formalism::datalog::RepositoryPtr program_repository,
                            ::tyr::formalism::datalog::RepositoryFactoryPtr repository_factory) :
    m_program(program),
    m_program_repository(std::move(program_repository)),
    m_repository_factory(std::move(repository_factory)),
    m_domains(analysis::compute_variable_domains(m_program)),
    m_strata(analysis::compute_rule_stratification(m_program)),
    m_listeners(analysis::compute_listeners(m_strata, *m_program_repository)),
    m_const_program_workspace(*this)
{
}

Program<LiftedTag>::Program(::tyr::formalism::datalog::ProgramView<LiftedTag> program,
                            ::tyr::formalism::datalog::RepositoryPtr program_repository,
                            ::tyr::formalism::datalog::RepositoryFactoryPtr repository_factory,
                            analysis::ProgramVariableDomains domains,
                            analysis::RuleStrata strata,
                            analysis::ListenerStrata listeners) :
    m_program(program),
    m_program_repository(std::move(program_repository)),
    m_repository_factory(std::move(repository_factory)),
    m_domains(std::move(domains)),
    m_strata(std::move(strata)),
    m_listeners(std::move(listeners)),
    m_const_program_workspace(*this)
{
}

}
