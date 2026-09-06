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

#include "tyr/formalism/planning/planning_domain.hpp"
#include "tyr/formalism/planning/planning_fdr_task.hpp"
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

template<typename F>
std::string structured(std::string_view name, F&& write_fields)
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

template<typename T>
std::string unary_operator(const ygg::Data<UnaryOperator<T>>& value)
{
    return fmt::format("({} {})", value.operator_kind, value.arg);
}

template<typename T>
std::string unary_operator(UnaryOperatorView<T> value)
{
    return fmt::format("({} {})", value.get_operator(), value.get_arg());
}

template<BinaryOperatorKind Operator, typename T>
std::string binary_operator(const ygg::Data<BinaryOperator<Operator, T>>& value)
{
    return fmt::format("({} {} {})", value.operator_kind, value.lhs, value.rhs);
}

template<BinaryOperatorKind Operator, typename T>
std::string binary_operator(BinaryOperatorView<Operator, T> value)
{
    return fmt::format("({} {} {})", value.get_operator(), value.get_lhs(), value.get_rhs());
}

template<typename T>
std::string multi_operator(const ygg::Data<MultiOperator<T>>& value)
{
    return fmt::format("({} {})", value.operator_kind, fmt::join(ygg::to_strings(value.args), " "));
}

template<typename T>
std::string multi_operator(MultiOperatorView<T> value)
{
    return fmt::format("({} {})", value.get_operator(), fmt::join(ygg::to_strings(value.get_args()), " "));
}

template<typename T>
std::string arithmetic_operator(const ygg::Data<ArithmeticOperator<T>>& value)
{
    return fmt::format("{}", value.value);
}

template<typename T>
std::string arithmetic_operator(ArithmeticOperatorView<T> value)
{
    return fmt::format("{}", value.get_variant());
}

template<typename T>
std::string boolean_operator(const ygg::Data<BooleanOperator<T>>& value)
{
    return fmt::format("{}", value.value);
}

template<typename T>
std::string boolean_operator(BooleanOperatorView<T> value)
{
    return fmt::format("{}", value.get_variant());
}

template<FactKind T>
std::string atom(const ygg::Data<Atom<::tyr::LiftedTag, T>>& value)
{
    return fmt::format("({} {})", value.predicate, fmt::join(ygg::to_strings(value.terms), " "));
}

template<FactKind T>
std::string atom(AtomView<::tyr::LiftedTag, T> value)
{
    return fmt::format("({} {})", value.get_predicate().get_name(), fmt::join(ygg::to_strings(value.get_terms()), " "));
}

template<::tyr::TaskKind T, FactKind F>
std::string literal(const ygg::Data<Literal<T, F>>& value)
{
    return value.polarity ? fmt::format("{}", value.atom) : fmt::format("(not {})", value.atom);
}

template<::tyr::TaskKind T, FactKind F>
std::string literal(LiteralView<T, F> value)
{
    return value.get_polarity() ? fmt::format("{}", value.get_atom()) : fmt::format("(not {})", value.get_atom());
}

template<FactKind T>
std::string ground_atom(const ygg::Data<Atom<::tyr::GroundTag, T>>& value)
{
    return fmt::format("({})", value.binding);
}

template<FactKind T>
std::string ground_atom(AtomView<::tyr::GroundTag, T> value)
{
    return fmt::format("{}", value.get_row());
}

template<FactKind T>
std::string function_term(const ygg::Data<FunctionTerm<::tyr::LiftedTag, T>>& value)
{
    return fmt::format("({} {})", value.function, fmt::join(ygg::to_strings(value.terms), " "));
}

template<FactKind T>
std::string function_term(FunctionTermView<::tyr::LiftedTag, T> value)
{
    return fmt::format("({} {})", value.get_function().get_name(), fmt::join(ygg::to_strings(value.get_terms()), " "));
}

template<FactKind T>
std::string ground_function_term(const ygg::Data<FunctionTerm<::tyr::GroundTag, T>>& value)
{
    return fmt::format("({})", value.binding);
}

template<FactKind T>
std::string ground_function_term(FunctionTermView<::tyr::GroundTag, T> value)
{
    return fmt::format("{}", value.get_row());
}

