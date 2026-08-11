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
#include "tyr/formalism/datalog/merge.hpp"
#include "tyr/formalism/datalog/repository.hpp"

namespace tyr::datalog
{

namespace fd = ::tyr::formalism::datalog;
namespace f = ::tyr::formalism;

fd::GroundProgramView remove_statically_inapplicable_rules(fd::GroundProgramView program, fd::Repository& repository)
{
    const auto static_fact_sets = TaggedFactSets<f::StaticTag>(program.get_predicates<f::StaticTag>(),
                                                               program.get_functions<f::StaticTag>(),
                                                               program.get_atoms<f::StaticTag>(),
                                                               program.get_fterm_values<f::StaticTag>(),
                                                               program.get_context());
    const auto fluent_fact_sets = TaggedFactSets<f::FluentTag>(program.get_predicates<f::FluentTag>(),
                                                               program.get_functions<f::FluentTag>(),
                                                               program.get_atoms<f::FluentTag>(),
                                                               program.get_fterm_values<f::FluentTag>(),
                                                               program.get_context());
    const auto fact_sets = FactSets { static_fact_sets, fluent_fact_sets };

    auto builder = fd::Builder {};
    auto context = fd::MergeContext { builder, repository };
    auto result = fd::checkout<fd::GroundProgram>(builder);
    const auto merge_all = [&](const auto elements, auto& destination)
    {
        for (const auto element : elements)
            destination.push_back(fd::merge_d2d(element, context).first.get_index());
    };

    merge_all(program.get_predicates<f::StaticTag>(), result->static_predicates);
    merge_all(program.get_predicates<f::FluentTag>(), result->fluent_predicates);
    merge_all(program.get_functions<f::StaticTag>(), result->static_functions);
    merge_all(program.get_functions<f::FluentTag>(), result->fluent_functions);
    merge_all(program.get_objects(), result->objects);
    merge_all(program.get_atoms<f::StaticTag>(), result->static_atoms);
    merge_all(program.get_atoms<f::FluentTag>(), result->fluent_atoms);
    merge_all(program.get_fterm_values<f::StaticTag>(), result->static_fterm_values);
    merge_all(program.get_fterm_values<f::FluentTag>(), result->fluent_fterm_values);

    if (const auto goal = program.get_goal())
        result->goal = fd::merge_d2d(*goal, context).first.get_index();
    if (const auto metric = program.get_metric())
        result->metric = fd::merge_d2d(*metric, context).first.get_index();

    const auto merge_applicable_rules = [&]<f::RelationKind R>()
    {
        for (const auto rule : program.get_rules<R>())
            if (is_statically_applicable(rule, fact_sets))
                result->get_rules<R>().push_back(fd::merge_d2d(rule, context).first.get_index());
    };
    merge_applicable_rules.template operator()<f::PredicateTag>();
    merge_applicable_rules.template operator()<f::FunctionTag>();

    return fd::get_or_create(repository, *result).first;
}

}
