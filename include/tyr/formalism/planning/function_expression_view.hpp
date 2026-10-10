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

#ifndef TYR_FORMALISM_PLANNING_FUNCTION_EXPRESSION_VIEW_HPP_
#define TYR_FORMALISM_PLANNING_FUNCTION_EXPRESSION_VIEW_HPP_

#include "tyr/formalism/planning/arithmetic_operator_view.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/function_expression_data.hpp"

#include <yggdrasil/containers/variant.hpp>
#include <yggdrasil/core/types.hpp>

namespace ygg
{
template<::tyr::TaskKind T>
inline constexpr bool stores_view_by_value_v<::tyr::formalism::planning::FunctionExpression<T>> = true;

template<::tyr::TaskKind T, ::tyr::formalism::planning::Context C>
class View<ygg::Data<::tyr::formalism::planning::FunctionExpression<T>>, C> : public ygg::DataViewBase<::tyr::formalism::planning::FunctionExpression<T>, C>
{
public:
    View(const ygg::Data<::tyr::formalism::planning::FunctionExpression<T>>& handle, const C& context) noexcept : ygg::DataViewBase<::tyr::formalism::planning::FunctionExpression<T>, C>(handle, context) {}

    auto get_variant() const noexcept { return ygg::make_view(this->get_data().variant, this->get_context()); }
};

template<::tyr::TaskKind T, typename C>
const C& get_canonical_context(const ygg::Data<::tyr::formalism::planning::FunctionExpression<T>>& element, const C& context) noexcept
{
    return std::visit(
        [&](const auto& arg) -> decltype(auto)
        {
            using Alternative = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<Alternative, ygg::float_t>)
                return context.get_root();
            else
                return ygg::make_view(arg, context).get_context();
        },
        element.variant);
}

}

#endif
