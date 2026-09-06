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
#include "tyr/datalog/policies/annotation_concept.hpp"
#include "tyr/datalog/policies/cost_concept.hpp"
#include "tyr/datalog/policies/termination_concept.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/planning/merge_datalog_decl.hpp"
#include "tyr/formalism/planning/merge_planning_decl.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/planning/ground/programs/translation_context.hpp"
#include "tyr/planning/ground/state_builder.hpp"
#include "tyr/planning/lifted/programs/translation_context.hpp"
#include "tyr/planning/lifted/state_builder.hpp"
#include "tyr/planning/task.hpp"

#include <concepts>
#include <vector>
#include <yggdrasil/core/config.hpp>

namespace tyr::planning
{

namespace detail
{

void insert_fluent_atoms_to_fact_set(const ygg::Builder<State<GroundTag>>& state,
                                     const formalism::planning::Repository& repository,
                                     const P2DTranslationContext<GroundTag>::FluentToFluentAtomMapping& fluent_to_fluent_atom,
                                     datalog::TaggedFactSets<formalism::FluentTag>& fact_sets);

void insert_numeric_variables_to_fact_set(const ygg::Builder<State<GroundTag>>& state,
                                          const formalism::planning::Repository& repository,
                                          const P2DTranslationContext<GroundTag>::FluentToFluentFunctionTermMapping& fluent_to_fluent_fterm,
                                          datalog::TaggedFactSets<formalism::FluentTag>& fact_sets);

void insert_fluent_atoms_to_fact_set(const ygg::Builder<State<LiftedTag>>& state,
                                     const formalism::planning::Repository& repository,
                                     const P2DTranslationContext<LiftedTag>::FluentToFluentPredicateMapping& fluent_to_fluent_predicate,
                                     formalism::planning::MergeDatalogContext& merge_context,
                                     datalog::TaggedFactSets<formalism::FluentTag>& fact_sets);

void insert_derived_atoms_to_fact_set(const ygg::Builder<State<LiftedTag>>& state,
                                      const formalism::planning::Repository& repository,
                                      const P2DTranslationContext<LiftedTag>::DerivedToFluentPredicateMapping& derived_to_fluent_predicate,
                                      formalism::planning::MergeDatalogContext& merge_context,
                                      datalog::TaggedFactSets<formalism::FluentTag>& fact_sets);

void insert_numeric_variables_to_fact_set(const ygg::Builder<State<LiftedTag>>& state,
                                          const formalism::planning::Repository& repository,
                                          formalism::planning::MergeDatalogContext& merge_context,
                                          datalog::TaggedFactSets<formalism::FluentTag>& fact_sets);

void read_derived_atoms_from_fact_set(ygg::Builder<State<LiftedTag>>& state,
                                      formalism::planning::Repository& repository,
                                      const D2PTranslationContext<LiftedTag>::FluentToDerivedPredicateMapping& fluent_to_derived_predicate,
                                      formalism::planning::Builder& planning_builder,
                                      const datalog::TaggedFactSets<formalism::FluentTag>& fact_sets,
                                      std::vector<formalism::datalog::PredicateBindingView<formalism::FluentTag>>& derived_bindings);

}

template<TaskKind Kind, datalog::AnnotationPolicyConcept AP, datalog::TerminationPolicyConcept TP, datalog::RuleCostPolicyConcept CP>
void insert_fluent_atoms_to_fact_set(const ygg::Builder<State<Kind>>& state,
                                     const formalism::planning::Repository& repository,
                                     const P2DTranslationContext<Kind>& translation_context,
                                     datalog::ProgramWorkspace<Kind, AP, TP, CP>& workspace)
{
    if constexpr (std::same_as<Kind, GroundTag>)
    {
        detail::insert_fluent_atoms_to_fact_set(state, repository, translation_context.fluent_to_fluent_atom, workspace.facts.fact_sets);
    }
    else
    {
        auto merge_context = formalism::planning::MergeDatalogContext { workspace.datalog_builder, workspace.workspace_repository };
        detail::insert_fluent_atoms_to_fact_set(state, repository, translation_context.fluent_to_fluent_predicate, merge_context, workspace.facts.fact_sets);
    }
}

template<TaskKind Kind, datalog::AnnotationPolicyConcept AP, datalog::TerminationPolicyConcept TP, datalog::RuleCostPolicyConcept CP>
void insert_numeric_variables_to_fact_set(const ygg::Builder<State<Kind>>& state,
                                          const formalism::planning::Repository& repository,
                                          const P2DTranslationContext<Kind>& translation_context,
                                          datalog::ProgramWorkspace<Kind, AP, TP, CP>& workspace)
{
    if constexpr (std::same_as<Kind, GroundTag>)
    {
        detail::insert_numeric_variables_to_fact_set(state, repository, translation_context.fluent_to_fluent_fterm, workspace.facts.fact_sets);
    }
    else
    {
        auto merge_context = formalism::planning::MergeDatalogContext { workspace.datalog_builder, workspace.workspace_repository };
        detail::insert_numeric_variables_to_fact_set(state, repository, merge_context, workspace.facts.fact_sets);
    }
}

template<datalog::AnnotationPolicyConcept AP, datalog::TerminationPolicyConcept TP, datalog::RuleCostPolicyConcept CP>
void insert_derived_atoms_to_fact_set(const ygg::Builder<State<LiftedTag>>& state,
                                      const formalism::planning::Repository& repository,
                                      const P2DTranslationContext<LiftedTag>& translation_context,
                                      datalog::ProgramWorkspace<LiftedTag, AP, TP, CP>& workspace)
{
    auto merge_context = formalism::planning::MergeDatalogContext { workspace.datalog_builder, workspace.workspace_repository };
    detail::insert_derived_atoms_to_fact_set(state, repository, translation_context.derived_to_fluent_predicate, merge_context, workspace.facts.fact_sets);
}

template<datalog::AnnotationPolicyConcept AP, datalog::TerminationPolicyConcept TP, datalog::RuleCostPolicyConcept CP>
void insert_extended_state(const ygg::Builder<State<LiftedTag>>& state,
                           const formalism::planning::Repository& repository,
                           const P2DTranslationContext<LiftedTag>& translation_context,
                           datalog::ProgramWorkspace<LiftedTag, AP, TP, CP>& workspace)
{
    workspace.facts.reset();
    insert_fluent_atoms_to_fact_set(state, repository, translation_context, workspace);
    insert_derived_atoms_to_fact_set(state, repository, translation_context, workspace);
    insert_numeric_variables_to_fact_set(state, repository, translation_context, workspace);
    workspace.facts.assignment_sets.insert(workspace.facts.fact_sets);
}

template<TaskKind Kind, datalog::AnnotationPolicyConcept AP, datalog::TerminationPolicyConcept TP, datalog::RuleCostPolicyConcept CP>
void insert_unextended_state(const ygg::Builder<State<Kind>>& state,
                             const formalism::planning::Repository& repository,
                             const P2DTranslationContext<Kind>& translation_context,
                             datalog::ProgramWorkspace<Kind, AP, TP, CP>& workspace)
{
    workspace.facts.reset();
    insert_fluent_atoms_to_fact_set(state, repository, translation_context, workspace);
    insert_numeric_variables_to_fact_set(state, repository, translation_context, workspace);
    if constexpr (std::same_as<Kind, LiftedTag>)
        workspace.facts.assignment_sets.insert(workspace.facts.fact_sets);
}

template<datalog::AnnotationPolicyConcept AP, datalog::TerminationPolicyConcept TP, datalog::RuleCostPolicyConcept CP>
void read_derived_atoms_from_fact_set(ygg::Builder<State<LiftedTag>>& state,
                                      formalism::planning::Repository& repository,
                                      const D2PTranslationContext<LiftedTag>& translation_context,
                                      datalog::ProgramWorkspace<LiftedTag, AP, TP, CP>& workspace,
                                      std::vector<formalism::datalog::PredicateBindingView<formalism::FluentTag>>& derived_bindings)
{
    detail::read_derived_atoms_from_fact_set(state,
                                             repository,
                                             translation_context.fluent_to_derived_predicate,
                                             workspace.planning_builder,
                                             workspace.facts.fact_sets,
                                             derived_bindings);
}

}

#endif
