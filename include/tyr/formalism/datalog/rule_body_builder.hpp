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

#ifndef TYR_FORMALISM_DATALOG_RULE_BODY_BUILDER_HPP_
#define TYR_FORMALISM_DATALOG_RULE_BODY_BUILDER_HPP_

#include "tyr/formalism/binary_operator_builder.hpp"
#include "tyr/formalism/datalog/rule_body_data.hpp"
#include "tyr/formalism/datalog/rule_body_view.hpp"
#include "tyr/formalism/literal_builder.hpp"
#include "tyr/formalism/variable_builder.hpp"

#include <optional>
#include <urd/elements/builder.hpp>
#include <urd/elements/children.hpp>
#include <variant>
#include <vector>

namespace urd
{

/// The builder of RuleBody: its record's members, each in its built form (urd::built_t).
template<typename C>
struct builder<::tyr::formalism::RuleBody<::tyr::GroundTag>, C>
{
    urd::index<::tyr::formalism::RuleBody<::tyr::GroundTag>> index;
    urd::built_t<urd::flat_vector<urd::index<::tyr::formalism::Literal<::tyr::GroundTag, ::tyr::formalism::StaticTag>>>, C> static_literals;
    urd::built_t<urd::flat_vector<urd::index<::tyr::formalism::Literal<::tyr::GroundTag, ::tyr::formalism::FluentTag>>>, C> fluent_literals;
    urd::built_t<urd::flat_vector<urd::index<::tyr::formalism::Literal<::tyr::GroundTag, ::tyr::formalism::DerivedTag>>>, C> derived_literals;
    urd::built_t<urd::flat_vector<urd::index<::tyr::formalism::BinaryOperator<::tyr::GroundTag, ::tyr::formalism::BooleanOperatorKind>>>, C>
        numeric_constraints;

    static constexpr auto fields() noexcept
    {
        return urd::Fields<urd::field<"static_literals", &builder::static_literals>,
                           urd::field<"fluent_literals", &builder::fluent_literals>,
                           urd::field<"derived_literals", &builder::derived_literals>,
                           urd::field<"numeric_constraints", &builder::numeric_constraints>> {};
    }

    static constexpr auto identifying_members() noexcept
    { return urd::Members<&builder::static_literals, &builder::fluent_literals, &builder::derived_literals, &builder::numeric_constraints> {}; }
};

}  // namespace urd

#endif
