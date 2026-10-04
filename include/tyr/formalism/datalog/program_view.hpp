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

#include "tyr/formalism/datalog/atom_index.hpp"
#include "tyr/formalism/datalog/conjunctive_condition_index.hpp"
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/datalog/function_term_value_index.hpp"
#include "tyr/formalism/datalog/metric_index.hpp"
#include "tyr/formalism/datalog/program_index.hpp"
#include "tyr/formalism/datalog/rule_index.hpp"
#include "tyr/formalism/datalog/rule_view.hpp"
#include "tyr/formalism/function_index.hpp"
#include "tyr/formalism/predicate_index.hpp"

#include <yggdrasil/containers/optional.hpp>
#include <yggdrasil/containers/vector.hpp>
#include <yggdrasil/core/types.hpp>
#include <yggdrasil/formalism/detail/view.hpp>

namespace ygg
{

template<::tyr::TaskKind T, ygg::formalism::SymbolContextFor<::tyr::formalism::datalog::Program<T>> C>
class View<ygg::Index<::tyr::formalism::datalog::Program<T>>, C> : public ygg::formalism::detail::View<ygg::Index<::tyr::formalism::datalog::Program<T>>, C>
{
public:
    View(ygg::Index<::tyr::formalism::datalog::Program<T>> handle, const C& context) noexcept :
        ygg::formalism::detail::View<ygg::Index<::tyr::formalism::datalog::Program<T>>, C>(handle, context)
    {
    }

    template<::tyr::formalism::FactKind F>
    auto get_predicates() const noexcept
    {
        return ygg::make_view(this->get_data().template get_predicates<F>(), *this->m_context);
    }
    template<::tyr::formalism::FactKind F>
    auto get_functions() const noexcept
    {
        return ygg::make_view(this->get_data().template get_functions<F>(), *this->m_context);
    }
    auto get_objects() const noexcept { return ygg::make_view(this->get_data().objects, *this->m_context); }
    template<::tyr::formalism::FactKind F>
    auto get_atoms() const noexcept
    {
        return ygg::make_view(this->get_data().template get_atoms<F>(), *this->m_context);
    }
    template<::tyr::formalism::FactKind F>
    auto get_fterm_values() const noexcept
    {
        return ygg::make_view(this->get_data().template get_fterm_values<F>(), *this->m_context);
    }
    auto get_goal() const noexcept { return ygg::make_view(this->get_data().goal, *this->m_context); }
    auto get_metric() const noexcept { return ygg::make_view(this->get_data().metric, *this->m_context); }
    template<::tyr::formalism::RelationKind R>
    auto get_rules() const noexcept
    {
        return ygg::make_view(this->get_data().template get_rules<R>(), *this->m_context);
    }
};

}

#endif
