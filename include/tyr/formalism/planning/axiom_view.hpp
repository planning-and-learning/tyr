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

#ifndef TYR_FORMALISM_PLANNING_AXIOM_VIEW_HPP_
#define TYR_FORMALISM_PLANNING_AXIOM_VIEW_HPP_

#include "tyr/formalism/declarations.hpp"
#include <yggdrasil/formalism/row_view.hpp>
#include "tyr/formalism/atom_view.hpp"
#include "tyr/formalism/planning/conjunctive_condition_view.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/variable_view.hpp"

#include <urd/elements/flat.hpp>

namespace urd
{
template<::tyr::TaskKind T, typename V>
struct view_mixin<::tyr::formalism::planning::Axiom<T>, V>
{
private:
    [[nodiscard]] const V& self() const noexcept { return static_cast<const V&>(*this); }

public:
    auto get_arity() const noexcept
        requires std::same_as<T, ::tyr::LiftedTag>
    { return get_body().get_arity(); }
    auto get_variables() const noexcept
        requires std::same_as<T, ::tyr::LiftedTag>
    { return self().template get<&urd::record_of<V>::variables>(); }
    auto get_body() const noexcept { return self().template get<&urd::record_of<V>::body>(); }
    auto get_head() const noexcept { return self().template get<&urd::record_of<V>::head>(); }

    auto get_axiom() const noexcept
        requires std::same_as<T, ::tyr::GroundTag>
    { return get_row().get_relation(); }
    auto get_row() const noexcept
        requires std::same_as<T, ::tyr::GroundTag>
    { return self().template get<&urd::record_of<V>::row>(); }
    auto get_objects() const noexcept
        requires std::same_as<T, ::tyr::GroundTag>
    { return get_row().get_objects(); }
    auto get_key() const noexcept
        requires std::same_as<T, ::tyr::GroundTag>
    { return get_row().get_key(); }
};
}  // namespace urd

#endif
