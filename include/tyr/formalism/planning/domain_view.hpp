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

#include <urd/elements/flat.hpp>

namespace urd
{
template<typename V>
struct view_mixin<::tyr::formalism::Domain, V>
{
private:
    [[nodiscard]] const V& self() const noexcept { return static_cast<const V&>(*this); }

public:
    decltype(auto) get_name() const noexcept { return self().template member<&urd::record_of<V>::name>(); }

    auto get_static_predicates() const noexcept { return self().template member<&urd::record_of<V>::static_predicates>(); }

    auto get_fluent_predicates() const noexcept { return self().template member<&urd::record_of<V>::fluent_predicates>(); }

    auto get_derived_predicates() const noexcept { return self().template member<&urd::record_of<V>::derived_predicates>(); }

    auto get_static_functions() const noexcept { return self().template member<&urd::record_of<V>::static_functions>(); }

    auto get_fluent_functions() const noexcept { return self().template member<&urd::record_of<V>::fluent_functions>(); }

    auto get_auxiliary_function() const noexcept { return self().template member<&urd::record_of<V>::auxiliary_function>(); }

    auto get_constants() const noexcept { return self().template member<&urd::record_of<V>::constants>(); }

    auto get_actions() const noexcept { return self().template member<&urd::record_of<V>::actions>(); }

    auto get_axioms() const noexcept { return self().template member<&urd::record_of<V>::axioms>(); }

    // user: one getter templated on the tag over the fields it chooses (next to the record), besides the plain getters.
    template<::tyr::formalism::FactKind T>
    auto get_predicates() const noexcept
    { return self().template member<::tyr::formalism::domain_predicates_of<T>>(); }

    // user: one getter templated on the tag over the fields it chooses (next to the record), besides the plain getters.
    template<::tyr::formalism::FactKind T>
    auto get_functions() const noexcept
    { return self().template member<::tyr::formalism::domain_functions_of<T>>(); }
};
}  // namespace urd

#endif
