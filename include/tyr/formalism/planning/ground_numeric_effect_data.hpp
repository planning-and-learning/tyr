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

#ifndef TYR_FORMALISM_PLANNING_GROUND_NUMERIC_EFFECT_DATA_HPP_
#define TYR_FORMALISM_PLANNING_GROUND_NUMERIC_EFFECT_DATA_HPP_

#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/ground_function_expression_data.hpp"
#include "tyr/formalism/planning/ground_function_term_index.hpp"
#include "tyr/formalism/planning/ground_numeric_effect_index.hpp"

#include <stdexcept>
#include <yggdrasil/core/types.hpp>
#include <yggdrasil/core/types_utils.hpp>

namespace ygg
{

template<::tyr::formalism::FactKind T>
struct Data<::tyr::formalism::planning::GroundNumericEffect<T>>
{
    static_assert(std::same_as<T, ::tyr::formalism::FluentTag> || std::same_as<T, ::tyr::formalism::AuxiliaryTag>,
                  "Unsupported GroundNumericEffect<T> specialization.");
    static constexpr auto default_operator = std::same_as<T, ::tyr::formalism::AuxiliaryTag> ? ::tyr::formalism::NumericEffectOperatorKind::Increase :
                                                                                               ::tyr::formalism::NumericEffectOperatorKind::Assign;

    ygg::Index<::tyr::formalism::planning::GroundNumericEffect<T>> index;
    ::tyr::formalism::NumericEffectOperatorKind operator_kind = default_operator;
    ygg::Index<::tyr::formalism::planning::GroundFunctionTerm<T>> fterm;
    ygg::Data<::tyr::formalism::planning::GroundFunctionExpression> fexpr;

    Data() = default;
    Data(::tyr::formalism::NumericEffectOperatorKind operator_kind_,
         ygg::Index<::tyr::formalism::planning::GroundFunctionTerm<T>> fterm_,
         ygg::Data<::tyr::formalism::planning::GroundFunctionExpression> fexpr_) :
        index(),
        operator_kind(operator_kind_),
        fterm(fterm_),
        fexpr(fexpr_)
    {
        if constexpr (std::same_as<T, ::tyr::formalism::AuxiliaryTag>)
            if (operator_kind != default_operator)
                throw std::invalid_argument("auxiliary numeric effect must be Increase");
    }
    // Python constructor
    template<typename C>
    Data(::tyr::formalism::NumericEffectOperatorKind operator_kind_,
         ::ygg::View<ygg::Index<::tyr::formalism::planning::GroundFunctionTerm<T>>, C> fterm_,
         ::ygg::View<ygg::Data<::tyr::formalism::planning::GroundFunctionExpression>, C> fexpr_) :
        index(),
        operator_kind(operator_kind_),
        fterm(),
        fexpr()
    {
        if constexpr (std::same_as<T, ::tyr::formalism::AuxiliaryTag>)
            if (operator_kind != default_operator)
                throw std::invalid_argument("auxiliary numeric effect must be Increase");
        ygg::set(fterm_, fterm);
        ygg::set(fexpr_, fexpr);
    }
    Data(const Data& other) = default;
    Data& operator=(const Data& other) = default;
    Data(Data&& other) = default;
    Data& operator=(Data&& other) = default;

    void clear() noexcept
    {
        ygg::clear(index);
        operator_kind = default_operator;
        ygg::clear(fterm);
        ygg::clear(fexpr);
    }

    auto cista_members() const noexcept { return std::tie(index, operator_kind, fterm, fexpr); }
    auto identifying_members() const noexcept { return std::tie(operator_kind, fterm, fexpr); }
};

static_assert(!ygg::uses_trivial_storage_v<::tyr::formalism::planning::GroundNumericEffect<::tyr::formalism::FluentTag>>);
}

#endif
