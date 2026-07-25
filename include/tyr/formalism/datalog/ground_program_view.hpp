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

#ifndef TYR_FORMALISM_DATALOG_GROUND_PROGRAM_VIEW_HPP_
#define TYR_FORMALISM_DATALOG_GROUND_PROGRAM_VIEW_HPP_

#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/datalog/ground_atom_index.hpp"
#include "tyr/formalism/datalog/ground_conjunctive_condition_index.hpp"
#include "tyr/formalism/datalog/ground_function_term_value_index.hpp"
#include "tyr/formalism/datalog/ground_program_index.hpp"
#include "tyr/formalism/datalog/ground_rule_index.hpp"
#include "tyr/formalism/datalog/metric_index.hpp"
#include "tyr/formalism/function_index.hpp"
#include "tyr/formalism/predicate_index.hpp"

#include <yggdrasil/containers/optional.hpp>
#include <yggdrasil/containers/vector.hpp>
#include <yggdrasil/core/types.hpp>

namespace ygg
{
using namespace ::tyr;
template<::tyr::formalism::datalog::Context C>
class View<ygg::Index<::tyr::formalism::datalog::GroundProgram>, C>
{
private:
    const C* m_context;
    ygg::Index<::tyr::formalism::datalog::GroundProgram> m_handle;

public:
    View(ygg::Index<::tyr::formalism::datalog::GroundProgram> data, const C& context) noexcept : m_context(&context), m_handle(data) {}

    const auto& get_data() const noexcept { return get_repository(*m_context)[m_handle]; }
    const auto& get_context() const noexcept { return *m_context; }
    const auto& get_handle() const noexcept { return m_handle; }

    auto get_index() const noexcept { return m_handle; }
    template<::tyr::formalism::FactKind T>
    auto get_predicates() const noexcept
    {
        return ygg::make_view(get_data().template get_predicates<T>(), *m_context);
    }
    template<::tyr::formalism::FactKind T>
    auto get_functions() const noexcept
    {
        return ygg::make_view(get_data().template get_functions<T>(), *m_context);
    }
    auto get_objects() const noexcept { return ygg::make_view(get_data().objects, *m_context); }
    template<::tyr::formalism::FactKind T>
    auto get_atoms() const noexcept
    {
        return ygg::make_view(get_data().template get_atoms<T>(), *m_context);
    }
    template<::tyr::formalism::FactKind T>
    auto get_fterm_values() const noexcept
    {
        return ygg::make_view(get_data().template get_fterm_values<T>(), *m_context);
    }
    auto get_goal() const noexcept { return ygg::make_view(get_data().goal, *m_context); }
    auto get_metric() const noexcept { return ygg::make_view(get_data().metric, *m_context); }
    template<::tyr::formalism::RelationKind R>
    auto get_ground_rules() const noexcept
    {
        return ygg::make_view(get_data().template get_ground_rules<R>(), *m_context);
    }

    auto identifying_members() const noexcept { return std::tie(m_handle, m_context->get_index()); }
};

}

#endif
