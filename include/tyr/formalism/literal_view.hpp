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

#ifndef TYR_FORMALISM_LITERAL_VIEW_HPP_
#define TYR_FORMALISM_LITERAL_VIEW_HPP_

#include "tyr/declarations.hpp"
#include "tyr/formalism/atom_view.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/predicate_view.hpp"

#include <fmt/base.h>
#include <urd/elements/flat.hpp>
#include <urd/elements/view.hpp>
#include <yggdrasil/formatting/without_format_spec.hpp>

namespace urd
{
template<::tyr::TaskKind T, ::tyr::formalism::FactKind F, typename V>
struct view_mixin<::tyr::formalism::Literal<T, F>, V>
{
private:
    [[nodiscard]] const V& self() const noexcept { return static_cast<const V&>(*this); }

public:
    auto atom() const noexcept { return self().template member<&urd::record_of<V>::atom>(); }

    auto polarity() const noexcept { return self().template member<&urd::record_of<V>::polarity>(); }
};
}  // namespace urd

/// A literal: its atom, or (not atom).
template<tyr::TaskKind T, tyr::formalism::FactKind F>
struct fmt::formatter<urd::index_view<tyr::formalism::Literal<T, F>, tyr::formalism::Repository>> : ygg::without_format_spec
{
    fmt::format_context::iterator format(const urd::index_view<tyr::formalism::Literal<T, F>, tyr::formalism::Repository>& value,
                                         fmt::format_context& context) const;
};

#endif
