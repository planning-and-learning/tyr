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

#ifndef TYR_FORMALISM_CONJUNCTIVE_CONDITION_CANONICALIZATION_HPP_
#define TYR_FORMALISM_CONJUNCTIVE_CONDITION_CANONICALIZATION_HPP_

#include "tyr/declarations.hpp"
#include "tyr/formalism/conjunctive_condition_data.hpp"
#include "tyr/formalism/declarations.hpp"

#include <urd/elements/data.hpp>
#include <yggdrasil/semantics/canonicalization.hpp>

/// The canonical form of a conjunctive condition (found by ADL from prepare_for_insert, of either repository): its
/// literals, facts and numeric constraints sorted.
namespace tyr::formalism
{

inline void canonicalize(urd::data<ConjunctiveCondition<LiftedTag>>& data)
{
    ygg::canonicalize(data.static_literals);
    ygg::canonicalize(data.fluent_literals);
    ygg::canonicalize(data.derived_literals);
    ygg::canonicalize(data.numeric_constraints);
}

inline void canonicalize(urd::data<ConjunctiveCondition<GroundTag>>& data)
{
    ygg::canonicalize(data.static_literals);
    ygg::canonicalize(data.derived_literals);
    ygg::canonicalize(data.positive_facts);
    ygg::canonicalize(data.negative_facts);
    ygg::canonicalize(data.numeric_constraints);
}

}

#endif
