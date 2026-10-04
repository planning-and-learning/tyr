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

#ifndef TYR_FORMALISM_PLANNING_DOMAIN_VIEW_HPP_
#define TYR_FORMALISM_PLANNING_DOMAIN_VIEW_HPP_

#include "tyr/formalism/function_view.hpp"
#include "tyr/formalism/object_view.hpp"
#include "tyr/formalism/planning/action_view.hpp"
#include "tyr/formalism/planning/axiom_view.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/domain_index.hpp"
#include "tyr/formalism/predicate_view.hpp"

#include <yggdrasil/containers/optional.hpp>
#include <yggdrasil/containers/vector.hpp>
#include <yggdrasil/core/types.hpp>
#include <yggdrasil/formalism/detail/view.hpp>

namespace ygg
{

template<ygg::formalism::SymbolContextFor<::tyr::formalism::planning::Domain> C>
class View<ygg::Index<::tyr::formalism::planning::Domain>, C> : public ygg::formalism::detail::View<ygg::Index<::tyr::formalism::planning::Domain>, C>
{
public:
    View(ygg::Index<::tyr::formalism::planning::Domain> handle, const C& context) noexcept :
        ygg::formalism::detail::View<ygg::Index<::tyr::formalism::planning::Domain>, C>(handle, context)
    {
    }

    const auto& get_name() const noexcept { return this->get_data().name; }
    template<::tyr::formalism::FactKind T>
    auto get_predicates() const noexcept
    {
        return ygg::make_view(this->get_data().template get_predicates<T>(), *this->m_context);
    }
    template<::tyr::formalism::FactKind T>
    auto get_functions() const noexcept
    {
        return ygg::make_view(this->get_data().template get_functions<T>(), *this->m_context);
    }
    auto get_auxiliary_function() const noexcept { return ygg::make_view(this->get_data().auxiliary_function, *this->m_context); }
    auto get_constants() const noexcept { return ygg::make_view(this->get_data().constants, *this->m_context); }
    auto get_actions() const noexcept { return ygg::make_view(this->get_data().actions, *this->m_context); }
    auto get_axioms() const noexcept { return ygg::make_view(this->get_data().axioms, *this->m_context); }
};

}

#endif
