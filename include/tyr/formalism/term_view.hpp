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

#ifndef TYR_FORMALISM_TERM_VIEW_HPP_
#define TYR_FORMALISM_TERM_VIEW_HPP_

#include <yggdrasil/core/types.hpp>
#include <yggdrasil/containers/variant.hpp>
#include "tyr/formalism/term_data.hpp"

namespace ygg
{
template<typename C>
class View<ygg::Data<::tyr::formalism::Term>, C>
{
private:
    const C* m_context;
    ygg::Data<::tyr::formalism::Term> m_handle;

public:
    View(ygg::Data<::tyr::formalism::Term> data, const C& context) noexcept : m_context(&context), m_handle(data) {}

    const auto& get_data() const noexcept { return m_handle; }
    const auto& get_context() const noexcept { return *m_context; }
    const auto& get_handle() const noexcept { return m_handle; }

    ygg::View<ygg::Data<::tyr::formalism::Term>::Variant, C> get_variant() const noexcept { return ygg::make_view(m_handle.value, *m_context); }

    auto identifying_members() const noexcept { return std::tie(m_handle, m_context->get_index()); }
};

template<typename C>
ygg::View<ygg::Data<::tyr::formalism::Term>, C> make_view(const ygg::Data<::tyr::formalism::Term>& element, const C& context) noexcept
{
    return ygg::View<ygg::Data<::tyr::formalism::Term>, C>(element,
                                          std::visit(
                                              [&](const auto& arg) -> const C&
                                              {
                                                  using Alternative = std::decay_t<decltype(arg)>;

                                                  if constexpr (std::is_same_v<Alternative, ::tyr::formalism::ParameterIndex>)
                                                      return context.get_root();
                                                  else
                                                      return ygg::make_view(arg, context).get_context();
                                              },
                                              element.value));
}
}

#endif
