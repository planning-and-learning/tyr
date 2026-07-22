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

#ifndef TYR_FORMALISM_DATALOG_GROUND_CONDITIONAL_EFFECT_DATA_HPP_
#define TYR_FORMALISM_DATALOG_GROUND_CONDITIONAL_EFFECT_DATA_HPP_

#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/datalog/ground_conditional_effect_index.hpp"
#include "tyr/formalism/datalog/ground_conjunctive_condition_index.hpp"
#include "tyr/formalism/datalog/ground_conjunctive_effect_index.hpp"

#include <yggdrasil/core/types.hpp>
#include <yggdrasil/core/types_utils.hpp>

namespace ygg
{
using namespace ::tyr;

template<>
struct Data<::tyr::formalism::datalog::GroundConditionalEffect>
{
    ygg::Index<::tyr::formalism::datalog::GroundConditionalEffect> index;
    ygg::Index<::tyr::formalism::datalog::GroundConjunctiveCondition> condition;
    ygg::Index<::tyr::formalism::datalog::GroundConjunctiveEffect> effect;

    Data() = default;
    Data(ygg::Index<::tyr::formalism::datalog::GroundConjunctiveCondition> condition_, ygg::Index<::tyr::formalism::datalog::GroundConjunctiveEffect> effect_) :
        index(),
        condition(condition_),
        effect(effect_)
    {
    }
    template<typename C>
    Data(::ygg::View<ygg::Index<::tyr::formalism::datalog::GroundConjunctiveCondition>, C> condition_,
         ::ygg::View<ygg::Index<::tyr::formalism::datalog::GroundConjunctiveEffect>, C> effect_) :
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

static_assert(ygg::uses_trivial_storage_v<::tyr::formalism::datalog::GroundConditionalEffect>);

}

#endif
