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

#include "tyr/formalism/datalog/formatter.hpp"

#include "tyr/formalism/datalog/datas.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/datalog/variable_dependency_graph.hpp"
#include "tyr/formalism/datalog/views.hpp"

#include <fmt/ostream.h>
#include <fmt/ranges.h>
#include <iterator>
#include <sstream>
#include <yggdrasil/formatting/cista_formatters.hpp>
#include <yggdrasil/io/iostream.hpp>

namespace tyr::formalism::datalog
{

namespace
{

template<TaskKind T>
std::string format_data(const ygg::Data<UnaryOperator<T>>& value)
{
    return fmt::format("({} {})", value.operator_kind, value.arg);
}

template<TaskKind T, BinaryOperatorKind O>
std::string format_data(const ygg::Data<BinaryOperator<T, O>>& value)
{
    return fmt::format("({} {} {})", value.operator_kind, value.lhs, value.rhs);
}

template<TaskKind T>
std::string format_data(const ygg::Data<MultiOperator<T>>& value)
{
    return fmt::format("({} {})", value.operator_kind, fmt::join(ygg::to_strings(value.args), " "));
}

template<TaskKind T>
std::string format_data(const ygg::Data<ArithmeticOperator<T>>& value)
{
    return fmt::format("{}", value.value);
}

template<TaskKind T>
std::string format_data(const ygg::Data<BooleanOperator<T>>& value)
{
    return fmt::format("{}", value.value);
}

template<TaskKind T>
std::string format_view(UnaryOperatorView<T> value)
{
    return fmt::format("({} {})", value.get_operator(), value.get_arg());
}

template<TaskKind T, BinaryOperatorKind O>
std::string format_view(BinaryOperatorView<T, O> value)
{
    return fmt::format("({} {} {})", value.get_operator(), value.get_lhs(), value.get_rhs());
}

template<TaskKind T>
std::string format_view(MultiOperatorView<T> value)
{
    return fmt::format("({} {})", value.get_operator(), fmt::join(ygg::to_strings(value.get_args()), " "));
}

template<TaskKind T>
std::string format_view(ArithmeticOperatorView<T> value)
{
    return fmt::format("{}", value.get_variant());
}

template<TaskKind T>
std::string format_view(BooleanOperatorView<T> value)
{
    return fmt::format("{}", value.get_variant());
}

template<FactKind F>
std::string format_data(const ygg::Data<Atom<LiftedTag, F>>& value)
{
    return fmt::format("({} {})", value.predicate, fmt::join(ygg::to_strings(value.terms), " "));
}

template<FactKind F>
std::string format_view(AtomView<LiftedTag, F> value)
{
    return fmt::format("({} {})", value.get_predicate().get_name(), fmt::join(ygg::to_strings(value.get_terms()), " "));
}

template<TaskKind T, FactKind F>
std::string format_data(const ygg::Data<Literal<T, F>>& value)
{
    if (value.polarity)
        return fmt::format("{}", value.atom);
    return fmt::format("(not {})", value.atom);
}

template<TaskKind T, FactKind F>
std::string format_view(LiteralView<T, F> value)
{
    if (value.get_polarity())
        return fmt::format("{}", value.get_atom());
    return fmt::format("(not {})", value.get_atom());
}

template<FactKind F>
std::string format_data(const ygg::Data<Atom<GroundTag, F>>& value)
{
    return fmt::format("({})", value.binding);
}

template<FactKind F>
std::string format_view(AtomView<GroundTag, F> value)
{
    return fmt::format("{}", value.get_row());
}

template<FactKind F>
std::string format_data(const ygg::Data<FunctionTerm<LiftedTag, F>>& value)
{
    return fmt::format("({} {})", value.function, fmt::join(ygg::to_strings(value.terms), " "));
}

template<FactKind F>
std::string format_view(FunctionTermView<LiftedTag, F> value)
{
    return fmt::format("({} {})", value.get_function().get_name(), fmt::join(ygg::to_strings(value.get_terms()), " "));
}

template<FactKind F>
std::string format_data(const ygg::Data<FunctionTerm<GroundTag, F>>& value)
{
    return fmt::format("({})", value.binding);
}

template<FactKind F>
std::string format_view(FunctionTermView<GroundTag, F> value)
{
    return fmt::format("{}", value.get_row());
}

template<FactKind F>
std::string format_data(const ygg::Data<FunctionTermValue<GroundTag, F>>& value)
{
    return fmt::format("(= {} {})", value.fterm, value.value);
}

template<FactKind F>
std::string format_view(FunctionTermValueView<GroundTag, F> value)
{
    return fmt::format("(= {} {})", value.get_fterm(), value.get_value());
}

template<TaskKind T, FactKind F>
std::string format_data(const ygg::Data<NumericEffect<T, F>>& value)
{
    return fmt::format("({} {} {})", value.operator_kind, value.fterm, value.fexpr);
}

template<TaskKind T, FactKind F>
std::string format_view(NumericEffectView<T, F> value)
{
    return fmt::format("({} {} {})", value.get_operator(), value.get_fterm(), value.get_fexpr());
}

template<TaskKind T, FactKind F>
std::string format_data(const ygg::Data<NumericEffectOperator<T, F>>& value)
{
    return fmt::format("{}", value.value);
}

template<TaskKind T, FactKind F>
std::string format_view(NumericEffectOperatorView<T, F> value)
{
    return fmt::format("{}", value.get_variant());
}

template<RelationKind R>
std::string format_data(const ygg::Data<Rule<LiftedTag, R>>& value)
{
    auto os = std::stringstream {};
    os << "Rule(\n";
    {
        ygg::IndentScope scope(os);
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "index = ", value.index);
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "variables = ", value.variables);
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "head = ", value.head);
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "body = ", value.body);
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "metric effects = ", value.metric_effects);
    }
    os << ygg::print_indent << ")";
    return os.str();
}

