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

#ifndef TYR_FORMALISM_PLANNING_TEMPORAL_PART_VIEW_HPP_
#define TYR_FORMALISM_PLANNING_TEMPORAL_PART_VIEW_HPP_

#include "tyr/declarations.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/planning/temporal_part_data.hpp"

#include <urd/elements/view.hpp>

namespace urd
{
template<::tyr::TaskKind T, typename V>
struct view_mixin<::tyr::formalism::TemporalPart<T>, V>
{
private:
    [[nodiscard]] const V& self() const noexcept { return static_cast<const V&>(*this); }

public:
    auto get_min_durations() const noexcept { return self().template member<&urd::record_of<V>::min_durations>(); }

    auto get_max_durations() const noexcept { return self().template member<&urd::record_of<V>::max_durations>(); }

    auto get_end_min_durations() const noexcept { return self().template member<&urd::record_of<V>::end_min_durations>(); }

    auto get_end_max_durations() const noexcept { return self().template member<&urd::record_of<V>::end_max_durations>(); }

    auto get_over_all() const noexcept { return self().template member<&urd::record_of<V>::over_all>(); }

    auto get_end_condition() const noexcept { return self().template member<&urd::record_of<V>::end_condition>(); }

    auto get_end_effects() const noexcept { return self().template member<&urd::record_of<V>::end_effects>(); }

    auto get_end_choices() const noexcept { return self().template member<&urd::record_of<V>::end_choices>(); }
};
}  // namespace urd

#endif
