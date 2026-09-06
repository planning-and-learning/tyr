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

namespace f = tyr::formalism;
namespace fd = tyr::formalism::datalog;

namespace tyr::datalog
{

FactsWorkspace<GroundTag>::FactsWorkspace(fd::PredicateListView<f::FluentTag> predicates,
                                          fd::FunctionListView<f::FluentTag> functions,
                                          fd::AtomListView<GroundTag, f::FluentTag> atoms,
                                          fd::FunctionTermValueListView<GroundTag, f::FluentTag> fterm_values,
                                          const fd::Repository& workspace_repository) :
    fact_sets(predicates, functions, atoms, fterm_values, workspace_repository)
{
}

void FactsWorkspace<GroundTag>::reset()
{
    fact_sets.reset();
}

ConstFactsWorkspace<GroundTag>::ConstFactsWorkspace(fd::PredicateListView<f::StaticTag> predicates,
                                                    fd::FunctionListView<f::StaticTag> functions,
                                                    fd::AtomListView<GroundTag, f::StaticTag> atoms,
                                                    fd::FunctionTermValueListView<GroundTag, f::StaticTag> fterm_values,
                                                    const fd::Repository& program_repository) :
    fact_sets(predicates, functions, atoms, fterm_values, program_repository)
{
}

}
