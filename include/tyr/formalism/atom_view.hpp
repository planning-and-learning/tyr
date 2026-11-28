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

#ifndef TYR_FORMALISM_ATOM_VIEW_HPP_
#define TYR_FORMALISM_ATOM_VIEW_HPP_

#include "tyr/declarations.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/formatter.hpp"
#include "tyr/formalism/object_view.hpp"
#include "tyr/formalism/parameters_view.hpp"
#include "tyr/formalism/predicate_view.hpp"

#include <concepts>
#include <fmt/base.h>
#include <urd/elements/concepts.hpp>
#include <urd/elements/flat.hpp>
#include <urd/elements/view.hpp>
#include <yggdrasil/formalism/row_view.hpp>
#include <yggdrasil/formatting/without_format_spec.hpp>

namespace urd
{
template<::tyr::TaskKind T, ::tyr::formalism::FactKind F, typename V>
struct view_mixin<::tyr::formalism::Atom<T, F>, V>
{
private:
    [[nodiscard]] const V& self() const noexcept { return static_cast<const V&>(*this); }

public:
    auto parameters() const noexcept
        requires std::same_as<T, ::tyr::LiftedTag>
    { return self().template member<&urd::record_of<V>::parameters>(); }

    auto predicate() const noexcept
    {
        if constexpr (std::same_as<T, ::tyr::GroundTag>)
        {
            return row().relation();
        }
        else
        {
            return self().template member<&urd::record_of<V>::predicate>();
        }
    }

    auto terms() const noexcept
        requires std::same_as<T, ::tyr::LiftedTag>
    { return self().template member<&urd::record_of<V>::terms>(); }

    auto row() const noexcept
        requires std::same_as<T, ::tyr::GroundTag>
    { return self().template member<&urd::record_of<V>::row>(); }

    static constexpr auto functions() noexcept
    {
        if constexpr (std::same_as<T, ::tyr::GroundTag>)
            return urd::Functions<urd::function<"predicate", &view_mixin::predicate>> {};
        else
            return urd::Functions<> {};
    }
};
}  // namespace urd

/// An atom: (predicate terms...).
template<tyr::TaskKind T, tyr::formalism::FactKind F>
struct fmt::formatter<urd::index_view<tyr::formalism::Atom<T, F>, tyr::formalism::Repository>> : ygg::without_format_spec
{
    fmt::format_context::iterator format(const urd::index_view<tyr::formalism::Atom<T, F>, tyr::formalism::Repository>& value,
                                         fmt::format_context& context) const;
};

#endif
