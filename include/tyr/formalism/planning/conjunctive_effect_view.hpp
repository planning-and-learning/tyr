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

#ifndef TYR_FORMALISM_PLANNING_CONJUNCTIVE_EFFECT_VIEW_HPP_
#define TYR_FORMALISM_PLANNING_CONJUNCTIVE_EFFECT_VIEW_HPP_

#include "tyr/formalism/planning/conjunctive_effect_index.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/fdr_fact_view.hpp"
#include "tyr/formalism/planning/literal_view.hpp"
#include "tyr/formalism/planning/numeric_effect_operator_view.hpp"

#include <yggdrasil/containers/optional.hpp>
#include <yggdrasil/containers/vector.hpp>
#include <yggdrasil/core/types.hpp>
#include <yggdrasil/formalism/detail/view.hpp>

namespace ygg
{

template<::tyr::TaskKind T, ygg::formalism::SymbolContextFor<::tyr::formalism::planning::ConjunctiveEffect<T>> C>
class View<ygg::Index<::tyr::formalism::planning::ConjunctiveEffect<T>>, C> :
    public ygg::formalism::detail::View<ygg::Index<::tyr::formalism::planning::ConjunctiveEffect<T>>, C>
{
public:
    View(ygg::Index<::tyr::formalism::planning::ConjunctiveEffect<T>> handle, const C& context) noexcept :
        ygg::formalism::detail::View<ygg::Index<::tyr::formalism::planning::ConjunctiveEffect<T>>, C>(handle, context)
    {
    }

    auto get_literals() const noexcept
        requires std::same_as<T, ::tyr::LiftedTag>
    {
        return ygg::make_view(this->get_data().literals, *this->m_context);
    }
    auto get_numeric_effects() const noexcept { return ygg::make_view(this->get_data().numeric_effects, *this->m_context); }
    auto get_auxiliary_numeric_effect() const noexcept { return ygg::make_view(this->get_data().auxiliary_numeric_effect, *this->m_context); }

    template<::tyr::formalism::PolarityKind F>
    auto get_facts() const noexcept
        requires std::same_as<T, ::tyr::GroundTag>
    {
        return ygg::make_view(this->get_data().template get_facts<F>(), *this->m_context);
    }
};

}

#endif
