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

#ifndef TYR_FORMALISM_PLANNING_TASK_BUILDER_HPP_
#define TYR_FORMALISM_PLANNING_TASK_BUILDER_HPP_

#include "tyr/formalism/atom_builder.hpp"
#include "tyr/formalism/function_term_value_builder.hpp"
#include "tyr/formalism/object_builder.hpp"
#include "tyr/formalism/planning/action_builder.hpp"
#include "tyr/formalism/planning/axiom_builder.hpp"
#include "tyr/formalism/planning/conjunctive_condition_builder.hpp"
#include "tyr/formalism/planning/domain_builder.hpp"
#include "tyr/formalism/planning/fdr_fact_builder.hpp"
#include "tyr/formalism/planning/fdr_variable_builder.hpp"
#include "tyr/formalism/planning/metric_builder.hpp"
#include "tyr/formalism/planning/task_data.hpp"
#include "tyr/formalism/planning/task_view.hpp"
#include "tyr/formalism/predicate_builder.hpp"

#include <optional>
#include <urd/elements/builder.hpp>
#include <variant>
#include <vector>

namespace urd
{

/// The builder of Task: its record's members; each child built or interned (urd::child).
template<typename C>
struct builder<::tyr::formalism::planning::Task<::tyr::LiftedTag>, C>
{
    urd::index<::tyr::formalism::planning::Task<::tyr::LiftedTag>> index;
    urd::flat_string name;
    urd::child<::tyr::formalism::planning::Domain, C> domain;
    std::vector<urd::child<::tyr::formalism::Predicate<::tyr::formalism::DerivedTag>, C>> derived_predicates;
    std::vector<urd::child<::tyr::formalism::Object, C>> objects;
    std::vector<urd::child<::tyr::formalism::Atom<::tyr::GroundTag, ::tyr::formalism::StaticTag>, C>> static_atoms;
    std::vector<urd::child<::tyr::formalism::Atom<::tyr::GroundTag, ::tyr::formalism::FluentTag>, C>> fluent_atoms;
    std::vector<urd::child<::tyr::formalism::FunctionTermValue<::tyr::GroundTag, ::tyr::formalism::StaticTag>, C>> static_fterm_values;
    std::vector<urd::child<::tyr::formalism::FunctionTermValue<::tyr::GroundTag, ::tyr::formalism::FluentTag>, C>> fluent_fterm_values;
    std::optional<urd::child<::tyr::formalism::FunctionTermValue<::tyr::GroundTag, ::tyr::formalism::AuxiliaryTag>, C>> auxiliary_fterm_value;
    urd::child<::tyr::formalism::planning::ConjunctiveCondition<::tyr::GroundTag>, C> goal;
    std::optional<urd::child<::tyr::formalism::planning::Metric, C>> metric;
    std::vector<urd::child<::tyr::formalism::planning::Axiom<::tyr::LiftedTag>, C>> axioms;

    static constexpr auto fields() noexcept
    {
        return urd::Fields<urd::field<"name", &builder::name>,
                           urd::field<"domain", &builder::domain>,
                           urd::field<"derived_predicates", &builder::derived_predicates>,
                           urd::field<"objects", &builder::objects>,
                           urd::field<"static_atoms", &builder::static_atoms>,
                           urd::field<"fluent_atoms", &builder::fluent_atoms>,
                           urd::field<"static_fterm_values", &builder::static_fterm_values>,
                           urd::field<"fluent_fterm_values", &builder::fluent_fterm_values>,
                           urd::field<"auxiliary_fterm_value", &builder::auxiliary_fterm_value>,
                           urd::field<"goal", &builder::goal>,
                           urd::field<"metric", &builder::metric>,
                           urd::field<"axioms", &builder::axioms>> {};
    }

