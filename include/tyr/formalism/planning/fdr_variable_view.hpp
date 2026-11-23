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

#ifndef TYR_FORMALISM_PLANNING_FDR_VARIABLE_VIEW_HPP_
#define TYR_FORMALISM_PLANNING_FDR_VARIABLE_VIEW_HPP_

#include "tyr/formalism/declarations.hpp"

#include <urd/elements/concepts.hpp>
#include <urd/elements/flat.hpp>
#include <urd/elements/view.hpp>

namespace urd
{
template<::tyr::formalism::FactKind T, typename V>
struct view_mixin<::tyr::formalism::FDRVariable<T>, V>
{
private:
    [[nodiscard]] const V& self() const noexcept { return static_cast<const V&>(*this); }

public:
    auto get_atoms() const noexcept { return self().template member<&urd::record_of<V>::atoms>(); }

    auto get_domain_size() const noexcept { return get_atoms().size() + 1; }

    static constexpr auto functions() noexcept { return urd::Functions<urd::function<"get_domain_size", &view_mixin::get_domain_size>> {}; }
};
}  // namespace urd

#endif
