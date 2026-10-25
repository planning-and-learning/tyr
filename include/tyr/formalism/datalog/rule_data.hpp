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
#include "tyr/formalism/datalog/boolean_operator_data.hpp"
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/datalog/numeric_effect_operator_data.hpp"
#include "tyr/formalism/declarations.hpp"

#include <tuple>
#include <utility>
#include <vector>
#include <yggdrasil/containers/vector.hpp>
#include <yggdrasil/core/types.hpp>
#include <yggdrasil/core/types_utils.hpp>

namespace tyr::formalism::datalog
{

template<TaskKind T, RelationKind R>
struct RuleHeadMember;

/// A predicate rule derives all its heads from one body instantiation.
template<TaskKind T>
struct RuleHeadMember<T, PredicateTag>
{
    template<typename C>
    using HeadView = std::vector<::ygg::View<ygg::Index<Atom<T, FluentTag>>, C>>;

    ygg::IndexList<Atom<T, FluentTag>> heads;

    auto& head_member() noexcept { return heads; }
    [[nodiscard]] const auto& head_member() const noexcept { return heads; }
};

template<TaskKind T>
struct RuleHeadMember<T, FunctionTag>
{
    template<typename C>
    using HeadView = ::ygg::View<ygg::Data<NumericEffectOperator<T, FluentTag>>, C>;

    ygg::Data<NumericEffectOperator<T, FluentTag>> head;

    auto& head_member() noexcept { return head; }
    [[nodiscard]] const auto& head_member() const noexcept { return head; }
};

}

namespace ygg
{

template<::tyr::formalism::RelationKind R>
struct Data<::tyr::formalism::datalog::Rule<::tyr::LiftedTag, R>> : ::tyr::formalism::datalog::RuleHeadMember<::tyr::LiftedTag, R>
{
    using Head = ::tyr::formalism::datalog::RuleHeadT<::tyr::LiftedTag, R>;

    template<typename C>
    using HeadView = typename ::tyr::formalism::datalog::RuleHeadMember<::tyr::LiftedTag, R>::template HeadView<C>;

    ygg::Index<::tyr::formalism::datalog::Rule<::tyr::LiftedTag, R>> index;
    ygg::IndexList<::tyr::formalism::Variable> variables;
    ygg::Index<::tyr::formalism::datalog::ConjunctiveCondition<::tyr::LiftedTag>> body;
    ygg::float_t cost { 0 };  ///< The edge cost of deriving a head.

    Data() = default;
    Data(ygg::IndexList<::tyr::formalism::Variable> variables_,
         ygg::Index<::tyr::formalism::datalog::ConjunctiveCondition<::tyr::LiftedTag>> body_,
         Head head_,
         ygg::float_t cost_ = 0) :
        index(),
        variables(std::move(variables_)),
        body(body_),
        cost(cost_)
    { this->head_member() = std::move(head_); }
    template<typename C>
    Data(const std::vector<::ygg::View<ygg::Index<::tyr::formalism::Variable>, C>>& variables_,
         ::ygg::View<ygg::Index<::tyr::formalism::datalog::ConjunctiveCondition<::tyr::LiftedTag>>, C> body_,
         const HeadView<C>& head_,
         ygg::float_t cost_ = 0) :
        index(),
        variables(),
        body(),
        cost(cost_)
    {
        set(variables_, variables);
        set(body_, body);
        set(head_, this->head_member());
    }
    Data(const Data& other) = delete;
    Data& operator=(const Data& other) = delete;
    Data(Data&& other) = default;
    Data& operator=(Data&& other) = default;
    ~Data() = default;

    auto cista_members() noexcept { return std::tie(index, variables, body, this->head_member(), cost); }
    auto cista_members() const noexcept { return std::tie(index, variables, body, this->head_member(), cost); }
    auto identifying_members() const noexcept { return std::tie(variables, body, this->head_member(), cost); }
    void clear() noexcept
    {
        std::apply([](auto&... member) { (ygg::clear(member), ...); }, cista_members());
    }
};

static_assert(!ygg::uses_trivial_storage_v<::tyr::formalism::datalog::Rule<::tyr::LiftedTag, ::tyr::formalism::PredicateTag>>);
static_assert(!ygg::uses_trivial_storage_v<::tyr::formalism::datalog::Rule<::tyr::LiftedTag, ::tyr::formalism::FunctionTag>>);

template<::tyr::formalism::RelationKind R>
struct Data<::tyr::formalism::datalog::Rule<::tyr::GroundTag, R>> : ::tyr::formalism::datalog::RuleHeadMember<::tyr::GroundTag, R>
{
    using Head = ::tyr::formalism::datalog::RuleHeadT<::tyr::GroundTag, R>;

    template<typename C>
    using HeadView = typename ::tyr::formalism::datalog::RuleHeadMember<::tyr::GroundTag, R>::template HeadView<C>;

    ygg::Index<::tyr::formalism::datalog::Rule<::tyr::GroundTag, R>> index;
    ygg::Index<::tyr::formalism::RelationBinding<::tyr::formalism::datalog::Rule<::tyr::LiftedTag, R>>> binding;
    ygg::Index<::tyr::formalism::datalog::ConjunctiveCondition<::tyr::GroundTag>> body;
    ygg::float_t cost { 0 };  ///< The edge cost of deriving a head.

    Data() = default;
    Data(ygg::Index<::tyr::formalism::RelationBinding<::tyr::formalism::datalog::Rule<::tyr::LiftedTag, R>>> binding_,
         ygg::Index<::tyr::formalism::datalog::ConjunctiveCondition<::tyr::GroundTag>> body_,
         Head head_,
         ygg::float_t cost_ = 0) :
        index(),
        binding(binding_),
        body(body_),
        cost(cost_)
    { this->head_member() = std::move(head_); }
    template<typename C>
    Data(::ygg::View<ygg::Index<::tyr::formalism::RelationBinding<::tyr::formalism::datalog::Rule<::tyr::LiftedTag, R>>>, C> binding_,
         ::ygg::View<ygg::Index<::tyr::formalism::datalog::ConjunctiveCondition<::tyr::GroundTag>>, C> body_,
         const HeadView<C>& head_,
         ygg::float_t cost_ = 0) :
        index(),
        binding(),
        body(),
        cost(cost_)
    {
        set(binding_, binding);
        set(body_, body);
        set(head_, this->head_member());
    }
    Data(const Data& other) = default;
    Data& operator=(const Data& other) = default;
    Data(Data&& other) = default;
    Data& operator=(Data&& other) = default;
    ~Data() = default;

    auto cista_members() noexcept { return std::tie(index, binding, body, this->head_member(), cost); }
    auto cista_members() const noexcept { return std::tie(index, binding, body, this->head_member(), cost); }
    static constexpr auto identifying_members() noexcept { return ygg::Members<&Data::binding> {}; }
    void clear() noexcept
    {
        std::apply([](auto&... member) { (ygg::clear(member), ...); }, cista_members());
    }
};

}

#endif
