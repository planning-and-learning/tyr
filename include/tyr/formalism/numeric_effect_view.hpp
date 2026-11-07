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

#ifndef TYR_FORMALISM_NUMERIC_EFFECT_VIEW_HPP_
#define TYR_FORMALISM_NUMERIC_EFFECT_VIEW_HPP_

#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/function_term_view.hpp"

#include <urd/elements/flat.hpp>

namespace urd
{
template<::tyr::TaskKind T, ::tyr::formalism::FactKind F, typename V>
struct view_mixin<::tyr::formalism::NumericEffect<T, F>, V>
{
private:
    [[nodiscard]] const V& self() const noexcept { return static_cast<const V&>(*this); }

public:
    static_assert(std::same_as<F, ::tyr::formalism::FluentTag> || std::same_as<F, ::tyr::formalism::AuxiliaryTag>,
                  "Unsupported NumericEffect<F> specialization.");

public:

    auto get_operator() const noexcept { return self().data().operator_kind; }
    auto get_fterm() const noexcept { return self().template get<&urd::record_of<V>::fterm>(); }
    auto get_fexpr() const noexcept { return self().template get<&urd::record_of<V>::fexpr>(); }
};
}  // namespace urd

#endif
