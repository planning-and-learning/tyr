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
#include "tyr/datalog/fact_sets.hpp"
#include "tyr/datalog/policies/annotation_concepts.hpp"
#include "tyr/datalog/policies/cost_concepts.hpp"
#include "tyr/datalog/policies/termination_concepts.hpp"
#include "tyr/formalism/copy.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/planning/interning.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/planning/applicability_decl.hpp"
#include "tyr/planning/programs/state_inputs.hpp"
#include "tyr/planning/state_builder.hpp"
#include "tyr/planning/task.hpp"

#include <concepts>
#include <cstddef>
#include <optional>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <utility>
#include <vector>
#include <yggdrasil/core/config.hpp>

namespace tyr::planning
{

/// A program row's ground atom in `to` (the task's repository or a layer over it), interned there with its row.
template<formalism::FactKind F>
std::pair<formalism::planning::AtomView<GroundTag, F>, bool>
copy_ground_atom(formalism::datalog::PredicateRowView<F> row, formalism::planning::Builder& builder, formalism::planning::Repository& to)
{
    auto atom = formalism::planning::checkout<formalism::Atom<GroundTag, F>>(builder);
    urd::store(atom->row, urd::resolve(formalism::copy_row(row, to).first, to));
    return to.insert(*atom, builder);
}

/// The ground atom of a row of `repository`'s, where it holds one.
template<formalism::FactKind F>
std::optional<formalism::planning::AtomView<GroundTag, F>> find_ground_atom(formalism::datalog::PredicateRowView<F> row,
                                                                            const formalism::planning::Repository& repository)
{
    auto atom = urd::data<formalism::Atom<GroundTag, F>> {};
    atom.row = row.index();
    return repository.find(atom);
}

/// The number of the task's derived predicates: the program's own follow them (dense indices).
std::size_t num_task_derived_predicates(formalism::planning::TaskView<LiftedTag> task);

/// Inserts into caller-prepared fact storage; reset facts first when replacing a state. The state's rows are the ground
/// program's: it is a layer over the ground task's repository.
/// ProgramExecutionContext populates assignment indices after insertion.
template<datalog::AnnotationPolicyConcept AP, datalog::TerminationPolicyConcept TP, datalog::RuleCostPolicyConcept CP>
void insert_unextended_state(const StateContext<GroundTag>& state,
                             const formalism::planning::Repository& repository,
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
                             const formalism::planning::Repository& repository,
                             const R& inputs,
                             datalog::ProgramWorkspace<LiftedTag, AP, TP, CP>& workspace)
{
    auto& fact_sets = workspace.facts.fact_sets.template get<formalism::FluentTag>();
    auto& workspace_repository = *workspace.workspace_repository;
    for (const auto atom_index : state.fact_storage().fluent_atoms())
    {
        const auto atom = urd::resolve(atom_index, repository);
        if (inputs.reads(atom.get_predicate()))
            datalog::insert(fact_sets, formalism::copy_row(atom.get_row(), workspace_repository).first);
    }
    for (const auto& [fterm_index, value] : state.numeric_storage().fterm_values())
    {
        const auto fterm = urd::resolve(fterm_index, repository);
        if (inputs.reads(fterm.get_function()))
            datalog::insert(fact_sets, formalism::copy_row(fterm.get_row(), workspace_repository).first, ygg::ClosedInterval<ygg::float_t>(value, value));
    }
}

/// insert_unextended_state, and the derived atoms.
template<StateInputsConcept R, datalog::AnnotationPolicyConcept AP, datalog::TerminationPolicyConcept TP, datalog::RuleCostPolicyConcept CP>
void insert_extended_state(const StateContext<LiftedTag>& state,
                           const formalism::planning::Repository& repository,
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
            datalog::insert(fact_sets, formalism::copy_row(atom.get_row(), workspace_repository).first);
    }
}

/// Sets the derived atoms of the task's derived predicates (the first `num_task_derived_predicates`) the program
/// derived in the state; `derived_bindings` is scratch.
template<datalog::AnnotationPolicyConcept AP, datalog::TerminationPolicyConcept TP, datalog::RuleCostPolicyConcept CP>
void read_derived_atoms_from_fact_set(urd::builder<State<LiftedTag>, StateRepository<LiftedTag>>& state,
                                      formalism::planning::Repository& repository,
                                      std::size_t num_task_derived_predicates,
                                      datalog::ProgramWorkspace<LiftedTag, AP, TP, CP>& workspace,
                                      urd::Store<urd::sequence<formalism::datalog::PredicateRowView<formalism::DerivedTag>>>& derived_bindings)
{
    const auto& fact_sets = workspace.facts.fact_sets.template get<formalism::DerivedTag>();
    derived_bindings.clear();
    for (std::size_t i = 0; i < num_task_derived_predicates && i < fact_sets.predicate.size(); ++i)
        for (const auto binding : datalog::rows(fact_sets, static_cast<ygg::uint_t>(i)))
            derived_bindings.push_back(binding);

    for (const auto binding : derived_bindings)
        state.set(copy_ground_atom(binding, workspace.planning_builder, repository).first);
}

}

#endif
