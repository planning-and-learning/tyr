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

#include "tyr/formalism/planning/formatter.hpp"

#include "../node_formatter.hpp"

#include "tyr/formalism/planning/planning_domain.hpp"
#include "tyr/formalism/planning/planning_task.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <cstddef>
#include <fmt/ostream.h>
#include <fmt/ranges.h>
#include <iterator>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <yggdrasil/formatting/cista_formatters.hpp>
#include <yggdrasil/formatting/formatter.hpp>
#include <yggdrasil/io/iostream.hpp>

namespace tyr::formalism::planning
{

namespace detail
{
namespace
{

template<typename F>
std::string structured(std::string_view name, F write_fields)
{
    auto os = std::stringstream {};
    os << name << "(\n";
    {
        ygg::IndentScope scope(os);
        write_fields(os);
    }
    os << ygg::print_indent << ")";
    return os.str();
}

template<typename T>
void field(std::ostream& os, std::string_view name, const T& value)
{
    os << ygg::print_indent;
    fmt::print(os, "{}{}\n", name, value);
}

template<FactKind T>
std::string fdr_variable(const urd::data<FDRVariable<T>>& value)
{
    return structured("FDRVariable",
                      [&](auto& os)
                      {
                          field(os, "index = ", value.index);
                          field(os, "atoms = ", value.atoms);
                      });
}

template<FactKind T>
std::string fdr_variable(FDRVariableView<T> value)
{
    return structured("FDRVariable",
                      [&](auto& os)
                      {
                          field(os, "index = ", value.index());
                          field(os, "atoms = ", value.get_atoms());
                      });
}

std::string invariant(const urd::data<Invariant>& value)
{
    return structured("Invariant",
                      [&](auto& os)
                      {
                          field(os, "index = ", value.index);
                          field(os, "num rigid variables = ", value.num_rigid_variables);
                          field(os, "num counted variables = ", value.num_counted_variables);
                          field(os, "atoms = ", value.atoms);
                      });
}

std::string invariant(InvariantView value)
{
    return structured("Invariant",
                      [&](auto& os)
                      {
                          field(os, "index = ", value.index());
                          field(os, "num rigid variables = ", value.get_num_rigid_variables());
                          field(os, "num counted variables = ", value.get_num_counted_variables());
                          field(os, "atoms = ", value.get_atoms());
                      });
}

template<FactKind T>
std::string fdr_fact(const urd::data<FDRFact<T>>& value)
{ return fmt::format("<{},{}>", value.variable, value.value); }

/// A fact's view, borrowed (a stored fact) or owned (a state's, computed on the fly).
template<typename V>
std::string fdr_fact_view(V value)
{
    const auto atom = value.get_atom();
    if (!atom.has_value())
    {
        return fmt::format("<{},{}>: (none-of {})",
                           value.get_variable().index(),
                           value.get_value(),
                           fmt::join(ygg::to_strings(value.get_variable().get_atoms()), " "));
    }
    return fmt::format("<{},{}>: {}", value.get_variable().index(), value.get_value(), atom.value());
}

template<typename View>
std::string named(View value)
{ return fmt::format("{}", value.get_name()); }

template<typename View>
std::string symbol(View value)
{ return fmt::format("{}/{}", value.get_name(), value.get_arity()); }

template<typename View>
std::string relation_binding(View value)
{
    auto result = fmt::format("({}", value.get_relation().get_name());
    // A borrowed binding's action may have no pool yet (no stored types): its values are read as objects directly.
    const auto objects = urd::transform(value.get_values(), [](ygg::Value v) { return urd::index<Object>(static_cast<std::uint32_t>(v)); });
    for (const auto object : urd::resolve(objects, value.context()))
        fmt::format_to(std::back_inserter(result), " {}", object);
    result.push_back(')');
    return result;
}

template<typename Tag>
std::string relation_binding(const urd::data<ygg::Row<Tag>>& value)
{ return fmt::format("{} {}", value.relation, fmt::join(ygg::to_strings(value.values), " ")); }

}  // namespace
}  // namespace detail


std::string to_string(const urd::data<FDRVariable<FluentTag>>& value) { return detail::fdr_variable(value); }

std::string to_string(const urd::data<Invariant>& value) { return detail::invariant(value); }
std::string to_string(const urd::data<FDRFact<FluentTag>>& value) { return detail::fdr_fact(value); }


std::string to_string(const urd::data<ConjunctiveCondition<LiftedTag>>& value)
{
    return detail::structured("ConjunctiveCondition",
                              [&](auto& os)
                              {
                                  detail::field(os, "index = ", value.index);
                                  detail::field(os, "variables = ", value.variables);
                                  detail::field(os, "static literals = ", value.static_literals);
                                  detail::field(os, "fluent literals = ", value.fluent_literals);
                                  detail::field(os, "derived literals = ", value.derived_literals);
                                  detail::field(os, "numeric constraints = ", value.numeric_constraints);
                              });
}

std::string to_string(const urd::data<ConjunctiveCondition<GroundTag>>& value)
{
    return detail::structured("GroundConjunctiveCondition",
                              [&](auto& os)
                              {
                                  detail::field(os, "index = ", value.index);
                                  detail::field(os, "static literals = ", value.static_literals);
                                  detail::field(os, "derived literals = ", value.derived_literals);
                                  detail::field(os, "positive facts = ", value.positive_facts);
                                  detail::field(os, "negative facts = ", value.negative_facts);
                                  detail::field(os, "numeric constraints = ", value.numeric_constraints);
                              });
}

std::string to_string(const urd::data<ConditionalEffect<LiftedTag>>& value)
{
    return detail::structured("ConditionalEffect",
                              [&](auto& os)
                              {
                                  detail::field(os, "index = ", value.index);
                                  detail::field(os, "variables = ", value.variables);
                                  detail::field(os, "condition = ", value.condition);
                                  detail::field(os, "effect = ", value.effect);
                              });
}

std::string to_string(const urd::data<ConditionalEffect<GroundTag>>& value)
{
    return detail::structured("GroundConditionalEffect",
                              [&](auto& os)
                              {
                                  detail::field(os, "index = ", value.index);
                                  detail::field(os, "condition = ", value.condition);
                                  detail::field(os, "effect = ", value.effect);
                              });
}

std::string to_string(const urd::data<ConjunctiveEffect<LiftedTag>>& value)
{
    return detail::structured("ConjunctiveEffect",
                              [&](auto& os)
                              {
                                  detail::field(os, "index = ", value.index);
                                  detail::field(os, "fluent literals = ", value.literals);
                                  detail::field(os, "fluent numeric effects = ", value.numeric_effects);
                                  detail::field(os, "auxiliary numeric effect = ", value.auxiliary_numeric_effect);
                              });
}

std::string to_string(const urd::data<ConjunctiveEffect<GroundTag>>& value)
{
    return detail::structured("GroundConjunctiveEffect",
                              [&](auto& os)
                              {
                                  detail::field(os, "index = ", value.index);
                                  detail::field(os, "add facts = ", value.add_facts);
                                  detail::field(os, "del facts = ", value.del_facts);
                                  detail::field(os, "fluent numeric effects = ", value.numeric_effects);
                                  detail::field(os, "auxiliary numeric effect = ", value.auxiliary_numeric_effect);
                              });
}

std::string to_string(const urd::data<Action<LiftedTag>>& value)
{
    return detail::structured("Action",
                              [&](auto& os)
                              {
                                  detail::field(os, "index = ", value.index);
                                  detail::field(os, "name = ", value.name);
                                  detail::field(os, "variables = ", value.variables);
                                  detail::field(os, "condition = ", value.condition);
                                  detail::field(os, "effects = ", value.effects);
                              });
}

std::string to_string(const urd::data<Action<GroundTag>>& value)
{
    return detail::structured("GroundAction",
                              [&](auto& os)
                              {
                                  detail::field(os, "index = ", value.index);
                                  detail::field(os, "row = ", value.row);
                                  detail::field(os, "condition = ", value.condition);
                                  detail::field(os, "effects = ", value.effects);
                              });
}

std::string to_string(const urd::data<Axiom<LiftedTag>>& value)
{
    return detail::structured("Axiom",
                              [&](auto& os)
                              {
                                  detail::field(os, "index = ", value.index);
                                  detail::field(os, "variables = ", value.variables);
                                  detail::field(os, "body = ", value.body);
                                  detail::field(os, "head = ", value.head);
                              });
}

std::string to_string(const urd::data<Axiom<GroundTag>>& value)
{
    return detail::structured("GroundAxiom",
                              [&](auto& os)
                              {
                                  detail::field(os, "index = ", value.index);
                                  detail::field(os, "row = ", value.row);
                                  detail::field(os, "body = ", value.body);
                                  detail::field(os, "head = ", value.head);
                              });
}

std::string to_string(const urd::data<Metric>& value) { return fmt::format("({} {})", value.optimization_direction, value.fexpr); }

std::string to_string(const urd::data<Task<LiftedTag>>& value)
{
    return detail::structured("LiftedTask",
                              [&](auto& os)
                              {
                                  detail::field(os, "index = ", value.index);
                                  detail::field(os, "name = ", value.name);
                                  detail::field(os, "derived predicates = ", value.derived_predicates);
                                  detail::field(os, "objects = ", value.objects);
                                  detail::field(os, "static atoms = ", value.static_atoms);
                                  detail::field(os, "fluent atoms = ", value.fluent_atoms);
                                  detail::field(os, "static numeric variables = ", value.static_fterm_values);
                                  detail::field(os, "fluent numeric variables = ", value.fluent_fterm_values);
                                  detail::field(os, "auxiliary numeric variable = ", value.auxiliary_fterm_value);
                                  detail::field(os, "goal = ", value.goal);
                                  detail::field(os, "metric = ", value.metric);
                                  detail::field(os, "axioms = ", value.axioms);
                              });
}

std::string to_string(const urd::data<Domain>& value)
{
    return detail::structured("Domain",
                              [&](auto& os)
                              {
                                  detail::field(os, "index = ", value.index);
                                  detail::field(os, "name = ", value.name);
                                  detail::field(os, "static predicates = ", value.static_predicates);
                                  detail::field(os, "fluent predicates = ", value.fluent_predicates);
                                  detail::field(os, "derived predicates = ", value.derived_predicates);
                                  detail::field(os, "static functions = ", value.static_functions);
                                  detail::field(os, "fluent functions = ", value.fluent_functions);
                                  detail::field(os, "auxiliary function = ", value.auxiliary_function);
                                  detail::field(os, "constants = ", value.constants);
                                  detail::field(os, "actions = ", value.actions);
                                  detail::field(os, "axioms = ", value.axioms);
                              });
}

std::string to_string(const urd::data<Task<GroundTag>>& value)
{
    return detail::structured("GroundTask",
                              [&](auto& os)
                              {
                                  detail::field(os, "index = ", value.index);
                                  detail::field(os, "name = ", value.name);
                                  detail::field(os, "derived predicates = ", value.derived_predicates);
                                  detail::field(os, "objects = ", value.objects);
                                  detail::field(os, "static atoms = ", value.static_atoms);
                                  detail::field(os, "fluent atoms = ", value.fluent_atoms);
                                  detail::field(os, "derived atoms = ", value.derived_atoms);
                                  detail::field(os, "static numeric variables = ", value.static_fterm_values);
                                  detail::field(os, "fluent numeric variables = ", value.fluent_fterm_values);
                                  detail::field(os, "auxiliary numeric variable = ", value.auxiliary_fterm_value);
                                  detail::field(os, "goal = ", value.goal);
                                  detail::field(os, "metric = ", value.metric);
                                  detail::field(os, "axioms = ", value.axioms);
                                  detail::field(os, "fluent variables = ", value.fluent_variables);
                                  detail::field(os, "fluent facts = ", value.fluent_facts);
                                  detail::field(os, "goal = ", value.goal);
                                  detail::field(os, "ground actions = ", value.ground_actions);
                                  detail::field(os, "ground axioms = ", value.ground_axioms);
                              });
}

std::string to_string(const urd::data<ygg::Row<Action<LiftedTag>>>& value) { return detail::relation_binding(value); }
std::string to_string(const urd::data<ygg::Row<Axiom<LiftedTag>>>& value) { return detail::relation_binding(value); }

static std::string as_text(VariableView value) { return detail::named(value); }
static std::string as_text(ObjectView value) { return detail::named(value); }
std::string to_string(TermView value) { return fmt::format("{}", value); }
static std::string as_text(PredicateView<StaticTag> value) { return detail::symbol(value); }
static std::string as_text(PredicateView<FluentTag> value) { return detail::symbol(value); }
static std::string as_text(PredicateView<DerivedTag> value) { return detail::symbol(value); }
static std::string as_text(FunctionView<StaticTag> value) { return detail::symbol(value); }
static std::string as_text(FunctionView<FluentTag> value) { return detail::symbol(value); }
static std::string as_text(FunctionView<AuxiliaryTag> value) { return detail::symbol(value); }
static std::string as_text(PredicateRowView<StaticTag> value) { return detail::relation_binding(value); }
static std::string as_text(PredicateRowView<FluentTag> value) { return detail::relation_binding(value); }
static std::string as_text(PredicateRowView<DerivedTag> value) { return detail::relation_binding(value); }
static std::string as_text(FunctionRowView<StaticTag> value) { return detail::relation_binding(value); }
static std::string as_text(FunctionRowView<FluentTag> value) { return detail::relation_binding(value); }
static std::string as_text(FunctionRowView<AuxiliaryTag> value) { return detail::relation_binding(value); }

static std::string as_text(ActionRowView value) { return detail::relation_binding(value); }
std::string to_string(ActionRowDataView value) { return detail::relation_binding(value); }

static std::string as_text(AxiomRowView value)
{ return fmt::format("{}", fmt::join(ygg::to_strings(urd::resolve(value.get<urd::index<Object>>(), value.context())), " ")); }

static std::string as_text(UnaryOperatorView<LiftedTag, ArithmeticOperatorKind> value) { return node_format::format(value); }
static std::string as_text(UnaryOperatorView<GroundTag, ArithmeticOperatorKind> value) { return node_format::format(value); }
static std::string as_text(BinaryOperatorView<LiftedTag, ArithmeticOperatorKind> value) { return node_format::format(value); }
static std::string as_text(BinaryOperatorView<LiftedTag, BooleanOperatorKind> value) { return node_format::format(value); }
static std::string as_text(BinaryOperatorView<GroundTag, ArithmeticOperatorKind> value) { return node_format::format(value); }
static std::string as_text(BinaryOperatorView<GroundTag, BooleanOperatorKind> value) { return node_format::format(value); }
static std::string as_text(MultiOperatorView<LiftedTag, ArithmeticOperatorKind> value) { return node_format::format(value); }
static std::string as_text(MultiOperatorView<GroundTag, ArithmeticOperatorKind> value) { return node_format::format(value); }

static std::string as_text(AtomView<LiftedTag, StaticTag> value) { return node_format::format(value); }
static std::string as_text(AtomView<LiftedTag, FluentTag> value) { return node_format::format(value); }
static std::string as_text(AtomView<LiftedTag, DerivedTag> value) { return node_format::format(value); }
static std::string as_text(LiteralView<LiftedTag, StaticTag> value) { return node_format::format(value); }
static std::string as_text(LiteralView<LiftedTag, FluentTag> value) { return node_format::format(value); }
static std::string as_text(LiteralView<LiftedTag, DerivedTag> value) { return node_format::format(value); }
static std::string as_text(AtomView<GroundTag, StaticTag> value) { return node_format::format(value); }
static std::string as_text(AtomView<GroundTag, FluentTag> value) { return node_format::format(value); }
static std::string as_text(AtomView<GroundTag, DerivedTag> value) { return node_format::format(value); }
static std::string as_text(LiteralView<GroundTag, StaticTag> value) { return node_format::format(value); }
static std::string as_text(LiteralView<GroundTag, FluentTag> value) { return node_format::format(value); }
static std::string as_text(LiteralView<GroundTag, DerivedTag> value) { return node_format::format(value); }
static std::string as_text(FunctionTermView<LiftedTag, StaticTag> value) { return node_format::format(value); }
static std::string as_text(FunctionTermView<LiftedTag, FluentTag> value) { return node_format::format(value); }
static std::string as_text(FunctionTermView<LiftedTag, AuxiliaryTag> value) { return node_format::format(value); }
static std::string as_text(FunctionTermView<GroundTag, StaticTag> value) { return node_format::format(value); }
static std::string as_text(FunctionTermView<GroundTag, FluentTag> value) { return node_format::format(value); }
static std::string as_text(FunctionTermView<GroundTag, AuxiliaryTag> value) { return node_format::format(value); }
static std::string as_text(FunctionTermValueView<GroundTag, StaticTag> value) { return node_format::format(value); }
static std::string as_text(FunctionTermValueView<GroundTag, FluentTag> value) { return node_format::format(value); }
static std::string as_text(FunctionTermValueView<GroundTag, AuxiliaryTag> value) { return node_format::format(value); }
static std::string as_text(NumericEffectView<LiftedTag, FluentTag> value) { return node_format::format(value); }
static std::string as_text(NumericEffectView<LiftedTag, AuxiliaryTag> value) { return node_format::format(value); }
static std::string as_text(NumericEffectView<GroundTag, FluentTag> value) { return node_format::format(value); }
static std::string as_text(NumericEffectView<GroundTag, AuxiliaryTag> value) { return node_format::format(value); }
static std::string as_text(FDRVariableView<FluentTag> value) { return detail::fdr_variable(value); }

static std::string as_text(InvariantView value) { return detail::invariant(value); }
std::string to_string(FDRFactView<FluentTag> value) { return detail::fdr_fact_view(value); }
std::string to_string(urd::owning_view<FDRFact<FluentTag>, Repository> value) { return detail::fdr_fact_view(value); }


static std::string as_text(ConjunctiveConditionView<LiftedTag> value)
{
    return detail::structured("ConjunctiveCondition",
                              [&](auto& os)
                              {
                                  detail::field(os, "index = ", value.index());
                                  detail::field(os, "variables = ", value.get_variables());
                                  detail::field(os, "static literals = ", value.template get_literals<StaticTag>());
                                  detail::field(os, "fluent literals = ", value.template get_literals<FluentTag>());
                                  detail::field(os, "derived literals = ", value.template get_literals<DerivedTag>());
                                  detail::field(os, "numeric constraints = ", value.get_numeric_constraints());
                              });
}

static std::string as_text(ConjunctiveConditionView<GroundTag> value)
{
    return detail::structured("GroundConjunctiveCondition",
                              [&](auto& os)
                              {
                                  detail::field(os, "index = ", value.index());
                                  detail::field(os, "static literals = ", value.template get_literals<StaticTag>());
                                  detail::field(os, "derived literals = ", value.template get_literals<DerivedTag>());
                                  detail::field(os, "positive facts = ", value.template get_facts<PositiveTag>());
                                  detail::field(os, "negative facts = ", value.template get_facts<NegativeTag>());
                                  detail::field(os, "numeric constraints = ", value.get_numeric_constraints());
                              });
}

static std::string as_text(ConditionalEffectView<LiftedTag> value)
{
    return detail::structured("ConditionalEffect",
                              [&](auto& os)
                              {
                                  detail::field(os, "index = ", value.index());
                                  detail::field(os, "variables = ", value.get_variables());
                                  detail::field(os, "condition = ", value.get_condition());
                                  detail::field(os, "effect = ", value.get_effect());
                              });
}

static std::string as_text(ConditionalEffectView<GroundTag> value)
{
    return detail::structured("GroundConditionalEffect",
                              [&](auto& os)
                              {
                                  detail::field(os, "index = ", value.index());
                                  detail::field(os, "condition = ", value.get_condition());
                                  detail::field(os, "effect = ", value.get_effect());
                              });
}

static std::string as_text(ConjunctiveEffectView<LiftedTag> value)
{
    return detail::structured("ConjunctiveEffect",
                              [&](auto& os)
                              {
                                  detail::field(os, "index = ", value.index());
                                  detail::field(os, "fluent literals = ", value.get_literals());
                                  detail::field(os, "fluent numeric effects = ", value.get_numeric_effects());
                                  detail::field(os, "auxiliary numeric effect = ", value.get_auxiliary_numeric_effect());
                              });
}

static std::string as_text(ConjunctiveEffectView<GroundTag> value)
{
    return detail::structured("GroundConjunctiveEffect",
                              [&](auto& os)
                              {
                                  detail::field(os, "index = ", value.index());
                                  detail::field(os, "add facts = ", value.template get_facts<PositiveTag>());
                                  detail::field(os, "del facts = ", value.template get_facts<NegativeTag>());
                                  detail::field(os, "fluent numeric effects = ", value.get_numeric_effects());
                                  detail::field(os, "auxiliary numeric effect = ", value.get_auxiliary_numeric_effect());
                              });
}

static std::string as_text(ActionView<LiftedTag> value)
{
    return detail::structured("Action",
                              [&](auto& os)
                              {
                                  detail::field(os, "index = ", value.index());
                                  detail::field(os, "name = ", value.get_name());
                                  detail::field(os, "original name = ", value.get_original_name());
                                  detail::field(os, "variables = ", value.get_variables());
                                  detail::field(os, "condition = ", value.get_condition());
                                  detail::field(os, "effects = ", value.get_effects());
                              });
}

static std::string as_text(ActionView<GroundTag> value)
{
    return detail::structured("GroundAction",
                              [&](auto& os)
                              {
                                  detail::field(os, "index = ", value.index());
                                  detail::field(os, "binding = ", value.get_row());
                                  detail::field(os, "action index = ", value.get_action().index());
                                  detail::field(os, "condition = ", value.get_condition());
                                  detail::field(os, "effects = ", value.get_effects());
                              });
}

static std::string as_text(AxiomView<LiftedTag> value)
{
    return detail::structured("Axiom",
                              [&](auto& os)
                              {
                                  detail::field(os, "index = ", value.index());
                                  detail::field(os, "variables = ", value.get_variables());
                                  detail::field(os, "body = ", value.get_body());
                                  detail::field(os, "head = ", value.get_head());
                              });
}

static std::string as_text(AxiomView<GroundTag> value)
{
    return detail::structured("GroundAxiom",
                              [&](auto& os)
                              {
                                  detail::field(os, "index = ", value.index());
                                  detail::field(os, "binding = ", value.get_row());
                                  detail::field(os, "axiom index = ", value.get_axiom().index());
                                  detail::field(os, "body = ", value.get_body());
                                  detail::field(os, "head = ", value.get_head());
                              });
}

static std::string as_text(MetricView value) { return fmt::format("({} {})", value.get_optimization_direction(), value.get_fexpr()); }

static std::string as_text(TaskView<LiftedTag> value)
{
    return detail::structured("LiftedTask",
                              [&](auto& os)
                              {
                                  detail::field(os, "index = ", value.index());
                                  detail::field(os, "name = ", value.get_name());
                                  detail::field(os, "derived predicates = ", value.get_derived_predicates());
                                  detail::field(os, "objects = ", value.get_objects());
                                  detail::field(os, "static atoms = ", value.template get_atoms<StaticTag>());
                                  detail::field(os, "fluent atoms = ", value.template get_atoms<FluentTag>());
                                  detail::field(os, "static numeric variables = ", value.template get_fterm_values<StaticTag>());
                                  detail::field(os, "fluent numeric variables = ", value.template get_fterm_values<FluentTag>());
                                  detail::field(os, "auxiliary numeric variable = ", value.get_auxiliary_fterm_value());
                                  detail::field(os, "goal = ", value.get_goal());
                                  detail::field(os, "metric = ", value.get_metric());
                                  detail::field(os, "axioms = ", value.get_axioms());
                              });
}

static std::string as_text(DomainView value)
{
    return detail::structured("Domain",
                              [&](auto& os)
                              {
                                  detail::field(os, "index = ", value.index());
                                  detail::field(os, "name = ", value.get_name());
                                  detail::field(os, "static predicates = ", value.template get_predicates<StaticTag>());
                                  detail::field(os, "fluent predicates = ", value.template get_predicates<FluentTag>());
                                  detail::field(os, "derived predicates = ", value.template get_predicates<DerivedTag>());
                                  detail::field(os, "static functions = ", value.template get_functions<StaticTag>());
                                  detail::field(os, "fluent functions = ", value.template get_functions<FluentTag>());
                                  detail::field(os, "auxiliary function = ", value.get_auxiliary_function());
                                  detail::field(os, "constants = ", value.get_constants());
                                  detail::field(os, "actions = ", value.get_actions());
                                  detail::field(os, "axioms = ", value.get_axioms());
                              });
}

static std::string as_text(TaskView<GroundTag> value)
{
    return detail::structured("GroundTask",
                              [&](auto& os)
                              {
                                  detail::field(os, "index = ", value.index());
                                  detail::field(os, "name = ", value.get_name());
                                  detail::field(os, "derived predicates = ", value.get_derived_predicates());
                                  detail::field(os, "objects = ", value.get_objects());
                                  detail::field(os, "static atoms = ", value.template get_atoms<StaticTag>());
                                  detail::field(os, "fluent atoms = ", value.template get_atoms<FluentTag>());
                                  detail::field(os, "derived atoms = ", value.template get_atoms<DerivedTag>());
                                  detail::field(os, "static numeric variables = ", value.template get_fterm_values<StaticTag>());
                                  detail::field(os, "fluent numeric variables = ", value.template get_fterm_values<FluentTag>());
                                  detail::field(os, "auxiliary numeric variable = ", value.get_auxiliary_fterm_value());
                                  detail::field(os, "goal = ", value.get_goal());
                                  detail::field(os, "metric = ", value.get_metric());
                                  detail::field(os, "axioms = ", value.get_axioms());
                                  detail::field(os, "fluent variables = ", value.get_fluent_variables());
                                  detail::field(os, "fluent facts = ", value.get_fluent_facts());
                                  detail::field(os, "goal = ", value.get_goal());
                                  detail::field(os, "ground actions = ", value.get_ground_actions());
                                  detail::field(os, "ground axioms = ", value.get_ground_axioms());
                              });
}

std::string to_string(const PlanningDomain& value) { return fmt::format("{}", value.get_domain()); }
std::string to_string(const PlanningTask<LiftedTag>& value) { return fmt::format("{}", value.get_task()); }
std::string to_string(const PlanningTask<GroundTag>& value) { return fmt::format("{}", value.get_task()); }

std::string to_string(const std::pair<ActionRowView, PlanFormatting>& value)
{
    auto result = fmt::format("({}", value.first.get_relation().get_original_name());
    const auto objects = urd::resolve(value.first.get<urd::index<Object>>(), value.first.context());
    for (std::size_t i = 0; i < value.first.get_relation().get_original_arity(); ++i)
        fmt::format_to(std::back_inserter(result), " {}", objects[i]);
    result.push_back(')');
    return result;
}

std::string to_string(const std::pair<ActionView<GroundTag>, PlanFormatting>& value)
{ return fmt::format("{}", std::make_pair(value.first.get_row(), value.second)); }


template<typename T>
    requires boost::mp11::mp_contains<SymbolRepositoryTypes, T>::value
std::string to_string(urd::index_view<T, Repository> value)
{ return as_text(value); }

template<typename R>
    requires boost::mp11::mp_contains<RelationRepositoryTypes, R>::value
std::string to_string(ygg::RowView<R, Repository> value)
{ return as_text(value); }

TYR_VIEW_TO_STRINGS(57, 8)

}  // namespace tyr::formalism::planning
