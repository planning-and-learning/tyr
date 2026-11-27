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

#ifndef TYR_FORMALISM_PLANNING_FORMATTER_HPP_
#define TYR_FORMALISM_PLANNING_FORMATTER_HPP_

#include "tyr/declarations.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/formatter.hpp"
#include "tyr/formalism/planning/declarations.hpp"

#include <cstddef>
#include <fmt/base.h>
#include <urd/elements/view.hpp>
#include <utility>

namespace tyr::formalism
{
/// Writes a plan's step (std::pair<View, PlanFormatting>) as a plan file lists it.
struct PlanFormatting
{
};
}  // namespace tyr::formalism

/// A plan's step: (original-name original arguments...), as a plan file lists it; of any view of an action binding (an
/// interned row, a row being built, a row record).
template<typename V>
    requires urd::ViewOf<V, ygg::Row<tyr::formalism::Action<tyr::LiftedTag>>>
struct fmt::formatter<std::pair<V, tyr::formalism::PlanFormatting>> : ygg::without_format_spec
{
    fmt::format_context::iterator format(const std::pair<V, tyr::formalism::PlanFormatting>& value, fmt::format_context& context) const
    {
        // Its relation in whichever form the row holds it; its objects through the row's repository (the relation's may
        // be a layer below the objects: a domain's).
        return urd::visit_view(value.first.get_relation(),
                               [&](const auto& relation)
                               {
                                   auto out = fmt::format_to(context.out(), "({}", relation.get_original_name());
                                   // Its original arguments only: an action's row has further columns after them.
                                   for (std::size_t column = 0; column < relation.get_original_arity(); ++column)
                                       out = fmt::format_to(
                                           out,
                                           " {}",
                                           urd::resolve(value.first.template get<urd::index<tyr::formalism::Object>>(column), value.first.context()).get_name());
                                   return fmt::format_to(out, ")");
                               });
    }
};

/// A plan's step of a ground action: its binding's.
template<>
struct fmt::formatter<std::pair<tyr::formalism::ActionView<tyr::GroundTag>, tyr::formalism::PlanFormatting>> : ygg::without_format_spec
{
    fmt::format_context::iterator format(const std::pair<tyr::formalism::ActionView<tyr::GroundTag>, tyr::formalism::PlanFormatting>& value,
                                         fmt::format_context& context) const;
};

#endif
