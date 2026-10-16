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

#ifndef TYR_DATALOG_RULE_SPLITTING_HPP_
#define TYR_DATALOG_RULE_SPLITTING_HPP_

#include "tyr/formalism/datalog/declarations.hpp"

#include <vector>
#include <yggdrasil/containers/associative_containers.hpp>
#include <yggdrasil/semantics/equal_to.hpp>
#include <yggdrasil/semantics/hash.hpp>

namespace tyr::datalog
{

struct RuleSplitting
{
    using Rule = ygg::Index<formalism::datalog::Rule<LiftedTag, formalism::PredicateTag>>;

    /// A rule that replaces part of a split source rule: the position of each of its variables among the source rule's variables.
    struct Origin
    {
        size_t num_source_variables;
        std::vector<ygg::uint_t> source_positions;
    };

    /// The main rule that replaces each split source rule; source rules index the source program's repository.
    ygg::UnorderedMap<Rule, Rule> main_rules;
    /// The origin of every rule that splitting created, main rules and projection rules alike.
    ygg::UnorderedMap<Rule, Origin> origins;
    /// The projection rule that derives each projection predicate.
    ygg::UnorderedMap<ygg::Index<formalism::Predicate<formalism::FluentTag>>, Rule> projection_rules;
};

/// Copies the program into the repository and projects the variables of its predicate rules that occur in no head and only
/// in some preconditions: the preconditions that use such a variable are replaced by an atom over their variables that the
/// rest of the rule needs, which a projection rule derives. Rules with numeric constraints or metric effects that depend on
/// their variables are copied unchanged. The minimal sum and max costs of every atom are preserved.
formalism::datalog::ProgramView<LiftedTag>
split_rules(formalism::datalog::ProgramView<LiftedTag> program, formalism::datalog::Repository& repository, RuleSplitting& splitting);

}

#endif
