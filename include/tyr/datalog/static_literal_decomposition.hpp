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

#ifndef TYR_DATALOG_STATIC_LITERAL_DECOMPOSITION_HPP_
#define TYR_DATALOG_STATIC_LITERAL_DECOMPOSITION_HPP_

#include "tyr/datalog/rule_mapping.hpp"
#include "tyr/formalism/datalog/declarations.hpp"

#include <yggdrasil/containers/associative_containers.hpp>
#include <yggdrasil/semantics/equal_to.hpp>
#include <yggdrasil/semantics/hash.hpp>

namespace tyr::datalog
{

struct StaticLiteralDecomposition
{
    using Rule = ygg::Index<formalism::datalog::Rule<LiftedTag, formalism::PredicateTag>>;

    /// Rewritten rules keep the variables of their source rules.
    RuleMapping rules;
};

/// Copies the program into the repository and replaces each positive static literal over three or more variables whose atoms
/// are exactly the join of their pairwise projections (a join dependency, e.g. when one argument determines the others) by
/// one new static literal per pair of its variables. The rules keep their meaning, and the consistency graph, which encodes
/// pairs, then checks these literals completely instead of leaving them to an exact check per binding.
formalism::datalog::ProgramView<LiftedTag>
decompose_static_literals(formalism::datalog::ProgramView<LiftedTag> program, formalism::datalog::Repository& repository, StaticLiteralDecomposition& result);

}

#endif
