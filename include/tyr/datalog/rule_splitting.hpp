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

#include "tyr/datalog/rule_mapping.hpp"
#include "tyr/declarations.hpp"
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/declarations.hpp"

#include <cstddef>
#include <urd/elements/equal_to.hpp>
#include <urd/elements/hash.hpp>
#include <urd/elements/index.hpp>
#include <urd/kinds.hpp>
#include <urd/repr/maps/std_map.hpp>
#include <urd/repr/maps/unordered_flat_map.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/repr/values/unordered_flat_set.hpp>
#include <urd/store.hpp>
#include <vector>
#include <yggdrasil/core/config.hpp>

namespace tyr::datalog
{

struct RuleSplitting
{
    using Rule = urd::index<formalism::Rule<LiftedTag>>;

    /// A rule that replaces part of a split source rule: the position of each of its variables among the source rule's variables.
    struct Origin
    {
        size_t num_source_variables = 0;
        urd::Store<urd::sequence<ygg::uint_t>> source_positions;
    };

    /// The rule that replaces each source rule: the main rule of a split one, or the rule itself.
    RuleMapping rules;
    /// The origin of every rule that splitting created, main rules and projection rules alike.
    urd::Store<urd::map<Rule, Origin>> origins;
    /// The projection rule that derives each projection predicate.
    urd::Store<urd::map<urd::index<formalism::Predicate<formalism::DerivedTag>>, Rule>> projection_rules;
};

/// Writes the program into `repository`, a layer over the program's (adding only its new records), and projects the variables of its predicate rules that occur
/// in no head and only in some preconditions: the preconditions that use such a variable are replaced by an atom over their variables that the rest of the rule
/// needs, which a projection rule of cost 0 derives. Rules with numeric constraints are kept as they are. The minimal sum and max costs of every atom are
/// preserved.
formalism::ProgramView<LiftedTag> split_rules(formalism::ProgramView<LiftedTag> program, formalism::Repository& repository, RuleSplitting& splitting);

}

#endif
