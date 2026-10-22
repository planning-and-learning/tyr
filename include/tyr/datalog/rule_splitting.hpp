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

#include <utility>
#include <vector>
#include <yggdrasil/containers/associative_containers.hpp>
#include <yggdrasil/serialization/cista_equal_to.hpp>
#include <yggdrasil/serialization/cista_hash.hpp>
#include <yggdrasil/semantics/equal_to.hpp>
#include <yggdrasil/semantics/hash.hpp>

namespace tyr::datalog
{

struct RuleSplitting
{
    using Rule = ygg::Index<formalism::datalog::Rule<LiftedTag, formalism::PredicateTag>>;

    /// A static relation over kept variables that replaces static literals over variables of a source rule that only they use.
    /// Each tuple stores one assignment of the eliminated variables that supports it, as source rule positions and objects.
    struct StaticProjection
    {
        std::vector<ygg::uint_t> kept_positions;
        ygg::UnorderedMap<ygg::IndexList<formalism::Object>, std::vector<std::pair<ygg::uint_t, ygg::Index<formalism::Object>>>> supports;
    };

    /// A rule that replaces part of a split source rule: the position of each of its variables among the source rule's variables,
    /// and the static projections its body uses (indices into static_projections).
    struct Origin
    {
        size_t num_source_variables;
        std::vector<ygg::uint_t> source_positions;
        std::vector<size_t> static_projections;
    };

    /// The main rule that replaces each split source rule; source rules index the source program's repository.
    ygg::UnorderedMap<Rule, Rule> main_rules;
    /// The origin of every rule that splitting created, main rules and projection rules alike.
    ygg::UnorderedMap<Rule, Origin> origins;
    /// The projection rule that derives each projection predicate.
    ygg::UnorderedMap<ygg::Index<formalism::Predicate<formalism::FluentTag>>, Rule> projection_rules;
    std::vector<StaticProjection> static_projections;
};

/// Copies the program into the repository and projects the variables of its predicate rules that occur in no head and only
/// in some preconditions: the preconditions that use such a variable are replaced by an atom over their variables that the
/// rest of the rule needs. If they are all static, the atoms are computed once into a new static predicate; otherwise a
/// projection rule of cost 0 derives them. Rules with numeric constraints are copied unchanged. The minimal sum and max
/// costs of every atom are preserved.
formalism::datalog::ProgramView<LiftedTag>
split_rules(formalism::datalog::ProgramView<LiftedTag> program, formalism::datalog::Repository& repository, RuleSplitting& splitting);

}

#endif
