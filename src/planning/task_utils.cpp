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

#include "tyr/planning/task_utils.hpp"

#include "tyr/analysis/declarations.hpp"
#include "tyr/datalog/fact_sets.hpp"
#include "tyr/formalism/datalog/merge.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/datalog/views.hpp"
#include "tyr/formalism/planning/merge_datalog.hpp"
#include "tyr/formalism/planning/merge_planning.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/planning/views.hpp"
#include "tyr/planning/lifted/state_builder.hpp"

#include <yggdrasil/core/config.hpp>

namespace f = tyr::formalism;
namespace fp = tyr::formalism::planning;

namespace tyr::planning
{
namespace detail
{

void insert_fluent_atoms_to_fact_set(const ygg::Builder<State<GroundTag>>& state,
                                     const fp::Repository& repository,
                                     const P2DTranslationContext<GroundTag>::FluentToFluentAtomMapping& fluent_to_fluent_atom,
                                     datalog::TaggedFactSets<f::FluentTag>& fact_sets)
{
    for (const auto fact : state.get_fluent_facts_view(repository))
        if (const auto atom = fact.get_atom())
            fact_sets.predicate.insert(fluent_to_fluent_atom.at(*atom));
}

void insert_numeric_variables_to_fact_set(const ygg::Builder<State<GroundTag>>& state,
                                          const fp::Repository& repository,
                                          const P2DTranslationContext<GroundTag>::FluentToFluentFunctionTermMapping& fluent_to_fluent_fterm,
                                          datalog::TaggedFactSets<f::FluentTag>& fact_sets)
{
    for (const auto& [fterm, value] : state.get_fluent_fterm_values_view(repository))
        if (const auto it = fluent_to_fluent_fterm.find(fterm); it != fluent_to_fluent_fterm.end())
            fact_sets.function.insert(it->second, value);
}

void insert_fluent_atoms_to_fact_set(const ygg::Builder<State<LiftedTag>>& state,
                                     const ::tyr::formalism::planning::Repository& repository,
                                     const P2DTranslationContext<LiftedTag>::FluentToFluentPredicateMapping& fluent_to_fluent_predicate,
                                     fp::MergeDatalogContext& merge_context,
                                     datalog::TaggedFactSets<f::FluentTag>& fact_sets)
{
    for (const auto fact : state.get_fluent_facts_view(repository))
        fact_sets.predicate.insert(fp::merge_p2d<f::FluentTag, f::FluentTag>(fact.get_atom().value(), fluent_to_fluent_predicate, merge_context).first);
}

void insert_derived_atoms_to_fact_set(const ygg::Builder<State<LiftedTag>>& state,
                                      const ::tyr::formalism::planning::Repository& repository,
                                      const P2DTranslationContext<LiftedTag>::DerivedToFluentPredicateMapping& derived_to_fluent_predicate,
                                      fp::MergeDatalogContext& merge_context,
                                      datalog::TaggedFactSets<f::FluentTag>& fact_sets)
{
    for (const auto atom : state.get_derived_atoms_view(repository))
        fact_sets.predicate.insert(fp::merge_p2d<f::DerivedTag, f::FluentTag>(atom, derived_to_fluent_predicate, merge_context).first);
}

void insert_numeric_variables_to_fact_set(const ygg::Builder<State<LiftedTag>>& state,
                                          const ::tyr::formalism::planning::Repository& repository,
                                          fp::MergeDatalogContext& merge_context,
                                          datalog::TaggedFactSets<f::FluentTag>& fact_sets)
{
    for (const auto& [fterm, value] : state.get_fluent_fterm_values_view(repository))
        fact_sets.function.insert(fp::merge_p2d(fterm, merge_context).first, value);
}

void read_derived_atoms_from_fact_set(ygg::Builder<State<LiftedTag>>& state,
                                      fp::Repository& repository,
                                      const D2PTranslationContext<LiftedTag>::FluentToDerivedPredicateMapping& fluent_to_derived_predicate,
                                      fp::Builder& planning_builder,
                                      const datalog::TaggedFactSets<f::FluentTag>& fact_sets,
                                      std::vector<::tyr::formalism::datalog::PredicateBindingView<f::FluentTag>>& derived_bindings)
{
    derived_bindings.clear();
    for (const auto& set : fact_sets.predicate.get_sets())
        for (const auto binding : set.get_bindings())
            if (fluent_to_derived_predicate.contains(binding.get_relation()))
                derived_bindings.push_back(binding);

    auto merge_context = fp::MergePlanningContext { planning_builder, repository };
    for (const auto binding : derived_bindings)
        state.set(fp::merge_atom_d2p<f::FluentTag, f::DerivedTag>(binding, fluent_to_derived_predicate, merge_context).first);
}
}
}
