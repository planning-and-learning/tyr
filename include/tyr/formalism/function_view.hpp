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

#ifndef TYR_FORMALISM_FUNCTION_VIEW_HPP_
#define TYR_FORMALISM_FUNCTION_VIEW_HPP_

#include <yggdrasil/core/types.hpp>
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/function_index.hpp"

namespace ygg
{
template<::tyr::formalism::FactKind T, typename C>
class View<ygg::Index<::tyr::formalism::Function<T>>, C>
{
private:
    const C* m_context;
    ygg::Index<::tyr::formalism::Function<T>> m_handle;

public:
    View(ygg::Index<::tyr::formalism::Function<T>> handle, const C& context) noexcept : m_context(&context), m_handle(handle) {}

    const auto& get_data() const noexcept { return get_repository(*m_context)[m_handle]; }
    const auto& get_context() const noexcept { return *m_context; }
    const auto& get_handle() const noexcept { return m_handle; }

    auto get_index() const noexcept { return m_handle; }
    const auto& get_name() const noexcept { return get_data().name; }
    auto get_arity() const noexcept { return get_data().arity; }

    auto identifying_members() const noexcept { return std::tie(m_handle, m_context->get_index()); }
};
}

#endif
