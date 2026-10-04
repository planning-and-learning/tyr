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

#ifndef TYR_FORMALISM_PREDICATE_VIEW_HPP_
#define TYR_FORMALISM_PREDICATE_VIEW_HPP_

#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/predicate_index.hpp"

#include <yggdrasil/core/types.hpp>
#include <yggdrasil/formalism/detail/view.hpp>

namespace ygg
{
template<::tyr::formalism::FactKind T, ygg::formalism::SymbolContextFor<::tyr::formalism::Predicate<T>> C>
class View<ygg::Index<::tyr::formalism::Predicate<T>>, C> : public ygg::formalism::detail::View<ygg::Index<::tyr::formalism::Predicate<T>>, C>
{
public:
    View(ygg::Index<::tyr::formalism::Predicate<T>> handle, const C& context) noexcept :
        ygg::formalism::detail::View<ygg::Index<::tyr::formalism::Predicate<T>>, C>(handle, context)
    {
    }

    const auto& get_name() const noexcept { return this->get_data().name; }
    auto get_arity() const noexcept { return this->get_data().arity; }
};
}

#endif
