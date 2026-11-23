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
#include "tyr/declarations.hpp"
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/declarations.hpp"

#include <urd/repr/maps/unordered_flat_map.hpp>
#include <urd/repr/values/unordered_flat_set.hpp>
#include <yggdrasil/core/closed_interval.hpp>
#include <yggdrasil/core/config.hpp>

namespace tyr::datalog
{

template<>
struct FactsWorkspace<GroundTag>
{
    MutableFactSets fact_sets;

    /// The program's facts of each mutable kind.
    FactsWorkspace(formalism::ProgramView<GroundTag> program, const formalism::Repository& workspace_repository);

    template<formalism::FactKind F>
    bool insert(formalism::PredicateRowKey<F> binding)
    { return tyr::datalog::insert(fact_sets, binding); }

    bool insert(formalism::FunctionRowKey<formalism::FluentTag> binding, const ygg::ClosedInterval<ygg::float_t>& interval)
    { return tyr::datalog::insert(fact_sets.get<formalism::FluentTag>(), binding, interval); }

    void reset();
};

template<>
struct ConstFactsWorkspace<GroundTag>
{
    TaggedFactSets<formalism::StaticTag> fact_sets;

    explicit ConstFactsWorkspace(formalism::PredicateListView<formalism::StaticTag> predicates,
                                 formalism::FunctionListView<formalism::StaticTag> functions,
                                 formalism::AtomListView<GroundTag, formalism::StaticTag> atoms,
                                 formalism::FunctionTermValueListView<GroundTag, formalism::StaticTag> fterm_values,
                                 const formalism::Repository& program_repository);
};

}

#endif
