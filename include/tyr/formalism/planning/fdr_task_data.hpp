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

#ifndef TYR_FORMALISM_PLANNING_FDR_TASK_DATA_HPP_
#define TYR_FORMALISM_PLANNING_FDR_TASK_DATA_HPP_

#include <yggdrasil/core/types.hpp>
#include <yggdrasil/core/types_utils.hpp>
#include "tyr/formalism/object_index.hpp"
#include "tyr/formalism/planning/axiom_index.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/domain_index.hpp"
#include "tyr/formalism/planning/fdr_fact_data.hpp"
#include "tyr/formalism/planning/fdr_task_index.hpp"
#include "tyr/formalism/planning/fdr_variable_index.hpp"
#include "tyr/formalism/planning/ground_action_index.hpp"
#include "tyr/formalism/planning/ground_atom_index.hpp"
#include "tyr/formalism/planning/ground_axiom_index.hpp"
#include "tyr/formalism/planning/ground_conjunctive_condition_index.hpp"
#include "tyr/formalism/planning/ground_function_term_index.hpp"
#include "tyr/formalism/planning/ground_function_term_value_index.hpp"
#include "tyr/formalism/planning/metric_index.hpp"
#include "tyr/formalism/predicate_index.hpp"

namespace ygg
{


template<>
struct Data<::tyr::formalism::planning::FDRTask>
{
    ygg::Index<::tyr::formalism::planning::FDRTask> index;
    ::cista::offset::string name;
    ygg::Index<::tyr::formalism::planning::Domain> domain;
    ygg::IndexList<::tyr::formalism::Predicate<::tyr::formalism::DerivedTag>> derived_predicates;
    ygg::IndexList<::tyr::formalism::Object> objects;
    ygg::IndexList<::tyr::formalism::planning::GroundAtom<::tyr::formalism::StaticTag>> static_atoms;
    ygg::IndexList<::tyr::formalism::planning::GroundAtom<::tyr::formalism::FluentTag>> fluent_atoms;
    ygg::IndexList<::tyr::formalism::planning::GroundAtom<::tyr::formalism::DerivedTag>> derived_atoms;
    ygg::IndexList<::tyr::formalism::planning::GroundFunctionTermValue<::tyr::formalism::StaticTag>> static_fterm_values;
    ygg::IndexList<::tyr::formalism::planning::GroundFunctionTermValue<::tyr::formalism::FluentTag>> fluent_fterm_values;
    ::cista::optional<ygg::Index<::tyr::formalism::planning::GroundFunctionTermValue<::tyr::formalism::AuxiliaryTag>>> auxiliary_fterm_value;
    ::cista::optional<ygg::Index<::tyr::formalism::planning::Metric>> metric;
    ygg::IndexList<::tyr::formalism::planning::Axiom> axioms;

    /// FDR-related
    ygg::IndexList<::tyr::formalism::planning::FDRVariable<::tyr::formalism::FluentTag>> fluent_variables;
    ygg::DataList<::tyr::formalism::planning::FDRFact<::tyr::formalism::FluentTag>> fluent_facts;
    ygg::Index<::tyr::formalism::planning::GroundConjunctiveCondition> goal;
    ygg::IndexList<::tyr::formalism::planning::GroundAction> ground_actions;
    ygg::IndexList<::tyr::formalism::planning::GroundAxiom> ground_axioms;