template<RelationKind R>
std::string format_view(RuleView<LiftedTag, R> value)
{
    auto os = std::stringstream {};
    os << "Rule(\n";
    {
        ygg::IndentScope scope(os);
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "index = ", value.get_index());
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "variables = ", value.get_variables());
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "head = ", value.get_head());
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "body = ", value.get_body());
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "metric effects = ", value.get_metric_effects());
    }
    os << ygg::print_indent << ")";
    return os.str();
}

template<RelationKind R>
std::string format_data(const ygg::Data<Rule<GroundTag, R>>& value)
{
    auto os = std::stringstream {};
    os << "GroundRule(\n";
    {
        ygg::IndentScope scope(os);
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "index = ", value.index);
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "head = ", value.head);
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "body = ", value.body);
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "metric effects = ", value.metric_effects);
    }
    os << ygg::print_indent << ")";
    return os.str();
}

template<RelationKind R>
std::string format_view(RuleView<GroundTag, R> value)
{
    auto os = std::stringstream {};
    os << "GroundRule(\n";
    {
        ygg::IndentScope scope(os);
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "index = ", value.get_index());
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "head = ", value.get_head());
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "body = ", value.get_body());
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "metric effects = ", value.get_metric_effects());
    }
    os << ygg::print_indent << ")";
    return os.str();
}

template<typename BindingView>
std::string format_binding(BindingView value)
{
    if constexpr (requires { value.get_relation().get_name(); })
    {
        auto result = fmt::format("({}", value.get_relation().get_name());
        for (const auto object : value.get_objects())
            fmt::format_to(std::back_inserter(result), " {}", object);
        result.push_back(')');
        return result;
    }
    return fmt::format("({})", fmt::join(ygg::to_strings(value.get_objects()), " "));
}

}  // namespace

