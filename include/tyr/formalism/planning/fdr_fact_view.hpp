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

#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/fdr_fact_data.hpp"
#include "tyr/formalism/planning/fdr_value.hpp"
#include "tyr/formalism/planning/fdr_variable_data.hpp"

#include <optional>

#include <yggdrasil/core/types.hpp>

namespace ygg
{
template<::tyr::formalism::FactKind T, ::tyr::formalism::planning::Context C>
class View<ygg::Data<::tyr::formalism::planning::FDRFact<T>>, C> : public ygg::DataViewBase<::tyr::formalism::planning::FDRFact<T>, C>
{
    // FDRContext::get_fact returns views of facts computed on the fly.
    static_assert(ygg::uses_trivial_storage_v<::tyr::formalism::planning::FDRFact<T>>);

public:
    View(const ygg::Data<::tyr::formalism::planning::FDRFact<T>>& handle, const C& context) noexcept : ygg::DataViewBase<::tyr::formalism::planning::FDRFact<T>, C>(handle, context) {}

    auto get_variable() const noexcept { return ygg::make_view(this->get_data().variable, this->get_context()); }
    auto get_value() const noexcept { return this->get_data().value; }
    auto has_value() const noexcept { return get_value() != ::tyr::formalism::planning::FDRValue::none(); }
    auto get_atom_index() const noexcept
    {
        return has_value() ? std::make_optional(this->get_repository()[this->get_data().variable].atoms[ygg::uint_t(get_value() - 1)]) : std::nullopt;
    }
    auto get_atom() const noexcept
    {
        const auto atom = get_atom_index();
        return atom ? std::make_optional(ygg::make_view(*atom, this->get_context())) : std::nullopt;
    }
};

/// Canonical context depends on variable.
template<::tyr::formalism::FactKind T, typename C>
const C& get_canonical_context(const ygg::Data<::tyr::formalism::planning::FDRFact<T>>& element, const C& context) noexcept
{
    return ygg::make_view(element.variable, context).get_context();
}

}

#endif
