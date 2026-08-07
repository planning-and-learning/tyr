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

namespace tyr::formalism::datalog::format
{

namespace
{

template<typename T>
std::string format_data(const ygg::Data<UnaryOperator<T>>& value)
{
    return fmt::format("({} {})", value.operator_kind, value.arg);
}

template<BinaryOperatorKind Operator, typename T>
std::string format_data(const ygg::Data<BinaryOperator<Operator, T>>& value)
{
    return fmt::format("({} {} {})", value.operator_kind, value.lhs, value.rhs);
}

template<typename T>
std::string format_data(const ygg::Data<MultiOperator<T>>& value)
{
    return fmt::format("({} {})", value.operator_kind, fmt::join(ygg::to_strings(value.args), " "));
}

template<typename T>
std::string format_data(const ygg::Data<ArithmeticOperator<T>>& value)
{
    return fmt::format("{}", value.value);
}

template<typename T>
std::string format_data(const ygg::Data<BooleanOperator<T>>& value)
{
    return fmt::format("{}", value.value);
}

template<typename T>
std::string format_view(UnaryOperatorView<T> value)
{
    return fmt::format("({} {})", value.get_operator(), value.get_arg());
}

template<BinaryOperatorKind Operator, typename T>
std::string format_view(BinaryOperatorView<Operator, T> value)
{
    return fmt::format("({} {} {})", value.get_operator(), value.get_lhs(), value.get_rhs());
}

template<typename T>
std::string format_view(MultiOperatorView<T> value)
{
    return fmt::format("({} {})", value.get_operator(), fmt::join(ygg::to_strings(value.get_args()), " "));
}

template<typename T>
std::string format_view(ArithmeticOperatorView<T> value)
{
    return fmt::format("{}", value.get_variant());
}

template<typename T>
std::string format_view(BooleanOperatorView<T> value)
{
    return fmt::format("{}", value.get_variant());
}

template<FactKind T>
std::string format_data(const ygg::Data<Atom<T>>& value)
{
    return fmt::format("({} {})", value.predicate, fmt::join(ygg::to_strings(value.terms), " "));
}

template<FactKind T>
std::string format_view(AtomView<T> value)
{
    return fmt::format("({} {})", value.get_predicate().get_name(), fmt::join(ygg::to_strings(value.get_terms()), " "));
}

template<FactKind T>
std::string format_data(const ygg::Data<Literal<T>>& value)
{
    if (value.polarity)
        return fmt::format("{}", value.atom);
    return fmt::format("(not {})", value.atom);
}

template<FactKind T>
std::string format_view(LiteralView<T> value)
{
    if (value.get_polarity())
        return fmt::format("{}", value.get_atom());
    return fmt::format("(not {})", value.get_atom());
}

template<FactKind T>
std::string format_data(const ygg::Data<GroundAtom<T>>& value)
{
    return fmt::format("({})", value.binding);
}

template<FactKind T>
std::string format_view(GroundAtomView<T> value)
{
    return fmt::format("{}", value.get_row());
}

template<FactKind T>
std::string format_data(const ygg::Data<GroundLiteral<T>>& value)
{
    if (value.polarity)
        return fmt::format("{}", value.atom);
    return fmt::format("(not {})", value.atom);
}

template<FactKind T>
std::string format_view(GroundLiteralView<T> value)
{
    if (value.get_polarity())
        return fmt::format("{}", value.get_atom());
    return fmt::format("(not {})", value.get_atom());
}

template<FactKind T>
std::string format_data(const ygg::Data<FunctionTerm<T>>& value)
{
    return fmt::format("({} {})", value.function, fmt::join(ygg::to_strings(value.terms), " "));
}

template<FactKind T>
std::string format_view(FunctionTermView<T> value)
{
    return fmt::format("({} {})", value.get_function().get_name(), fmt::join(ygg::to_strings(value.get_terms()), " "));
}

template<FactKind T>
std::string format_data(const ygg::Data<GroundFunctionTerm<T>>& value)
{
    return fmt::format("({})", value.binding);
}

template<FactKind T>
std::string format_view(GroundFunctionTermView<T> value)
{
    return fmt::format("{}", value.get_row());
}

template<FactKind T>
std::string format_data(const ygg::Data<GroundFunctionTermValue<T>>& value)
{
    return fmt::format("(= {} {})", value.fterm, value.value);
}

template<FactKind T>
std::string format_view(GroundFunctionTermValueView<T> value)
{
    return fmt::format("(= {} {})", value.get_fterm(), value.get_value());
}

template<FactKind T>
std::string format_data(const ygg::Data<NumericEffect<T>>& value)
{
    return fmt::format("({} {} {})", value.operator_kind, value.fterm, value.fexpr);
}

template<FactKind T>
std::string format_view(NumericEffectView<T> value)
{
    return fmt::format("({} {} {})", value.get_operator(), value.get_fterm(), value.get_fexpr());
}

template<FactKind T>
std::string format_data(const ygg::Data<NumericEffectOperator<T>>& value)
{
    return fmt::format("{}", value.value);
}

template<FactKind T>
std::string format_view(NumericEffectOperatorView<T> value)
{
    return fmt::format("{}", value.get_variant());
}

template<FactKind T>
std::string format_data(const ygg::Data<GroundNumericEffect<T>>& value)
{
    return fmt::format("({} {} {})", value.operator_kind, value.fterm, value.fexpr);
}

template<FactKind T>
std::string format_view(GroundNumericEffectView<T> value)
{
    return fmt::format("({} {} {})", value.get_operator(), value.get_fterm(), value.get_fexpr());
}

template<FactKind T>
std::string format_data(const ygg::Data<GroundNumericEffectOperator<T>>& value)
{
    return fmt::format("{}", value.value);
}

template<FactKind T>
std::string format_view(GroundNumericEffectOperatorView<T> value)
{
    return fmt::format("{}", value.get_variant());
}

template<RelationKind R>
std::string format_data(const ygg::Data<Rule<R>>& value)
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
std::string format_view(RuleView<R> value)
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
std::string format_data(const ygg::Data<GroundRule<R>>& value)
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
std::string format_view(GroundRuleView<R> value)
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

std::string to_string(const ygg::Data<UnaryOperator<ygg::Data<FunctionExpression>>>& value) { return format_data(value); }
std::string to_string(const ygg::Data<UnaryOperator<ygg::Data<GroundFunctionExpression>>>& value) { return format_data(value); }
std::string to_string(const ygg::Data<BinaryOperator<ArithmeticOperatorKind, ygg::Data<FunctionExpression>>>& value) { return format_data(value); }
std::string to_string(const ygg::Data<BinaryOperator<BooleanOperatorKind, ygg::Data<FunctionExpression>>>& value) { return format_data(value); }
std::string to_string(const ygg::Data<BinaryOperator<ArithmeticOperatorKind, ygg::Data<GroundFunctionExpression>>>& value) { return format_data(value); }
std::string to_string(const ygg::Data<BinaryOperator<BooleanOperatorKind, ygg::Data<GroundFunctionExpression>>>& value) { return format_data(value); }
std::string to_string(const ygg::Data<MultiOperator<ygg::Data<FunctionExpression>>>& value) { return format_data(value); }
std::string to_string(const ygg::Data<MultiOperator<ygg::Data<GroundFunctionExpression>>>& value) { return format_data(value); }
std::string to_string(const ygg::Data<ArithmeticOperator<ygg::Data<FunctionExpression>>>& value) { return format_data(value); }
std::string to_string(const ygg::Data<ArithmeticOperator<ygg::Data<GroundFunctionExpression>>>& value) { return format_data(value); }
std::string to_string(const ygg::Data<BooleanOperator<ygg::Data<FunctionExpression>>>& value) { return format_data(value); }
std::string to_string(const ygg::Data<BooleanOperator<ygg::Data<GroundFunctionExpression>>>& value) { return format_data(value); }

std::string to_string(LiftedUnaryOperatorView value) { return format_view(value); }
std::string to_string(GroundUnaryOperatorView value) { return format_view(value); }
std::string to_string(LiftedBinaryOperatorView<ArithmeticOperatorKind> value) { return format_view(value); }
std::string to_string(LiftedBinaryOperatorView<BooleanOperatorKind> value) { return format_view(value); }
std::string to_string(GroundBinaryOperatorView<ArithmeticOperatorKind> value) { return format_view(value); }
std::string to_string(GroundBinaryOperatorView<BooleanOperatorKind> value) { return format_view(value); }
std::string to_string(LiftedMultiOperatorView value) { return format_view(value); }
std::string to_string(GroundMultiOperatorView value) { return format_view(value); }
std::string to_string(LiftedArithmeticOperatorView value) { return format_view(value); }
std::string to_string(GroundArithmeticOperatorView value) { return format_view(value); }
std::string to_string(LiftedBooleanOperatorView value) { return format_view(value); }
std::string to_string(GroundBooleanOperatorView value) { return format_view(value); }

std::string to_string(const ygg::Data<Atom<StaticTag>>& value) { return format_data(value); }
std::string to_string(const ygg::Data<Atom<FluentTag>>& value) { return format_data(value); }
std::string to_string(const ygg::Data<Literal<StaticTag>>& value) { return format_data(value); }
std::string to_string(const ygg::Data<Literal<FluentTag>>& value) { return format_data(value); }
std::string to_string(const ygg::Data<GroundAtom<StaticTag>>& value) { return format_data(value); }
std::string to_string(const ygg::Data<GroundAtom<FluentTag>>& value) { return format_data(value); }
std::string to_string(const ygg::Data<GroundLiteral<StaticTag>>& value) { return format_data(value); }
std::string to_string(const ygg::Data<GroundLiteral<FluentTag>>& value) { return format_data(value); }
std::string to_string(const ygg::Data<FunctionTerm<StaticTag>>& value) { return format_data(value); }
std::string to_string(const ygg::Data<FunctionTerm<FluentTag>>& value) { return format_data(value); }
std::string to_string(const ygg::Data<FunctionTerm<AuxiliaryTag>>& value) { return format_data(value); }
std::string to_string(const ygg::Data<GroundFunctionTerm<StaticTag>>& value) { return format_data(value); }
std::string to_string(const ygg::Data<GroundFunctionTerm<FluentTag>>& value) { return format_data(value); }
std::string to_string(const ygg::Data<GroundFunctionTerm<AuxiliaryTag>>& value) { return format_data(value); }
std::string to_string(const ygg::Data<GroundFunctionTermValue<StaticTag>>& value) { return format_data(value); }
std::string to_string(const ygg::Data<GroundFunctionTermValue<FluentTag>>& value) { return format_data(value); }
std::string to_string(const ygg::Data<GroundFunctionTermValue<AuxiliaryTag>>& value) { return format_data(value); }
std::string to_string(const ygg::Data<NumericEffect<FluentTag>>& value) { return format_data(value); }
std::string to_string(const ygg::Data<NumericEffectOperator<FluentTag>>& value) { return format_data(value); }
std::string to_string(const ygg::Data<GroundNumericEffect<FluentTag>>& value) { return format_data(value); }
std::string to_string(const ygg::Data<GroundNumericEffectOperator<FluentTag>>& value) { return format_data(value); }

std::string to_string(AtomView<StaticTag> value) { return format_view(value); }
std::string to_string(AtomView<FluentTag> value) { return format_view(value); }
std::string to_string(LiteralView<StaticTag> value) { return format_view(value); }
std::string to_string(LiteralView<FluentTag> value) { return format_view(value); }
std::string to_string(GroundAtomView<StaticTag> value) { return format_view(value); }
std::string to_string(GroundAtomView<FluentTag> value) { return format_view(value); }
std::string to_string(GroundLiteralView<StaticTag> value) { return format_view(value); }
std::string to_string(GroundLiteralView<FluentTag> value) { return format_view(value); }
std::string to_string(FunctionTermView<StaticTag> value) { return format_view(value); }
std::string to_string(FunctionTermView<FluentTag> value) { return format_view(value); }
std::string to_string(FunctionTermView<AuxiliaryTag> value) { return format_view(value); }
std::string to_string(GroundFunctionTermView<StaticTag> value) { return format_view(value); }
std::string to_string(GroundFunctionTermView<FluentTag> value) { return format_view(value); }
std::string to_string(GroundFunctionTermView<AuxiliaryTag> value) { return format_view(value); }
std::string to_string(GroundFunctionTermValueView<StaticTag> value) { return format_view(value); }
std::string to_string(GroundFunctionTermValueView<FluentTag> value) { return format_view(value); }
std::string to_string(GroundFunctionTermValueView<AuxiliaryTag> value) { return format_view(value); }
std::string to_string(NumericEffectView<FluentTag> value) { return format_view(value); }
std::string to_string(NumericEffectOperatorView<FluentTag> value) { return format_view(value); }
std::string to_string(GroundNumericEffectView<FluentTag> value) { return format_view(value); }
std::string to_string(GroundNumericEffectOperatorView<FluentTag> value) { return format_view(value); }

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

std::string to_string(const ygg::Data<RelationBinding<Rule<PredicateTag>>>& value)
{
    return fmt::format("{} {}", value.relation, fmt::join(ygg::to_strings(value.objects), " "));
}

std::string to_string(const ygg::Data<RelationBinding<Rule<FunctionTag>>>& value)
{
    return fmt::format("{} {}", value.relation, fmt::join(ygg::to_strings(value.objects), " "));
}

std::string to_string(const ygg::Data<FunctionExpression>& value) { return fmt::format("{}", value.value); }
std::string to_string(FunctionExpressionView value) { return fmt::format("{}", value.get_variant()); }
std::string to_string(const ygg::Data<GroundFunctionExpression>& value) { return fmt::format("{}", value.value); }
std::string to_string(GroundFunctionExpressionView value) { return fmt::format("{}", value.get_variant()); }
std::string to_string(const ygg::Data<Metric>& value) { return fmt::format("{}", value.fexpr); }
std::string to_string(MetricView value) { return fmt::format("{}", value.get_fexpr()); }

std::string to_string(const ygg::Data<ConjunctiveCondition>& value)
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

std::string to_string(ConjunctiveConditionView value)
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

std::string to_string(const ygg::Data<ConjunctiveEffect>& value)
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

std::string to_string(ConjunctiveEffectView value)
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

std::string to_string(const ygg::Data<ConditionalEffect>& value)
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

std::string to_string(ConditionalEffectView value)
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

std::string to_string(const ygg::Data<Rule<PredicateTag>>& value) { return format_data(value); }
std::string to_string(const ygg::Data<Rule<FunctionTag>>& value) { return format_data(value); }
std::string to_string(RuleView<PredicateTag> value) { return format_view(value); }
std::string to_string(RuleView<FunctionTag> value) { return format_view(value); }

std::string to_string(const ygg::Data<GroundConjunctiveCondition>& value)
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

std::string to_string(GroundConjunctiveConditionView value)
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

std::string to_string(const ygg::Data<GroundConjunctiveEffect>& value)
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

std::string to_string(GroundConjunctiveEffectView value)
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

std::string to_string(const ygg::Data<GroundConditionalEffect>& value)
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

std::string to_string(GroundConditionalEffectView value)
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

std::string to_string(const ygg::Data<GroundRule<PredicateTag>>& value) { return format_data(value); }
std::string to_string(const ygg::Data<GroundRule<FunctionTag>>& value) { return format_data(value); }
std::string to_string(GroundRuleView<PredicateTag> value) { return format_view(value); }
std::string to_string(GroundRuleView<FunctionTag> value) { return format_view(value); }

std::string to_string(const ygg::Data<Program>& value)
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

std::string to_string(const ygg::Data<GroundProgram>& value)
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
        fmt::print(os, "{}{}\n", "ground rules = ", value.predicate_ground_rules);
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "numeric ground rules = ", value.function_ground_rules);
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

}  // namespace tyr::formalism::datalog::format
