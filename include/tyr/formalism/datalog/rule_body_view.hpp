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

#ifndef TYR_FORMALISM_DATALOG_RULE_BODY_VIEW_HPP_
#define TYR_FORMALISM_DATALOG_RULE_BODY_VIEW_HPP_

#include "tyr/declarations.hpp"
#include "tyr/formalism/binary_operator_view.hpp"
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/datalog/rule_body_data.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/literal_view.hpp"
#include "tyr/formalism/variable_view.hpp"

#include <fmt/base.h>
#include <urd/elements/concepts.hpp>
#include <urd/elements/flat.hpp>
#include <urd/elements/view.hpp>
#include <yggdrasil/formatting/without_format_spec.hpp>

namespace urd
{
template<typename V>
struct view_mixin<::tyr::formalism::RuleBody<::tyr::GroundTag>, V>
{
private:
    [[nodiscard]] const V& self() const noexcept { return static_cast<const V&>(*this); }

public:
    auto get_static_literals() const noexcept { return self().template member<&urd::record_of<V>::static_literals>(); }

    auto get_fluent_literals() const noexcept { return self().template member<&urd::record_of<V>::fluent_literals>(); }

    auto get_derived_literals() const noexcept { return self().template member<&urd::record_of<V>::derived_literals>(); }

    auto get_numeric_constraints() const noexcept { return self().template member<&urd::record_of<V>::numeric_constraints>(); }

    // user: one getter templated on the tag over the fields it chooses (next to the record), besides the plain getters.
    template<::tyr::formalism::FactKind F>
    auto get_literals() const noexcept
    { return self().template member<::tyr::formalism::rule_body_literals_of<F>>(); }

    static constexpr auto functions() noexcept { return urd::Functions<> {}; }
};
}  // namespace urd

/// A ground rule's body: its literals and constraints, separated by commas.
template<>
struct fmt::formatter<urd::index_view<tyr::formalism::RuleBody<tyr::GroundTag>, tyr::formalism::Repository>> : ygg::without_format_spec
{
    fmt::format_context::iterator format(const urd::index_view<tyr::formalism::RuleBody<tyr::GroundTag>, tyr::formalism::Repository>& value,
                                         fmt::format_context& context) const;
};

#endif
