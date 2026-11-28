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

#ifndef TYR_FORMALISM_PLANNING_FDR_FACT_VIEW_HPP_
#define TYR_FORMALISM_PLANNING_FDR_FACT_VIEW_HPP_

#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/fdr_fact_data.hpp"
#include "tyr/formalism/planning/fdr_value.hpp"
#include "tyr/formalism/planning/fdr_variable_data.hpp"

#include <fmt/base.h>
#include <optional>
#include <type_traits>
#include <urd/elements/concepts.hpp>
#include <urd/elements/data.hpp>
#include <urd/elements/flat.hpp>
#include <urd/elements/view.hpp>
#include <yggdrasil/formatting/without_format_spec.hpp>

/// A fact: its variable and value.
template<::tyr::formalism::FactKind T, typename V>
struct urd::view_mixin<::tyr::formalism::FDRFact<T>, V>
{
private:
    [[nodiscard]] const V& self() const noexcept { return static_cast<const V&>(*this); }

public:
    auto variable() const noexcept { return self().template member<&urd::record_of<V>::variable>(); }

    auto value() const noexcept { return self().template member<&urd::record_of<V>::value>(); }

    auto has_value() const noexcept { return value() != ::tyr::formalism::FDRValue::none(); }

    auto atom_index() const noexcept { return has_value() ? std::make_optional(self().context()[variable().index()]->atoms[value().value - 1]) : std::nullopt; }

    auto atom() const noexcept { return urd::resolve(atom_index(), self().context()); }

    static constexpr auto functions() noexcept
    {
        return urd::Functions<urd::function<"has_value", &view_mixin::has_value>,
                              urd::function<"atom_index", &view_mixin::atom_index>,
                              urd::function<"atom", &view_mixin::atom>> {};
    }
};

static_assert(std::is_trivially_copyable_v<urd::data<::tyr::formalism::FDRFact<::tyr::formalism::FluentTag>>>);

/// A fact as the literal it stands for: its atom, or (and (not a1) ...) where its variable holds none of its atoms.
template<>
struct fmt::formatter<tyr::formalism::FDRFactView<tyr::formalism::FluentTag>> : ygg::without_format_spec
{
    fmt::format_context::iterator format(const tyr::formalism::FDRFactView<tyr::formalism::FluentTag>& value, fmt::format_context& context) const;
};

/// A fact computed on the fly (a state's), as a stored one.
template<>
struct fmt::formatter<urd::owning_view<tyr::formalism::FDRFact<tyr::formalism::FluentTag>, tyr::formalism::Repository>> : ygg::without_format_spec
{
    fmt::format_context::iterator format(const urd::owning_view<tyr::formalism::FDRFact<tyr::formalism::FluentTag>, tyr::formalism::Repository>& value,
                                         fmt::format_context& context) const;
};

#endif
