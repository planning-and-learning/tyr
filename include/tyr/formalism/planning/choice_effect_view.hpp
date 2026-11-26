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

#ifndef TYR_FORMALISM_PLANNING_CHOICE_EFFECT_VIEW_HPP_
#define TYR_FORMALISM_PLANNING_CHOICE_EFFECT_VIEW_HPP_

#include "tyr/declarations.hpp"
#include "tyr/formalism/conjunctive_condition_view.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/planning/choice_effect_data.hpp"
#include "tyr/formalism/planning/outcome_view.hpp"
#include "tyr/formalism/variable_view.hpp"

#include <concepts>
#include <urd/elements/view.hpp>

namespace urd
{
template<::tyr::TaskKind T, typename V>
struct view_mixin<::tyr::formalism::ChoiceEffect<T>, V>
{
private:
    [[nodiscard]] const V& self() const noexcept { return static_cast<const V&>(*this); }

public:
    auto get_variables() const noexcept
        requires std::same_as<T, ::tyr::LiftedTag>
    { return self().template member<&urd::record_of<V>::variables>(); }

    auto get_condition() const noexcept { return self().template member<&urd::record_of<V>::condition>(); }

    auto get_outcomes() const noexcept { return self().template member<&urd::record_of<V>::outcomes>(); }

    auto get_probabilities() const noexcept { return self().template member<&urd::record_of<V>::probabilities>(); }

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

#endif
