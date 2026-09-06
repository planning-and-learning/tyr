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

#ifndef TYR_FORMALISM_DATALOG_NUMERIC_EFFECT_DATA_HPP_
#define TYR_FORMALISM_DATALOG_NUMERIC_EFFECT_DATA_HPP_

#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/datalog/function_expression_data.hpp"
#include "tyr/formalism/datalog/function_term_index.hpp"
#include "tyr/formalism/datalog/numeric_effect_index.hpp"

#include <yggdrasil/core/types.hpp>
#include <yggdrasil/core/types_utils.hpp>

namespace ygg
{

template<::tyr::TaskKind T, ::tyr::formalism::FactKind F>
struct Data<::tyr::formalism::datalog::NumericEffect<T, F>>
{
    static_assert(std::same_as<F, ::tyr::formalism::FluentTag>, "Datalog numeric effects are currently only supported for fluent functions.");

    ygg::Index<::tyr::formalism::datalog::NumericEffect<T, F>> index;
    ::tyr::formalism::NumericEffectOperatorKind operator_kind = ::tyr::formalism::NumericEffectOperatorKind::Assign;
    ygg::Index<::tyr::formalism::datalog::FunctionTerm<T, F>> fterm;
    ygg::Data<::tyr::formalism::datalog::FunctionExpression<T>> fexpr;

    Data() = default;
    Data(::tyr::formalism::NumericEffectOperatorKind operator_kind_,
         ygg::Index<::tyr::formalism::datalog::FunctionTerm<T, F>> fterm_,
         ygg::Data<::tyr::formalism::datalog::FunctionExpression<T>> fexpr_) :
        index(),
        operator_kind(operator_kind_),
        fterm(fterm_),
        fexpr(fexpr_)
    {
    }
    template<typename C>
    Data(::tyr::formalism::NumericEffectOperatorKind operator_kind_,
         ::ygg::View<ygg::Index<::tyr::formalism::datalog::FunctionTerm<T, F>>, C> fterm_,
         ::ygg::View<ygg::Data<::tyr::formalism::datalog::FunctionExpression<T>>, C> fexpr_) :
        index(),
        operator_kind(operator_kind_),
        fterm(),
        fexpr()
    {
        set(fterm_, fterm);
        set(fexpr_, fexpr);
    }

    void clear() noexcept
    {
        ygg::clear(index);
        operator_kind = ::tyr::formalism::NumericEffectOperatorKind::Assign;
        ygg::clear(fterm);
        ygg::clear(fexpr);
    }

    auto cista_members() const noexcept { return std::tie(index, operator_kind, fterm, fexpr); }
    auto identifying_members() const noexcept { return std::tie(operator_kind, fterm, fexpr); }
};

static_assert(!ygg::uses_trivial_storage_v<::tyr::formalism::datalog::NumericEffect<::tyr::LiftedTag, ::tyr::formalism::FluentTag>>);
static_assert(!ygg::uses_trivial_storage_v<::tyr::formalism::datalog::NumericEffect<::tyr::GroundTag, ::tyr::formalism::FluentTag>>);

}

#endif
