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

#ifndef TYR_FORMALISM_FUNCTION_VIEW_HPP_
#define TYR_FORMALISM_FUNCTION_VIEW_HPP_

#include "tyr/formalism/declarations.hpp"

#include <ranges>
#include <urd/elements/flat.hpp>
#include <yggdrasil/formalism/values.hpp>

namespace urd
{
template<::tyr::formalism::FactKind T, typename V>
struct view_mixin<::tyr::formalism::Function<T>, V>
{
private:
    [[nodiscard]] const V& self() const noexcept { return static_cast<const V&>(*this); }

public:
    const auto& get_name() const noexcept { return self().data().name; }
    auto get_arity() const noexcept { return self().data().arity; }
    /// Its rows' column types: one object per argument (a key of row pools, as the repository declares its pool).
    auto get_column_types() const
    {
        return std::views::iota(std::size_t { 0 }, std::size_t(get_arity()))
               | std::views::transform([](std::size_t) { return ygg::type_id<urd::index<::tyr::formalism::Object>>; });
    }
};
}  // namespace urd

#endif
