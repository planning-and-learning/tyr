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

#include "tyr/datalog/lifted/workspaces/facts.hpp"

#include "tyr/formalism/datalog/program_view.hpp"
#include "tyr/formalism/repository.hpp"

namespace tyr::datalog
{

FactsWorkspace<LiftedTag>::FactsWorkspace(formalism::ProgramView<LiftedTag> program, const formalism::Repository& workspace_repository) :
    fact_sets(program, workspace_repository),
    delta_fact_sets(program, workspace_repository),
    repository(&workspace_repository)
{ insert_program_facts(fact_sets, program); }

void FactsWorkspace<LiftedTag>::reset()
{
    clear(fact_sets);
    clear(delta_fact_sets);
}

ConstFactsWorkspace<LiftedTag>::ConstFactsWorkspace(formalism::PredicateListView<formalism::StaticTag> predicates,
                                                    formalism::FunctionListView<formalism::StaticTag> functions,
                                                    formalism::AtomListView<GroundTag, formalism::StaticTag> atoms,
                                                    formalism::FunctionTermValueListView<GroundTag, formalism::StaticTag> fterm_values,
                                                    const formalism::Repository& program_repository) :
    fact_sets(predicates, functions, atoms, fterm_values, program_repository)
{
}

}
