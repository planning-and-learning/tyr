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

#include "tyr/datalog/ground/workspaces/facts.hpp"

namespace tyr::datalog
{

FactsWorkspace<GroundTag>::FactsWorkspace(formalism::datalog::PredicateListView<formalism::FluentTag> predicates,
                                          formalism::datalog::FunctionListView<formalism::FluentTag> functions,
                                          formalism::datalog::AtomListView<GroundTag, formalism::FluentTag> atoms,
                                          formalism::datalog::FunctionTermValueListView<GroundTag, formalism::FluentTag> fterm_values,
                                          const formalism::datalog::Repository& workspace_repository) :
    fact_sets(predicates, functions, atoms, fterm_values, workspace_repository)
{
}

void FactsWorkspace<GroundTag>::reset() { fact_sets.reset(); }

ConstFactsWorkspace<GroundTag>::ConstFactsWorkspace(formalism::datalog::PredicateListView<formalism::StaticTag> predicates,
                                                    formalism::datalog::FunctionListView<formalism::StaticTag> functions,
                                                    formalism::datalog::AtomListView<GroundTag, formalism::StaticTag> atoms,
                                                    formalism::datalog::FunctionTermValueListView<GroundTag, formalism::StaticTag> fterm_values,
                                                    const formalism::datalog::Repository& program_repository) :
    fact_sets(predicates, functions, atoms, fterm_values, program_repository)
{
}

}
