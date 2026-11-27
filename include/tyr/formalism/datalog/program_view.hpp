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

#include "tyr/declarations.hpp"
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/datalog/program_data.hpp"
#include "tyr/formalism/datalog/rule_view.hpp"
#include "tyr/formalism/declarations.hpp"

#include <algorithm>
#include <cstddef>
#include <fmt/base.h>
#include <urd/elements/flat.hpp>
#include <urd/elements/view.hpp>
#include <yggdrasil/formatting/without_format_spec.hpp>

namespace urd
{
template<::tyr::TaskKind T, typename V>
struct view_mixin<::tyr::formalism::Program<T>, V>
{
private:
    [[nodiscard]] const V& self() const noexcept { return static_cast<const V&>(*this); }

public:
    auto get_static_predicates() const noexcept { return self().template member<&urd::record_of<V>::static_predicates>(); }

    auto get_fluent_predicates() const noexcept { return self().template member<&urd::record_of<V>::fluent_predicates>(); }

    auto get_derived_predicates() const noexcept { return self().template member<&urd::record_of<V>::derived_predicates>(); }

    auto get_static_functions() const noexcept { return self().template member<&urd::record_of<V>::static_functions>(); }

    auto get_fluent_functions() const noexcept { return self().template member<&urd::record_of<V>::fluent_functions>(); }

    auto get_objects() const noexcept { return self().template member<&urd::record_of<V>::objects>(); }

    auto get_static_atoms() const noexcept { return self().template member<&urd::record_of<V>::static_atoms>(); }

    auto get_fluent_atoms() const noexcept { return self().template member<&urd::record_of<V>::fluent_atoms>(); }

    auto get_derived_atoms() const noexcept { return self().template member<&urd::record_of<V>::derived_atoms>(); }

    auto get_static_fterm_values() const noexcept { return self().template member<&urd::record_of<V>::static_fterm_values>(); }

    auto get_fluent_fterm_values() const noexcept { return self().template member<&urd::record_of<V>::fluent_fterm_values>(); }

    auto get_goal() const noexcept { return self().template member<&urd::record_of<V>::goal>(); }

    auto get_rules() const noexcept { return self().template member<&urd::record_of<V>::rules>(); }

    // user: one getter templated on the tag over the fields it chooses (next to the record), besides the plain getters.
    template<::tyr::formalism::FactKind F>
    auto get_predicates() const noexcept
    { return self().template member<::tyr::formalism::program_predicates_of<F, T>>(); }

    // user: one getter templated on the tag over the fields it chooses (next to the record), besides the plain getters.
    template<::tyr::formalism::FactKind F>
    auto get_functions() const noexcept
    { return self().template member<::tyr::formalism::program_functions_of<F, T>>(); }

    // user: one getter templated on the tag over the fields it chooses (next to the record), besides the plain getters.
    template<::tyr::formalism::FactKind F>
    auto get_atoms() const noexcept
    { return self().template member<::tyr::formalism::program_atoms_of<F, T>>(); }

    // user: one getter templated on the tag over the fields it chooses (next to the record), besides the plain getters.
    template<::tyr::formalism::FactKind F>
    auto get_fterm_values() const noexcept
    { return self().template member<::tyr::formalism::program_fterm_values_of<F, T>>(); }
};
}  // namespace urd

namespace tyr::formalism
{
/// The size of a table indexed by the indices of a program's rules: they sit in the program's layers, where its
/// transforms left them, so they need not be dense.
template<typename Rules>
std::size_t rule_index_bound(const Rules& rules)
{
    auto bound = std::size_t { 0 };
    for (const auto rule : rules)
        bound = std::max(bound, static_cast<std::size_t>(rule.index().value) + 1);
    return bound;
}
}

/// A program: its predicates, functions and objects as comments, then its facts, function values and rules, one per line, and its goal as a query ?- body.
template<tyr::TaskKind T>
struct fmt::formatter<urd::index_view<tyr::formalism::Program<T>, tyr::formalism::Repository>> : ygg::without_format_spec
{
    fmt::format_context::iterator format(const urd::index_view<tyr::formalism::Program<T>, tyr::formalism::Repository>& value,
                                         fmt::format_context& context) const;
};

#endif
