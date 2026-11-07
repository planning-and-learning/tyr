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

#ifndef TYR_FORMALISM_PLANNING_TASK_DATA_HPP_
#define TYR_FORMALISM_PLANNING_TASK_DATA_HPP_

#include "tyr/formalism/declarations.hpp"
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include "tyr/formalism/planning/action_index.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/fdr_fact_data.hpp"

#include <optional>
#include <string>
#include <tuple>
#include <utility>
#include <vector>
#include <urd/elements/flat.hpp>
#include <urd/repr/core/dependent_false.hpp>

namespace urd
{

template<>
struct data<::tyr::formalism::planning::Task<::tyr::LiftedTag>>
{
    urd::index<::tyr::formalism::planning::Task<::tyr::LiftedTag>> index;
    urd::flat_string name;
    urd::index<::tyr::formalism::planning::Domain> domain;
    urd::flat_vector<urd::index<::tyr::formalism::Predicate<::tyr::formalism::DerivedTag>>> derived_predicates;
    urd::flat_vector<urd::index<::tyr::formalism::Object>> objects;
    urd::flat_vector<urd::index<::tyr::formalism::planning::Atom<::tyr::GroundTag, ::tyr::formalism::StaticTag>>> static_atoms;
    urd::flat_vector<urd::index<::tyr::formalism::planning::Atom<::tyr::GroundTag, ::tyr::formalism::FluentTag>>> fluent_atoms;
    urd::flat_vector<urd::index<::tyr::formalism::planning::FunctionTermValue<::tyr::GroundTag, ::tyr::formalism::StaticTag>>> static_fterm_values;
    urd::flat_vector<urd::index<::tyr::formalism::planning::FunctionTermValue<::tyr::GroundTag, ::tyr::formalism::FluentTag>>> fluent_fterm_values;
    urd::flat_optional<urd::index<::tyr::formalism::planning::FunctionTermValue<::tyr::GroundTag, ::tyr::formalism::AuxiliaryTag>>> auxiliary_fterm_value;
    urd::index<::tyr::formalism::planning::ConjunctiveCondition<::tyr::GroundTag>> goal;
    urd::flat_optional<urd::index<::tyr::formalism::planning::Metric>> metric;
    urd::flat_vector<urd::index<::tyr::formalism::planning::Axiom<::tyr::LiftedTag>>> axioms;

    data() = default;
    data(urd::flat_string name_,
         urd::index<::tyr::formalism::planning::Domain> domain_,
         urd::flat_vector<urd::index<::tyr::formalism::Predicate<::tyr::formalism::DerivedTag>>> derived_predicates_,
         urd::flat_vector<urd::index<::tyr::formalism::Object>> objects_,
         urd::flat_vector<urd::index<::tyr::formalism::planning::Atom<::tyr::GroundTag, ::tyr::formalism::StaticTag>>> static_atoms_,
         urd::flat_vector<urd::index<::tyr::formalism::planning::Atom<::tyr::GroundTag, ::tyr::formalism::FluentTag>>> fluent_atoms_,
         urd::flat_vector<urd::index<::tyr::formalism::planning::FunctionTermValue<::tyr::GroundTag, ::tyr::formalism::StaticTag>>> static_fterm_values_,
         urd::flat_vector<urd::index<::tyr::formalism::planning::FunctionTermValue<::tyr::GroundTag, ::tyr::formalism::FluentTag>>> fluent_fterm_values_,
         urd::flat_optional<urd::index<::tyr::formalism::planning::FunctionTermValue<::tyr::GroundTag, ::tyr::formalism::AuxiliaryTag>>> auxiliary_fterm_value_,
         urd::index<::tyr::formalism::planning::ConjunctiveCondition<::tyr::GroundTag>> goal_,
         urd::flat_optional<urd::index<::tyr::formalism::planning::Metric>> metric_,
         urd::flat_vector<urd::index<::tyr::formalism::planning::Axiom<::tyr::LiftedTag>>> axioms_) :
        index(),
        name(std::move(name_)),
        domain(domain_),
        derived_predicates(std::move(derived_predicates_)),
        objects(std::move(objects_)),
        static_atoms(std::move(static_atoms_)),
        fluent_atoms(std::move(fluent_atoms_)),
        static_fterm_values(std::move(static_fterm_values_)),
        fluent_fterm_values(std::move(fluent_fterm_values_)),
        auxiliary_fterm_value(std::move(auxiliary_fterm_value_)),
        goal(goal_),
        metric(std::move(metric_)),
        axioms(std::move(axioms_))
    {
    }
    data(const data& other) = delete;
    data& operator=(const data& other) = delete;
    data(data&& other) noexcept = default;
    data& operator=(data&& other) noexcept = default;

