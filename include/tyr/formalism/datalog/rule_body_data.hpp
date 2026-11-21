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

#ifndef TYR_FORMALISM_DATALOG_RULE_BODY_DATA_HPP_
#define TYR_FORMALISM_DATALOG_RULE_BODY_DATA_HPP_

#include "tyr/formalism/binary_operator_data.hpp"
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/declarations.hpp"

#include <tuple>
#include <urd/elements/flat.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <utility>
#include <vector>

namespace urd
{

template<>
struct data<::tyr::formalism::RuleBody<::tyr::GroundTag>>
{
    urd::index<::tyr::formalism::RuleBody<::tyr::GroundTag>> index;
    urd::flat_vector<urd::index<::tyr::formalism::Literal<::tyr::GroundTag, ::tyr::formalism::StaticTag>>> static_literals;
    urd::flat_vector<urd::index<::tyr::formalism::Literal<::tyr::GroundTag, ::tyr::formalism::FluentTag>>> fluent_literals;
    urd::flat_vector<urd::index<::tyr::formalism::Literal<::tyr::GroundTag, ::tyr::formalism::DerivedTag>>> derived_literals;
    urd::flat_vector<urd::index<::tyr::formalism::BinaryOperator<::tyr::GroundTag, ::tyr::formalism::BooleanOperatorKind>>> numeric_constraints;

    data() = default;

    data(urd::flat_vector<urd::index<::tyr::formalism::Literal<::tyr::GroundTag, ::tyr::formalism::StaticTag>>> static_literals_,
         urd::flat_vector<urd::index<::tyr::formalism::Literal<::tyr::GroundTag, ::tyr::formalism::FluentTag>>> fluent_literals_,
         urd::flat_vector<urd::index<::tyr::formalism::Literal<::tyr::GroundTag, ::tyr::formalism::DerivedTag>>> derived_literals_,
         urd::flat_vector<urd::index<::tyr::formalism::BinaryOperator<::tyr::GroundTag, ::tyr::formalism::BooleanOperatorKind>>> numeric_constraints_) :
        index(),
        static_literals(std::move(static_literals_)),
        fluent_literals(std::move(fluent_literals_)),
        derived_literals(std::move(derived_literals_)),
        numeric_constraints(std::move(numeric_constraints_))
    {
    }

    data(const data& other) = delete;
    data& operator=(const data& other) = delete;
    data(data&& other) = default;
    data& operator=(data&& other) = default;
    ~data() = default;

    static constexpr auto fields() noexcept
    {
        return urd::Fields<urd::field<"static_literals", &data::static_literals>,
                           urd::field<"fluent_literals", &data::fluent_literals>,
                           urd::field<"derived_literals", &data::derived_literals>,
                           urd::field<"numeric_constraints", &data::numeric_constraints>> {};
    }

    static constexpr auto identifying_members() noexcept
    { return urd::Members<&data::static_literals, &data::fluent_literals, &data::derived_literals, &data::numeric_constraints> {}; }

    static constexpr auto cista_members() noexcept
    { return urd::Members<&data::index, &data::static_literals, &data::fluent_literals, &data::derived_literals, &data::numeric_constraints> {}; }
};

}

namespace tyr::formalism
{
/// The member holding the F-literals of a rule body.
template<FactKind F>
inline constexpr auto rule_body_literals_of = tagged_member_pointer<StaticFluentDerivedTags,
                                                                    F,
                                                                    &urd::data<RuleBody<GroundTag>>::static_literals,
                                                                    &urd::data<RuleBody<GroundTag>>::fluent_literals,
                                                                    &urd::data<RuleBody<GroundTag>>::derived_literals>;
}

#endif