std::string to_string(const ygg::Data<UnaryOperator<LiftedTag>>& value) { return format_data(value); }
std::string to_string(const ygg::Data<UnaryOperator<GroundTag>>& value) { return format_data(value); }
std::string to_string(const ygg::Data<BinaryOperator<LiftedTag, ArithmeticOperatorKind>>& value) { return format_data(value); }
std::string to_string(const ygg::Data<BinaryOperator<LiftedTag, BooleanOperatorKind>>& value) { return format_data(value); }
std::string to_string(const ygg::Data<BinaryOperator<GroundTag, ArithmeticOperatorKind>>& value) { return format_data(value); }
std::string to_string(const ygg::Data<BinaryOperator<GroundTag, BooleanOperatorKind>>& value) { return format_data(value); }
std::string to_string(const ygg::Data<MultiOperator<LiftedTag>>& value) { return format_data(value); }
std::string to_string(const ygg::Data<MultiOperator<GroundTag>>& value) { return format_data(value); }
std::string to_string(const ygg::Data<ArithmeticOperator<LiftedTag>>& value) { return format_data(value); }
std::string to_string(const ygg::Data<ArithmeticOperator<GroundTag>>& value) { return format_data(value); }
std::string to_string(const ygg::Data<BooleanOperator<LiftedTag>>& value) { return format_data(value); }
std::string to_string(const ygg::Data<BooleanOperator<GroundTag>>& value) { return format_data(value); }

std::string to_string(UnaryOperatorView<LiftedTag> value) { return format_view(value); }
std::string to_string(UnaryOperatorView<GroundTag> value) { return format_view(value); }
std::string to_string(BinaryOperatorView<LiftedTag, ArithmeticOperatorKind> value) { return format_view(value); }
std::string to_string(BinaryOperatorView<LiftedTag, BooleanOperatorKind> value) { return format_view(value); }
std::string to_string(BinaryOperatorView<GroundTag, ArithmeticOperatorKind> value) { return format_view(value); }
std::string to_string(BinaryOperatorView<GroundTag, BooleanOperatorKind> value) { return format_view(value); }
std::string to_string(MultiOperatorView<LiftedTag> value) { return format_view(value); }
std::string to_string(MultiOperatorView<GroundTag> value) { return format_view(value); }
std::string to_string(ArithmeticOperatorView<LiftedTag> value) { return format_view(value); }
std::string to_string(ArithmeticOperatorView<GroundTag> value) { return format_view(value); }
std::string to_string(BooleanOperatorView<LiftedTag> value) { return format_view(value); }
std::string to_string(BooleanOperatorView<GroundTag> value) { return format_view(value); }

std::string to_string(const ygg::Data<Atom<LiftedTag, StaticTag>>& value) { return format_data(value); }
std::string to_string(const ygg::Data<Atom<LiftedTag, FluentTag>>& value) { return format_data(value); }
std::string to_string(const ygg::Data<Literal<LiftedTag, StaticTag>>& value) { return format_data(value); }
std::string to_string(const ygg::Data<Literal<LiftedTag, FluentTag>>& value) { return format_data(value); }
std::string to_string(const ygg::Data<Atom<GroundTag, StaticTag>>& value) { return format_data(value); }
std::string to_string(const ygg::Data<Atom<GroundTag, FluentTag>>& value) { return format_data(value); }
std::string to_string(const ygg::Data<Literal<GroundTag, StaticTag>>& value) { return format_data(value); }
std::string to_string(const ygg::Data<Literal<GroundTag, FluentTag>>& value) { return format_data(value); }
std::string to_string(const ygg::Data<FunctionTerm<LiftedTag, StaticTag>>& value) { return format_data(value); }
std::string to_string(const ygg::Data<FunctionTerm<LiftedTag, FluentTag>>& value) { return format_data(value); }
std::string to_string(const ygg::Data<FunctionTerm<LiftedTag, AuxiliaryTag>>& value) { return format_data(value); }
std::string to_string(const ygg::Data<FunctionTerm<GroundTag, StaticTag>>& value) { return format_data(value); }
std::string to_string(const ygg::Data<FunctionTerm<GroundTag, FluentTag>>& value) { return format_data(value); }
std::string to_string(const ygg::Data<FunctionTerm<GroundTag, AuxiliaryTag>>& value) { return format_data(value); }
std::string to_string(const ygg::Data<FunctionTermValue<GroundTag, StaticTag>>& value) { return format_data(value); }
std::string to_string(const ygg::Data<FunctionTermValue<GroundTag, FluentTag>>& value) { return format_data(value); }
std::string to_string(const ygg::Data<FunctionTermValue<GroundTag, AuxiliaryTag>>& value) { return format_data(value); }
std::string to_string(const ygg::Data<NumericEffect<LiftedTag, FluentTag>>& value) { return format_data(value); }
std::string to_string(const ygg::Data<NumericEffectOperator<LiftedTag, FluentTag>>& value) { return format_data(value); }
std::string to_string(const ygg::Data<NumericEffect<GroundTag, FluentTag>>& value) { return format_data(value); }
std::string to_string(const ygg::Data<NumericEffectOperator<GroundTag, FluentTag>>& value) { return format_data(value); }

