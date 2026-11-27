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

#include "tyr/declarations.hpp"
#include "tyr/formalism/conjunctive_condition_data.hpp"
#include "tyr/formalism/conjunctive_condition_view.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/formatter.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/action_index.hpp"
#include "tyr/formalism/planning/choice_effect_view.hpp"
#include "tyr/formalism/planning/conditional_effect_view.hpp"
#include "tyr/formalism/planning/temporal_part_view.hpp"
#include "tyr/formalism/variable_view.hpp"

#include <concepts>
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

    auto get_choices() const noexcept { return self().template member<&urd::record_of<V>::choices>(); }

    /// The duration, over-all and end parts of a durative action; none for an instantaneous one.
    auto get_temporal_part() const noexcept { return self().template member<&urd::record_of<V>::temporal_part>(); }

    auto get_arity() const noexcept
        requires std::same_as<T, ::tyr::LiftedTag>
    { return get_variables().size(); }

    /// Its rows' column types (a key of row pools): one object column per argument, computed.
    [[nodiscard]] ygg::UniformColumnTypes get_column_types() const
        requires std::same_as<T, ::tyr::LiftedTag>
    { return ygg::uniform_column_types<urd::index<::tyr::formalism::Object>>(get_variables().size()); }

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

/// An action: (:action name :parameters (...) :precondition C :effect E); a ground one is named after its binding.
template<tyr::TaskKind T>
struct fmt::formatter<urd::index_view<tyr::formalism::Action<T>, tyr::formalism::Repository>> : ygg::without_format_spec
{
    fmt::format_context::iterator format(const urd::index_view<tyr::formalism::Action<T>, tyr::formalism::Repository>& value,
                                         fmt::format_context& context) const;
};

/// An action's binding held by a view: as a stored one.
template<>
struct fmt::formatter<tyr::formalism::ActionRowBuilderView> : ygg::without_format_spec
{
    fmt::format_context::iterator format(const tyr::formalism::ActionRowBuilderView& value, fmt::format_context& context) const;
};

#endif