    template<::tyr::formalism::FactKind T>
    [[nodiscard]] const auto& get_atoms() const
    { return ::tyr::formalism::tagged_member<::tyr::formalism::StaticFluentTags, T, &data::static_atoms, &data::fluent_atoms>(*this); }

    template<::tyr::formalism::FactKind T>
    [[nodiscard]] const auto& get_fterm_values() const
    { return ::tyr::formalism::tagged_member<::tyr::formalism::StaticFluentTags, T, &data::static_fterm_values, &data::fluent_fterm_values>(*this); }

    static constexpr auto cista_members() noexcept
    {
        return urd::Members<&data::index,
                            &data::name,
                            &data::domain,
                            &data::derived_predicates,
                            &data::objects,
                            &data::static_atoms,
                            &data::fluent_atoms,
                            &data::static_fterm_values,
                            &data::fluent_fterm_values,
                            &data::auxiliary_fterm_value,
                            &data::goal,
                            &data::metric,
                            &data::axioms> {};
    }
    static constexpr auto identifying_members() noexcept
    {
        return urd::Members<&data::name,
                            &data::domain,
                            &data::derived_predicates,
                            &data::objects,
                            &data::static_atoms,
                            &data::fluent_atoms,
                            &data::static_fterm_values,
                            &data::fluent_fterm_values,
                            &data::auxiliary_fterm_value,
                            &data::goal,
                            &data::metric,
                            &data::axioms> {};
    }
};

template<>
struct data<::tyr::formalism::planning::Task<::tyr::GroundTag>>
{
    urd::index<::tyr::formalism::planning::Task<::tyr::GroundTag>> index;
    urd::flat_string name;
    urd::index<::tyr::formalism::planning::Domain> domain;
    urd::flat_vector<urd::index<::tyr::formalism::Predicate<::tyr::formalism::DerivedTag>>> derived_predicates;
    urd::flat_vector<urd::index<::tyr::formalism::Object>> objects;
    urd::flat_vector<urd::index<::tyr::formalism::planning::Atom<::tyr::GroundTag, ::tyr::formalism::StaticTag>>> static_atoms;
    urd::flat_vector<urd::index<::tyr::formalism::planning::Atom<::tyr::GroundTag, ::tyr::formalism::FluentTag>>> fluent_atoms;
    urd::flat_vector<urd::index<::tyr::formalism::planning::Atom<::tyr::GroundTag, ::tyr::formalism::DerivedTag>>> derived_atoms;
    urd::flat_vector<urd::index<::tyr::formalism::planning::FunctionTermValue<::tyr::GroundTag, ::tyr::formalism::StaticTag>>> static_fterm_values;
    urd::flat_vector<urd::index<::tyr::formalism::planning::FunctionTermValue<::tyr::GroundTag, ::tyr::formalism::FluentTag>>> fluent_fterm_values;
    urd::flat_optional<urd::index<::tyr::formalism::planning::FunctionTermValue<::tyr::GroundTag, ::tyr::formalism::AuxiliaryTag>>> auxiliary_fterm_value;
    urd::flat_optional<urd::index<::tyr::formalism::planning::Metric>> metric;
    urd::flat_vector<urd::index<::tyr::formalism::planning::Axiom<::tyr::LiftedTag>>> axioms;

    /// FDR-related
    urd::flat_vector<urd::index<::tyr::formalism::planning::FDRVariable<::tyr::formalism::FluentTag>>> fluent_variables;
    urd::flat_vector<urd::data<::tyr::formalism::planning::FDRFact<::tyr::formalism::FluentTag>>> fluent_facts;
    urd::index<::tyr::formalism::planning::ConjunctiveCondition<::tyr::GroundTag>> goal;
    urd::flat_vector<urd::index<::tyr::formalism::planning::Action<::tyr::GroundTag>>> ground_actions;
    urd::flat_vector<urd::index<::tyr::formalism::planning::Axiom<::tyr::GroundTag>>> ground_axioms;

