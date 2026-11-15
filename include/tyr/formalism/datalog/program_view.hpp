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
    template<::tyr::formalism::FactKind F>
    auto get_predicates() const noexcept
    {
        return self().template member<::tyr::formalism::datalog::predicates_of<F, T>>();
    }
    template<::tyr::formalism::FactKind F>
    auto get_functions() const noexcept
    {
        return self().template member<::tyr::formalism::datalog::functions_of<F, T>>();
    }
    auto get_objects() const noexcept { return self().template member<&urd::record_of<V>::objects>(); }
    template<::tyr::formalism::FactKind F>
    auto get_atoms() const noexcept
    {
        return self().template member<::tyr::formalism::datalog::atoms_of<F, T>>();
    }
    template<::tyr::formalism::FactKind F>
    auto get_fterm_values() const noexcept
    {
        return self().template member<::tyr::formalism::datalog::fterm_values_of<F, T>>();
    }
    auto get_goal() const noexcept { return self().template member<&urd::record_of<V>::goal>(); }
    auto get_rules() const noexcept { return self().template member<&urd::record_of<V>::rules>(); }
};
}  // namespace urd

#endif