template<FactKind T>
std::string ground_function_term_value(const ygg::Data<FunctionTermValue<::tyr::GroundTag, T>>& value)
{
    return fmt::format("(= {} {})", value.fterm, value.value);
}

template<FactKind T>
std::string ground_function_term_value(FunctionTermValueView<::tyr::GroundTag, T> value)
{
    return fmt::format("(= {} {})", value.get_fterm(), value.get_value());
}

template<::tyr::TaskKind T, FactKind F>
std::string numeric_effect(const ygg::Data<NumericEffect<T, F>>& value)
{
    return fmt::format("({} {} {})", value.operator_kind, value.fterm, value.fexpr);
}

template<::tyr::TaskKind T, FactKind F>
std::string numeric_effect(NumericEffectView<T, F> value)
{
    return fmt::format("({} {} {})", value.get_operator(), value.get_fterm(), value.get_fexpr());
}

template<::tyr::TaskKind T, FactKind F>
std::string numeric_effect_operator(const ygg::Data<NumericEffectOperator<T, F>>& value)
{
    return fmt::format("{}", value.value);
}

template<::tyr::TaskKind T, FactKind F>
std::string numeric_effect_operator(NumericEffectOperatorView<T, F> value)
{
    return fmt::format("{}", value.get_variant());
}

template<FactKind T>
std::string fdr_variable(const ygg::Data<FDRVariable<T>>& value)
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
                          field(os, "index = ", value.get_index());
                          field(os, "atoms = ", value.get_atoms());
                      });
}

template<FactKind T>
std::string fdr_fact(const ygg::Data<FDRFact<T>>& value)
{
    return fmt::format("<{},{}>", value.variable, value.value);
}

template<FactKind T>
std::string fdr_fact(FDRFactView<T> value)
{
    const auto atom = value.get_atom();
    if (!atom.has_value())
    {
        return fmt::format("<{},{}>: (none-of {})",
                           value.get_variable().get_index(),
                           value.get_value(),
                           fmt::join(ygg::to_strings(value.get_variable().get_atoms()), " "));
    }
    return fmt::format("<{},{}>: {}", value.get_variable().get_index(), value.get_value(), atom.value());
}

template<typename View>
std::string named(View value)
{
    return fmt::format("{}", value.get_name());
}

template<typename View>
std::string symbol(View value)
{
    return fmt::format("{}/{}", value.get_name(), value.get_arity());
}

template<typename View>
std::string relation_binding(View value)
{
    auto result = fmt::format("({}", value.get_relation().get_name());
    for (const auto object : value.get_objects())
        fmt::format_to(std::back_inserter(result), " {}", object);
    result.push_back(')');
    return result;
}

template<typename Tag>
std::string relation_binding(const ygg::Data<RelationBinding<Tag>>& value)
{
    return fmt::format("{} {}", value.relation, fmt::join(ygg::to_strings(value.objects), " "));
}

}  // namespace detail

std::string to_string(const ygg::Data<LiftedUnaryOperatorType>& value) { return detail::unary_operator(value); }
std::string to_string(const ygg::Data<GroundUnaryOperatorType>& value) { return detail::unary_operator(value); }
std::string to_string(const ygg::Data<LiftedBinaryOperatorType<ArithmeticOperatorKind>>& value) { return detail::binary_operator(value); }
std::string to_string(const ygg::Data<LiftedBinaryOperatorType<BooleanOperatorKind>>& value) { return detail::binary_operator(value); }
std::string to_string(const ygg::Data<GroundBinaryOperatorType<ArithmeticOperatorKind>>& value) { return detail::binary_operator(value); }
std::string to_string(const ygg::Data<GroundBinaryOperatorType<BooleanOperatorKind>>& value) { return detail::binary_operator(value); }
std::string to_string(const ygg::Data<LiftedMultiOperatorType>& value) { return detail::multi_operator(value); }
std::string to_string(const ygg::Data<GroundMultiOperatorType>& value) { return detail::multi_operator(value); }
std::string to_string(const ygg::Data<ArithmeticOperator<ygg::Data<FunctionExpression<::tyr::LiftedTag>>>>& value)
{
    return detail::arithmetic_operator(value);
}
std::string to_string(const ygg::Data<ArithmeticOperator<ygg::Data<FunctionExpression<::tyr::GroundTag>>>>& value)
{
    return detail::arithmetic_operator(value);
}
std::string to_string(const ygg::Data<BooleanOperator<ygg::Data<FunctionExpression<::tyr::LiftedTag>>>>& value) { return detail::boolean_operator(value); }
std::string to_string(const ygg::Data<BooleanOperator<ygg::Data<FunctionExpression<::tyr::GroundTag>>>>& value) { return detail::boolean_operator(value); }

