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

#ifndef TYR_FORMALISM_DATALOG_RULE_DATA_HPP_
#define TYR_FORMALISM_DATALOG_RULE_DATA_HPP_

#include "tyr/declarations.hpp"
#include "tyr/formalism/declarations.hpp"

#include <boost/mp11/detail/mp_list.hpp>
#include <urd/elements/concepts.hpp>
#include <urd/elements/data.hpp>
#include <urd/elements/flat.hpp>
#include <urd/elements/index.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/tags.hpp>
#include <utility>
#include <yggdrasil/core/config.hpp>

namespace urd
{
template<>
struct data<::tyr::formalism::Rule<::tyr::LiftedTag>>
{
    urd::key<::tyr::formalism::Rule<::tyr::LiftedTag>> index;
    urd::flat_vector<urd::index<::tyr::formalism::Variable>> variables;
    urd::index<::tyr::formalism::ConjunctiveCondition<::tyr::LiftedTag>> body;
    urd::flat_vector<urd::index<::tyr::formalism::Atom<::tyr::LiftedTag, ::tyr::formalism::FluentTag>>> fluent_heads;
    urd::flat_vector<urd::index<::tyr::formalism::Atom<::tyr::LiftedTag, ::tyr::formalism::DerivedTag>>> derived_heads;
    ygg::float_t cost { 0 };  ///< The edge cost of deriving a head.

    data() = default;

    data(urd::flat_vector<urd::index<::tyr::formalism::Variable>> variables_,
         urd::index<::tyr::formalism::ConjunctiveCondition<::tyr::LiftedTag>> body_,
         urd::flat_vector<urd::index<::tyr::formalism::Atom<::tyr::LiftedTag, ::tyr::formalism::FluentTag>>> fluent_heads_,
         urd::flat_vector<urd::index<::tyr::formalism::Atom<::tyr::LiftedTag, ::tyr::formalism::DerivedTag>>> derived_heads_,
         ygg::float_t cost_ = 0) :
        index(),
        variables(std::move(variables_)),
        body(body_),
        fluent_heads(std::move(fluent_heads_)),
        derived_heads(std::move(derived_heads_)),
        cost(cost_)
    {
    }

    data(const data& other) = delete;
    data& operator=(const data& other) = delete;
    data(data&& other) = default;
    data& operator=(data&& other) = default;
    ~data() = default;

    static constexpr auto fields() noexcept
    {
        return urd::Fields<urd::field<"variables", &data::variables>,
                           urd::field<"body", &data::body>,
                           urd::field<"fluent_heads", &data::fluent_heads, urd::sorted, urd::unique>,
                           urd::field<"derived_heads", &data::derived_heads, urd::sorted, urd::unique>,
                           urd::field<"cost", &data::cost>> {};
    }

    static constexpr auto cista_members() noexcept
    { return urd::cista_members_of<data>(); }

    [[nodiscard]] urd::key<::tyr::formalism::Rule<::tyr::LiftedTag>> key() const noexcept { return urd::key<::tyr::formalism::Rule<::tyr::LiftedTag>>(index); }
};

template<>
struct data<::tyr::formalism::Rule<::tyr::GroundTag>>
{
    urd::key<::tyr::formalism::Rule<::tyr::GroundTag>> index;
    urd::index<::tyr::formalism::RuleBody<::tyr::GroundTag>> body;
    urd::flat_vector<urd::index<::tyr::formalism::Atom<::tyr::GroundTag, ::tyr::formalism::FluentTag>>> fluent_heads;
    urd::flat_vector<urd::index<::tyr::formalism::Atom<::tyr::GroundTag, ::tyr::formalism::DerivedTag>>> derived_heads;
    ygg::float_t cost { 0 };  ///< The edge cost of deriving a head.

    data() = default;

    data(urd::index<::tyr::formalism::RuleBody<::tyr::GroundTag>> body_,
         urd::flat_vector<urd::index<::tyr::formalism::Atom<::tyr::GroundTag, ::tyr::formalism::FluentTag>>> fluent_heads_,
         urd::flat_vector<urd::index<::tyr::formalism::Atom<::tyr::GroundTag, ::tyr::formalism::DerivedTag>>> derived_heads_,
         ygg::float_t cost_ = 0) :
        index(),
        body(body_),
        fluent_heads(std::move(fluent_heads_)),
        derived_heads(std::move(derived_heads_)),
        cost(cost_)
    {
    }

    data(const data& other) = delete;
    data& operator=(const data& other) = delete;
    data(data&& other) = default;
    data& operator=(data&& other) = default;
    ~data() = default;

    static constexpr auto fields() noexcept
    {
        return urd::Fields<urd::field<"body", &data::body>,
                           urd::field<"fluent_heads", &data::fluent_heads, urd::sorted, urd::unique>,
                           urd::field<"derived_heads", &data::derived_heads, urd::sorted, urd::unique>,
                           urd::field<"cost", &data::cost>> {};
    }

    /// Identified by its content: identical ground rules are one rule.

    static constexpr auto cista_members() noexcept
    { return urd::cista_members_of<data>(); }

    [[nodiscard]] urd::key<::tyr::formalism::Rule<::tyr::GroundTag>> key() const noexcept { return urd::key<::tyr::formalism::Rule<::tyr::GroundTag>>(index); }
};
}

namespace tyr::formalism
{
/// The member holding the F-heads of a rule: the atoms of the task's fluent predicates it derives, and of the derived
/// predicates (the task's, or the program's own).
template<FactKind F, TaskKind T>
inline constexpr auto heads_of =
    tagged_member_pointer<boost::mp11::mp_list<FluentTag, DerivedTag>, F, &urd::data<Rule<T>>::fluent_heads, &urd::data<Rule<T>>::derived_heads>;
}

#endif
