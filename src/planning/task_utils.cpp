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
#include "tyr/datalog/lifted/assignment_sets.hpp"
#include "tyr/datalog/workspaces/program.hpp"
#include "tyr/formalism/datalog/merge.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/datalog/views.hpp"
#include "tyr/formalism/planning/merge_datalog.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/planning/views.hpp"
#include "tyr/planning/lifted/state_builder.hpp"

#include <yggdrasil/core/config.hpp>

namespace f = tyr::formalism;
namespace fp = tyr::formalism::planning;

namespace tyr::planning
{

void insert_fluent_atoms_to_fact_set(const ygg::Builder<State<LiftedTag>>& state,
                                     const ::tyr::formalism::planning::Repository& repository,
                                     const ygg::UnorderedMap<::tyr::formalism::planning::PredicateView<::tyr::formalism::FluentTag>,
                                                             ::tyr::formalism::datalog::PredicateView<::tyr::formalism::FluentTag>>& fluent_to_fluent_predicate,
                                     fp::MergeDatalogContext& merge_context,
                                     datalog::TaggedFactSets<f::FluentTag>& fact_sets)
{
    for (const auto fact : state.get_fluent_facts_view(repository))
        fact_sets.predicate.insert(fp::merge_p2d<f::FluentTag, f::FluentTag>(fact.get_atom().value(), fluent_to_fluent_predicate, merge_context).first);
}

void insert_derived_atoms_to_fact_set(
    const ygg::Builder<State<LiftedTag>>& state,
    const ::tyr::formalism::planning::Repository& repository,
    const ygg::UnorderedMap<::tyr::formalism::planning::PredicateView<::tyr::formalism::DerivedTag>,
                            ::tyr::formalism::datalog::PredicateView<::tyr::formalism::FluentTag>>& derived_to_fluent_predicate,
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

void insert_extended_state(const ygg::Builder<State<LiftedTag>>& state_builder,
                           const fp::Repository& atoms_context,
                           const P2DTranslationContext<LiftedTag>& translation_context,
                           fp::MergeDatalogContext& merge_context,
                           datalog::TaggedFactSets<f::FluentTag>& fact_sets,
                           datalog::TaggedAssignmentSets<f::FluentTag>& assignment_sets)
{
    fact_sets.reset();
    assignment_sets.reset();

    insert_fluent_atoms_to_fact_set(state_builder, atoms_context, translation_context.fluent_to_fluent_predicate, merge_context, fact_sets);
    insert_derived_atoms_to_fact_set(state_builder, atoms_context, translation_context.derived_to_fluent_predicate, merge_context, fact_sets);
    insert_numeric_variables_to_fact_set(state_builder, atoms_context, merge_context, fact_sets);

    assignment_sets.insert(fact_sets);
}

void insert_unextended_state(const ygg::Builder<State<LiftedTag>>& state_builder,
                             const fp::Repository& atoms_context,
                             const P2DTranslationContext<LiftedTag>& translation_context,
                             fp::MergeDatalogContext& merge_context,
                             datalog::TaggedFactSets<f::FluentTag>& fact_sets,
                             datalog::TaggedAssignmentSets<f::FluentTag>& assignment_sets)
{
    fact_sets.reset();
    assignment_sets.reset();

    insert_fluent_atoms_to_fact_set(state_builder, atoms_context, translation_context.fluent_to_fluent_predicate, merge_context, fact_sets);
    insert_numeric_variables_to_fact_set(state_builder, atoms_context, merge_context, fact_sets);

    assignment_sets.insert(fact_sets);
}

}
