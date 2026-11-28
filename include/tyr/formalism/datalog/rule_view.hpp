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

#ifndef TYR_FORMALISM_DATALOG_RULE_VIEW_HPP_
#define TYR_FORMALISM_DATALOG_RULE_VIEW_HPP_

#include "tyr/declarations.hpp"
#include "tyr/formalism/atom_view.hpp"
#include "tyr/formalism/conjunctive_condition_view.hpp"
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/datalog/rule_body_view.hpp"
#include "tyr/formalism/datalog/rule_data.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/numeric_effect_view.hpp"
#include "tyr/formalism/variable_view.hpp"

#include <concepts>
#include <cstddef>
#include <fmt/base.h>
#include <ranges>
#include <urd/elements/concepts.hpp>
#include <urd/elements/flat.hpp>
#include <urd/elements/index.hpp>
#include <urd/elements/view.hpp>
#include <yggdrasil/formalism/row_view.hpp>
#include <yggdrasil/formalism/values.hpp>
#include <yggdrasil/formatting/without_format_spec.hpp>

namespace urd
{
template<::tyr::TaskKind T, typename V>
struct view_mixin<::tyr::formalism::Rule<T>, V>
{
private:
    [[nodiscard]] const V& self() const noexcept { return static_cast<const V&>(*this); }

public:
    auto variables() const noexcept
        requires std::same_as<T, ::tyr::LiftedTag>
    { return self().template member<&urd::record_of<V>::variables>(); }

    auto body() const noexcept { return self().template member<&urd::record_of<V>::body>(); }

    auto fluent_heads() const noexcept { return self().template member<&urd::record_of<V>::fluent_heads>(); }

    auto derived_heads() const noexcept { return self().template member<&urd::record_of<V>::derived_heads>(); }

    auto cost() const noexcept { return self().template member<&urd::record_of<V>::cost>(); }

    // user: one getter templated on the tag over the fields it chooses (next to the record), besides the plain getters.
    template<::tyr::formalism::FactKind F>
    auto heads() const noexcept
    { return self().template member<::tyr::formalism::heads_of<F, T>>(); }

    /// Its rows' column types (a key of row pools): one object column per argument, computed.
    [[nodiscard]] ygg::UniformColumnTypes column_types() const
        requires std::same_as<T, ::tyr::LiftedTag>
    {
        return ygg::uniform_column_types<urd::index<::tyr::formalism::Object>>(
            urd::visit_view(body(), [](const auto& body) { return static_cast<std::size_t>(body.arity()); }));
    }

    static constexpr auto functions() noexcept
    {
        if constexpr (std::same_as<T, ::tyr::GroundTag>)
            return urd::Functions<> {};
        else
            return urd::Functions<urd::function<"column_types", &view_mixin::column_types>> {};
    }
};
}  // namespace urd

/// A rule: heads :- body.  % cost c (its heads separated by commas; without a body, heads.).
template<tyr::TaskKind T>
struct fmt::formatter<urd::index_view<tyr::formalism::Rule<T>, tyr::formalism::Repository>> : ygg::without_format_spec
{
    fmt::format_context::iterator format(const urd::index_view<tyr::formalism::Rule<T>, tyr::formalism::Repository>& value, fmt::format_context& context) const;
};

#endif
