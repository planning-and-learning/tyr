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

#ifndef TYR_TESTS_PLANNING_SUCCESSORS_INITIAL_SUCCESSOR_TABLES_HPP_
#define TYR_TESTS_PLANNING_SUCCESSORS_INITIAL_SUCCESSOR_TABLES_HPP_

#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/formatter.hpp"
#include "tyr/formalism/repository.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/nodes/node_view.hpp"
#include "tyr/planning/states/state_view.hpp"
#include "tyr/planning/successors/successor_view.hpp"

#include <string>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <yggdrasil/serialization/dictionaries.hpp>

namespace tyr::tests
{

/// The tables the initial successors fixture is written in (tests/support/generators/planning/successors/
/// generate_initial_successors.py registers the same): a successor, its label and the label's action binding (its action
/// and objects by name), its
/// distributions and outcomes, an outcome's node and state; a state's storages (generically, by their fields).
template<TaskKind Kind>
void register_initial_successor_tables(ygg::serialization::Dictionaries& dictionaries)
{
    using Repository = planning::StateRepository<Kind>;
    using Indices = urd::Store<urd::sequence<std::string>>;
    dictionaries.template register_table<planning::SuccessorBuilderView<Kind>>("successors", "S");
    dictionaries.template register_table<urd::borrowed_view<planning::Transition, Repository, urd::builder<planning::Transition, Repository>>>("labels", "L");
    dictionaries.template register_table<formalism::ActionRowView>("bindings", "B");
    dictionaries.template register_table<
        urd::borrowed_view<ygg::Row<formalism::Action<LiftedTag>>, Repository, urd::builder<ygg::Row<formalism::Action<LiftedTag>>, Repository>>>("built_bindings",
                                                                                                                                                "b");
    dictionaries.template register_table<formalism::ActionView<LiftedTag>>("actions", "A", Indices { "name" });
    dictionaries.template register_table<formalism::ObjectView>("objects", "o", Indices { "name" });
    dictionaries.template register_table<urd::borrowed_view<planning::Distribution<Kind>, Repository, urd::builder<planning::Distribution<Kind>, Repository>>>(
        "distributions",
        "D");
    dictionaries.template register_table<urd::borrowed_view<planning::OutcomeNode<Kind>, Repository, urd::builder<planning::OutcomeNode<Kind>, Repository>>>(
        "outcomes",
        "O");
    dictionaries.template register_table<planning::NodeBuilderView<Kind>>("nodes", "N");
    // A state and its storages, built (as the generator writes them).
    dictionaries.template register_table<planning::StateBuilderView<Kind>>("states", "s");
    dictionaries.template register_table<planning::FactStorageBuilderView<Kind>>("fact_storages", "F");
    dictionaries.template register_table<planning::AtomStorageBuilderView<Kind>>("atom_storages", "a");
    dictionaries.template register_table<planning::NumericStorageBuilderView<Kind>>("numeric_storages", "n");
    dictionaries.template register_table<planning::TemporalStorageBuilderView<Kind>>("temporal_storages", "T");
}

}

#endif