    static constexpr auto identifying_members() noexcept
    {
        return urd::Members<&builder::name,
                            &builder::domain,
                            &builder::derived_predicates,
                            &builder::objects,
                            &builder::static_atoms,
                            &builder::fluent_atoms,
                            &builder::static_fterm_values,
                            &builder::fluent_fterm_values,
                            &builder::auxiliary_fterm_value,
                            &builder::goal,
                            &builder::metric,
                            &builder::axioms> {};
    }
};

/// The builder of Task: its record's members; each child built or interned (urd::child).
template<typename C>
struct builder<::tyr::formalism::planning::Task<::tyr::GroundTag>, C>
{
    urd::index<::tyr::formalism::planning::Task<::tyr::GroundTag>> index;
    urd::flat_string name;
    urd::child<::tyr::formalism::planning::Domain, C> domain;
    std::vector<urd::child<::tyr::formalism::Predicate<::tyr::formalism::DerivedTag>, C>> derived_predicates;
    std::vector<urd::child<::tyr::formalism::Object, C>> objects;
    std::vector<urd::child<::tyr::formalism::Atom<::tyr::GroundTag, ::tyr::formalism::StaticTag>, C>> static_atoms;
    std::vector<urd::child<::tyr::formalism::Atom<::tyr::GroundTag, ::tyr::formalism::FluentTag>, C>> fluent_atoms;
    std::vector<urd::child<::tyr::formalism::Atom<::tyr::GroundTag, ::tyr::formalism::DerivedTag>, C>> derived_atoms;
    std::vector<urd::child<::tyr::formalism::FunctionTermValue<::tyr::GroundTag, ::tyr::formalism::StaticTag>, C>> static_fterm_values;
    std::vector<urd::child<::tyr::formalism::FunctionTermValue<::tyr::GroundTag, ::tyr::formalism::FluentTag>, C>> fluent_fterm_values;
    std::optional<urd::child<::tyr::formalism::FunctionTermValue<::tyr::GroundTag, ::tyr::formalism::AuxiliaryTag>, C>> auxiliary_fterm_value;
    urd::child<::tyr::formalism::planning::ConjunctiveCondition<::tyr::GroundTag>, C> goal;
    std::optional<urd::child<::tyr::formalism::planning::Metric, C>> metric;
    std::vector<urd::child<::tyr::formalism::planning::Axiom<::tyr::LiftedTag>, C>> axioms;
    std::vector<urd::child<::tyr::formalism::planning::FDRVariable<::tyr::formalism::FluentTag>, C>> fluent_variables;
    std::vector<urd::builder<::tyr::formalism::planning::FDRFact<::tyr::formalism::FluentTag>, C>> fluent_facts;
    std::vector<urd::child<::tyr::formalism::planning::Action<::tyr::GroundTag>, C>> ground_actions;
    std::vector<urd::child<::tyr::formalism::planning::Axiom<::tyr::GroundTag>, C>> ground_axioms;

    static constexpr auto fields() noexcept
    {
        return urd::Fields<urd::field<"name", &builder::name>,
                           urd::field<"domain", &builder::domain>,
                           urd::field<"derived_predicates", &builder::derived_predicates>,
                           urd::field<"objects", &builder::objects>,
                           urd::field<"static_atoms", &builder::static_atoms>,
                           urd::field<"fluent_atoms", &builder::fluent_atoms>,
                           urd::field<"derived_atoms", &builder::derived_atoms>,
                           urd::field<"static_fterm_values", &builder::static_fterm_values>,
                           urd::field<"fluent_fterm_values", &builder::fluent_fterm_values>,
                           urd::field<"auxiliary_fterm_value", &builder::auxiliary_fterm_value>,
                           urd::field<"goal", &builder::goal>,
                           urd::field<"metric", &builder::metric>,
                           urd::field<"axioms", &builder::axioms>,
                           urd::field<"fluent_variables", &builder::fluent_variables>,
                           urd::field<"fluent_facts", &builder::fluent_facts>,
                           urd::field<"ground_actions", &builder::ground_actions>,
                           urd::field<"ground_axioms", &builder::ground_axioms>> {};
    }

    static constexpr auto identifying_members() noexcept { return urd::members_of(fields()); }
};

}  // namespace urd

#endif
