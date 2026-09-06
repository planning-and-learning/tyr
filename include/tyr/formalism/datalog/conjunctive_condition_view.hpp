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

#ifndef TYR_FORMALISM_DATALOG_CONJUNCTIVE_CONDITION_VIEW_HPP_
#define TYR_FORMALISM_DATALOG_CONJUNCTIVE_CONDITION_VIEW_HPP_

#include "tyr/formalism/datalog/boolean_operator_view.hpp"
#include "tyr/formalism/datalog/conjunctive_condition_index.hpp"
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/datalog/literal_view.hpp"
#include "tyr/formalism/variable_view.hpp"

#include <yggdrasil/containers/vector.hpp>
#include <yggdrasil/core/types.hpp>

namespace ygg
{

template<::tyr::TaskKind T, ::tyr::formalism::datalog::Context C>
class View<ygg::Index<::tyr::formalism::datalog::ConjunctiveCondition<T>>, C>
{
private:
    const C* m_context;
    ygg::Index<::tyr::formalism::datalog::ConjunctiveCondition<T>> m_handle;

public:
    View(ygg::Index<::tyr::formalism::datalog::ConjunctiveCondition<T>> handle, const C& context) noexcept : m_context(&context), m_handle(handle) {}

    const auto& get_data() const noexcept { return get_repository(*m_context)[m_handle]; }
    const auto& get_context() const noexcept { return *m_context; }
    const auto& get_handle() const noexcept { return m_handle; }

    auto get_index() const noexcept { return m_handle; }
    auto get_variables() const noexcept
        requires std::same_as<T, ::tyr::LiftedTag>
    {
        return ygg::make_view(get_data().variables, *m_context);
    }
    template<::tyr::formalism::FactKind F>
    auto get_literals() const noexcept
    {
        return ygg::make_view(get_data().template get_literals<F>(), *m_context);
    }
    auto get_numeric_constraints() const noexcept { return ygg::make_view(get_data().numeric_constraints, *m_context); }
    auto get_arity() const noexcept
        requires std::same_as<T, ::tyr::LiftedTag>
    {
        return get_data().variables.size();
    }

    auto identifying_members() const noexcept { return std::tie(m_handle, m_context->get_index()); }
};

}

#endif
