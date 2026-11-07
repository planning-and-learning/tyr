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
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>

#include "tyr/analysis/declarations.hpp"
#include "tyr/datalog/fact_sets.hpp"
#include "tyr/formalism/datalog/copy.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/datalog/atom_view.hpp"
#include "tyr/formalism/datalog/binary_operator_view.hpp"
#include "tyr/formalism/datalog/conditional_effect_view.hpp"
#include "tyr/formalism/datalog/conjunctive_condition_view.hpp"
#include "tyr/formalism/datalog/conjunctive_effect_view.hpp"
#include "tyr/formalism/datalog/function_term_value_view.hpp"
#include "tyr/formalism/datalog/function_term_view.hpp"
#include "tyr/formalism/datalog/literal_view.hpp"
#include "tyr/formalism/datalog/multi_operator_view.hpp"
#include "tyr/formalism/datalog/numeric_effect_view.hpp"
#include "tyr/formalism/datalog/program_view.hpp"
#include "tyr/formalism/datalog/rule_view.hpp"
#include "tyr/formalism/datalog/unary_operator_view.hpp"
#include "tyr/formalism/declarations.hpp"
#include <yggdrasil/formalism/binding_view.hpp>
#include "tyr/formalism/function_view.hpp"
#include "tyr/formalism/object_view.hpp"
#include "tyr/formalism/parameter_index.hpp"
#include "tyr/formalism/predicate_view.hpp"
#include "tyr/formalism/variable_view.hpp"
#include "tyr/formalism/planning/merge_datalog.hpp"
#include "tyr/formalism/planning/merge_planning.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/planning/action_view.hpp"
#include "tyr/formalism/planning/atom_view.hpp"
#include "tyr/formalism/planning/axiom_view.hpp"
#include "tyr/formalism/planning/binary_operator_view.hpp"
#include "tyr/formalism/planning/conditional_effect_view.hpp"
#include "tyr/formalism/planning/conjunctive_condition_view.hpp"
#include "tyr/formalism/planning/conjunctive_effect_view.hpp"
#include "tyr/formalism/planning/domain_view.hpp"
#include "tyr/formalism/planning/fdr_fact_view.hpp"
#include "tyr/formalism/planning/fdr_variable_view.hpp"
#include "tyr/formalism/planning/function_term_value_view.hpp"
#include "tyr/formalism/planning/function_term_view.hpp"
#include "tyr/formalism/planning/invariant_view.hpp"
#include "tyr/formalism/planning/literal_view.hpp"
#include "tyr/formalism/planning/metric_view.hpp"
#include "tyr/formalism/planning/multi_operator_view.hpp"
#include "tyr/formalism/planning/numeric_effect_view.hpp"
#include "tyr/formalism/planning/task_view.hpp"
#include "tyr/formalism/planning/unary_operator_view.hpp"
#include "tyr/planning/state_builder.hpp"


