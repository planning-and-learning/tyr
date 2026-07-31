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

#ifndef TYR_FORMALISM_PLANNING_FDR_FACT_VIEW_HPP_
#define TYR_FORMALISM_PLANNING_FDR_FACT_VIEW_HPP_

#include <yggdrasil/core/types.hpp>
#include "tyr/formalism/object_index.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/fdr_fact_data.hpp"
#include "tyr/formalism/planning/fdr_value.hpp"

namespace ygg
{
template<::tyr::formalism::FactKind T, ::tyr::formalism::planning::Context C>
class View<ygg::Data<::tyr::formalism::planning::FDRFact<T>>, C>
{
private:
    const C* m_context;
    ygg::Data<::tyr::formalism::planning::FDRFact<T>> m_handle;

public:
    View(ygg::Data<::tyr::formalism::planning::FDRFact<T>> handle, const C& context) noexcept : m_context(&context), m_handle(handle) {}

    const auto& get_data() const noexcept { return m_handle; }
    const auto& get_context() const noexcept { return *m_context; }
    const auto& get_handle() const noexcept { return m_handle; }

    auto get_variable() const noexcept { return ygg::make_view(get_data().variable, *m_context); }
    auto get_value() const noexcept { return get_data().value; }
    auto has_value() const noexcept { return get_value() != ::tyr::formalism::planning::FDRValue::none(); }
    auto get_atom() const noexcept { return has_value() ? std::make_optional(get_variable().get_atoms()[ygg::uint_t(get_value() - 1)]) : std::nullopt; }

    auto identifying_members() const noexcept { return std::tie(m_handle, m_context->get_index()); }
};

/// Canonical context depends on variable.
template<::tyr::formalism::FactKind T, typename C>
auto make_view(const ygg::Data<::tyr::formalism::planning::FDRFact<T>>& element, const C& context) noexcept
{
    return ygg::View<ygg::Data<::tyr::formalism::planning::FDRFact<T>>, C>(element, ygg::make_view(element.variable, context).get_context());
}

}

#endif
