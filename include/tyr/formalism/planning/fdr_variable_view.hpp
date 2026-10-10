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

#ifndef TYR_FORMALISM_PLANNING_FDR_VARIABLE_VIEW_HPP_
#define TYR_FORMALISM_PLANNING_FDR_VARIABLE_VIEW_HPP_

#include "tyr/formalism/planning/declarations.hpp"

#include <yggdrasil/core/types.hpp>

namespace ygg
{
template<::tyr::formalism::FactKind T, ygg::formalism::SymbolContextFor<::tyr::formalism::planning::FDRVariable<T>> C>
class View<ygg::Index<::tyr::formalism::planning::FDRVariable<T>>, C> :
    public ygg::IndexViewBase<::tyr::formalism::planning::FDRVariable<T>, C>
{
public:
    View(ygg::Index<::tyr::formalism::planning::FDRVariable<T>> handle, const C& context) noexcept :
        ygg::IndexViewBase<::tyr::formalism::planning::FDRVariable<T>, C>(handle, context)
    {
    }

    auto get_domain_size() const noexcept { return get_atoms().size() + 1; }
    auto get_atoms() const noexcept { return ygg::make_view(this->get_data().atoms, this->get_context()); }
};

}

#endif
