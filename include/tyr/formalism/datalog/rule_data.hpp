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

#include "tyr/formalism/binding_index.hpp"
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include "tyr/formalism/binary_operator_data.hpp"
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/numeric_effect_data.hpp"
#include "tyr/formalism/declarations.hpp"

#include <utility>
#include <vector>
#include <urd/elements/flat.hpp>
#include <yggdrasil/core/config.hpp>

namespace tyr::formalism::datalog
{

template<TaskKind T, RelationKind R>
struct RuleHeadMember;

/// A predicate rule derives all its heads from one body instantiation.
template<TaskKind T>
struct RuleHeadMember<T, PredicateTag>
{
    template<typename C>
    using HeadView = urd::Store<urd::sequence<urd::index_view<Atom<T, FluentTag>, C>>>;

    urd::flat_vector<urd::index<Atom<T, FluentTag>>> heads;

    auto& head_member() noexcept { return heads; }
    [[nodiscard]] const auto& head_member() const noexcept { return heads; }
    /// The head member, for the records' cista_members.
    static constexpr auto head_pointer() noexcept { return &RuleHeadMember::heads; }
};

template<TaskKind T>
struct RuleHeadMember<T, FunctionTag>
{
    template<typename C>
    using HeadView = urd::index_view<NumericEffect<T, FluentTag>, C>;

    urd::index<NumericEffect<T, FluentTag>> head;

    auto& head_member() noexcept { return head; }
    [[nodiscard]] const auto& head_member() const noexcept { return head; }
    /// The head member, for the records' cista_members.
    static constexpr auto head_pointer() noexcept { return &RuleHeadMember::head; }
};

}

namespace urd
{

template<::tyr::formalism::RelationKind R>
struct data<::tyr::formalism::datalog::Rule<::tyr::LiftedTag, R>> : ::tyr::formalism::datalog::RuleHeadMember<::tyr::LiftedTag, R>
{
    using Head = ::tyr::formalism::datalog::RuleHeadT<::tyr::LiftedTag, R>;

    template<typename C>
    using HeadView = typename ::tyr::formalism::datalog::RuleHeadMember<::tyr::LiftedTag, R>::template HeadView<C>;

    urd::index<::tyr::formalism::datalog::Rule<::tyr::LiftedTag, R>> index;
    urd::flat_vector<urd::index<::tyr::formalism::Variable>> variables;
    urd::index<::tyr::formalism::datalog::ConjunctiveCondition<::tyr::LiftedTag>> body;
    ygg::float_t cost { 0 };  ///< The edge cost of deriving a head.

    data() = default;
    data(urd::flat_vector<urd::index<::tyr::formalism::Variable>> variables_,
         urd::index<::tyr::formalism::datalog::ConjunctiveCondition<::tyr::LiftedTag>> body_,
         Head head_,
         ygg::float_t cost_ = 0) :
        index(),
        variables(std::move(variables_)),
        body(body_),
        cost(cost_)
    { this->head_member() = std::move(head_); }
    data(const data& other) = delete;
    data& operator=(const data& other) = delete;
    data(data&& other) = default;
    data& operator=(data&& other) = default;

    static constexpr auto cista_members() noexcept
    { return urd::Members<&data::index, &data::variables, &data::body, data::head_pointer(), &data::cost> {}; }
    /// The head (or heads), as the base holds it.
    [[nodiscard]] const auto& get_head() const noexcept { return this->head_member(); }
    static constexpr auto identifying_members() noexcept { return urd::Members<&data::variables, &data::body, &data::get_head, &data::cost> {}; }
};

template<::tyr::formalism::RelationKind R>
struct data<::tyr::formalism::datalog::Rule<::tyr::GroundTag, R>> : ::tyr::formalism::datalog::RuleHeadMember<::tyr::GroundTag, R>
{
    using Head = ::tyr::formalism::datalog::RuleHeadT<::tyr::GroundTag, R>;

    template<typename C>
    using HeadView = typename ::tyr::formalism::datalog::RuleHeadMember<::tyr::GroundTag, R>::template HeadView<C>;

    urd::index<::tyr::formalism::datalog::Rule<::tyr::GroundTag, R>> index;
    ygg::formalism::RelationBindingIndex<::tyr::formalism::datalog::Rule<::tyr::LiftedTag, R>, ::tyr::formalism::ObjectTag> binding;
    urd::index<::tyr::formalism::datalog::ConjunctiveCondition<::tyr::GroundTag>> body;
    ygg::float_t cost { 0 };  ///< The edge cost of deriving a head.

    data() = default;
    data(ygg::formalism::RelationBindingIndex<::tyr::formalism::datalog::Rule<::tyr::LiftedTag, R>, ::tyr::formalism::ObjectTag> binding_,
         urd::index<::tyr::formalism::datalog::ConjunctiveCondition<::tyr::GroundTag>> body_,
         Head head_,
         ygg::float_t cost_ = 0) :
        index(),
        binding(binding_),
        body(body_),
        cost(cost_)
    { this->head_member() = std::move(head_); }

    static constexpr auto cista_members() noexcept
    { return urd::Members<&data::index, &data::binding, &data::body, data::head_pointer(), &data::cost> {}; }
    static constexpr auto identifying_members() noexcept { return urd::Members<&data::binding> {}; }
};

}

#endif
