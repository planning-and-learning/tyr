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

#ifndef TYR_DATALOG_GROUND_WORKSPACES_FACTS_HPP_
#define TYR_DATALOG_GROUND_WORKSPACES_FACTS_HPP_

#include "tyr/datalog/declarations.hpp"
#include "tyr/datalog/fact_sets.hpp"
#include "tyr/formalism/datalog/repository.hpp"

#include <yggdrasil/containers/associative_containers.hpp>
#include <yggdrasil/core/closed_interval.hpp>
#include <yggdrasil/core/config.hpp>

namespace tyr::datalog
{

template<>
struct FactsWorkspace<GroundTag>
{
    TaggedFactSets<formalism::FluentTag> fact_sets;

    explicit FactsWorkspace(formalism::datalog::PredicateListView<formalism::FluentTag> predicates,
                            formalism::datalog::FunctionListView<formalism::FluentTag> functions,
                            formalism::datalog::AtomListView<GroundTag, formalism::FluentTag> atoms,
                            formalism::datalog::FunctionTermValueListView<GroundTag, formalism::FluentTag> fterm_values,
                            const formalism::datalog::Repository& workspace_repository);

    bool insert(formalism::datalog::PredicateBindingView<formalism::FluentTag> binding) { return fact_sets.predicate.insert(binding); }

    bool insert(formalism::datalog::FunctionBindingView<formalism::FluentTag> binding, ygg::ClosedInterval<ygg::float_t> interval)
    {
        return fact_sets.function.insert(binding, interval);
    }

    void reset();
};

template<>
struct ConstFactsWorkspace<GroundTag>
{
    const TaggedFactSets<formalism::StaticTag> fact_sets;

    explicit ConstFactsWorkspace(formalism::datalog::PredicateListView<formalism::StaticTag> predicates,
                                 formalism::datalog::FunctionListView<formalism::StaticTag> functions,
                                 formalism::datalog::AtomListView<GroundTag, formalism::StaticTag> atoms,
                                 formalism::datalog::FunctionTermValueListView<GroundTag, formalism::StaticTag> fterm_values,
                                 const formalism::datalog::Repository& program_repository);
};

}

#endif
