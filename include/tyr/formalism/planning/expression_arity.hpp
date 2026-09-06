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

#ifndef TYR_FORMALISM_PLANNING_EXPRESSION_ARITY_HPP_
#define TYR_FORMALISM_PLANNING_EXPRESSION_ARITY_HPP_

#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/planning/views.hpp"

#include <numeric>
#include <yggdrasil/semantics/equal_to.hpp>
#include <yggdrasil/semantics/hash.hpp>

namespace tyr::formalism::planning
{

/**
 * collect_parameters
 */

void collect_parameters(TermView element, ParameterList& result);

template<FactKind T>
void collect_parameters(AtomView<LiftedTag, T> element, ParameterList& result);

/**
 * Implementations
 */

inline void collect_parameters(TermView element, ParameterList& result)
{
    visit(
        [&](auto&& arg)
        {
            using Alternative = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<Alternative, ParameterIndex>)
                result.push_back(arg);
            else if constexpr (std::is_same_v<Alternative, ObjectView>) {}
            else
                static_assert(ygg::dependent_false<Alternative>::value, "Missing case");
        },
        element.get_variant());
}

template<FactKind T>
inline void collect_parameters(AtomView<LiftedTag, T> element, ParameterList& result)
{
    for (const auto term : element.get_terms())
        collect_parameters(term, result);
}

template<FactKind T>
inline auto collect_parameters(AtomView<LiftedTag, T> element)
{
    auto result = ParameterList {};
    collect_parameters(element, result);
    return result;
}

}

#endif