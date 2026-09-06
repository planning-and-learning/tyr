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

#ifndef TYR_FORMALISM_DATALOG_CONDITIONAL_EFFECT_DATA_HPP_
#define TYR_FORMALISM_DATALOG_CONDITIONAL_EFFECT_DATA_HPP_

#include "tyr/formalism/datalog/conditional_effect_index.hpp"
#include "tyr/formalism/datalog/conjunctive_condition_index.hpp"
#include "tyr/formalism/datalog/conjunctive_effect_index.hpp"
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/variable_index.hpp"

#include <yggdrasil/core/types.hpp>
#include <yggdrasil/core/types_utils.hpp>

namespace ygg
{

template<>
struct Data<::tyr::formalism::datalog::ConditionalEffect<::tyr::LiftedTag>>
{
    ygg::Index<::tyr::formalism::datalog::ConditionalEffect<::tyr::LiftedTag>> index;
    ygg::IndexList<::tyr::formalism::Variable> variables;
    ygg::Index<::tyr::formalism::datalog::ConjunctiveCondition<::tyr::LiftedTag>> condition;
    ygg::Index<::tyr::formalism::datalog::ConjunctiveEffect<::tyr::LiftedTag>> effect;

    Data() = default;
    Data(ygg::IndexList<::tyr::formalism::Variable> variables_,
         ygg::Index<::tyr::formalism::datalog::ConjunctiveCondition<::tyr::LiftedTag>> condition_,
         ygg::Index<::tyr::formalism::datalog::ConjunctiveEffect<::tyr::LiftedTag>> effect_) :
        index(),
        variables(std::move(variables_)),
        condition(condition_),
        effect(effect_)
    {
    }
    template<typename C>
    Data(const std::vector<::ygg::View<ygg::Index<::tyr::formalism::Variable>, C>>& variables_,
         ::ygg::View<ygg::Index<::tyr::formalism::datalog::ConjunctiveCondition<::tyr::LiftedTag>>, C> condition_,
         ::ygg::View<ygg::Index<::tyr::formalism::datalog::ConjunctiveEffect<::tyr::LiftedTag>>, C> effect_) :
        index(),
        variables(),
        condition(),
        effect()
    {
        set(variables_, variables);
        set(condition_, condition);
        set(effect_, effect);
    }
    Data(const Data& other) = delete;
    Data& operator=(const Data& other) = delete;
    Data(Data&& other) = default;
    Data& operator=(Data&& other) = default;

    void clear() noexcept
    {
        ygg::clear(index);
        ygg::clear(variables);
        ygg::clear(condition);
        ygg::clear(effect);
    }

    auto cista_members() const noexcept { return std::tie(index, variables, condition, effect); }
    auto identifying_members() const noexcept { return std::tie(variables, condition, effect); }
};

static_assert(!ygg::uses_trivial_storage_v<::tyr::formalism::datalog::ConditionalEffect<::tyr::LiftedTag>>);

template<>
struct Data<::tyr::formalism::datalog::ConditionalEffect<::tyr::GroundTag>>
{
    ygg::Index<::tyr::formalism::datalog::ConditionalEffect<::tyr::GroundTag>> index;
    ygg::Index<::tyr::formalism::datalog::ConjunctiveCondition<::tyr::GroundTag>> condition;
    ygg::Index<::tyr::formalism::datalog::ConjunctiveEffect<::tyr::GroundTag>> effect;

    Data() = default;
    Data(ygg::Index<::tyr::formalism::datalog::ConjunctiveCondition<::tyr::GroundTag>> condition_,
         ygg::Index<::tyr::formalism::datalog::ConjunctiveEffect<::tyr::GroundTag>> effect_) :
        index(),
        condition(condition_),
        effect(effect_)
    {
    }
    template<typename C>
    Data(::ygg::View<ygg::Index<::tyr::formalism::datalog::ConjunctiveCondition<::tyr::GroundTag>>, C> condition_,
         ::ygg::View<ygg::Index<::tyr::formalism::datalog::ConjunctiveEffect<::tyr::GroundTag>>, C> effect_) :
        index(),
        condition(),
        effect()
    {
        set(condition_, condition);
        set(effect_, effect);
    }
    Data(const Data& other) = default;
    Data& operator=(const Data& other) = default;
    Data(Data&& other) = default;
    Data& operator=(Data&& other) = default;

    void clear() noexcept
    {
        ygg::clear(index);
        ygg::clear(condition);
        ygg::clear(effect);
    }

    auto cista_members() const noexcept { return std::tie(index, condition, effect); }
    auto identifying_members() const noexcept { return std::tie(condition, effect); }
};

static_assert(ygg::uses_trivial_storage_v<::tyr::formalism::datalog::ConditionalEffect<::tyr::GroundTag>>);

}

#endif
