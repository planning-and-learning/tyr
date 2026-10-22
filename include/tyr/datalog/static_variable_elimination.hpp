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

#ifndef TYR_DATALOG_STATIC_VARIABLE_ELIMINATION_HPP_
#define TYR_DATALOG_STATIC_VARIABLE_ELIMINATION_HPP_

#include "tyr/formalism/datalog/declarations.hpp"

#include <utility>
#include <vector>
#include <yggdrasil/containers/associative_containers.hpp>
#include <yggdrasil/serialization/cista_equal_to.hpp>
#include <yggdrasil/serialization/cista_hash.hpp>
#include <yggdrasil/semantics/equal_to.hpp>
#include <yggdrasil/semantics/hash.hpp>

namespace tyr::datalog
{

struct StaticVariableElimination
{
    using Rule = ygg::Index<formalism::datalog::Rule<LiftedTag, formalism::PredicateTag>>;

    /// A static predicate over kept variables that replaces the static literals over variables that only they use. Each atom
    /// stores one assignment of the eliminated variables that supports it, as source rule positions and objects.
    struct Projection
    {
        std::vector<ygg::uint_t> kept_positions;
        ygg::UnorderedMap<ygg::IndexList<formalism::Object>, std::vector<std::pair<ygg::uint_t, ygg::Index<formalism::Object>>>> supports;
    };

    /// A rewritten rule: the position of each of its variables among the source rule's variables, and its projections.
    struct Origin
    {
        size_t num_source_variables;
        std::vector<ygg::uint_t> source_positions;
        std::vector<Projection> projections;
    };

    /// The rule that replaces each predicate rule of the source program, rewritten or copied; source rules index the source
    /// program's repository.
    ygg::UnorderedMap<Rule, Rule> rules;
    /// The origin of every rewritten rule.
    ygg::UnorderedMap<Rule, Origin> origins;
};

/// Copies the program into the repository and eliminates the variables of its predicate rules that occur in no head and only
/// in static literals: those literals are joined once over the static facts into a new static predicate over the variables
/// that the rest of the rule needs (at most two, so that the consistency graph encodes it exactly). Rules with numeric
/// constraints are copied unchanged. Static literals cost nothing, so the minimal sum and max costs of every atom are kept.
formalism::datalog::ProgramView<LiftedTag>
eliminate_static_variables(formalism::datalog::ProgramView<LiftedTag> program, formalism::datalog::Repository& repository, StaticVariableElimination& result);

/// The binding of the source rule: the rewritten rule's objects at their source positions, and the eliminated variables from
/// the supports of the kept objects.
void restore_eliminated_variables(const StaticVariableElimination::Origin& origin,
                                  const ygg::IndexList<formalism::Object>& binding,
                                  ygg::IndexList<formalism::Object>& source_binding);

}

#endif
