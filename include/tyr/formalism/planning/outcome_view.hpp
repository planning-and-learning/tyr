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

#ifndef TYR_FORMALISM_PLANNING_OUTCOME_VIEW_HPP_
#define TYR_FORMALISM_PLANNING_OUTCOME_VIEW_HPP_

#include "tyr/declarations.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/planning/conditional_effect_view.hpp"
#include "tyr/formalism/planning/outcome_data.hpp"

#include <fmt/base.h>
#include <urd/elements/view.hpp>
#include <yggdrasil/formatting/without_format_spec.hpp>

namespace urd
{
template<::tyr::TaskKind T, typename V>
struct view_mixin<::tyr::formalism::Outcome<T>, V>
{
private:
    [[nodiscard]] const V& self() const noexcept { return static_cast<const V&>(*this); }

public:
    auto get_effects() const noexcept { return self().template member<&urd::record_of<V>::effects>(); }

    auto get_choices() const noexcept { return self().template member<&urd::record_of<V>::choices>(); }
};
}  // namespace urd

/// An outcome of a choice of nature: (and effects... choices...), its effects as conditional effects, then its nested choices.
template<tyr::TaskKind T>
struct fmt::formatter<urd::index_view<tyr::formalism::Outcome<T>, tyr::formalism::Repository>> : ygg::without_format_spec
{
    fmt::format_context::iterator format(const urd::index_view<tyr::formalism::Outcome<T>, tyr::formalism::Repository>& value,
                                         fmt::format_context& context) const;
};

#endif
