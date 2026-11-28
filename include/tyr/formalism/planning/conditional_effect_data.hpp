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

#ifndef TYR_FORMALISM_PLANNING_CONDITIONAL_EFFECT_DATA_HPP_
#define TYR_FORMALISM_PLANNING_CONDITIONAL_EFFECT_DATA_HPP_

#include "tyr/declarations.hpp"
#include "tyr/formalism/declarations.hpp"

#include <type_traits>
#include <urd/elements/concepts.hpp>
#include <urd/elements/data.hpp>
#include <urd/elements/flat.hpp>
#include <urd/elements/index.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <utility>

namespace urd
{

template<>
struct data<::tyr::formalism::ConditionalEffect<::tyr::LiftedTag>>
{
    urd::key<::tyr::formalism::ConditionalEffect<::tyr::LiftedTag>> index;
    urd::flat_vector<urd::index<::tyr::formalism::Variable>> variables;
    urd::index<::tyr::formalism::ConjunctiveCondition<::tyr::LiftedTag>> condition;
    urd::index<::tyr::formalism::ConjunctiveEffect<::tyr::LiftedTag>> effect;

    data() = default;

    data(urd::flat_vector<urd::index<::tyr::formalism::Variable>> variables_,
         urd::index<::tyr::formalism::ConjunctiveCondition<::tyr::LiftedTag>> condition_,
         urd::index<::tyr::formalism::ConjunctiveEffect<::tyr::LiftedTag>> effect_) :
        index(),
        variables(std::move(variables_)),
        condition(condition_),
        effect(effect_)
    {
    }

    data(const data& other) = delete;
    data& operator=(const data& other) = delete;
    data(data&& other) = default;
    data& operator=(data&& other) = default;
    ~data() = default;

    static constexpr auto fields() noexcept
    { return urd::Fields<urd::field<"variables", &data::variables>, urd::field<"condition", &data::condition>, urd::field<"effect", &data::effect>> {}; }

    static constexpr auto cista_members() noexcept { return urd::cista_members_of<data>(); }

    [[nodiscard]] urd::key<::tyr::formalism::ConditionalEffect<::tyr::LiftedTag>> key() const noexcept
    { return urd::key<::tyr::formalism::ConditionalEffect<::tyr::LiftedTag>>(index); }
};

template<>
struct data<::tyr::formalism::ConditionalEffect<::tyr::GroundTag>>
{
    urd::key<::tyr::formalism::ConditionalEffect<::tyr::GroundTag>> index;
    urd::index<::tyr::formalism::ConjunctiveCondition<::tyr::GroundTag>> condition;
    urd::index<::tyr::formalism::ConjunctiveEffect<::tyr::GroundTag>> effect;

    data() = default;

    data(urd::index<::tyr::formalism::ConjunctiveCondition<::tyr::GroundTag>> condition_,
         urd::index<::tyr::formalism::ConjunctiveEffect<::tyr::GroundTag>> effect_) :
        index(),
        condition(condition_),
        effect(effect_)
    {
    }

    static constexpr bool copyable() noexcept
    {
        static_assert(std::is_trivially_copyable_v<data>, "a copyable record is trivially copyable");
        return true;
    }

    static constexpr auto fields() noexcept { return urd::Fields<urd::field<"condition", &data::condition>, urd::field<"effect", &data::effect>> {}; }

    static constexpr auto cista_members() noexcept { return urd::cista_members_of<data>(); }

    [[nodiscard]] urd::key<::tyr::formalism::ConditionalEffect<::tyr::GroundTag>> key() const noexcept
    { return urd::key<::tyr::formalism::ConditionalEffect<::tyr::GroundTag>>(index); }
};

}

#endif
