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

#ifndef TYR_FORMALISM_DATALOG_GROUNDER_HPP_
#define TYR_FORMALISM_DATALOG_GROUNDER_HPP_

#include "tyr/analysis/declarations.hpp"
#include "tyr/formalism/atom_view.hpp"
#include "tyr/formalism/binary_operator_view.hpp"
#include "tyr/formalism/conjunctive_condition_view.hpp"
#include "tyr/formalism/datalog/canonicalization.hpp"
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/datalog/formatter.hpp"
#include "tyr/formalism/datalog/program_view.hpp"
#include "tyr/formalism/datalog/rule_body_view.hpp"
#include "tyr/formalism/datalog/rule_view.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/function_term_value_view.hpp"
#include "tyr/formalism/function_term_view.hpp"
#include "tyr/formalism/function_view.hpp"
#include "tyr/formalism/grounder.hpp"
#include "tyr/formalism/grounding_context.hpp"
#include "tyr/formalism/literal_view.hpp"
#include "tyr/formalism/multi_operator_view.hpp"
#include "tyr/formalism/numeric_effect_view.hpp"
#include "tyr/formalism/object_view.hpp"
#include "tyr/formalism/parameter_index.hpp"
#include "tyr/formalism/predicate_view.hpp"
#include "tyr/formalism/repository.hpp"
#include "tyr/formalism/unary_operator_view.hpp"
#include "tyr/formalism/variable_view.hpp"

#include <utility>
#include <yggdrasil/formalism/row_view.hpp>

/// `ground` of Datalog's records: a rule's body and its binding (the shared records' in formalism/grounder.hpp).
namespace tyr::formalism
{

/// A lifted rule body as a ground rule's body.
std::pair<RuleBodyView<GroundTag>, bool> ground(RuleBodyView<LiftedTag> element, GroundingContext<Builder, Repository>& context);

std::pair<RuleRowView, bool> ground_binding(RuleView<LiftedTag> element, GroundingContext<Builder, Repository>& context);

inline std::pair<RuleBodyView<GroundTag>, bool> ground(RuleBodyView<LiftedTag> element, GroundingContext<Builder, Repository>& context)
{
    // Fetch and clear
    auto conj_cond = checkout<RuleBody<GroundTag>>(context.builder());

    // Fill data
    for (const auto literal : element.template get_literals<StaticTag>())
        urd::append(conj_cond->static_literals, ground(literal, context).first);
    for (const auto literal : element.template get_literals<FluentTag>())
        urd::append(conj_cond->fluent_literals, ground(literal, context).first);
    for (const auto literal : element.template get_literals<DerivedTag>())
        urd::append(conj_cond->derived_literals, ground(literal, context).first);
    for (const auto numeric_constraint : element.get_numeric_constraints())
        urd::append(conj_cond->numeric_constraints, ground(numeric_constraint, context).first);

    // Canonicalize and Serialize
    return context.destination().insert(*conj_cond, context.builder());
}

inline std::pair<RuleRowView, bool> ground_binding(RuleView<LiftedTag> element, GroundingContext<Builder, Repository>& context)
{
    auto binding = checkout<ygg::Row<Rule<LiftedTag>>>(context.builder());

    urd::store(binding->relation, element);
    for (const auto object : context.binding())
        binding->values.push_back(object.value);

    return context.destination().insert(*binding, context.builder());
}

}

#endif
