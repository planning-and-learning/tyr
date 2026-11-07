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

#ifndef TYR_FORMALISM_PLANNING_CONJUNCTIVE_EFFECT_VIEW_HPP_
#define TYR_FORMALISM_PLANNING_CONJUNCTIVE_EFFECT_VIEW_HPP_

#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/fdr_fact_view.hpp"
#include "tyr/formalism/literal_view.hpp"
#include "tyr/formalism/numeric_effect_view.hpp"

#include <urd/elements/flat.hpp>

namespace urd
{
template<::tyr::TaskKind T, typename V>
struct view_mixin<::tyr::formalism::planning::ConjunctiveEffect<T>, V>
{
private:
    [[nodiscard]] const V& self() const noexcept { return static_cast<const V&>(*this); }

public:
    auto get_literals() const noexcept
        requires std::same_as<T, ::tyr::LiftedTag>
    { return self().template get<&urd::record_of<V>::literals>(); }
    auto get_numeric_effects() const noexcept { return self().template get<&urd::record_of<V>::numeric_effects>(); }
    auto get_auxiliary_numeric_effect() const noexcept { return self().template get<&urd::record_of<V>::auxiliary_numeric_effect>(); }

    template<::tyr::formalism::PolarityKind F>
    auto get_facts() const noexcept
        requires std::same_as<T, ::tyr::GroundTag>
    { return urd::resolve(self().data().template get_facts<F>(), self().context()); }
};
}  // namespace urd

#endif
