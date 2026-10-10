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

#ifndef TYR_FORMALISM_DATALOG_PROGRAM_VIEW_HPP_
#define TYR_FORMALISM_DATALOG_PROGRAM_VIEW_HPP_

#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/datalog/rule_view.hpp"
#include "tyr/formalism/declarations.hpp"

#include <yggdrasil/containers/optional.hpp>
#include <yggdrasil/containers/vector.hpp>
#include <yggdrasil/core/types.hpp>

namespace ygg
{

template<::tyr::TaskKind T, ygg::formalism::SymbolContextFor<::tyr::formalism::datalog::Program<T>> C>
class View<ygg::Index<::tyr::formalism::datalog::Program<T>>, C> : public ygg::IndexViewBase<::tyr::formalism::datalog::Program<T>, C>
{
public:
    View(ygg::Index<::tyr::formalism::datalog::Program<T>> handle, const C& context) noexcept :
        ygg::IndexViewBase<::tyr::formalism::datalog::Program<T>, C>(handle, context)
    {
    }

    template<::tyr::formalism::FactKind F>
    auto get_predicates() const noexcept
    {
        return ygg::make_view(this->get_data().template get_predicates<F>(), this->get_context());
    }
    template<::tyr::formalism::FactKind F>
    auto get_functions() const noexcept
    {
        return ygg::make_view(this->get_data().template get_functions<F>(), this->get_context());
    }
    auto get_objects() const noexcept { return ygg::make_view(this->get_data().objects, this->get_context()); }
    template<::tyr::formalism::FactKind F>
    auto get_atoms() const noexcept
    {
        return ygg::make_view(this->get_data().template get_atoms<F>(), this->get_context());
    }
    template<::tyr::formalism::FactKind F>
    auto get_fterm_values() const noexcept
    {
        return ygg::make_view(this->get_data().template get_fterm_values<F>(), this->get_context());
    }
    auto get_goal() const noexcept { return ygg::make_view(this->get_data().goal, this->get_context()); }
    auto get_metric() const noexcept { return ygg::make_view(this->get_data().metric, this->get_context()); }
    template<::tyr::formalism::RelationKind R>
    auto get_rules() const noexcept
    {
        return ygg::make_view(this->get_data().template get_rules<R>(), this->get_context());
    }
};

}

#endif
