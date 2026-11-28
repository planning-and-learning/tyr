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

#ifndef TYR_FORMALISM_PLANNING_DOMAIN_BUILDER_HPP_
#define TYR_FORMALISM_PLANNING_DOMAIN_BUILDER_HPP_

#include "tyr/declarations.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/enums.hpp"
#include "tyr/formalism/function_builder.hpp"
#include "tyr/formalism/object_builder.hpp"
#include "tyr/formalism/planning/action_builder.hpp"
#include "tyr/formalism/planning/axiom_builder.hpp"
#include "tyr/formalism/planning/domain_data.hpp"
#include "tyr/formalism/planning/domain_view.hpp"
#include "tyr/formalism/predicate_builder.hpp"

#include <urd/elements/builder.hpp>
#include <urd/elements/children.hpp>
#include <urd/elements/concepts.hpp>
#include <urd/elements/flat.hpp>
#include <urd/elements/index.hpp>

namespace urd
{

/// The builder of Domain: its record's members, each in its built form (urd::built_t).
template<typename C>
struct builder<::tyr::formalism::Domain, C>
{
    urd::key<::tyr::formalism::Domain> index;
    urd::built_t<urd::flat_string, C> name;
    urd::built_t<urd::flat_vector<::tyr::formalism::RequirementKind>, C> requirements;
    urd::built_t<urd::flat_vector<urd::index<::tyr::formalism::Predicate<::tyr::formalism::StaticTag>>>, C> static_predicates;
    urd::built_t<urd::flat_vector<urd::index<::tyr::formalism::Predicate<::tyr::formalism::FluentTag>>>, C> fluent_predicates;
    urd::built_t<urd::flat_vector<urd::index<::tyr::formalism::Predicate<::tyr::formalism::DerivedTag>>>, C> derived_predicates;
    urd::built_t<urd::flat_vector<urd::index<::tyr::formalism::Function<::tyr::formalism::StaticTag>>>, C> static_functions;
    urd::built_t<urd::flat_vector<urd::index<::tyr::formalism::Function<::tyr::formalism::FluentTag>>>, C> fluent_functions;
    urd::built_t<urd::flat_optional<urd::index<::tyr::formalism::Function<::tyr::formalism::AuxiliaryTag>>>, C> auxiliary_function;
    urd::built_t<urd::flat_vector<urd::index<::tyr::formalism::Object>>, C> constants;
    urd::built_t<urd::flat_vector<urd::index<::tyr::formalism::Action<::tyr::LiftedTag>>>, C> actions;
    urd::built_t<urd::flat_vector<urd::index<::tyr::formalism::Axiom<::tyr::LiftedTag>>>, C> axioms;

    static constexpr auto fields() noexcept
    {
        return urd::Fields<urd::field<"name", &builder::name>,
                           urd::field<"requirements", &builder::requirements>,
                           urd::field<"static_predicates", &builder::static_predicates>,
                           urd::field<"fluent_predicates", &builder::fluent_predicates>,
                           urd::field<"derived_predicates", &builder::derived_predicates>,
                           urd::field<"static_functions", &builder::static_functions>,
                           urd::field<"fluent_functions", &builder::fluent_functions>,
                           urd::field<"auxiliary_function", &builder::auxiliary_function>,
                           urd::field<"constants", &builder::constants>,
                           urd::field<"actions", &builder::actions>,
                           urd::field<"axioms", &builder::axioms>> {};
    }

};

}  // namespace urd

#endif
