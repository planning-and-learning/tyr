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
#include "tyr/formalism/datalog/interning.hpp"
#include "tyr/formalism/datalog/program_view.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/datalog/rule_body_view.hpp"
#include "tyr/formalism/datalog/rule_view.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/function_term_value_view.hpp"
#include "tyr/formalism/function_term_view.hpp"
#include "tyr/formalism/function_view.hpp"
#include "tyr/formalism/literal_view.hpp"
#include "tyr/formalism/multi_operator_view.hpp"
#include "tyr/formalism/numeric_effect_view.hpp"
#include "tyr/formalism/object_view.hpp"
#include "tyr/formalism/parameter_index.hpp"
#include "tyr/formalism/planning/grounder.hpp"
#include "tyr/formalism/predicate_view.hpp"
#include "tyr/formalism/unary_operator_view.hpp"
#include "tyr/formalism/variable_view.hpp"
#include "tyr/formalism/write_context.hpp"

#include <yggdrasil/formalism/row_view.hpp>

namespace tyr::formalism::datalog
{

/**
 * ground
 */

/// The shared records are grounded as the planning formalism grounds them (one repository holds both).
using planning::ground;

/// A lifted rule body as a ground rule's body.
std::pair<ConjunctiveConditionView<GroundTag>, bool> ground(ConjunctiveConditionView<LiftedTag> element, WriteContext<Builder, Repository>& context);

/**
 * ground_binding
 */

template<FactKind F>
std::pair<PredicateRowView<F>, bool> ground_binding(AtomView<LiftedTag, F> element, WriteContext<Builder, Repository>& context);

/// The key of the atom's row, interned as ground_binding does, without its view (which finds the layer that owns the row).
template<FactKind F>
std::pair<PredicateRowIndex<F>, bool> ground_row(AtomView<LiftedTag, F> element, WriteContext<Builder, Repository>& context);

/// Grounds the binding without interning the enclosing GroundFunctionTerm symbol.
template<FactKind F>
std::pair<FunctionRowView<F>, bool> ground_binding(FunctionTermView<LiftedTag, F> element, WriteContext<Builder, Repository>& context);

std::pair<RuleRowView, bool> ground_binding(RuleView<LiftedTag> element, WriteContext<Builder, Repository>& context);

/**
 * try_ground
 */

template<FactKind F>
std::optional<FunctionRowView<F>> try_ground_binding(FunctionTermView<LiftedTag, F> element, WriteContext<Builder, Repository>& context);

template<FactKind F>
std::optional<PredicateRowView<F>> try_ground_binding(AtomView<LiftedTag, F> element, WriteContext<Builder, Repository>& context);

/**
 * ground
 */

/// Substitute terms into retained row values (objects as their canonical Values) without interning a binding.
inline void ground(TermListView terms, const WriteContext<Builder, Repository>& context, std::vector<ygg::Value>& values)
{
    values.clear();
    for (const auto term : terms)
        visit(
            [&]<typename Alternative>(Alternative arg)
            {
                if constexpr (std::same_as<Alternative, ParameterIndex>)
                    values.push_back(context.binding()[arg.value].value);
                else if constexpr (std::same_as<Alternative, ObjectView>)
                    values.push_back(arg.index().value);
                else
                    static_assert(false, "Missing case");
            },
            term);
}

inline std::pair<ConjunctiveConditionView<GroundTag>, bool> ground(ConjunctiveConditionView<LiftedTag> element, WriteContext<Builder, Repository>& context)
{
    // Fetch and clear
    auto conj_cond = datalog::checkout<RuleBody<GroundTag>>(context.builder());

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

/**
 * ground_binding
 */

template<FactKind F>
std::pair<PredicateRowIndex<F>, bool> ground_row(AtomView<LiftedTag, F> element, WriteContext<Builder, Repository>& context)
{
    auto binding = datalog::checkout<ygg::Row<Predicate<F>>>(context.builder());

    urd::store(binding->relation, element.get_predicate());
    ground(element.get_terms(), context, binding->values);

    const auto [row, created] = context.destination().insert(*binding, context.builder());
    return { row.index(), created };
}

template<FactKind F>
std::pair<PredicateRowView<F>, bool> ground_binding(AtomView<LiftedTag, F> element, WriteContext<Builder, Repository>& context)
{
    const auto [key, created] = ground_row(element, context);
    return { context.destination().view(key), created };
}

template<FactKind F>
std::pair<FunctionRowView<F>, bool> ground_binding(FunctionTermView<LiftedTag, F> element, WriteContext<Builder, Repository>& context)
{ return ground(element.get_terms(), element.get_function(), context); }

inline std::pair<RuleRowView, bool> ground_binding(RuleView<LiftedTag> element, WriteContext<Builder, Repository>& context)
{
    auto binding = datalog::checkout<ygg::Row<Rule<LiftedTag>>>(context.builder());

    urd::store(binding->relation, element);
    for (const auto object : context.binding())
        binding->values.push_back(object.value);

    return context.destination().insert(*binding, context.builder());
}

/**
 * try_ground
 */

template<FactKind F>
std::optional<FunctionRowView<F>> try_ground_binding(FunctionTermView<LiftedTag, F> element, WriteContext<Builder, Repository>& context)
{
    auto binding = datalog::checkout<ygg::Row<Function<F>>>(context.builder());

    urd::store(binding->relation, element.get_function());
    ground(element.get_terms(), context, binding->values);

    return context.destination().find(*binding, context.builder());
}

template<FactKind F>
std::optional<PredicateRowView<F>> try_ground_binding(AtomView<LiftedTag, F> element, WriteContext<Builder, Repository>& context)
{
    auto binding = datalog::checkout<ygg::Row<Predicate<F>>>(context.builder());

    urd::store(binding->relation, element.get_predicate());
    ground(element.get_terms(), context, binding->values);

    return context.destination().find(*binding, context.builder());
}
}

#ifndef TYR_HEADER_INSTANTIATION

namespace tyr::formalism::datalog
{

extern template std::pair<PredicateRowView<StaticTag>, bool> ground_binding(AtomView<LiftedTag, StaticTag> element, WriteContext<Builder, Repository>& context);
extern template std::pair<PredicateRowView<FluentTag>, bool> ground_binding(AtomView<LiftedTag, FluentTag> element, WriteContext<Builder, Repository>& context);
extern template std::pair<PredicateRowView<DerivedTag>, bool> ground_binding(AtomView<LiftedTag, DerivedTag> element,
                                                                             WriteContext<Builder, Repository>& context);

extern template std::pair<FunctionRowView<StaticTag>, bool> ground_binding(FunctionTermView<LiftedTag, StaticTag> element,
                                                                           WriteContext<Builder, Repository>& context);
extern template std::pair<FunctionRowView<FluentTag>, bool> ground_binding(FunctionTermView<LiftedTag, FluentTag> element,
                                                                           WriteContext<Builder, Repository>& context);

extern template std::optional<FunctionRowView<StaticTag>> try_ground_binding(FunctionTermView<LiftedTag, StaticTag> element,
                                                                             WriteContext<Builder, Repository>& context);
extern template std::optional<FunctionRowView<FluentTag>> try_ground_binding(FunctionTermView<LiftedTag, FluentTag> element,
                                                                             WriteContext<Builder, Repository>& context);

extern template std::optional<PredicateRowView<StaticTag>> try_ground_binding(AtomView<LiftedTag, StaticTag> element,
                                                                              WriteContext<Builder, Repository>& context);
extern template std::optional<PredicateRowView<FluentTag>> try_ground_binding(AtomView<LiftedTag, FluentTag> element,
                                                                              WriteContext<Builder, Repository>& context);
extern template std::optional<PredicateRowView<DerivedTag>> try_ground_binding(AtomView<LiftedTag, DerivedTag> element,
                                                                               WriteContext<Builder, Repository>& context);

}

#endif

#endif
