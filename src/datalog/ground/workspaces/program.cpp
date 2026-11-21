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
#include "tyr/formalism/expression_properties.hpp"

#include <boost/mp11.hpp>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace tyr::datalog
{
namespace
{
void validate_rules(formalism::ProgramView<GroundTag> program, const FactSets& fact_sets)
{
    for (const auto rule : program.get_rules())
    {
        if (!is_statically_applicable(rule, fact_sets))
            throw std::invalid_argument("Ground Datalog programs must remove statically inapplicable rules before execution");
        boost::mp11::mp_for_each<MutableTags>(
            [&]<typename F>(F)
            {
                for (const auto literal : rule.get_body().template get_literals<F>())
                    if (!literal.get_polarity())
                    {
                        throw std::invalid_argument("Ground Datalog rule " + std::to_string(static_cast<ygg::uint_t>(rule.index().value))
                                                    + " contains a negative fluent or derived literal; only positive rule bodies are supported");
                    }
            });
    }
}

void validate_program(formalism::ProgramView<GroundTag> program, const FactSets& fact_sets) { validate_rules(program, fact_sets); }

void initialize_dependencies(formalism::ProgramView<GroundTag> program, GroundRuleDependencies& dependencies)
{
    for (const auto rule : program.get_rules())
    {
        const auto body = rule.get_body();
        boost::mp11::mp_for_each<MutableTags>(
            [&]<typename F>(F)
            {
                for (const auto literal : body.template get_literals<F>())
                    if (literal.get_polarity())
                        dependencies.precondition_to_rules.template get<F>().update(literal.get_atom().get_row().index(),
                                                                                    [&](auto& rules, bool) { rules.push_back(rule); });
            });
    }
}
}

ConstProgramWorkspace<GroundTag>::ConstProgramWorkspace(formalism::ProgramView<GroundTag> program_) :
    program(program_),
    facts(program.template get_predicates<formalism::StaticTag>(),
          program.template get_functions<formalism::StaticTag>(),
          program.template get_atoms<formalism::StaticTag>(),
          program.template get_fterm_values<formalism::StaticTag>(),
          program.context()),
    dependencies(program)
{
    auto mutable_fact_sets = MutableFactSets(program, program.context());
    insert_program_facts(mutable_fact_sets, program);
    validate_program(program, FactSets { facts.fact_sets, mutable_fact_sets });
    initialize_dependencies(program, dependencies);
}

}
