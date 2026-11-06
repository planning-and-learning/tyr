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
#include <urd/elements/flat.hpp>

/// A fact: its variable and value.
template<::tyr::formalism::FactKind T, typename V>
struct urd::view_mixin<::tyr::formalism::planning::FDRFact<T>, V>
{
private:
    [[nodiscard]] const V& self() const noexcept { return static_cast<const V&>(*this); }

public:
    auto get_variable() const noexcept { return self().template get<&urd::record_of<V>::variable>(); }
    auto get_value() const noexcept { return self().data().value; }
    auto has_value() const noexcept { return get_value() != ::tyr::formalism::planning::FDRValue::none(); }
    auto get_atom_index() const noexcept
    { return has_value() ? std::make_optional(self().context()[self().data().variable].atoms[get_value().value - 1]) : std::nullopt; }
    auto get_atom() const noexcept { return urd::resolve(get_atom_index(), self().context()); }
};

/// A fact read through a context: its view through the context that owns its variable, which borrows it (never a
/// temporary: a fact computed on the fly is viewed as an urd::owned_view).
template<::tyr::formalism::FactKind T>
struct urd::resolution<urd::data<::tyr::formalism::planning::FDRFact<T>>>
{
    template<typename M, typename C>
        requires std::constructible_from<urd::borrowed<urd::data<::tyr::formalism::planning::FDRFact<T>>>, M>
    static urd::borrowed_view<::tyr::formalism::planning::FDRFact<T>, C> of(M&& fact, const C& context)
    { return { std::forward<M>(fact), context.view(fact.variable).context() }; }
};
static_assert(std::is_trivially_copyable_v<urd::data<::tyr::formalism::planning::FDRFact<::tyr::formalism::FluentTag>>>);

#endif
