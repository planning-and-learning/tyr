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

#ifndef TYR_FORMALISM_PLANNING_CONDITIONAL_EFFECT_VIEW_HPP_
#define TYR_FORMALISM_PLANNING_CONDITIONAL_EFFECT_VIEW_HPP_

#include "tyr/formalism/planning/conjunctive_condition_view.hpp"
#include "tyr/formalism/planning/conjunctive_effect_view.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/variable_view.hpp"

#include <yggdrasil/core/types.hpp>

namespace ygg
{

template<::tyr::TaskKind T, ygg::formalism::SymbolContextFor<::tyr::formalism::planning::ConditionalEffect<T>> C>
class View<ygg::Index<::tyr::formalism::planning::ConditionalEffect<T>>, C> :
    public ygg::IndexViewBase<::tyr::formalism::planning::ConditionalEffect<T>, C>
{
public:
    View(ygg::Index<::tyr::formalism::planning::ConditionalEffect<T>> handle, const C& context) noexcept :
        ygg::IndexViewBase<::tyr::formalism::planning::ConditionalEffect<T>, C>(handle, context)
    {
    }

    auto get_arity() const noexcept
        requires std::same_as<T, ::tyr::LiftedTag>
    {
        return get_variables().size();
    }
    auto get_variables() const noexcept
        requires std::same_as<T, ::tyr::LiftedTag>
    {
        return ygg::make_view(this->get_data().variables, this->get_context());
    }
    auto get_condition() const noexcept { return ygg::make_view(this->get_data().condition, this->get_context()); }
    auto get_effect() const noexcept { return ygg::make_view(this->get_data().effect, this->get_context()); }
};

}

#endif
