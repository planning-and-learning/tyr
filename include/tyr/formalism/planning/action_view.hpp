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

#ifndef TYR_FORMALISM_PLANNING_ACTION_VIEW_HPP_
#define TYR_FORMALISM_PLANNING_ACTION_VIEW_HPP_

#include "tyr/formalism/conjunctive_condition_data.hpp"
#include "tyr/formalism/conjunctive_condition_view.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/planning/action_index.hpp"
#include "tyr/formalism/planning/conditional_effect_view.hpp"
#include "tyr/formalism/variable_view.hpp"

#include <ranges>
#include <urd/elements/flat.hpp>
#include <yggdrasil/formalism/row_view.hpp>
#include <yggdrasil/formalism/values.hpp>

namespace urd
{
template<::tyr::TaskKind T, typename V>
struct view_mixin<::tyr::formalism::Action<T>, V>
{
private:
    [[nodiscard]] const V& self() const noexcept { return static_cast<const V&>(*this); }

public:
    const auto& get_name() const noexcept
        requires std::same_as<T, ::tyr::LiftedTag>
    { return self().template member<&urd::record_of<V>::name>(); }

    const auto& get_original_name() const noexcept
        requires std::same_as<T, ::tyr::LiftedTag>
    { return self().template member<&urd::record_of<V>::original_name>(); }

    auto get_original_arity() const noexcept
        requires std::same_as<T, ::tyr::LiftedTag>
    { return self().template member<&urd::record_of<V>::original_arity>(); }

    auto get_variables() const noexcept
        requires std::same_as<T, ::tyr::LiftedTag>
    { return self().template member<&urd::record_of<V>::variables>(); }

    auto get_row() const noexcept
        requires std::same_as<T, ::tyr::GroundTag>
    { return self().template member<&urd::record_of<V>::row>(); }

    auto get_condition() const noexcept { return self().template member<&urd::record_of<V>::condition>(); }

    auto get_effects() const noexcept { return self().template member<&urd::record_of<V>::effects>(); }

    auto get_arity() const noexcept
        requires std::same_as<T, ::tyr::LiftedTag>
    { return get_variables().size(); }

    /// Its rows' column types: one object per variable (a key of row pools, as the repository declares its pool).
    auto get_column_types() const
        requires std::same_as<T, ::tyr::LiftedTag>
    {
        return std::views::iota(std::size_t { 0 }, std::size_t(get_arity()))
               | std::views::transform([](std::size_t) { return ygg::type_id<urd::index<::tyr::formalism::Object>>; });
    }

    auto get_action() const noexcept
        requires std::same_as<T, ::tyr::GroundTag>
    { return get_row().get_relation(); }

    static constexpr auto functions() noexcept
    {
        if constexpr (std::same_as<T, ::tyr::GroundTag>)
            return urd::Functions<urd::function<"get_action", &view_mixin::get_action>> {};
        else
            return urd::Functions<urd::function<"get_arity", &view_mixin::get_arity>, urd::function<"get_column_types", &view_mixin::get_column_types>> {};
    }
};
}  // namespace urd

#endif
