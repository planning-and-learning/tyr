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

#ifndef TYR_FORMALISM_DATALOG_COPY_HPP_
#define TYR_FORMALISM_DATALOG_COPY_HPP_

#include "tyr/formalism/copy.hpp"
#include "tyr/formalism/datalog/interning.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/atom_view.hpp"
#include "tyr/formalism/binary_operator_view.hpp"
#include "tyr/formalism/datalog/conjunctive_condition_view.hpp"
#include "tyr/formalism/datalog/conjunctive_effect_view.hpp"
#include "tyr/formalism/function_term_value_view.hpp"
#include "tyr/formalism/function_term_view.hpp"
#include "tyr/formalism/literal_view.hpp"
#include "tyr/formalism/multi_operator_view.hpp"
#include "tyr/formalism/numeric_effect_view.hpp"
#include "tyr/formalism/datalog/program_view.hpp"
#include "tyr/formalism/datalog/rule_view.hpp"
#include "tyr/formalism/unary_operator_view.hpp"
#include "tyr/formalism/declarations.hpp"
#include <yggdrasil/formalism/row_view.hpp>
#include "tyr/formalism/function_view.hpp"
#include "tyr/formalism/object_view.hpp"
#include "tyr/formalism/parameter_index.hpp"
#include "tyr/formalism/predicate_view.hpp"
#include "tyr/formalism/variable_view.hpp"

namespace tyr::formalism::datalog
{

/// Copies everything of the program but its rules into the program data.
template<TaskKind T>
void copy_except_rules(ProgramView<T> element, urd::data<Program<T>>& result, WriteContext<Builder, Repository>& context)
{
    const auto copy_all = [&](const auto elements, auto& destination)
    {
        for (const auto e : elements)
            destination.push_back(copy(e, context).first.index());
    };

    copy_all(element.template get_predicates<StaticTag>(), result.static_predicates);
    copy_all(element.template get_predicates<FluentTag>(), result.fluent_predicates);
    copy_all(element.template get_functions<StaticTag>(), result.static_functions);
    copy_all(element.template get_functions<FluentTag>(), result.fluent_functions);
    copy_all(element.get_objects(), result.objects);
    copy_all(element.template get_atoms<StaticTag>(), result.static_atoms);
    copy_all(element.template get_atoms<FluentTag>(), result.fluent_atoms);
    copy_all(element.template get_fterm_values<StaticTag>(), result.static_fterm_values);
    copy_all(element.template get_fterm_values<FluentTag>(), result.fluent_fterm_values);

    if (const auto goal = element.get_goal())
        result.goal = copy(*goal, context).first.index();
}

}

#endif
