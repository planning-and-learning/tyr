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
#ifndef TYR_FORMALISM_PLANNING_INVARIANT_VIEW_HPP_
#define TYR_FORMALISM_PLANNING_INVARIANT_VIEW_HPP_

#include "tyr/formalism/planning/declarations.hpp"

#include <yggdrasil/core/types.hpp>

namespace ygg
{
template<ygg::formalism::SymbolContextFor<::tyr::formalism::planning::Invariant> C>
class View<ygg::Index<::tyr::formalism::planning::Invariant>, C> : public ygg::IndexViewBase<::tyr::formalism::planning::Invariant, C>
{
public:
    View(ygg::Index<::tyr::formalism::planning::Invariant> handle, const C& context) noexcept :
        ygg::IndexViewBase<::tyr::formalism::planning::Invariant, C>(handle, context)
    {
    }

    auto get_num_rigid_variables() const noexcept { return this->get_data().num_rigid_variables; }
    auto get_num_counted_variables() const noexcept { return this->get_data().num_counted_variables; }
    auto get_atoms() const noexcept { return ygg::make_view(this->get_data().atoms, this->get_context()); }
};

}

#endif
