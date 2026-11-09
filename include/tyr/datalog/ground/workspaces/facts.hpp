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

#include <urd/repr/maps/unordered_flat_map.hpp>
#include <urd/repr/values/unordered_flat_set.hpp>
#include <urd/store.hpp>
#include <utility>
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

    bool insert(formalism::datalog::PredicateRowIndex<formalism::FluentTag> binding) { return tyr::datalog::insert(fact_sets, binding); }

    bool insert(formalism::datalog::FunctionRowIndex<formalism::FluentTag> binding, const ygg::ClosedInterval<ygg::float_t>& interval)
    { return tyr::datalog::insert(fact_sets, binding, interval); }

    void reset();
};

template<>
struct ConstFactsWorkspace<GroundTag>
{
    TaggedFactSets<formalism::StaticTag> fact_sets;

    explicit ConstFactsWorkspace(formalism::datalog::PredicateListView<formalism::StaticTag> predicates,
                                 formalism::datalog::FunctionListView<formalism::StaticTag> functions,
                                 formalism::datalog::AtomListView<GroundTag, formalism::StaticTag> atoms,
                                 formalism::datalog::FunctionTermValueListView<GroundTag, formalism::StaticTag> fterm_values,
                                 const formalism::datalog::Repository& program_repository);
};

}

#endif