std::string to_string(AtomView<LiftedTag, StaticTag> value) { return format_view(value); }
std::string to_string(AtomView<LiftedTag, FluentTag> value) { return format_view(value); }
std::string to_string(LiteralView<LiftedTag, StaticTag> value) { return format_view(value); }
std::string to_string(LiteralView<LiftedTag, FluentTag> value) { return format_view(value); }
std::string to_string(AtomView<GroundTag, StaticTag> value) { return format_view(value); }
std::string to_string(AtomView<GroundTag, FluentTag> value) { return format_view(value); }
std::string to_string(LiteralView<GroundTag, StaticTag> value) { return format_view(value); }
std::string to_string(LiteralView<GroundTag, FluentTag> value) { return format_view(value); }
std::string to_string(FunctionTermView<LiftedTag, StaticTag> value) { return format_view(value); }
std::string to_string(FunctionTermView<LiftedTag, FluentTag> value) { return format_view(value); }
std::string to_string(FunctionTermView<LiftedTag, AuxiliaryTag> value) { return format_view(value); }
std::string to_string(FunctionTermView<GroundTag, StaticTag> value) { return format_view(value); }
std::string to_string(FunctionTermView<GroundTag, FluentTag> value) { return format_view(value); }
std::string to_string(FunctionTermView<GroundTag, AuxiliaryTag> value) { return format_view(value); }
std::string to_string(FunctionTermValueView<GroundTag, StaticTag> value) { return format_view(value); }
std::string to_string(FunctionTermValueView<GroundTag, FluentTag> value) { return format_view(value); }
std::string to_string(FunctionTermValueView<GroundTag, AuxiliaryTag> value) { return format_view(value); }
std::string to_string(NumericEffectView<LiftedTag, FluentTag> value) { return format_view(value); }
std::string to_string(NumericEffectOperatorView<LiftedTag, FluentTag> value) { return format_view(value); }
std::string to_string(NumericEffectView<GroundTag, FluentTag> value) { return format_view(value); }
std::string to_string(NumericEffectOperatorView<GroundTag, FluentTag> value) { return format_view(value); }

std::string to_string(VariableView value) { return fmt::format("{}", value.get_name()); }
std::string to_string(ObjectView value) { return fmt::format("{}", value.get_name()); }
std::string to_string(TermView value) { return fmt::format("{}", value.get_variant()); }
std::string to_string(PredicateView<StaticTag> value) { return fmt::format("{}/{}", value.get_name(), value.get_arity()); }
std::string to_string(PredicateView<FluentTag> value) { return fmt::format("{}/{}", value.get_name(), value.get_arity()); }
std::string to_string(FunctionView<StaticTag> value) { return fmt::format("{}/{}", value.get_name(), value.get_arity()); }
std::string to_string(FunctionView<FluentTag> value) { return fmt::format("{}/{}", value.get_name(), value.get_arity()); }
std::string to_string(FunctionView<AuxiliaryTag> value) { return fmt::format("{}/{}", value.get_name(), value.get_arity()); }
std::string to_string(PredicateBindingView<StaticTag> value) { return format_binding(value); }
std::string to_string(PredicateBindingView<FluentTag> value) { return format_binding(value); }
std::string to_string(FunctionBindingView<StaticTag> value) { return format_binding(value); }
std::string to_string(FunctionBindingView<FluentTag> value) { return format_binding(value); }
std::string to_string(FunctionBindingView<AuxiliaryTag> value) { return format_binding(value); }
std::string to_string(RuleBindingView<PredicateTag> value) { return format_binding(value); }
std::string to_string(RuleBindingView<FunctionTag> value) { return format_binding(value); }

