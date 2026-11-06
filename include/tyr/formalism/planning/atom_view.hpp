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

#ifndef TYR_FORMALISM_PLANNING_ATOM_VIEW_HPP_
#define TYR_FORMALISM_PLANNING_ATOM_VIEW_HPP_

#include "tyr/formalism/binding_view.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/predicate_view.hpp"

#include <urd/elements/flat.hpp>

namespace urd
{
template<::tyr::TaskKind T, ::tyr::formalism::FactKind F, typename V>
struct view_mixin<::tyr::formalism::planning::Atom<T, F>, V>
{
private:
    [[nodiscard]] const V& self() const noexcept { return static_cast<const V&>(*this); }

public:
    auto get_predicate() const noexcept
    {
        if constexpr (std::same_as<T, ::tyr::GroundTag>)
        {
            return get_row().get_relation();
        }
        else
        {
            return self().template get<&urd::record_of<V>::predicate>();
        }
    }
    auto get_terms() const noexcept
        requires std::same_as<T, ::tyr::LiftedTag>
    { return self().template get<&urd::record_of<V>::terms>(); }

    auto get_row() const noexcept
        requires std::same_as<T, ::tyr::GroundTag>
    { return self().template get<&urd::record_of<V>::binding>(); }
    auto get_objects() const noexcept
        requires std::same_as<T, ::tyr::GroundTag>
    { return get_row().get_objects(); }
    auto get_key() const noexcept
        requires std::same_as<T, ::tyr::GroundTag>
    { return get_row().get_key(); }
};
}  // namespace urd

#endif
