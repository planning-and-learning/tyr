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

#include "tyr/formalism/declarations.hpp"
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include "tyr/formalism/planning/declarations.hpp"

#include <tuple>
#include <utility>
#include <vector>
#include <urd/elements/flat.hpp>

namespace urd
{

template<>
struct data<::tyr::formalism::planning::ConditionalEffect<::tyr::LiftedTag>>
{
    urd::index<::tyr::formalism::planning::ConditionalEffect<::tyr::LiftedTag>> index;
    urd::flat_vector<urd::index<::tyr::formalism::Variable>> variables;
    urd::index<::tyr::formalism::planning::ConjunctiveCondition<::tyr::LiftedTag>> condition;
    urd::index<::tyr::formalism::planning::ConjunctiveEffect<::tyr::LiftedTag>> effect;

    data() = default;
    data(urd::flat_vector<urd::index<::tyr::formalism::Variable>> variables_,
         urd::index<::tyr::formalism::planning::ConjunctiveCondition<::tyr::LiftedTag>> condition_,
         urd::index<::tyr::formalism::planning::ConjunctiveEffect<::tyr::LiftedTag>> effect_) :
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

    static constexpr auto cista_members() noexcept { return urd::Members<&data::index, &data::variables, &data::condition, &data::effect> {}; }
    static constexpr auto identifying_members() noexcept { return urd::Members<&data::variables, &data::condition, &data::effect> {}; }
};

template<>
struct data<::tyr::formalism::planning::ConditionalEffect<::tyr::GroundTag>>
{
    urd::index<::tyr::formalism::planning::ConditionalEffect<::tyr::GroundTag>> index;
    urd::index<::tyr::formalism::planning::ConjunctiveCondition<::tyr::GroundTag>> condition;
    urd::index<::tyr::formalism::planning::ConjunctiveEffect<::tyr::GroundTag>> effect;

    data() = default;
    data(urd::index<::tyr::formalism::planning::ConjunctiveCondition<::tyr::GroundTag>> condition_,
         urd::index<::tyr::formalism::planning::ConjunctiveEffect<::tyr::GroundTag>> effect_) :
        index(),
        condition(condition_),
        effect(effect_)
    {
    }

    static constexpr auto cista_members() noexcept { return urd::Members<&data::index, &data::condition, &data::effect> {}; }
    static constexpr auto identifying_members() noexcept { return urd::Members<&data::condition, &data::effect> {}; }
};

}

#endif
