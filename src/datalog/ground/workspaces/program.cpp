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

namespace tyr::datalog
{
namespace
{
template<formalism::RelationKind R>
void validate_rules(formalism::datalog::ProgramView<GroundTag> program, const FactSets& fact_sets)
{
    for (const auto rule : program.template get_rules<R>())
    {
        if (!is_statically_applicable(rule, fact_sets))
            throw std::invalid_argument("Ground Datalog programs must remove statically inapplicable rules before execution");
        for (const auto literal : rule.get_body().template get_literals<formalism::FluentTag>())
            if (!literal.get_polarity())
            {
                const auto kind = std::is_same_v<R, formalism::PredicateTag> ? "predicate" : "function";
                throw std::invalid_argument("Ground Datalog " + std::string(kind) + " rule " + std::to_string(ygg::uint_t(rule.get_index()))
                                            + " contains a negative fluent literal; only positive fluent rule bodies are supported");
            }
    }
}

void validate_program(formalism::datalog::ProgramView<GroundTag> program, const FactSets& fact_sets)
{
    validate_rules<formalism::PredicateTag>(program, fact_sets);
    validate_rules<formalism::FunctionTag>(program, fact_sets);
}

template<formalism::RelationKind R>
void initialize_dependencies(formalism::datalog::ProgramView<GroundTag> program, GroundRuleDependencies<R>& dependencies)
{
    for (const auto rule : program.template get_rules<R>())
    {
        const auto body = rule.get_body();
        for (const auto literal : body.template get_literals<formalism::FluentTag>())
            if (literal.get_polarity())
                dependencies.fluent_precondition_to_rules.update(literal.get_atom().get_row().get_index(), [&](auto& rules, bool) { rules.push_back(rule); });

        for (const auto term : formalism::datalog::collect_fluent_reads(rule))
            dependencies.fluent_function_term_to_rules.update(term.get_row().get_index(), [&](auto& rules, bool) { rules.push_back(rule); });
    }
}
}

ConstProgramWorkspace<GroundTag>::ConstProgramWorkspace(formalism::datalog::ProgramView<GroundTag> program_) :
    program(program_),
    facts(program.template get_predicates<formalism::StaticTag>(),
          program.template get_functions<formalism::StaticTag>(),
          program.template get_atoms<formalism::StaticTag>(),
          program.template get_fterm_values<formalism::StaticTag>(),
          program.get_context()),
    predicate_rules(program.template get_predicates<formalism::FluentTag>().size(), program.template get_functions<formalism::FluentTag>().size()),
    function_rules(program.template get_predicates<formalism::FluentTag>().size(), program.template get_functions<formalism::FluentTag>().size())
{
    const auto fluent_fact_sets = TaggedFactSets<formalism::FluentTag>(program.template get_predicates<formalism::FluentTag>(),
                                                                       program.template get_functions<formalism::FluentTag>(),
                                                                       program.template get_atoms<formalism::FluentTag>(),
                                                                       program.template get_fterm_values<formalism::FluentTag>(),
                                                                       program.get_context());
    validate_program(program, FactSets { facts.fact_sets, fluent_fact_sets });
    initialize_dependencies(program, predicate_rules);
    initialize_dependencies(program, function_rules);
}

}
