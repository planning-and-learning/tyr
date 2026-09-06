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

#include "tyr/datalog/lifted/program.hpp"

#include "tyr/analysis/domains.hpp"

namespace tyr::datalog
{

Program<LiftedTag>::Program(formalism::datalog::ProgramView<LiftedTag> program,
                            formalism::datalog::RepositoryPtr program_repository,
                            formalism::datalog::RepositoryFactoryPtr repository_factory) :
    m_program(program),
    m_program_repository(std::move(program_repository)),
    m_repository_factory(std::move(repository_factory)),
    m_analysis(analysis::analyze_program(m_program)),
    m_strata(analysis::compute_rule_stratification(m_program)),
    m_listeners(analysis::compute_listeners(m_strata, *m_program_repository)),
    m_const_program_workspace(*this)
{
    m_analysis.compatibility_graphs = {};
}

}
