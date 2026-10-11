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
#include "tyr/formalism/datalog/copy.hpp"
#include "tyr/formalism/datalog/repository.hpp"

namespace tyr::datalog
{

formalism::datalog::ProgramView<GroundTag> remove_statically_inapplicable_rules(formalism::datalog::ProgramView<GroundTag> program,
                                                                                formalism::datalog::Repository& repository)
{
    const auto static_fact_sets = TaggedFactSets<formalism::StaticTag>(program.get_predicates<formalism::StaticTag>(),
                                                                       program.get_functions<formalism::StaticTag>(),
                                                                       program.get_atoms<formalism::StaticTag>(),
                                                                       program.get_fterm_values<formalism::StaticTag>(),
                                                                       program.get_context());
    const auto fluent_fact_sets = TaggedFactSets<formalism::FluentTag>(program.get_predicates<formalism::FluentTag>(),
                                                                       program.get_functions<formalism::FluentTag>(),
                                                                       program.get_atoms<formalism::FluentTag>(),
                                                                       program.get_fterm_values<formalism::FluentTag>(),
                                                                       program.get_context());
    const auto fact_sets = FactSets { static_fact_sets, fluent_fact_sets };

    auto builder = formalism::datalog::Builder {};
    auto context = formalism::datalog::CopyContext { builder, repository };
    auto result = formalism::datalog::checkout<formalism::datalog::Program<GroundTag>>(builder);
    const auto merge_all = [&](const auto elements, auto& destination)
    {
        for (const auto element : elements)
            destination.push_back(formalism::datalog::copy(element, context).first.get_index());
    };

    merge_all(program.get_predicates<formalism::StaticTag>(), result->static_predicates);
    merge_all(program.get_predicates<formalism::FluentTag>(), result->fluent_predicates);
    merge_all(program.get_functions<formalism::StaticTag>(), result->static_functions);
    merge_all(program.get_functions<formalism::FluentTag>(), result->fluent_functions);
    merge_all(program.get_objects(), result->objects);
    merge_all(program.get_atoms<formalism::StaticTag>(), result->static_atoms);
    merge_all(program.get_atoms<formalism::FluentTag>(), result->fluent_atoms);
    merge_all(program.get_fterm_values<formalism::StaticTag>(), result->static_fterm_values);
    merge_all(program.get_fterm_values<formalism::FluentTag>(), result->fluent_fterm_values);

    if (const auto goal = program.get_goal())
        result->goal = formalism::datalog::copy(*goal, context).first.get_index();
    if (const auto metric = program.get_metric())
        result->metric = formalism::datalog::copy(*metric, context).first.get_index();

    const auto merge_applicable_rules = [&]<formalism::RelationKind R>()
    {
        for (const auto rule : program.get_rules<R>())
            if (is_statically_applicable(rule, fact_sets))
                result->get_rules<R>().push_back(formalism::datalog::copy(rule, context).first.get_index());
    };
    merge_applicable_rules.template operator()<formalism::PredicateTag>();
    merge_applicable_rules.template operator()<formalism::FunctionTag>();

    return formalism::datalog::insert(repository, *result).first;
}

}