std::string to_string(const ygg::Data<Atom<::tyr::LiftedTag, StaticTag>>& value) { return detail::atom(value); }
std::string to_string(const ygg::Data<Atom<::tyr::LiftedTag, FluentTag>>& value) { return detail::atom(value); }
std::string to_string(const ygg::Data<Atom<::tyr::LiftedTag, DerivedTag>>& value) { return detail::atom(value); }
std::string to_string(const ygg::Data<Literal<::tyr::LiftedTag, StaticTag>>& value) { return detail::literal(value); }
std::string to_string(const ygg::Data<Literal<::tyr::LiftedTag, FluentTag>>& value) { return detail::literal(value); }
std::string to_string(const ygg::Data<Literal<::tyr::LiftedTag, DerivedTag>>& value) { return detail::literal(value); }
std::string to_string(const ygg::Data<Atom<::tyr::GroundTag, StaticTag>>& value) { return detail::ground_atom(value); }
std::string to_string(const ygg::Data<Atom<::tyr::GroundTag, FluentTag>>& value) { return detail::ground_atom(value); }
std::string to_string(const ygg::Data<Atom<::tyr::GroundTag, DerivedTag>>& value) { return detail::ground_atom(value); }
std::string to_string(const ygg::Data<Literal<::tyr::GroundTag, StaticTag>>& value) { return detail::literal(value); }
std::string to_string(const ygg::Data<Literal<::tyr::GroundTag, FluentTag>>& value) { return detail::literal(value); }
std::string to_string(const ygg::Data<Literal<::tyr::GroundTag, DerivedTag>>& value) { return detail::literal(value); }
std::string to_string(const ygg::Data<FunctionTerm<::tyr::LiftedTag, StaticTag>>& value) { return detail::function_term(value); }
std::string to_string(const ygg::Data<FunctionTerm<::tyr::LiftedTag, FluentTag>>& value) { return detail::function_term(value); }
std::string to_string(const ygg::Data<FunctionTerm<::tyr::LiftedTag, AuxiliaryTag>>& value) { return detail::function_term(value); }
std::string to_string(const ygg::Data<FunctionTerm<::tyr::GroundTag, StaticTag>>& value) { return detail::ground_function_term(value); }
std::string to_string(const ygg::Data<FunctionTerm<::tyr::GroundTag, FluentTag>>& value) { return detail::ground_function_term(value); }
std::string to_string(const ygg::Data<FunctionTerm<::tyr::GroundTag, AuxiliaryTag>>& value) { return detail::ground_function_term(value); }
std::string to_string(const ygg::Data<FunctionTermValue<::tyr::GroundTag, StaticTag>>& value) { return detail::ground_function_term_value(value); }
std::string to_string(const ygg::Data<FunctionTermValue<::tyr::GroundTag, FluentTag>>& value) { return detail::ground_function_term_value(value); }
std::string to_string(const ygg::Data<FunctionTermValue<::tyr::GroundTag, AuxiliaryTag>>& value) { return detail::ground_function_term_value(value); }
std::string to_string(const ygg::Data<NumericEffect<::tyr::LiftedTag, FluentTag>>& value) { return detail::numeric_effect(value); }
std::string to_string(const ygg::Data<NumericEffect<::tyr::LiftedTag, AuxiliaryTag>>& value) { return detail::numeric_effect(value); }
std::string to_string(const ygg::Data<NumericEffect<::tyr::GroundTag, FluentTag>>& value) { return detail::numeric_effect(value); }
std::string to_string(const ygg::Data<NumericEffect<::tyr::GroundTag, AuxiliaryTag>>& value) { return detail::numeric_effect(value); }
std::string to_string(const ygg::Data<NumericEffectOperator<::tyr::LiftedTag, FluentTag>>& value) { return detail::numeric_effect_operator(value); }
std::string to_string(const ygg::Data<NumericEffectOperator<::tyr::LiftedTag, AuxiliaryTag>>& value) { return detail::numeric_effect_operator(value); }
std::string to_string(const ygg::Data<NumericEffectOperator<::tyr::GroundTag, FluentTag>>& value) { return detail::numeric_effect_operator(value); }
std::string to_string(const ygg::Data<NumericEffectOperator<::tyr::GroundTag, AuxiliaryTag>>& value) { return detail::numeric_effect_operator(value); }
std::string to_string(const ygg::Data<FDRVariable<FluentTag>>& value) { return detail::fdr_variable(value); }
std::string to_string(const ygg::Data<FDRFact<FluentTag>>& value) { return detail::fdr_fact(value); }

