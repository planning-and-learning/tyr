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

#include "tyr/datalog/ground/program.hpp"

namespace tyr::datalog
{

Program<GroundTag>::Program(formalism::datalog::ProgramView<GroundTag> program,
                            formalism::datalog::RepositoryPtr program_repository,
                            formalism::datalog::RepositoryFactoryPtr repository_factory) :
    m_program_repository(std::move(program_repository)),
    m_repository_factory(std::move(repository_factory)),
    m_const_program_workspace(program)
{
}

}
