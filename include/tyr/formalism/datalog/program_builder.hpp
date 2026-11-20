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

#ifndef TYR_FORMALISM_DATALOG_PROGRAM_BUILDER_HPP_
#define TYR_FORMALISM_DATALOG_PROGRAM_BUILDER_HPP_

#include "tyr/formalism/atom_builder.hpp"
#include "tyr/formalism/conjunctive_condition_builder.hpp"
#include "tyr/formalism/datalog/program_data.hpp"
#include "tyr/formalism/datalog/program_view.hpp"
#include "tyr/formalism/datalog/rule_body_builder.hpp"
#include "tyr/formalism/datalog/rule_builder.hpp"
#include "tyr/formalism/function_builder.hpp"
#include "tyr/formalism/function_term_value_builder.hpp"
#include "tyr/formalism/object_builder.hpp"
#include "tyr/formalism/predicate_builder.hpp"

#include <optional>
#include <urd/elements/builder.hpp>
#include <urd/elements/children.hpp>
#include <variant>
#include <vector>

namespace urd
{

/// The builder of Program: its record's members, each in its built form (urd::built_t).
template<::tyr::TaskKind T, typename C>
struct builder<::tyr::formalism::datalog::Program<T>, C>
{
    urd::index<::tyr::formalism::datalog::Program<T>> index;
    urd::built_t<urd::flat_vector<urd::index<::tyr::formalism::Predicate<::tyr::formalism::StaticTag>>>, C> static_predicates;
    urd::built_t<urd::flat_vector<urd::index<::tyr::formalism::Predicate<::tyr::formalism::FluentTag>>>, C> fluent_predicates;
    urd::built_t<urd::flat_vector<urd::index<::tyr::formalism::Predicate<::tyr::formalism::DerivedTag>>>, C> derived_predicates;
    urd::built_t<urd::flat_vector<urd::index<::tyr::formalism::Function<::tyr::formalism::StaticTag>>>, C> static_functions;
    urd::built_t<urd::flat_vector<urd::index<::tyr::formalism::Function<::tyr::formalism::FluentTag>>>, C> fluent_functions;
    urd::built_t<urd::flat_vector<urd::index<::tyr::formalism::Object>>, C> objects;
    urd::built_t<urd::flat_vector<urd::index<::tyr::formalism::Atom<::tyr::GroundTag, ::tyr::formalism::StaticTag>>>, C> static_atoms;
    urd::built_t<urd::flat_vector<urd::index<::tyr::formalism::Atom<::tyr::GroundTag, ::tyr::formalism::FluentTag>>>, C> fluent_atoms;
    urd::built_t<urd::flat_vector<urd::index<::tyr::formalism::Atom<::tyr::GroundTag, ::tyr::formalism::DerivedTag>>>, C> derived_atoms;
    urd::built_t<urd::flat_vector<urd::index<::tyr::formalism::FunctionTermValue<::tyr::GroundTag, ::tyr::formalism::StaticTag>>>, C> static_fterm_values;
    urd::built_t<urd::flat_vector<urd::index<::tyr::formalism::FunctionTermValue<::tyr::GroundTag, ::tyr::formalism::FluentTag>>>, C> fluent_fterm_values;
    urd::built_t<urd::flat_optional<urd::index<::tyr::formalism::datalog::RuleBody<::tyr::GroundTag>>>, C> goal;
    urd::built_t<urd::flat_vector<urd::index<::tyr::formalism::datalog::Rule<T>>>, C> rules;

    static constexpr auto fields() noexcept
    {
        return urd::Fields<urd::field<"static_predicates", &builder::static_predicates>,
                           urd::field<"fluent_predicates", &builder::fluent_predicates>,
                           urd::field<"derived_predicates", &builder::derived_predicates>,
                           urd::field<"static_functions", &builder::static_functions>,
                           urd::field<"fluent_functions", &builder::fluent_functions>,
                           urd::field<"objects", &builder::objects>,
                           urd::field<"static_atoms", &builder::static_atoms>,
                           urd::field<"fluent_atoms", &builder::fluent_atoms>,
                           urd::field<"derived_atoms", &builder::derived_atoms>,
                           urd::field<"static_fterm_values", &builder::static_fterm_values>,
                           urd::field<"fluent_fterm_values", &builder::fluent_fterm_values>,
                           urd::field<"goal", &builder::goal>,
                           urd::field<"rules", &builder::rules>> {};
    }

    static constexpr auto identifying_members() noexcept
    {
        return urd::Members<&builder::static_predicates,
                            &builder::fluent_predicates,
                            &builder::derived_predicates,
                            &builder::static_functions,
                            &builder::fluent_functions,
                            &builder::objects,
                            &builder::static_atoms,
                            &builder::fluent_atoms,
                            &builder::derived_atoms,
                            &builder::static_fterm_values,
                            &builder::fluent_fterm_values,
                            &builder::goal,
                            &builder::rules> {};
    }
};

}  // namespace urd

#endif