std::string to_string(const ygg::Data<FunctionExpression<::tyr::LiftedTag>>& value) { return fmt::format("{}", value.value); }
std::string to_string(const ygg::Data<FunctionExpression<::tyr::GroundTag>>& value) { return fmt::format("{}", value.value); }

std::string to_string(const ygg::Data<ConjunctiveCondition<::tyr::LiftedTag>>& value)
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

std::string to_string(const ygg::Data<ConjunctiveCondition<::tyr::GroundTag>>& value)
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

std::string to_string(const ygg::Data<ConditionalEffect<::tyr::LiftedTag>>& value)
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

std::string to_string(const ygg::Data<ConditionalEffect<::tyr::GroundTag>>& value)
{
    return detail::structured("GroundConditionalEffect",
                              [&](auto& os)
                              {
                                  detail::field(os, "index = ", value.index);
                                  detail::field(os, "condition = ", value.condition);
                                  detail::field(os, "effect = ", value.effect);
                              });
}

std::string to_string(const ygg::Data<ConjunctiveEffect<::tyr::LiftedTag>>& value)
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

std::string to_string(const ygg::Data<ConjunctiveEffect<::tyr::GroundTag>>& value)
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

std::string to_string(const ygg::Data<Action<::tyr::LiftedTag>>& value)
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

std::string to_string(const ygg::Data<Action<::tyr::GroundTag>>& value)
{
    return detail::structured("GroundAction",
                              [&](auto& os)
                              {
                                  detail::field(os, "index = ", value.index);
                                  detail::field(os, "binding = ", value.binding);
                                  detail::field(os, "condition = ", value.condition);
                                  detail::field(os, "effects = ", value.effects);
                              });
}

std::string to_string(const ygg::Data<Axiom<::tyr::LiftedTag>>& value)
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

std::string to_string(const ygg::Data<Axiom<::tyr::GroundTag>>& value)
{
    return detail::structured("GroundAxiom",
                              [&](auto& os)
                              {
                                  detail::field(os, "index = ", value.index);
                                  detail::field(os, "binding = ", value.binding);
                                  detail::field(os, "body = ", value.body);
                                  detail::field(os, "head = ", value.head);
                              });
}

std::string to_string(const ygg::Data<Metric>& value) { return fmt::format("({} {})", value.optimization_direction, value.fexpr); }

