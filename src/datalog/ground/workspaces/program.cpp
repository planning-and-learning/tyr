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

#include "tyr/datalog/ground/workspaces/program.hpp"

#include "tyr/datalog/applicability.hpp"
#include "tyr/formalism/datalog/expression_properties.hpp"

#include <stdexcept>
#include <string>
#include <type_traits>

namespace f = tyr::formalism;
namespace fd = tyr::formalism::datalog;

namespace tyr::datalog
{
namespace
{
template<f::RelationKind R>
void validate_rules(fd::ProgramView<GroundTag> program, const FactSets& fact_sets)
{
    for (const auto rule : program.template get_rules<R>())
    {
        if (!is_statically_applicable(rule, fact_sets))
            throw std::invalid_argument("Ground Datalog programs must remove statically inapplicable rules before execution");
        for (const auto literal : rule.get_body().template get_literals<f::FluentTag>())
            if (!literal.get_polarity())
            {
                const auto kind = std::is_same_v<R, f::PredicateTag> ? "predicate" : "function";
                throw std::invalid_argument("Ground Datalog " + std::string(kind) + " rule " + std::to_string(ygg::uint_t(rule.get_index()))
                                            + " contains a negative fluent literal; only positive fluent rule bodies are supported");
            }
    }
}

void validate_program(fd::ProgramView<GroundTag> program, const FactSets& fact_sets)
{
    validate_rules<f::PredicateTag>(program, fact_sets);
    validate_rules<f::FunctionTag>(program, fact_sets);
}

template<f::RelationKind R>
void initialize_dependencies(fd::ProgramView<GroundTag> program, GroundRuleDependencies<R>& dependencies)
{
    for (const auto rule : program.template get_rules<R>())
    {
        const auto body = rule.get_body();
        for (const auto literal : body.template get_literals<f::FluentTag>())
            if (literal.get_polarity())
                dependencies.fluent_precondition_to_rules.update(literal.get_atom().get_row(), [&](auto& rules, bool) { rules.push_back(rule); });

        for (const auto term : fd::collect_fluent_reads(rule))
            dependencies.fluent_function_term_to_rules.update(term.get_row(), [&](auto& rules, bool) { rules.push_back(rule); });
    }
}
}

ConstProgramWorkspace<GroundTag>::ConstProgramWorkspace(fd::ProgramView<GroundTag> program_) :
    program(program_),
    facts(program.template get_predicates<f::StaticTag>(),
          program.template get_functions<f::StaticTag>(),
          program.template get_atoms<f::StaticTag>(),
          program.template get_fterm_values<f::StaticTag>(),
          program.get_context()),
    predicate_rules(program.template get_predicates<f::FluentTag>().size(), program.template get_functions<f::FluentTag>().size()),
    function_rules(program.template get_predicates<f::FluentTag>().size(), program.template get_functions<f::FluentTag>().size())
{
    const auto fluent_fact_sets = TaggedFactSets<f::FluentTag>(program.template get_predicates<f::FluentTag>(),
                                                               program.template get_functions<f::FluentTag>(),
                                                               program.template get_atoms<f::FluentTag>(),
                                                               program.template get_fterm_values<f::FluentTag>(),
                                                               program.get_context());
    validate_program(program, FactSets { facts.fact_sets, fluent_fact_sets });
    initialize_dependencies(program, predicate_rules);
    initialize_dependencies(program, function_rules);
}

}