    data() = default;
    data(urd::flat_string name_,
         urd::index<::tyr::formalism::planning::Domain> domain_,
         urd::flat_vector<urd::index<::tyr::formalism::Predicate<::tyr::formalism::DerivedTag>>> derived_predicates_,
         urd::flat_vector<urd::index<::tyr::formalism::Object>> objects_,
         urd::flat_vector<urd::index<::tyr::formalism::planning::Atom<::tyr::GroundTag, ::tyr::formalism::StaticTag>>> static_atoms_,
         urd::flat_vector<urd::index<::tyr::formalism::planning::Atom<::tyr::GroundTag, ::tyr::formalism::FluentTag>>> fluent_atoms_,
         urd::flat_vector<urd::index<::tyr::formalism::planning::Atom<::tyr::GroundTag, ::tyr::formalism::DerivedTag>>> derived_atoms_,
         urd::flat_vector<urd::index<::tyr::formalism::planning::FunctionTermValue<::tyr::GroundTag, ::tyr::formalism::StaticTag>>> static_fterm_values_,
         urd::flat_vector<urd::index<::tyr::formalism::planning::FunctionTermValue<::tyr::GroundTag, ::tyr::formalism::FluentTag>>> fluent_fterm_values_,
         urd::flat_optional<urd::index<::tyr::formalism::planning::FunctionTermValue<::tyr::GroundTag, ::tyr::formalism::AuxiliaryTag>>> auxiliary_fterm_value_,
         urd::flat_optional<urd::index<::tyr::formalism::planning::Metric>> metric_,
         urd::flat_vector<urd::index<::tyr::formalism::planning::Axiom<::tyr::LiftedTag>>> axioms_,
         urd::flat_vector<urd::index<::tyr::formalism::planning::FDRVariable<::tyr::formalism::FluentTag>>> fluent_variables_,
         urd::flat_vector<urd::data<::tyr::formalism::planning::FDRFact<::tyr::formalism::FluentTag>>> fluent_facts_,
         urd::index<::tyr::formalism::planning::ConjunctiveCondition<::tyr::GroundTag>> goal_,
         urd::flat_vector<urd::index<::tyr::formalism::planning::Action<::tyr::GroundTag>>> ground_actions_,
         urd::flat_vector<urd::index<::tyr::formalism::planning::Axiom<::tyr::GroundTag>>> ground_axioms_) :
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
        auxiliary_fterm_value(std::move(auxiliary_fterm_value_)),
        metric(std::move(metric_)),
        axioms(std::move(axioms_)),
        fluent_variables(std::move(fluent_variables_)),
        fluent_facts(std::move(fluent_facts_)),
        goal(goal_),
        ground_actions(std::move(ground_actions_)),
        ground_axioms(std::move(ground_axioms_))
    {
    }
    data(const data& other) = delete;
    data& operator=(const data& other) = delete;
    data(data&& other) noexcept = default;
    data& operator=(data&& other) noexcept = default;

    template<::tyr::formalism::FactKind T>
    [[nodiscard]] const auto& get_atoms() const
    { return ::tyr::formalism::tagged_member<::tyr::formalism::StaticFluentDerivedTags, T, &data::static_atoms, &data::fluent_atoms, &data::derived_atoms>(*this); }

    template<::tyr::formalism::FactKind T>
    [[nodiscard]] const auto& get_fterm_values() const
    { return ::tyr::formalism::tagged_member<::tyr::formalism::StaticFluentTags, T, &data::static_fterm_values, &data::fluent_fterm_values>(*this); }

    [[nodiscard]] const auto& get_fluent_variables() const { return fluent_variables; }

    static constexpr auto cista_members() noexcept
    {
        return urd::Members<&data::index,
                            &data::name,
                            &data::domain,
                            &data::derived_predicates,
                            &data::objects,
                            &data::static_atoms,
                            &data::fluent_atoms,
                            &data::derived_atoms,
                            &data::static_fterm_values,
                            &data::fluent_fterm_values,
                            &data::auxiliary_fterm_value,
                            &data::metric,
                            &data::axioms,
                            &data::fluent_variables,
                            &data::fluent_facts,
                            &data::goal,
                            &data::ground_actions,
                            &data::ground_axioms> {};
    }
    static constexpr auto identifying_members() noexcept
    {
        return urd::Members<&data::name,
                            &data::domain,
                            &data::derived_predicates,
                            &data::objects,
                            &data::static_atoms,
                            &data::fluent_atoms,
                            &data::derived_atoms,
                            &data::static_fterm_values,
                            &data::fluent_fterm_values,
                            &data::auxiliary_fterm_value,
                            &data::metric,
                            &data::axioms,
                            &data::fluent_variables,
                            &data::fluent_facts,
                            &data::goal,
                            &data::ground_actions,
                            &data::ground_axioms> {};
    }
};

}

#endif
