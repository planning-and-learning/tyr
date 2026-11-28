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

#include "tyr/declarations.hpp"
#include "tyr/formalism/atom_builder.hpp"
#include "tyr/formalism/conjunctive_condition_builder.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/enums.hpp"
#include "tyr/formalism/function_term_value_builder.hpp"
#include "tyr/formalism/object_builder.hpp"
#include "tyr/formalism/planning/action_builder.hpp"
#include "tyr/formalism/planning/axiom_builder.hpp"
#include "tyr/formalism/planning/domain_builder.hpp"
#include "tyr/formalism/planning/fdr_fact_builder.hpp"
#include "tyr/formalism/planning/fdr_fact_data.hpp"
#include "tyr/formalism/planning/fdr_variable_builder.hpp"
#include "tyr/formalism/planning/metric_builder.hpp"
#include "tyr/formalism/planning/task_data.hpp"
#include "tyr/formalism/planning/task_view.hpp"
#include "tyr/formalism/predicate_builder.hpp"

#include <urd/elements/builder.hpp>
#include <urd/elements/children.hpp>
#include <urd/elements/concepts.hpp>
#include <urd/elements/flat.hpp>
#include <urd/elements/index.hpp>

namespace urd
{

/// The builder of Task: its record's members, each in its built form (urd::built_t).
template<typename C>
struct builder<::tyr::formalism::Task<::tyr::LiftedTag>, C>
{
    urd::key<::tyr::formalism::Task<::tyr::LiftedTag>> index;
    urd::built_t<urd::flat_string, C> name;
    urd::built_t<urd::flat_vector<::tyr::formalism::RequirementKind>, C> requirements;
    urd::built_t<urd::index<::tyr::formalism::Domain>, C> domain;
    urd::built_t<urd::flat_vector<urd::index<::tyr::formalism::Predicate<::tyr::formalism::DerivedTag>>>, C> derived_predicates;
    urd::built_t<urd::flat_vector<urd::index<::tyr::formalism::Object>>, C> objects;
    urd::built_t<urd::flat_vector<urd::index<::tyr::formalism::Atom<::tyr::GroundTag, ::tyr::formalism::StaticTag>>>, C> static_atoms;
    urd::built_t<urd::flat_vector<urd::index<::tyr::formalism::Atom<::tyr::GroundTag, ::tyr::formalism::FluentTag>>>, C> fluent_atoms;
    urd::built_t<urd::flat_vector<urd::index<::tyr::formalism::FunctionTermValue<::tyr::GroundTag, ::tyr::formalism::StaticTag>>>, C> static_fterm_values;
    urd::built_t<urd::flat_vector<urd::index<::tyr::formalism::FunctionTermValue<::tyr::GroundTag, ::tyr::formalism::FluentTag>>>, C> fluent_fterm_values;
    urd::built_t<urd::flat_optional<urd::index<::tyr::formalism::FunctionTermValue<::tyr::GroundTag, ::tyr::formalism::AuxiliaryTag>>>, C>
        auxiliary_fterm_value;
    urd::built_t<urd::index<::tyr::formalism::ConjunctiveCondition<::tyr::GroundTag>>, C> goal;
    urd::built_t<urd::flat_optional<urd::index<::tyr::formalism::Metric>>, C> metric;
    urd::built_t<urd::flat_vector<urd::index<::tyr::formalism::Axiom<::tyr::LiftedTag>>>, C> axioms;
    urd::built_t<urd::flat_vector<urd::index<::tyr::formalism::TimedLiteral>>, C> timed_literals;

    static constexpr auto fields() noexcept
    {
        return urd::Fields<urd::field<"name", &builder::name>,
                           urd::field<"requirements", &builder::requirements>,
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
                           urd::field<"axioms", &builder::axioms>,
                           urd::field<"timed_literals", &builder::timed_literals>> {};
    }
};

/// The builder of Task: its record's members, each in its built form (urd::built_t).
template<typename C>
struct builder<::tyr::formalism::Task<::tyr::GroundTag>, C>
{
    urd::key<::tyr::formalism::Task<::tyr::GroundTag>> index;
    urd::built_t<urd::flat_string, C> name;
    urd::built_t<urd::flat_vector<::tyr::formalism::RequirementKind>, C> requirements;
    urd::built_t<urd::index<::tyr::formalism::Domain>, C> domain;
    urd::built_t<urd::flat_vector<urd::index<::tyr::formalism::Predicate<::tyr::formalism::DerivedTag>>>, C> derived_predicates;
    urd::built_t<urd::flat_vector<urd::index<::tyr::formalism::Object>>, C> objects;
    urd::built_t<urd::flat_vector<urd::index<::tyr::formalism::Atom<::tyr::GroundTag, ::tyr::formalism::StaticTag>>>, C> static_atoms;
    urd::built_t<urd::flat_vector<urd::index<::tyr::formalism::Atom<::tyr::GroundTag, ::tyr::formalism::FluentTag>>>, C> fluent_atoms;
    urd::built_t<urd::flat_vector<urd::index<::tyr::formalism::Atom<::tyr::GroundTag, ::tyr::formalism::DerivedTag>>>, C> derived_atoms;
    urd::built_t<urd::flat_vector<urd::index<::tyr::formalism::FunctionTermValue<::tyr::GroundTag, ::tyr::formalism::StaticTag>>>, C> static_fterm_values;
    urd::built_t<urd::flat_vector<urd::index<::tyr::formalism::FunctionTermValue<::tyr::GroundTag, ::tyr::formalism::FluentTag>>>, C> fluent_fterm_values;
    urd::built_t<urd::flat_optional<urd::index<::tyr::formalism::FunctionTermValue<::tyr::GroundTag, ::tyr::formalism::AuxiliaryTag>>>, C>
        auxiliary_fterm_value;
    urd::built_t<urd::index<::tyr::formalism::ConjunctiveCondition<::tyr::GroundTag>>, C> goal;
    urd::built_t<urd::flat_optional<urd::index<::tyr::formalism::Metric>>, C> metric;
    urd::built_t<urd::flat_vector<urd::index<::tyr::formalism::Axiom<::tyr::LiftedTag>>>, C> axioms;
    urd::built_t<urd::flat_vector<urd::index<::tyr::formalism::TimedLiteral>>, C> timed_literals;
    urd::built_t<urd::flat_vector<urd::index<::tyr::formalism::FDRVariable<::tyr::formalism::FluentTag>>>, C> fluent_variables;
    urd::built_t<urd::flat_vector<urd::data<::tyr::formalism::FDRFact<::tyr::formalism::FluentTag>>>, C> fluent_facts;
    urd::built_t<urd::flat_vector<urd::index<::tyr::formalism::Action<::tyr::GroundTag>>>, C> ground_actions;
    urd::built_t<urd::flat_vector<urd::index<::tyr::formalism::Axiom<::tyr::GroundTag>>>, C> ground_axioms;

    static constexpr auto fields() noexcept
    {
        return urd::Fields<urd::field<"name", &builder::name>,
                           urd::field<"requirements", &builder::requirements>,
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
                           urd::field<"timed_literals", &builder::timed_literals>,
                           urd::field<"fluent_variables", &builder::fluent_variables>,
                           urd::field<"fluent_facts", &builder::fluent_facts>,
                           urd::field<"ground_actions", &builder::ground_actions>,
                           urd::field<"ground_axioms", &builder::ground_axioms>> {};
    }
};

}  // namespace urd

#endif
