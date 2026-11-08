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

#ifndef TYR_FORMALISM_DATALOG_CONJUNCTIVE_CONDITION_VIEW_HPP_
#define TYR_FORMALISM_DATALOG_CONJUNCTIVE_CONDITION_VIEW_HPP_

#include "tyr/formalism/binary_operator_view.hpp"
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/literal_view.hpp"
#include "tyr/formalism/variable_view.hpp"

#include <urd/elements/flat.hpp>

namespace urd
{
template<::tyr::TaskKind T, typename V>
struct view_mixin<::tyr::formalism::datalog::ConjunctiveCondition<T>, V>
{
private:
    [[nodiscard]] const V& self() const noexcept { return static_cast<const V&>(*this); }

public:
    auto get_variables() const noexcept
        requires std::same_as<T, ::tyr::LiftedTag>
    { return self().template member<&urd::record_of<V>::variables>(); }
    template<::tyr::formalism::FactKind F>
    auto get_literals() const noexcept
    { return urd::resolve(self().data().template get_literals<F>(), self().context()); }
    auto get_numeric_constraints() const noexcept { return self().template member<&urd::record_of<V>::numeric_constraints>(); }
    auto get_arity() const noexcept
        requires std::same_as<T, ::tyr::LiftedTag>
    { return self().data().variables.size(); }
};
}  // namespace urd

#endif
