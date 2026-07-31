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

#ifndef TYR_PLANNING_TASK_UTILS_HPP_
#define TYR_PLANNING_TASK_UTILS_HPP_

#include "tyr/analysis/declarations.hpp"
#include "tyr/datalog/declarations.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/planning/merge_datalog_decl.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/planning/lifted/programs/translation_context.hpp"
#include "tyr/planning/lifted/state_builder.hpp"
#include "tyr/planning/task.hpp"

#include <yggdrasil/core/config.hpp>

namespace tyr::planning
{

extern void
insert_fluent_atoms_to_fact_set(const ygg::Builder<State<LiftedTag>>& state,
                                const ::tyr::formalism::planning::Repository& repository,
                                const ygg::UnorderedMap<::tyr::formalism::planning::PredicateView<::tyr::formalism::FluentTag>,
                                                        ::tyr::formalism::datalog::PredicateView<::tyr::formalism::FluentTag>>& fluent_to_fluent_predicate,
                                ::tyr::formalism::planning::MergeDatalogContext& merge_context,
                                datalog::TaggedFactSets<::tyr::formalism::FluentTag>& fact_sets);

void insert_derived_atoms_to_fact_set(
    const ygg::Builder<State<LiftedTag>>& state,
    const ::tyr::formalism::planning::Repository& repository,
    const ygg::UnorderedMap<::tyr::formalism::planning::PredicateView<::tyr::formalism::DerivedTag>,
                            ::tyr::formalism::datalog::PredicateView<::tyr::formalism::FluentTag>>& derived_to_fluent_predicate,
    ::tyr::formalism::planning::MergeDatalogContext& merge_context,
    datalog::TaggedFactSets<::tyr::formalism::FluentTag>& fact_sets);

void insert_numeric_variables_to_fact_set(const ygg::Builder<State<LiftedTag>>& state,
                                          const ::tyr::formalism::planning::Repository& repository,
                                          ::tyr::formalism::planning::MergeDatalogContext& merge_context,
                                          datalog::TaggedFactSets<::tyr::formalism::FluentTag>& fact_sets);

void insert_extended_state(const ygg::Builder<State<LiftedTag>>& state_builder,
                           const ::tyr::formalism::planning::Repository& atoms_context,
                           const P2DTranslationContext<LiftedTag>& translation_context,
                           ::tyr::formalism::planning::MergeDatalogContext& merge_context,
                           datalog::TaggedFactSets<::tyr::formalism::FluentTag>& fact_sets,
                           datalog::TaggedAssignmentSets<::tyr::formalism::FluentTag>& assignment_sets);

void insert_unextended_state(const ygg::Builder<State<LiftedTag>>& state_builder,
                             const ::tyr::formalism::planning::Repository& atoms_context,
                             const P2DTranslationContext<LiftedTag>& translation_context,
                             ::tyr::formalism::planning::MergeDatalogContext& merge_context,
                             datalog::TaggedFactSets<::tyr::formalism::FluentTag>& fact_sets,
                             datalog::TaggedAssignmentSets<::tyr::formalism::FluentTag>& assignment_sets);

}

#endif
