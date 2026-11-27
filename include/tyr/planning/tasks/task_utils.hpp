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

#ifndef TYR_PLANNING_TASKS_TASK_UTILS_HPP_
#define TYR_PLANNING_TASKS_TASK_UTILS_HPP_

#include "tyr/analysis/declarations.hpp"
#include "tyr/datalog/declarations.hpp"
#include "tyr/datalog/fact_sets.hpp"
#include "tyr/datalog/policies/annotation_concepts.hpp"
#include "tyr/datalog/policies/cost_concepts.hpp"
#include "tyr/datalog/policies/termination_concepts.hpp"
#include "tyr/formalism/repository.hpp"
#include "tyr/planning/successors/applicability_decl.hpp"
#include "tyr/planning/programs/state_inputs.hpp"
#include "tyr/planning/states/state_builder.hpp"
#include "tyr/planning/states/state_repository.hpp"
#include "tyr/planning/tasks/task.hpp"

#include <concepts>
#include <cstddef>
#include <optional>
#include <urd/elements/copy.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <utility>
#include <vector>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/formalism/copy.hpp>

namespace tyr::planning
{

/// The ground atom of a row of `repository`'s, where it holds one.
template<formalism::FactKind F>
std::optional<formalism::AtomView<GroundTag, F>> find_ground_atom(formalism::PredicateRowView<F> row, const formalism::Repository& repository)
{
    auto atom = urd::data<formalism::Atom<GroundTag, F>> {};
    atom.row = row.index();
    return repository.find(atom);
}

/// The number of the task's derived predicates: the program's own follow them (dense indices).
std::size_t num_task_derived_predicates(formalism::TaskView<LiftedTag> task);

/// Inserts into caller-prepared fact storage; reset facts first when replacing a state. The state's rows are the ground
/// program's: it is a layer over the ground task's repository.
/// ProgramExecutionContext populates assignment indices after insertion.
template<datalog::AnnotationPolicyConcept AP, datalog::TerminationPolicyConcept TP, datalog::RuleCostPolicyConcept CP>
void insert_unextended_state(const StateContext<GroundTag>& state,
                             const formalism::Repository& repository,
                             datalog::ProgramWorkspace<GroundTag, AP, TP, CP>& workspace)
{
    auto& fact_sets = workspace.facts.fact_sets.template get<formalism::FluentTag>();
    for (const auto atom : state.fact_storage().fluent_atoms())
        datalog::insert(fact_sets, urd::resolve(atom, repository).get_row().index());
    for (const auto& [fterm, value] : state.numeric_storage().fterm_values())
        datalog::insert(fact_sets, urd::resolve(fterm, repository).get_row().index(), ygg::ClosedInterval<ygg::float_t>(value, value));
}

/// Inserts into caller-prepared fact storage; reset facts first when replacing a state. Each state row the program reads
/// is stored in the workspace layer (the state's own layer is a sibling of the program's).
/// ProgramExecutionContext populates lifted assignment indices after insertion.
template<StateInputsConcept R, datalog::AnnotationPolicyConcept AP, datalog::TerminationPolicyConcept TP, datalog::RuleCostPolicyConcept CP>
void insert_unextended_state(const StateContext<LiftedTag>& state,
                             const formalism::Repository& repository,
                             const R& inputs,
                             datalog::ProgramWorkspace<LiftedTag, AP, TP, CP>& workspace)
{
    auto& fact_sets = workspace.facts.fact_sets.template get<formalism::FluentTag>();
    auto& workspace_repository = *workspace.workspace_repository;
    for (const auto atom_index : state.fact_storage().fluent_atoms())
    {
        const auto atom = urd::resolve(atom_index, repository);
        if (inputs.reads(atom.get_predicate()))
            datalog::insert(fact_sets, urd::copy(atom.get_row(), workspace_repository, workspace.datalog_builder));
    }
    for (const auto& [fterm_index, value] : state.numeric_storage().fterm_values())
    {
        const auto fterm = urd::resolve(fterm_index, repository);
        if (inputs.reads(fterm.get_function()))
            datalog::insert(fact_sets,
                            urd::copy(fterm.get_row(), workspace_repository, workspace.datalog_builder),
                            ygg::ClosedInterval<ygg::float_t>(value, value));
    }
}

/// insert_unextended_state, and the derived atoms.
template<StateInputsConcept R, datalog::AnnotationPolicyConcept AP, datalog::TerminationPolicyConcept TP, datalog::RuleCostPolicyConcept CP>
void insert_extended_state(const StateContext<LiftedTag>& state,
                           const formalism::Repository& repository,
                           const R& inputs,
                           datalog::ProgramWorkspace<LiftedTag, AP, TP, CP>& workspace)
{
    insert_unextended_state(state, repository, inputs, workspace);
    auto& fact_sets = workspace.facts.fact_sets.template get<formalism::DerivedTag>();
    auto& workspace_repository = *workspace.workspace_repository;
    for (const auto atom_index : state.atom_storage().atoms())
    {
        const auto atom = urd::resolve(atom_index, repository);
        if (inputs.reads(atom.get_predicate()))
            datalog::insert(fact_sets, urd::copy(atom.get_row(), workspace_repository, workspace.datalog_builder));
    }
}

/// Sets the derived atoms the program derived in the state: each a row of the task's (the program is a layer over the
/// task's repository), its atom.
template<datalog::AnnotationPolicyConcept AP, datalog::TerminationPolicyConcept TP, datalog::RuleCostPolicyConcept CP>
void read_derived_atoms_from_fact_set(urd::builder<State<GroundTag>, StateRepository<GroundTag>>& state,
                                      const formalism::Repository& repository,
                                      const datalog::ProgramWorkspace<GroundTag, AP, TP, CP>& workspace)
{
    const auto& fact_sets = workspace.facts.fact_sets.template get<formalism::DerivedTag>();
    for (ygg::uint_t i = 0; i < fact_sets.predicate.size(); ++i)
        for (const auto row : datalog::rows(fact_sets, i))
            state.set(find_ground_atom(row, repository).value());
}

/// Sets the derived atoms of the task's derived predicates (the first `num_task_derived_predicates`) the program
/// derived in the state; `derived_bindings` is scratch.
template<datalog::AnnotationPolicyConcept AP, datalog::TerminationPolicyConcept TP, datalog::RuleCostPolicyConcept CP>
void read_derived_atoms_from_fact_set(urd::builder<State<LiftedTag>, StateRepository<LiftedTag>>& state,
                                      formalism::Repository& repository,
                                      std::size_t num_task_derived_predicates,
                                      datalog::ProgramWorkspace<LiftedTag, AP, TP, CP>& workspace,
                                      urd::Store<urd::sequence<formalism::PredicateRowView<formalism::DerivedTag>>>& derived_bindings)
{
    const auto& fact_sets = workspace.facts.fact_sets.template get<formalism::DerivedTag>();
    derived_bindings.clear();
    for (std::size_t i = 0; i < num_task_derived_predicates && i < fact_sets.predicate.size(); ++i)
        for (const auto binding : datalog::rows(fact_sets, static_cast<ygg::uint_t>(i)))
            derived_bindings.push_back(binding);

    for (const auto binding : derived_bindings)
    {
        auto atom = formalism::checkout<formalism::Atom<GroundTag, formalism::DerivedTag>>(workspace.planning_builder);
        urd::store(atom->row, urd::copy(binding, repository, workspace.planning_builder));
        state.set(repository.insert(*atom, workspace.planning_builder).first);
    }
}

}

#endif