std::string to_string(const ygg::Data<RelationBinding<Rule<LiftedTag, PredicateTag>>>& value)
{
    return fmt::format("{} {}", value.relation, fmt::join(ygg::to_strings(value.objects), " "));
}

std::string to_string(const ygg::Data<RelationBinding<Rule<LiftedTag, FunctionTag>>>& value)
{
    return fmt::format("{} {}", value.relation, fmt::join(ygg::to_strings(value.objects), " "));
}

std::string to_string(const ygg::Data<FunctionExpression<LiftedTag>>& value) { return fmt::format("{}", value.value); }
std::string to_string(FunctionExpressionView<LiftedTag> value) { return fmt::format("{}", value.get_variant()); }
std::string to_string(const ygg::Data<FunctionExpression<GroundTag>>& value) { return fmt::format("{}", value.value); }
std::string to_string(FunctionExpressionView<GroundTag> value) { return fmt::format("{}", value.get_variant()); }
std::string to_string(const ygg::Data<Metric>& value) { return fmt::format("{}", value.fexpr); }
std::string to_string(MetricView value) { return fmt::format("{}", value.get_fexpr()); }

std::string to_string(const ygg::Data<ConjunctiveCondition<LiftedTag>>& value)
{
    auto os = std::stringstream {};
    os << "ConjunctiveCondition(\n";
    {
        ygg::IndentScope scope(os);
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "index = ", value.index);
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "variables = ", value.variables);
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "static literals = ", value.static_literals);
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "fluent literals = ", value.fluent_literals);
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "numeric constraints = ", value.numeric_constraints);
    }
    os << ygg::print_indent << ")";
    return os.str();
}

std::string to_string(ConjunctiveConditionView<LiftedTag> value)
{
    auto os = std::stringstream {};
    os << "ConjunctiveCondition(\n";
    {
        ygg::IndentScope scope(os);
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "index = ", value.get_index());
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "variables = ", value.get_variables());
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "static literals = ", value.template get_literals<StaticTag>());
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "fluent literals = ", value.template get_literals<FluentTag>());
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "numeric constraints = ", value.get_numeric_constraints());
    }
    os << ygg::print_indent << ")";
    return os.str();
}

std::string to_string(const ygg::Data<ConjunctiveEffect<LiftedTag>>& value)
{
    auto os = std::stringstream {};
    os << "ConjunctiveEffect(\n";
    {
        ygg::IndentScope scope(os);
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "index = ", value.index);
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "numeric effects = ", value.numeric_effects);
    }
    os << ygg::print_indent << ")";
    return os.str();
}

std::string to_string(ConjunctiveEffectView<LiftedTag> value)
{
    auto os = std::stringstream {};
    os << "ConjunctiveEffect(\n";
    {
        ygg::IndentScope scope(os);
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "index = ", value.get_index());
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "numeric effects = ", value.get_numeric_effects());
    }
    os << ygg::print_indent << ")";
    return os.str();
}

std::string to_string(const ygg::Data<ConditionalEffect<LiftedTag>>& value)
{
    auto os = std::stringstream {};
    os << "ConditionalEffect(\n";
    {
        ygg::IndentScope scope(os);
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "index = ", value.index);
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "variables = ", value.variables);
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "condition = ", value.condition);
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "effect = ", value.effect);
    }
    os << ygg::print_indent << ")";
    return os.str();
}

