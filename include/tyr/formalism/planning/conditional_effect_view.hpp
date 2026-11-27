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

#ifndef TYR_FORMALISM_PLANNING_CONDITIONAL_EFFECT_VIEW_HPP_
#define TYR_FORMALISM_PLANNING_CONDITIONAL_EFFECT_VIEW_HPP_

#include "tyr/declarations.hpp"
#include "tyr/formalism/conjunctive_condition_view.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/planning/conjunctive_effect_view.hpp"
#include "tyr/formalism/variable_view.hpp"

#include <concepts>
#include <fmt/base.h>
#include <urd/elements/concepts.hpp>
#include <urd/elements/flat.hpp>
#include <urd/elements/view.hpp>
#include <yggdrasil/formatting/without_format_spec.hpp>

namespace urd
{
template<::tyr::TaskKind T, typename V>
struct view_mixin<::tyr::formalism::ConditionalEffect<T>, V>
{
private:
    [[nodiscard]] const V& self() const noexcept { return static_cast<const V&>(*this); }

public:
    auto get_variables() const noexcept
        requires std::same_as<T, ::tyr::LiftedTag>
    { return self().template member<&urd::record_of<V>::variables>(); }

    auto get_condition() const noexcept { return self().template member<&urd::record_of<V>::condition>(); }

    auto get_effect() const noexcept { return self().template member<&urd::record_of<V>::effect>(); }

    auto get_arity() const noexcept
        requires std::same_as<T, ::tyr::LiftedTag>
    { return get_variables().size(); }

    static constexpr auto functions() noexcept
    {
        if constexpr (std::same_as<T, ::tyr::GroundTag>)
            return urd::Functions<> {};
        else
            return urd::Functions<urd::function<"get_arity", &view_mixin::get_arity>> {};
    }
};
}  // namespace urd

/// A conditional effect: (forall (variables) (when condition effect)), without the forall or the when it does not need.
template<tyr::TaskKind T>
struct fmt::formatter<urd::index_view<tyr::formalism::ConditionalEffect<T>, tyr::formalism::Repository>> : ygg::without_format_spec
{
    fmt::format_context::iterator format(const urd::index_view<tyr::formalism::ConditionalEffect<T>, tyr::formalism::Repository>& value,
                                         fmt::format_context& context) const;
};

#endif
