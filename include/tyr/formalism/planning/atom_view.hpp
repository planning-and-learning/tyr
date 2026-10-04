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

#ifndef TYR_FORMALISM_PLANNING_ATOM_VIEW_HPP_
#define TYR_FORMALISM_PLANNING_ATOM_VIEW_HPP_

#include "tyr/formalism/binding_view.hpp"
#include "tyr/formalism/planning/atom_index.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/predicate_view.hpp"
#include "tyr/formalism/term_data.hpp"

#include <yggdrasil/containers/array.hpp>
#include <yggdrasil/containers/vector.hpp>
#include <yggdrasil/core/types.hpp>
#include <yggdrasil/formalism/detail/view.hpp>

namespace ygg
{
template<::tyr::TaskKind T, ::tyr::formalism::FactKind F, ygg::formalism::SymbolContextFor<::tyr::formalism::planning::Atom<T, F>> C>
class View<ygg::Index<::tyr::formalism::planning::Atom<T, F>>, C> : public ygg::formalism::detail::View<ygg::Index<::tyr::formalism::planning::Atom<T, F>>, C>
{
public:
    View(ygg::Index<::tyr::formalism::planning::Atom<T, F>> handle, const C& context) noexcept :
        ygg::formalism::detail::View<ygg::Index<::tyr::formalism::planning::Atom<T, F>>, C>(handle, context)
    {
    }

    auto get_predicate() const noexcept
    {
        if constexpr (std::same_as<T, ::tyr::GroundTag>)
        {
            return get_row().get_relation();
        }
        else
        {
            return ygg::make_view(this->get_data().predicate, *this->m_context);
        }
    }
    auto get_terms() const noexcept
        requires std::same_as<T, ::tyr::LiftedTag>
    {
        return ygg::make_view(this->get_data().terms, *this->m_context);
    }

    auto get_row() const noexcept
        requires std::same_as<T, ::tyr::GroundTag>
    {
        return ygg::make_view(this->get_data().binding, *this->m_context);
    }
    auto get_objects() const noexcept
        requires std::same_as<T, ::tyr::GroundTag>
    {
        return get_row().get_objects();
    }
    auto get_key() const noexcept
        requires std::same_as<T, ::tyr::GroundTag>
    {
        return get_row().get_key();
    }
};

}

#endif