std::string to_string(ConditionalEffectView<LiftedTag> value)
{
    auto os = std::stringstream {};
    os << "ConditionalEffect(\n";
    {
        ygg::IndentScope scope(os);
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "index = ", value.get_index());
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "variables = ", value.get_variables());
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "condition = ", value.get_condition());
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "effect = ", value.get_effect());
    }
    os << ygg::print_indent << ")";
    return os.str();
}

std::string to_string(const ygg::Data<Rule<LiftedTag, PredicateTag>>& value) { return format_data(value); }
std::string to_string(const ygg::Data<Rule<LiftedTag, FunctionTag>>& value) { return format_data(value); }
std::string to_string(RuleView<LiftedTag, PredicateTag> value) { return format_view(value); }
std::string to_string(RuleView<LiftedTag, FunctionTag> value) { return format_view(value); }

std::string to_string(const ygg::Data<ConjunctiveCondition<GroundTag>>& value)
{
    auto os = std::stringstream {};
    os << "GroundConjunctiveCondition(\n";
    {
        ygg::IndentScope scope(os);
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "index = ", value.index);
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "static literals = ", value.static_literals);
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "fluent literals = ", value.fluent_literals);
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "numeric constraints = ", value.numeric_constraints);
    }
    os << ygg::print_indent << ")";
    return os.str();
}

std::string to_string(ConjunctiveConditionView<GroundTag> value)
{
    auto os = std::stringstream {};
    os << "GroundConjunctiveCondition(\n";
    {
        ygg::IndentScope scope(os);
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "index = ", value.get_index());
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "static literals = ", value.template get_literals<StaticTag>());
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "fluent literals = ", value.template get_literals<FluentTag>());
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "numeric constraints = ", value.get_numeric_constraints());
    }
    os << ygg::print_indent << ")";
    return os.str();
}

std::string to_string(const ygg::Data<ConjunctiveEffect<GroundTag>>& value)
{
    auto os = std::stringstream {};
    os << "GroundConjunctiveEffect(\n";
    {
        ygg::IndentScope scope(os);
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "index = ", value.index);
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "numeric effects = ", value.numeric_effects);
    }
    os << ygg::print_indent << ")";
    return os.str();
}

std::string to_string(ConjunctiveEffectView<GroundTag> value)
{
    auto os = std::stringstream {};
    os << "GroundConjunctiveEffect(\n";
    {
        ygg::IndentScope scope(os);
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "index = ", value.get_index());
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "numeric effects = ", value.get_numeric_effects());
    }
    os << ygg::print_indent << ")";
    return os.str();
}

std::string to_string(const ygg::Data<ConditionalEffect<GroundTag>>& value)
{
    auto os = std::stringstream {};
    os << "GroundConditionalEffect(\n";
    {
        ygg::IndentScope scope(os);
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "index = ", value.index);
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "condition = ", value.condition);
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "effect = ", value.effect);
    }
    os << ygg::print_indent << ")";
    return os.str();
}

std::string to_string(ConditionalEffectView<GroundTag> value)
{
    auto os = std::stringstream {};
    os << "GroundConditionalEffect(\n";
    {
        ygg::IndentScope scope(os);
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "index = ", value.get_index());
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "condition = ", value.get_condition());
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "effect = ", value.get_effect());
    }
    os << ygg::print_indent << ")";
    return os.str();
}

std::string to_string(const ygg::Data<Rule<GroundTag, PredicateTag>>& value) { return format_data(value); }
std::string to_string(const ygg::Data<Rule<GroundTag, FunctionTag>>& value) { return format_data(value); }
std::string to_string(RuleView<GroundTag, PredicateTag> value) { return format_view(value); }
std::string to_string(RuleView<GroundTag, FunctionTag> value) { return format_view(value); }

