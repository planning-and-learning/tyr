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

#ifndef TYR_FORMALISM_PLANNING_METRIC_VIEW_HPP_
#define TYR_FORMALISM_PLANNING_METRIC_VIEW_HPP_

#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/metric_data.hpp"

#include <urd/elements/flat.hpp>

namespace urd
{
template<typename V>
struct view_mixin<::tyr::formalism::planning::Metric, V>
{
private:
    [[nodiscard]] const V& self() const noexcept { return static_cast<const V&>(*this); }

public:
    auto get_optimization_direction() const noexcept { return self().data().optimization_direction; }
    auto get_fexpr() const noexcept { return self().template get<&urd::record_of<V>::fexpr>(); }
};
}  // namespace urd

#endif