namespace tyr::planning::detail
{

void insert_fluent_atoms_to_fact_set(const urd::data<State<GroundTag>>& state,
                                     const formalism::planning::Repository& repository,
                                     const P2DTranslationContext<GroundTag>::FluentToFluentAtomMapping& fluent_to_fluent_atom,
                                     datalog::TaggedFactSets<formalism::FluentTag>& fact_sets)
{
    for (const auto atom : state.get_atoms<formalism::FluentTag>(repository))
        fact_sets.predicate.insert(fluent_to_fluent_atom.at(urd::resolve(atom, repository)));
}

void insert_numeric_variables_to_fact_set(const urd::data<State<GroundTag>>& state,
                                          const formalism::planning::Repository& repository,
                                          const P2DTranslationContext<GroundTag>::FluentToFluentFunctionTermMapping& fluent_to_fluent_fterm,
                                          datalog::TaggedFactSets<formalism::FluentTag>& fact_sets)
{
    for (const auto& [fterm, value] : state.get_fterm_values<formalism::FluentTag>())
        if (const auto it = fluent_to_fluent_fterm.find(urd::resolve(fterm, repository)); it != fluent_to_fluent_fterm.end())
            fact_sets.function.insert(it->second, value);
}

void insert_fluent_atoms_to_fact_set(const urd::data<State<LiftedTag>>& state,
                                     const formalism::planning::Repository& repository,
                                     const P2DTranslationContext<LiftedTag>::FluentToFluentPredicateMapping& fluent_to_fluent_predicate,
                                     formalism::WriteContext<formalism::datalog::Builder, formalism::datalog::Repository>& merge_context,
                                     datalog::TaggedFactSets<formalism::FluentTag>& fact_sets)
{
    for (const auto atom_index : state.get_atoms<formalism::FluentTag>(repository))
    {
        const auto atom = urd::resolve(atom_index, repository);
        if (const auto it = fluent_to_fluent_predicate.find(atom.get_predicate()); it != fluent_to_fluent_predicate.end())
            fact_sets.predicate.insert(formalism::planning::merge_p2d(atom.get_row(), it->second, merge_context).first);
    }
}

void insert_derived_atoms_to_fact_set(const urd::data<State<LiftedTag>>& state,
                                      const formalism::planning::Repository& repository,
                                      const P2DTranslationContext<LiftedTag>::DerivedToFluentPredicateMapping& derived_to_fluent_predicate,
                                      formalism::WriteContext<formalism::datalog::Builder, formalism::datalog::Repository>& merge_context,
                                      datalog::TaggedFactSets<formalism::FluentTag>& fact_sets)
{
    for (const auto atom_index : state.get_atoms<formalism::DerivedTag>(repository))
    {
        const auto atom = urd::resolve(atom_index, repository);
        if (const auto it = derived_to_fluent_predicate.find(atom.get_predicate()); it != derived_to_fluent_predicate.end())
            fact_sets.predicate.insert(formalism::planning::merge_p2d(atom.get_row(), it->second, merge_context).first);
    }
}

void insert_numeric_variables_to_fact_set(const urd::data<State<LiftedTag>>& state,
                                          const formalism::planning::Repository& repository,
                                          const P2DTranslationContext<LiftedTag>::FluentToFluentFunctionMapping& fluent_to_fluent_function,
                                          formalism::WriteContext<formalism::datalog::Builder, formalism::datalog::Repository>& merge_context,
                                          datalog::TaggedFactSets<formalism::FluentTag>& fact_sets)
{
    for (const auto& [fterm_index, value] : state.get_fterm_values<formalism::FluentTag>())
    {
        const auto fterm = urd::resolve(fterm_index, repository);
        if (const auto it = fluent_to_fluent_function.find(fterm.get_function()); it != fluent_to_fluent_function.end())
            fact_sets.function.insert(formalism::planning::merge_p2d(fterm.get_row(), it->second, merge_context).first, value);
    }
}

void read_derived_atoms_from_fact_set(urd::builder<State<LiftedTag>>& state,
                                      formalism::planning::Repository& repository,
                                      const D2PTranslationContext<LiftedTag>::FluentToDerivedPredicateMapping& fluent_to_derived_predicate,
                                      formalism::planning::Builder& planning_builder,
                                      const datalog::TaggedFactSets<formalism::FluentTag>& fact_sets,
                                      urd::Store<urd::sequence<formalism::datalog::PredicateBindingView<formalism::FluentTag>>>& derived_bindings)
{
    derived_bindings.clear();
    for (const auto& set : fact_sets.predicate.get_sets())
        for (const auto binding : set.get_bindings())
            if (fluent_to_derived_predicate.contains(binding.get_relation()))
                derived_bindings.push_back(binding);

    auto merge_context = formalism::WriteContext { planning_builder, repository };
    for (const auto binding : derived_bindings)
        state.set(formalism::planning::merge_atom_d2p<formalism::FluentTag, formalism::DerivedTag>(binding, fluent_to_derived_predicate, merge_context).first);
}
}