std::string to_string(const ygg::Data<Program<LiftedTag>>& value)
{
    auto os = std::stringstream {};
    os << "Program(\n";
    {
        ygg::IndentScope scope(os);
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "index = ", value.index);
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "static predicates =", value.static_predicates);
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "fluent predicates = ", value.fluent_predicates);
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "static functions = ", value.static_functions);
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "fluent functions = ", value.fluent_functions);
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "objects = ", value.objects);
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "static atoms = ", value.static_atoms);
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "fluent atoms = ", value.fluent_atoms);
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "static fterms = ", value.static_fterm_values);
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "fluent fterms = ", value.fluent_fterm_values);
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "goal = ", value.goal);
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "rules = ", value.predicate_rules);
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "numeric rules = ", value.function_rules);
    }
    os << ygg::print_indent << ")";
    return os.str();
}

std::string to_string(ProgramView<LiftedTag> value)
{
    auto os = std::stringstream {};
    os << "Program(\n";
    {
        ygg::IndentScope scope(os);
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "index = ", value.get_index());
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "static predicates =", value.template get_predicates<StaticTag>());
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "fluent predicates = ", value.template get_predicates<FluentTag>());
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "static functions = ", value.template get_functions<StaticTag>());
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "fluent functions = ", value.template get_functions<FluentTag>());
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "objects = ", value.get_objects());
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "static atoms = ", value.template get_atoms<StaticTag>());
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "fluent atoms = ", value.template get_atoms<FluentTag>());
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "static fterms = ", value.template get_fterm_values<StaticTag>());
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "fluent fterms = ", value.template get_fterm_values<FluentTag>());
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "goal = ", value.get_goal());
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "rules = ", value.template get_rules<PredicateTag>());
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "numeric rules = ", value.template get_rules<FunctionTag>());
    }
    os << ygg::print_indent << ")";
    return os.str();
}

std::string to_string(const ygg::Data<Program<GroundTag>>& value)
{
    auto os = std::stringstream {};
    os << "GroundProgram(\n";
    {
        ygg::IndentScope scope(os);
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "index = ", value.index);
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "static predicates =", value.static_predicates);
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "fluent predicates = ", value.fluent_predicates);
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "static functions = ", value.static_functions);
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "fluent functions = ", value.fluent_functions);
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "objects = ", value.objects);
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "static atoms = ", value.static_atoms);
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "fluent atoms = ", value.fluent_atoms);
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "static fterms = ", value.static_fterm_values);
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "fluent fterms = ", value.fluent_fterm_values);
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "goal = ", value.goal);
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "ground rules = ", value.predicate_rules);
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "numeric ground rules = ", value.function_rules);
    }
    os << ygg::print_indent << ")";
    return os.str();
}

std::string to_string(ProgramView<GroundTag> value)
{
    auto os = std::stringstream {};
    os << "GroundProgram(\n";
    {
        ygg::IndentScope scope(os);
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "index = ", value.get_index());
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "static predicates =", value.template get_predicates<StaticTag>());
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "fluent predicates = ", value.template get_predicates<FluentTag>());
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "static functions = ", value.template get_functions<StaticTag>());
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "fluent functions = ", value.template get_functions<FluentTag>());
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "objects = ", value.get_objects());
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "static atoms = ", value.template get_atoms<StaticTag>());
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "fluent atoms = ", value.template get_atoms<FluentTag>());
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "static fterms = ", value.template get_fterm_values<StaticTag>());
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "fluent fterms = ", value.template get_fterm_values<FluentTag>());
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "goal = ", value.get_goal());
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "ground rules = ", value.template get_rules<PredicateTag>());
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "numeric ground rules = ", value.template get_rules<FunctionTag>());
    }
    os << ygg::print_indent << ")";
    return os.str();
}

std::string to_string(const VariableDependencyGraph& value)
{
    auto os = std::stringstream {};
    os << "graph {\n";
    const auto k = value.k();
    for (ygg::uint_t i = 0; i < k; ++i)
        fmt::print(os, "n{} [label=\"V{}\"];\n", i, i);
    for (ygg::uint_t i = 0; i < k; ++i)
        for (ygg::uint_t j = i + 1; j < k; ++j)
            if (value.binary().has_dependency(i, j))
                fmt::print(os, "n{} -- n{};\n", i, j);
    os << "}\n";
    return os.str();
}

}  // namespace tyr::formalism::datalog