std::string to_string(const ygg::Data<Task>& value)
{
    return detail::structured("Task",
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

std::string to_string(const ygg::Data<Domain>& value)
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

std::string to_string(const ygg::Data<FDRTask>& value)
{
    return detail::structured("FDRTask",
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

std::string to_string(const ygg::Data<RelationBinding<Action<::tyr::LiftedTag>>>& value) { return detail::relation_binding(value); }
std::string to_string(const ygg::Data<RelationBinding<Axiom<::tyr::LiftedTag>>>& value) { return detail::relation_binding(value); }

std::string to_string(VariableView value) { return detail::named(value); }
std::string to_string(ObjectView value) { return detail::named(value); }
std::string to_string(TermView value) { return fmt::format("{}", value.get_variant()); }
std::string to_string(PredicateView<StaticTag> value) { return detail::symbol(value); }
std::string to_string(PredicateView<FluentTag> value) { return detail::symbol(value); }
std::string to_string(PredicateView<DerivedTag> value) { return detail::symbol(value); }
std::string to_string(FunctionView<StaticTag> value) { return detail::symbol(value); }
std::string to_string(FunctionView<FluentTag> value) { return detail::symbol(value); }
std::string to_string(FunctionView<AuxiliaryTag> value) { return detail::symbol(value); }
std::string to_string(PredicateBindingView<StaticTag> value) { return detail::relation_binding(value); }
std::string to_string(PredicateBindingView<FluentTag> value) { return detail::relation_binding(value); }
std::string to_string(PredicateBindingView<DerivedTag> value) { return detail::relation_binding(value); }
std::string to_string(FunctionBindingView<StaticTag> value) { return detail::relation_binding(value); }
std::string to_string(FunctionBindingView<FluentTag> value) { return detail::relation_binding(value); }
std::string to_string(FunctionBindingView<AuxiliaryTag> value) { return detail::relation_binding(value); }
std::string to_string(ActionBindingView value) { return detail::relation_binding(value); }

std::string to_string(AxiomBindingView value) { return fmt::format("{}", fmt::join(ygg::to_strings(value.get_objects()), " ")); }

std::string to_string(LiftedUnaryOperatorView value) { return detail::unary_operator(value); }
std::string to_string(GroundUnaryOperatorView value) { return detail::unary_operator(value); }
std::string to_string(LiftedBinaryOperatorView<ArithmeticOperatorKind> value) { return detail::binary_operator(value); }
std::string to_string(LiftedBinaryOperatorView<BooleanOperatorKind> value) { return detail::binary_operator(value); }
std::string to_string(GroundBinaryOperatorView<ArithmeticOperatorKind> value) { return detail::binary_operator(value); }
std::string to_string(GroundBinaryOperatorView<BooleanOperatorKind> value) { return detail::binary_operator(value); }
std::string to_string(LiftedMultiOperatorView value) { return detail::multi_operator(value); }
std::string to_string(GroundMultiOperatorView value) { return detail::multi_operator(value); }
std::string to_string(LiftedArithmeticOperatorView value) { return detail::arithmetic_operator(value); }
std::string to_string(GroundArithmeticOperatorView value) { return detail::arithmetic_operator(value); }
std::string to_string(LiftedBooleanOperatorView value) { return detail::boolean_operator(value); }
std::string to_string(GroundBooleanOperatorView value) { return detail::boolean_operator(value); }

std::string to_string(AtomView<::tyr::LiftedTag, StaticTag> value) { return detail::atom(value); }
std::string to_string(AtomView<::tyr::LiftedTag, FluentTag> value) { return detail::atom(value); }
std::string to_string(AtomView<::tyr::LiftedTag, DerivedTag> value) { return detail::atom(value); }
std::string to_string(LiteralView<::tyr::LiftedTag, StaticTag> value) { return detail::literal(value); }
std::string to_string(LiteralView<::tyr::LiftedTag, FluentTag> value) { return detail::literal(value); }
std::string to_string(LiteralView<::tyr::LiftedTag, DerivedTag> value) { return detail::literal(value); }
std::string to_string(AtomView<::tyr::GroundTag, StaticTag> value) { return detail::ground_atom(value); }
std::string to_string(AtomView<::tyr::GroundTag, FluentTag> value) { return detail::ground_atom(value); }
std::string to_string(AtomView<::tyr::GroundTag, DerivedTag> value) { return detail::ground_atom(value); }
std::string to_string(LiteralView<::tyr::GroundTag, StaticTag> value) { return detail::literal(value); }
std::string to_string(LiteralView<::tyr::GroundTag, FluentTag> value) { return detail::literal(value); }
std::string to_string(LiteralView<::tyr::GroundTag, DerivedTag> value) { return detail::literal(value); }
std::string to_string(FunctionTermView<::tyr::LiftedTag, StaticTag> value) { return detail::function_term(value); }
std::string to_string(FunctionTermView<::tyr::LiftedTag, FluentTag> value) { return detail::function_term(value); }
std::string to_string(FunctionTermView<::tyr::LiftedTag, AuxiliaryTag> value) { return detail::function_term(value); }
std::string to_string(FunctionTermView<::tyr::GroundTag, StaticTag> value) { return detail::ground_function_term(value); }
std::string to_string(FunctionTermView<::tyr::GroundTag, FluentTag> value) { return detail::ground_function_term(value); }
std::string to_string(FunctionTermView<::tyr::GroundTag, AuxiliaryTag> value) { return detail::ground_function_term(value); }
std::string to_string(FunctionTermValueView<::tyr::GroundTag, StaticTag> value) { return detail::ground_function_term_value(value); }
std::string to_string(FunctionTermValueView<::tyr::GroundTag, FluentTag> value) { return detail::ground_function_term_value(value); }
std::string to_string(FunctionTermValueView<::tyr::GroundTag, AuxiliaryTag> value) { return detail::ground_function_term_value(value); }
std::string to_string(NumericEffectView<::tyr::LiftedTag, FluentTag> value) { return detail::numeric_effect(value); }
std::string to_string(NumericEffectView<::tyr::LiftedTag, AuxiliaryTag> value) { return detail::numeric_effect(value); }
std::string to_string(NumericEffectView<::tyr::GroundTag, FluentTag> value) { return detail::numeric_effect(value); }
std::string to_string(NumericEffectView<::tyr::GroundTag, AuxiliaryTag> value) { return detail::numeric_effect(value); }
std::string to_string(NumericEffectOperatorView<::tyr::LiftedTag, FluentTag> value) { return detail::numeric_effect_operator(value); }
std::string to_string(NumericEffectOperatorView<::tyr::LiftedTag, AuxiliaryTag> value) { return detail::numeric_effect_operator(value); }
std::string to_string(NumericEffectOperatorView<::tyr::GroundTag, FluentTag> value) { return detail::numeric_effect_operator(value); }
std::string to_string(NumericEffectOperatorView<::tyr::GroundTag, AuxiliaryTag> value) { return detail::numeric_effect_operator(value); }
std::string to_string(FDRVariableView<FluentTag> value) { return detail::fdr_variable(value); }
std::string to_string(FDRFactView<FluentTag> value) { return detail::fdr_fact(value); }

std::string to_string(FunctionExpressionView<::tyr::LiftedTag> value) { return fmt::format("{}", value.get_variant()); }
std::string to_string(FunctionExpressionView<::tyr::GroundTag> value) { return fmt::format("{}", value.get_variant()); }

std::string to_string(ConjunctiveConditionView<::tyr::LiftedTag> value)
{
    return detail::structured("ConjunctiveCondition",
                              [&](auto& os)
                              {
                                  detail::field(os, "index = ", value.get_index());
                                  detail::field(os, "variables = ", value.get_variables());
                                  detail::field(os, "static literals = ", value.template get_literals<StaticTag>());
                                  detail::field(os, "fluent literals = ", value.template get_literals<FluentTag>());
                                  detail::field(os, "derived literals = ", value.template get_literals<DerivedTag>());
                                  detail::field(os, "numeric constraints = ", value.get_numeric_constraints());
                              });
}

std::string to_string(ConjunctiveConditionView<::tyr::GroundTag> value)
{
    return detail::structured("GroundConjunctiveCondition",
                              [&](auto& os)
                              {
                                  detail::field(os, "index = ", value.get_index());
                                  detail::field(os, "static literals = ", value.template get_literals<StaticTag>());
                                  detail::field(os, "derived literals = ", value.template get_literals<DerivedTag>());
                                  detail::field(os, "positive facts = ", value.template get_facts<PositiveTag>());
                                  detail::field(os, "negative facts = ", value.template get_facts<NegativeTag>());
                                  detail::field(os, "numeric constraints = ", value.get_numeric_constraints());
                              });
}

std::string to_string(ConditionalEffectView<::tyr::LiftedTag> value)
{
    return detail::structured("ConditionalEffect",
                              [&](auto& os)
                              {
                                  detail::field(os, "index = ", value.get_index());
                                  detail::field(os, "variables = ", value.get_variables());
                                  detail::field(os, "condition = ", value.get_condition());
                                  detail::field(os, "effect = ", value.get_effect());
                              });
}

std::string to_string(ConditionalEffectView<::tyr::GroundTag> value)
{
    return detail::structured("GroundConditionalEffect",
                              [&](auto& os)
                              {
                                  detail::field(os, "index = ", value.get_index());
                                  detail::field(os, "condition = ", value.get_condition());
                                  detail::field(os, "effect = ", value.get_effect());
                              });
}

std::string to_string(ConjunctiveEffectView<::tyr::LiftedTag> value)
{
    return detail::structured("ConjunctiveEffect",
                              [&](auto& os)
                              {
                                  detail::field(os, "index = ", value.get_index());
                                  detail::field(os, "fluent literals = ", value.get_literals());
                                  detail::field(os, "fluent numeric effects = ", value.get_numeric_effects());
                                  detail::field(os, "auxiliary numeric effect = ", value.get_auxiliary_numeric_effect());
                              });
}

std::string to_string(ConjunctiveEffectView<::tyr::GroundTag> value)
{
    return detail::structured("GroundConjunctiveEffect",
                              [&](auto& os)
                              {
                                  detail::field(os, "index = ", value.get_index());
                                  detail::field(os, "add facts = ", value.template get_facts<PositiveTag>());
                                  detail::field(os, "del facts = ", value.template get_facts<NegativeTag>());
                                  detail::field(os, "fluent numeric effects = ", value.get_numeric_effects());
                                  detail::field(os, "auxiliary numeric effect = ", value.get_auxiliary_numeric_effect());
                              });
}

std::string to_string(ActionView<::tyr::LiftedTag> value)
{
    return detail::structured("Action",
                              [&](auto& os)
                              {
                                  detail::field(os, "index = ", value.get_index());
                                  detail::field(os, "name = ", value.get_name());
                                  detail::field(os, "original name = ", value.get_original_name());
                                  detail::field(os, "variables = ", value.get_variables());
                                  detail::field(os, "condition = ", value.get_condition());
                                  detail::field(os, "effects = ", value.get_effects());
                              });
}

std::string to_string(ActionView<::tyr::GroundTag> value)
{
    return detail::structured("GroundAction",
                              [&](auto& os)
                              {
                                  detail::field(os, "index = ", value.get_index());
                                  detail::field(os, "binding = ", value.get_row());
                                  detail::field(os, "action index = ", value.get_action().get_index());
                                  detail::field(os, "condition = ", value.get_condition());
                                  detail::field(os, "effects = ", value.get_effects());
                              });
}

std::string to_string(AxiomView<::tyr::LiftedTag> value)
{
    return detail::structured("Axiom",
                              [&](auto& os)
                              {
                                  detail::field(os, "index = ", value.get_index());
                                  detail::field(os, "variables = ", value.get_variables());
                                  detail::field(os, "body = ", value.get_body());
                                  detail::field(os, "head = ", value.get_head());
                              });
}

std::string to_string(AxiomView<::tyr::GroundTag> value)
{
    return detail::structured("GroundAxiom",
                              [&](auto& os)
                              {
                                  detail::field(os, "index = ", value.get_index());
                                  detail::field(os, "binding = ", value.get_row());
                                  detail::field(os, "axiom index = ", value.get_axiom().get_index());
                                  detail::field(os, "body = ", value.get_body());
                                  detail::field(os, "head = ", value.get_head());
                              });
}

std::string to_string(MetricView value) { return fmt::format("({} {})", value.get_optimization_direction(), value.get_fexpr()); }

std::string to_string(TaskView value)
{
    return detail::structured("Task",
                              [&](auto& os)
                              {
                                  detail::field(os, "index = ", value.get_index());
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

std::string to_string(DomainView value)
{
    return detail::structured("Domain",
                              [&](auto& os)
                              {
                                  detail::field(os, "index = ", value.get_index());
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

std::string to_string(FDRTaskView value)
{
    return detail::structured("FDRTask",
                              [&](auto& os)
                              {
                                  detail::field(os, "index = ", value.get_index());
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
std::string to_string(const PlanningTask& value) { return fmt::format("{}", value.get_task()); }
std::string to_string(const PlanningFDRTask& value) { return fmt::format("{}", value.get_task()); }

std::string to_string(const std::pair<ActionBindingView, PlanFormatting>& value)
{
    auto result = fmt::format("({}", value.first.get_relation().get_original_name());
    for (std::size_t i = 0; i < value.first.get_relation().get_original_arity(); ++i)
        fmt::format_to(std::back_inserter(result), " {}", value.first.get_objects()[i]);
    result.push_back(')');
    return result;
}

std::string to_string(const std::pair<ActionView<::tyr::GroundTag>, PlanFormatting>& value)
{
    return fmt::format("{}", std::make_pair(value.first.get_row(), value.second));
}

}  // namespace tyr::formalism::planning
