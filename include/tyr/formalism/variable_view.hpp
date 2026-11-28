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

#ifndef TYR_FORMALISM_VARIABLE_VIEW_HPP_
#define TYR_FORMALISM_VARIABLE_VIEW_HPP_

#include "tyr/formalism/declarations.hpp"

#include <fmt/base.h>
#include <urd/elements/view.hpp>
#include <yggdrasil/formatting/without_format_spec.hpp>

namespace urd
{
template<typename V>
struct view_mixin<::tyr::formalism::Variable, V>
{
private:
    [[nodiscard]] const V& self() const noexcept { return static_cast<const V&>(*this); }

public:
    const auto& name() const noexcept { return self().template member<&urd::record_of<V>::name>(); }
};
}  // namespace urd

/// A variable: its name (?x).
template<>
struct fmt::formatter<urd::index_view<tyr::formalism::Variable, tyr::formalism::Repository>> : ygg::without_format_spec
{
    fmt::format_context::iterator format(const urd::index_view<tyr::formalism::Variable, tyr::formalism::Repository>& value,
                                         fmt::format_context& context) const;
};

#endif
