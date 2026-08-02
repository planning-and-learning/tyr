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

#include "tyr/formalism/datalog/expression_properties.hpp"

#include <type_traits>
#include <yggdrasil/containers/variant.hpp>

namespace f = tyr::formalism;
namespace fd = tyr::formalism::datalog;

namespace tyr::datalog
{
namespace
{
void collect_head_terms(fd::GroundAtomView<f::FluentTag>, ygg::UnorderedSet<fd::GroundFunctionTermView<f::FluentTag>>&) {}

void collect_head_terms(fd::GroundNumericEffectOperatorView<f::FluentTag> head, ygg::UnorderedSet<fd::GroundFunctionTermView<f::FluentTag>>& terms)
{
    fd::collect_fterms<f::FluentTag>(head, terms);
}

template<f::RelationKind R>
void initialize_dependencies(fd::ProgramView<GroundTag> program, GroundRuleDependencies<R>& dependencies)
{
    for (const auto rule : program.template get_ground_rules<R>())
    {
        const auto body = rule.get_body();
        for (const auto literal : body.template get_literals<f::FluentTag>())
            if (literal.get_polarity())
                dependencies.fluent_precondition_to_rules.update(literal.get_atom().get_row(), [&](auto& rules, bool) { rules.push_back(rule); });

        auto fluent_terms = ygg::UnorderedSet<fd::GroundFunctionTermView<f::FluentTag>>();
        for (const auto numeric_constraint : body.get_numeric_constraints())
            fd::collect_fterms<f::FluentTag>(numeric_constraint, fluent_terms);
        collect_head_terms(rule.get_head(), fluent_terms);

        for (const auto term : fluent_terms)
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
    initialize_dependencies(program, predicate_rules);
    initialize_dependencies(program, function_rules);
}

}