    Data() = default;
    Data(::cista::offset::string name_,
         ygg::Index<::tyr::formalism::planning::Domain> domain_,
         ygg::IndexList<::tyr::formalism::Predicate<::tyr::formalism::DerivedTag>> derived_predicates_,
         ygg::IndexList<::tyr::formalism::Object> objects_,
         ygg::IndexList<::tyr::formalism::planning::GroundAtom<::tyr::formalism::StaticTag>> static_atoms_,
         ygg::IndexList<::tyr::formalism::planning::GroundAtom<::tyr::formalism::FluentTag>> fluent_atoms_,
         ygg::IndexList<::tyr::formalism::planning::GroundAtom<::tyr::formalism::DerivedTag>> derived_atoms_,
         ygg::IndexList<::tyr::formalism::planning::GroundFunctionTermValue<::tyr::formalism::StaticTag>> static_fterm_values_,
         ygg::IndexList<::tyr::formalism::planning::GroundFunctionTermValue<::tyr::formalism::FluentTag>> fluent_fterm_values_,
         ::cista::optional<ygg::Index<::tyr::formalism::planning::GroundFunctionTermValue<::tyr::formalism::AuxiliaryTag>>> auxiliary_fterm_value_,
         ::cista::optional<ygg::Index<::tyr::formalism::planning::Metric>> metric_,
         ygg::IndexList<::tyr::formalism::planning::Axiom> axioms_,
         ygg::IndexList<::tyr::formalism::planning::FDRVariable<::tyr::formalism::FluentTag>> fluent_variables_,
         ygg::DataList<::tyr::formalism::planning::FDRFact<::tyr::formalism::FluentTag>> fluent_facts_,
         ygg::Index<::tyr::formalism::planning::GroundConjunctiveCondition> goal_,
         ygg::IndexList<::tyr::formalism::planning::GroundAction> ground_actions_,
         ygg::IndexList<::tyr::formalism::planning::GroundAxiom> ground_axioms_) :
        index(),
        name(std::move(name_)),
        domain(domain_),
        derived_predicates(std::move(derived_predicates_)),
        objects(std::move(objects_)),
        static_atoms(std::move(static_atoms_)),
        fluent_atoms(std::move(fluent_atoms_)),
        derived_atoms(std::move(derived_atoms_)),
        static_fterm_values(std::move(static_fterm_values_)),
        fluent_fterm_values(std::move(fluent_fterm_values_)),
        auxiliary_fterm_value(auxiliary_fterm_value_),
        metric(metric_),
        axioms(std::move(axioms_)),
        fluent_variables(std::move(fluent_variables_)),
        fluent_facts(std::move(fluent_facts_)),
        goal(goal_),
        ground_actions(std::move(ground_actions_)),
        ground_axioms(std::move(ground_axioms_))
    {
    }
    // Python constructor
    template<typename C>
    Data(const std::string& name_,
         ::ygg::View<ygg::Index<::tyr::formalism::planning::Domain>, C> domain_,
         const std::vector<::ygg::View<ygg::Index<::tyr::formalism::Predicate<::tyr::formalism::DerivedTag>>, C>>& derived_predicates_,
         const std::vector<::ygg::View<ygg::Index<::tyr::formalism::Object>, C>>& objects_,
         const std::vector<::ygg::View<ygg::Index<::tyr::formalism::planning::GroundAtom<::tyr::formalism::StaticTag>>, C>>& static_atoms_,
         const std::vector<::ygg::View<ygg::Index<::tyr::formalism::planning::GroundAtom<::tyr::formalism::FluentTag>>, C>>& fluent_atoms_,
         const std::vector<::ygg::View<ygg::Index<::tyr::formalism::planning::GroundAtom<::tyr::formalism::DerivedTag>>, C>>& derived_atoms_,
         const std::vector<::ygg::View<ygg::Index<::tyr::formalism::planning::GroundFunctionTermValue<::tyr::formalism::StaticTag>>, C>>& static_fterm_values_,
         const std::vector<::ygg::View<ygg::Index<::tyr::formalism::planning::GroundFunctionTermValue<::tyr::formalism::FluentTag>>, C>>& fluent_fterm_values_,
         const std::optional<::ygg::View<ygg::Index<::tyr::formalism::planning::GroundFunctionTermValue<::tyr::formalism::AuxiliaryTag>>, C>>& auxiliary_fterm_value_,
         const std::optional<::ygg::View<ygg::Index<::tyr::formalism::planning::Metric>, C>>& metric_,
         const std::vector<::ygg::View<ygg::Index<::tyr::formalism::planning::Axiom>, C>>& axioms_,
         const std::vector<::ygg::View<ygg::Index<::tyr::formalism::planning::FDRVariable<::tyr::formalism::FluentTag>>, C>>& fluent_variables_,
         const std::vector<::ygg::View<ygg::Data<::tyr::formalism::planning::FDRFact<::tyr::formalism::FluentTag>>, C>>& fluent_facts_,
         ::ygg::View<ygg::Index<::tyr::formalism::planning::GroundConjunctiveCondition>, C> goal_,
         const std::vector<::ygg::View<ygg::Index<::tyr::formalism::planning::GroundAction>, C>>& ground_actions_,
         const std::vector<::ygg::View<ygg::Index<::tyr::formalism::planning::GroundAxiom>, C>>& ground_axioms_) :
        index(),
        name(name_),
        domain(),
        derived_predicates(),
        objects(),
        static_atoms(),
        fluent_atoms(),
        derived_atoms(),
        static_fterm_values(),
        fluent_fterm_values(),
        auxiliary_fterm_value(),
        metric(),
        axioms(),
        fluent_variables(),
        fluent_facts(),
        goal(),
        ground_actions(),
        ground_axioms()
    {
        set(domain_, domain);
        set(derived_predicates_, derived_predicates);
        set(objects_, objects);
        set(static_atoms_, static_atoms);
        set(fluent_atoms_, fluent_atoms);
        set(derived_atoms_, derived_atoms);
        set(static_fterm_values_, static_fterm_values);
        set(fluent_fterm_values_, fluent_fterm_values);
        set(auxiliary_fterm_value_, auxiliary_fterm_value);
        set(metric_, metric);
        set(axioms_, axioms);
        set(fluent_variables_, fluent_variables);
        set(fluent_facts_, fluent_facts);
        set(goal_, goal);
        set(ground_actions_, ground_actions);
        set(ground_axioms_, ground_axioms);
    }
    Data(const Data& other) = delete;
    Data& operator=(const Data& other) = delete;
    Data(Data&& other) = default;
    Data& operator=(Data&& other) = default;

