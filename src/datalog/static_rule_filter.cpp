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

#include "tyr/datalog/static_rule_filter.hpp"

#include "tyr/datalog/applicability.hpp"
#include "tyr/formalism/datalog/program_builder.hpp"
#include "tyr/formalism/detail/repository_instantiations.hpp"
#include "tyr/formalism/interning.hpp"
#include "tyr/formalism/repository.hpp"

namespace tyr::datalog
{

formalism::ProgramView<GroundTag> remove_statically_inapplicable_rules(formalism::ProgramView<GroundTag> program, formalism::Repository& repository)
{
    const auto static_fact_sets = TaggedFactSets<formalism::StaticTag>(program.get_predicates<formalism::StaticTag>(),
                                                                       program.get_functions<formalism::StaticTag>(),
                                                                       program.get_atoms<formalism::StaticTag>(),
                                                                       program.get_fterm_values<formalism::StaticTag>(),
                                                                       program.context());
    auto mutable_fact_sets = MutableFactSets(program, program.context());
    insert_program_facts(mutable_fact_sets, program);
    const auto fact_sets = FactSets { static_fact_sets, mutable_fact_sets };

    auto builder = formalism::Builder {};
    auto result = formalism::checkout<formalism::Program<GroundTag>>(builder);
    formalism::list_except_rules(program, *result);

    for (const auto rule : program.get_rules())
        if (is_statically_applicable(rule, fact_sets))
            urd::append(result->rules, rule);

    return repository.insert(*result, builder).first;
}

}
