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

#include "tyr/formalism/datalog/repository.hpp"

namespace a = tyr::analysis;
namespace f = tyr::formalism;
namespace fd = tyr::formalism::datalog;

namespace tyr::datalog
{

FactsWorkspace<LiftedTag>::FactsWorkspace(fd::PredicateListView<f::FluentTag> predicates,
                                          fd::FunctionListView<f::FluentTag> functions,
                                          const analysis::PredicateDomainMap<formalism::FluentTag>& predicate_domains,
                                          const analysis::FunctionDomainMap<formalism::FluentTag>& function_domains,
                                          size_t num_objects,
                                          fd::AtomListView<GroundTag, formalism::FluentTag> atoms,
                                          fd::FunctionTermValueListView<GroundTag, formalism::FluentTag> fterm_values,
                                          const formalism::datalog::Repository& workspace_repository) :
    fact_sets(predicates, functions, atoms, fterm_values, workspace_repository),
    assignment_sets(predicates, functions, predicate_domains, function_domains, num_objects, fact_sets)
{
}

void FactsWorkspace<LiftedTag>::reset()
{
    fact_sets.reset();
    assignment_sets.reset();
}

ConstFactsWorkspace<LiftedTag>::ConstFactsWorkspace(fd::PredicateListView<f::StaticTag> predicates,
                                                    fd::FunctionListView<f::StaticTag> functions,
                                                    const analysis::PredicateDomainMap<formalism::StaticTag>& predicate_domains,
                                                    const analysis::FunctionDomainMap<formalism::StaticTag>& function_domains,
                                                    size_t num_objects,
                                                    fd::AtomListView<GroundTag, f::StaticTag> atoms,
                                                    fd::FunctionTermValueListView<GroundTag, f::StaticTag> fterm_values,
                                                    const formalism::datalog::Repository& program_repository) :
    fact_sets(predicates, functions, atoms, fterm_values, program_repository),
    assignment_sets(predicates, functions, predicate_domains, function_domains, num_objects, fact_sets)
{
}

}
