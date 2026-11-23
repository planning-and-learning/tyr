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
struct builder<::tyr::formalism::Program<T>, C>
{
    urd::key<::tyr::formalism::Program<T>> index;
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
    urd::built_t<urd::flat_optional<urd::index<::tyr::formalism::RuleBody<::tyr::GroundTag>>>, C> goal;
    urd::built_t<urd::flat_vector<urd::index<::tyr::formalism::Rule<T>>>, C> rules;

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

namespace tyr::formalism
{

/// Lists everything of the program but its rules in a program builder of a layer over the program's (its records, as
/// they are).
template<TaskKind T>
void list_except_rules(ProgramView<T> element, urd::builder<Program<T>, Repository>& result)
{
    const auto list_all = [&](const auto elements, auto& destination)
    {
        for (const auto e : elements)
            urd::append(destination, e);
    };

    list_all(element.template get_predicates<StaticTag>(), result.static_predicates);
    list_all(element.template get_predicates<FluentTag>(), result.fluent_predicates);
    list_all(element.template get_predicates<DerivedTag>(), result.derived_predicates);
    list_all(element.template get_functions<StaticTag>(), result.static_functions);
    list_all(element.template get_functions<FluentTag>(), result.fluent_functions);
    list_all(element.get_objects(), result.objects);
    list_all(element.template get_atoms<StaticTag>(), result.static_atoms);
    list_all(element.template get_atoms<FluentTag>(), result.fluent_atoms);
    list_all(element.template get_atoms<DerivedTag>(), result.derived_atoms);
    list_all(element.template get_fterm_values<StaticTag>(), result.static_fterm_values);
    list_all(element.template get_fterm_values<FluentTag>(), result.fluent_fterm_values);

    if (const auto goal = element.get_goal())
        urd::store(result.goal, *goal);
}

}

#endif
