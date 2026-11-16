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

#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/datalog/program_data.hpp"
#include "tyr/formalism/datalog/rule_view.hpp"
#include "tyr/formalism/declarations.hpp"

#include <urd/elements/flat.hpp>

namespace urd
{
template<::tyr::TaskKind T, typename V>
struct view_mixin<::tyr::formalism::datalog::Program<T>, V>
{
private:
    [[nodiscard]] const V& self() const noexcept { return static_cast<const V&>(*this); }

public:
    auto get_static_predicates() const noexcept { return self().template member<&urd::record_of<V>::static_predicates>(); }
    auto get_fluent_predicates() const noexcept { return self().template member<&urd::record_of<V>::fluent_predicates>(); }
    auto get_static_functions() const noexcept { return self().template member<&urd::record_of<V>::static_functions>(); }
    auto get_fluent_functions() const noexcept { return self().template member<&urd::record_of<V>::fluent_functions>(); }
    auto get_objects() const noexcept { return self().template member<&urd::record_of<V>::objects>(); }
    auto get_static_atoms() const noexcept { return self().template member<&urd::record_of<V>::static_atoms>(); }
    auto get_fluent_atoms() const noexcept { return self().template member<&urd::record_of<V>::fluent_atoms>(); }
    auto get_static_fterm_values() const noexcept { return self().template member<&urd::record_of<V>::static_fterm_values>(); }
    auto get_fluent_fterm_values() const noexcept { return self().template member<&urd::record_of<V>::fluent_fterm_values>(); }
    auto get_goal() const noexcept { return self().template member<&urd::record_of<V>::goal>(); }
    auto get_rules() const noexcept { return self().template member<&urd::record_of<V>::rules>(); }
    // user: one getter templated on the tag over the fields it chooses (next to the record), besides the plain getters.
    template<::tyr::formalism::FactKind F>
    auto get_predicates() const noexcept
    { return self().template member<::tyr::formalism::datalog::predicates_of<F, T>>(); }
    // user: one getter templated on the tag over the fields it chooses (next to the record), besides the plain getters.
    template<::tyr::formalism::FactKind F>
    auto get_functions() const noexcept
    { return self().template member<::tyr::formalism::datalog::functions_of<F, T>>(); }
    // user: one getter templated on the tag over the fields it chooses (next to the record), besides the plain getters.
    template<::tyr::formalism::FactKind F>
    auto get_atoms() const noexcept
    { return self().template member<::tyr::formalism::datalog::atoms_of<F, T>>(); }
    // user: one getter templated on the tag over the fields it chooses (next to the record), besides the plain getters.
    template<::tyr::formalism::FactKind F>
    auto get_fterm_values() const noexcept
    { return self().template member<::tyr::formalism::datalog::fterm_values_of<F, T>>(); }
};
}  // namespace urd

#endif
