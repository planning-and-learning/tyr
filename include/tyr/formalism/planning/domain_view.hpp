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

#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/function_view.hpp"
#include "tyr/formalism/object_view.hpp"
#include "tyr/formalism/planning/action_view.hpp"
#include "tyr/formalism/planning/axiom_view.hpp"
#include "tyr/formalism/planning/domain_data.hpp"
#include "tyr/formalism/predicate_view.hpp"

#include <fmt/base.h>
#include <urd/elements/flat.hpp>
#include <urd/elements/view.hpp>
#include <yggdrasil/formatting/without_format_spec.hpp>

namespace urd
{
template<typename V>
struct view_mixin<::tyr::formalism::Domain, V>
{
private:
    [[nodiscard]] const V& self() const noexcept { return static_cast<const V&>(*this); }

public:
    decltype(auto) name() const noexcept { return self().template member<&urd::record_of<V>::name>(); }

    /// What it requires of a planner, as its parser set them from what it uses.
    decltype(auto) requirements() const noexcept { return self().template member<&urd::record_of<V>::requirements>(); }

    auto static_predicates() const noexcept { return self().template member<&urd::record_of<V>::static_predicates>(); }

    auto fluent_predicates() const noexcept { return self().template member<&urd::record_of<V>::fluent_predicates>(); }

    auto derived_predicates() const noexcept { return self().template member<&urd::record_of<V>::derived_predicates>(); }

    auto static_functions() const noexcept { return self().template member<&urd::record_of<V>::static_functions>(); }

    auto fluent_functions() const noexcept { return self().template member<&urd::record_of<V>::fluent_functions>(); }

    auto auxiliary_function() const noexcept { return self().template member<&urd::record_of<V>::auxiliary_function>(); }

    auto constants() const noexcept { return self().template member<&urd::record_of<V>::constants>(); }

    auto actions() const noexcept { return self().template member<&urd::record_of<V>::actions>(); }

    auto axioms() const noexcept { return self().template member<&urd::record_of<V>::axioms>(); }

    // user: one getter templated on the tag over the fields it chooses (next to the record), besides the plain getters.
    template<::tyr::formalism::FactKind T>
    auto predicates() const noexcept
    { return self().template member<::tyr::formalism::domain_predicates_of<T>>(); }

    // user: one getter templated on the tag over the fields it chooses (next to the record), besides the plain getters.
    template<::tyr::formalism::FactKind T>
    auto functions() const noexcept
    { return self().template member<::tyr::formalism::domain_functions_of<T>>(); }
};
}  // namespace urd

/// A domain: (define (domain name) its requirements, constants, predicates and functions (one per line), axioms and actions (one per line)).
template<>
struct fmt::formatter<urd::index_view<tyr::formalism::Domain, tyr::formalism::Repository>> : ygg::without_format_spec
{
    fmt::format_context::iterator format(const urd::index_view<tyr::formalism::Domain, tyr::formalism::Repository>& value, fmt::format_context& context) const;
};

#endif
