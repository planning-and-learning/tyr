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

#ifndef TYR_DATALOG_LIFTED_WORKSPACES_FACTS_HPP_
#define TYR_DATALOG_LIFTED_WORKSPACES_FACTS_HPP_

#include "tyr/datalog/declarations.hpp"
#include "tyr/datalog/fact_sets.hpp"
#include "tyr/datalog/lifted/assignment_sets.hpp"
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/declarations.hpp"

#include <yggdrasil/core/closed_interval.hpp>

namespace tyr::datalog
{
template<>
struct FactsWorkspace<LiftedTag>
{
    TaggedFactSets<::tyr::formalism::FluentTag> fact_sets;
    TaggedAssignmentSets<::tyr::formalism::FluentTag> assignment_sets;

    explicit FactsWorkspace(::tyr::formalism::datalog::PredicateListView<::tyr::formalism::FluentTag> predicates,
                            ::tyr::formalism::datalog::FunctionListView<::tyr::formalism::FluentTag> functions,
                            const analysis::PredicateDomainMap<::tyr::formalism::FluentTag>& predicate_domains,
                            const analysis::FunctionDomainMap<::tyr::formalism::FluentTag>& function_domains,
                            size_t num_objects,
                            ::tyr::formalism::datalog::GroundAtomListView<::tyr::formalism::FluentTag> atoms,
                            ::tyr::formalism::datalog::GroundFunctionTermValueListView<::tyr::formalism::FluentTag> fterm_values,
                            const ::tyr::formalism::datalog::Repository& workspace_repository);

    bool insert(::tyr::formalism::datalog::PredicateBindingView<::tyr::formalism::FluentTag> binding)
    {
        if (!fact_sets.predicate.insert(binding))
            return false;
        assignment_sets.predicate.insert(binding);
        return true;
    }

    bool insert(::tyr::formalism::datalog::FunctionBindingView<::tyr::formalism::FluentTag> binding, ygg::ClosedInterval<ygg::float_t> interval)
    {
        if (!fact_sets.function.insert(binding, interval))
            return false;
        assignment_sets.function.insert(binding, interval);
        return true;
    }

    void reset();
};

template<>
struct ConstFactsWorkspace<LiftedTag>
{
    const TaggedFactSets<::tyr::formalism::StaticTag> fact_sets;
    const TaggedAssignmentSets<::tyr::formalism::StaticTag> assignment_sets;

    explicit ConstFactsWorkspace(::tyr::formalism::datalog::PredicateListView<::tyr::formalism::StaticTag> predicates,
                                 ::tyr::formalism::datalog::FunctionListView<::tyr::formalism::StaticTag> functions,
                                 const analysis::PredicateDomainMap<::tyr::formalism::StaticTag>& predicate_domains,
                                 const analysis::FunctionDomainMap<::tyr::formalism::StaticTag>& function_domains,
                                 size_t num_objects,
                                 ::tyr::formalism::datalog::GroundAtomListView<::tyr::formalism::StaticTag> atoms,
                                 ::tyr::formalism::datalog::GroundFunctionTermValueListView<::tyr::formalism::StaticTag> fterm_values,
                                 const ::tyr::formalism::datalog::Repository& program_repository);
};

}

#endif
