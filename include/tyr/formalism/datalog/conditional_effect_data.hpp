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
using namespace ::tyr;

template<>
struct Data<::tyr::formalism::datalog::ConditionalEffect>
{
    ygg::Index<::tyr::formalism::datalog::ConditionalEffect> index;
    ygg::IndexList<::tyr::formalism::Variable> variables;
    ygg::Index<::tyr::formalism::datalog::ConjunctiveCondition> condition;
    ygg::Index<::tyr::formalism::datalog::ConjunctiveEffect> effect;

    Data() = default;
    Data(ygg::IndexList<::tyr::formalism::Variable> variables_,
         ygg::Index<::tyr::formalism::datalog::ConjunctiveCondition> condition_,
         ygg::Index<::tyr::formalism::datalog::ConjunctiveEffect> effect_) :
        index(),
        variables(std::move(variables_)),
        condition(condition_),
        effect(effect_)
    {
    }
    template<typename C>
    Data(const std::vector<::ygg::View<ygg::Index<::tyr::formalism::Variable>, C>>& variables_,
         ::ygg::View<ygg::Index<::tyr::formalism::datalog::ConjunctiveCondition>, C> condition_,
         ::ygg::View<ygg::Index<::tyr::formalism::datalog::ConjunctiveEffect>, C> effect_) :
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

static_assert(!ygg::uses_trivial_storage_v<::tyr::formalism::datalog::ConditionalEffect>);

}

#endif
