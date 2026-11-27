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

#ifndef TYR_FORMALISM_PREDICATE_VIEW_HPP_
#define TYR_FORMALISM_PREDICATE_VIEW_HPP_

#include "tyr/formalism/declarations.hpp"

#include <cstddef>
#include <fmt/base.h>
#include <ranges>
#include <urd/elements/concepts.hpp>
#include <urd/elements/flat.hpp>
#include <urd/elements/index.hpp>
#include <urd/elements/view.hpp>
#include <yggdrasil/formalism/values.hpp>
#include <yggdrasil/formatting/without_format_spec.hpp>

namespace urd
{
template<::tyr::formalism::FactKind T, typename V>
struct view_mixin<::tyr::formalism::Predicate<T>, V>
{
private:
    [[nodiscard]] const V& self() const noexcept { return static_cast<const V&>(*this); }

public:
    const auto& get_name() const noexcept { return self().template member<&urd::record_of<V>::name>(); }

    auto get_arity() const noexcept { return self().template member<&urd::record_of<V>::arity>(); }

    /// Its rows' column types (a key of row pools): one object column per argument, computed.
    [[nodiscard]] ygg::UniformColumnTypes get_column_types() const
    { return ygg::uniform_column_types<urd::index<::tyr::formalism::Object>>(static_cast<std::size_t>(get_arity())); }

    static constexpr auto functions() noexcept { return urd::Functions<urd::function<"get_column_types", &view_mixin::get_column_types>> {}; }
};
}  // namespace urd

/// A predicate as a domain declares it: (name ?x0 ?x1 ...).
template<tyr::formalism::FactKind F>
struct fmt::formatter<urd::index_view<tyr::formalism::Predicate<F>, tyr::formalism::Repository>> : ygg::without_format_spec
{
    fmt::format_context::iterator format(const urd::index_view<tyr::formalism::Predicate<F>, tyr::formalism::Repository>& value,
                                         fmt::format_context& context) const;
};

#endif