    void clear() noexcept
    {
        ygg::clear(index);
        ygg::clear(name);
        ygg::clear(domain);
        ygg::clear(derived_predicates);
        ygg::clear(objects);
        ygg::clear(static_atoms);
        ygg::clear(fluent_atoms);
        ygg::clear(derived_atoms);
        ygg::clear(static_fterm_values);
        ygg::clear(fluent_fterm_values);
        ygg::clear(auxiliary_fterm_value);
        ygg::clear(metric);
        ygg::clear(axioms);
        ygg::clear(fluent_variables);
        ygg::clear(fluent_facts);
        ygg::clear(goal);
        ygg::clear(ground_actions);
        ygg::clear(ground_axioms);
    }

    template<::tyr::formalism::FactKind T>
    const auto& get_atoms() const
    {
        if constexpr (std::same_as<T, ::tyr::formalism::StaticTag>)
            return static_atoms;
        else if constexpr (std::same_as<T, ::tyr::formalism::FluentTag>)
            return fluent_atoms;
        else if constexpr (std::same_as<T, ::tyr::formalism::DerivedTag>)
            return derived_atoms;
        else
            static_assert(ygg::dependent_false<T>::value, "Missing case");
    }

    template<::tyr::formalism::FactKind T>
    const auto& get_fterm_values() const
    {
        if constexpr (std::same_as<T, ::tyr::formalism::StaticTag>)
            return static_fterm_values;
        else if constexpr (std::same_as<T, ::tyr::formalism::FluentTag>)
            return fluent_fterm_values;
        else
            static_assert(ygg::dependent_false<T>::value, "Missing case");
    }

    const auto& get_fluent_variables() const { return fluent_variables; }

    auto cista_members() const noexcept
    {
        return std::tie(index,
                        name,
                        domain,
                        derived_predicates,
                        objects,
                        static_atoms,
                        fluent_atoms,
                        derived_atoms,
                        static_fterm_values,
                        fluent_fterm_values,
                        auxiliary_fterm_value,
                        metric,
                        axioms,
                        fluent_variables,
                        fluent_facts,
                        goal,
                        ground_actions,
                        ground_axioms);
    }
    auto identifying_members() const noexcept
    {
        return std::tie(name,
                        domain,
                        derived_predicates,
                        objects,
                        static_atoms,
                        fluent_atoms,
                        derived_atoms,
                        static_fterm_values,
                        fluent_fterm_values,
                        auxiliary_fterm_value,
                        metric,
                        axioms,
                        fluent_variables,
                        fluent_facts,
                        goal,
                        ground_actions,
                        ground_axioms);
    }
};

static_assert(!ygg::uses_trivial_storage_v<::tyr::formalism::planning::FDRTask>);
}

#endif
