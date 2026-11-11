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

#include "../node_formatter.hpp"
#include "tyr/formalism/atom_data.hpp"
#include "tyr/formalism/atom_view.hpp"
#include "tyr/formalism/binary_operator_data.hpp"
#include "tyr/formalism/binary_operator_view.hpp"
#include "tyr/formalism/datalog/conjunctive_condition_data.hpp"
#include "tyr/formalism/datalog/conjunctive_condition_view.hpp"
#include "tyr/formalism/datalog/conjunctive_effect_data.hpp"
#include "tyr/formalism/datalog/conjunctive_effect_view.hpp"
#include "tyr/formalism/datalog/program_data.hpp"
#include "tyr/formalism/datalog/program_view.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/datalog/rule_data.hpp"
#include "tyr/formalism/datalog/rule_view.hpp"
#include "tyr/formalism/datalog/variable_dependency_graph.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/function_data.hpp"
#include "tyr/formalism/function_term_data.hpp"
#include "tyr/formalism/function_term_value_data.hpp"
#include "tyr/formalism/function_term_value_view.hpp"
#include "tyr/formalism/function_term_view.hpp"
#include "tyr/formalism/function_view.hpp"
#include "tyr/formalism/literal_data.hpp"
#include "tyr/formalism/literal_view.hpp"
#include "tyr/formalism/multi_operator_data.hpp"
#include "tyr/formalism/multi_operator_view.hpp"
#include "tyr/formalism/numeric_effect_data.hpp"
#include "tyr/formalism/numeric_effect_view.hpp"
#include "tyr/formalism/object_data.hpp"
#include "tyr/formalism/object_view.hpp"
#include "tyr/formalism/parameter_index.hpp"
#include "tyr/formalism/predicate_data.hpp"
#include "tyr/formalism/predicate_view.hpp"
#include "tyr/formalism/unary_operator_data.hpp"
#include "tyr/formalism/unary_operator_view.hpp"
#include "tyr/formalism/variable_data.hpp"
#include "tyr/formalism/variable_view.hpp"

#include <fmt/ostream.h>
#include <fmt/ranges.h>
#include <iterator>
#include <sstream>
#include <utility>
#include <yggdrasil/formalism/row_data.hpp>
#include <yggdrasil/formalism/row_view.hpp>
#include <yggdrasil/formatting/cista_formatters.hpp>
#include <yggdrasil/io/iostream.hpp>

namespace tyr::formalism::datalog
{

namespace
{

template<RelationKind R>
std::string format_data(const urd::data<Rule<LiftedTag, R>>& value)
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
        if constexpr (std::same_as<R, PredicateTag>)
            fmt::print(os, "{}{}\n", "heads = ", value.heads);
        else
            fmt::print(os, "{}{}\n", "head = ", value.head);
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "body = ", value.body);
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "cost = ", value.cost);
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
        fmt::print(os, "{}{}\n", "index = ", value.index());
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "variables = ", value.get_variables());
        os << ygg::print_indent;
        if constexpr (std::same_as<R, PredicateTag>)
            fmt::print(os, "{}{}\n", "heads = ", value.get_heads());
        else
            fmt::print(os, "{}{}\n", "head = ", value.get_head());
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "body = ", value.get_body());
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "cost = ", value.get_cost());
    }
    os << ygg::print_indent << ")";
    return os.str();
}

template<RelationKind R>
std::string format_data(const urd::data<Rule<GroundTag, R>>& value)
{
    auto os = std::stringstream {};
    os << "GroundRule(\n";
    {
        ygg::IndentScope scope(os);
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "index = ", value.index);
        os << ygg::print_indent;
        if constexpr (std::same_as<R, PredicateTag>)
            fmt::print(os, "{}{}\n", "heads = ", value.heads);
        else
            fmt::print(os, "{}{}\n", "head = ", value.head);
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "body = ", value.body);
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "cost = ", value.cost);
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
        fmt::print(os, "{}{}\n", "index = ", value.index());
        os << ygg::print_indent;
        if constexpr (std::same_as<R, PredicateTag>)
            fmt::print(os, "{}{}\n", "heads = ", value.get_heads());
        else
            fmt::print(os, "{}{}\n", "head = ", value.get_head());
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "body = ", value.get_body());
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "cost = ", value.get_cost());
    }
    os << ygg::print_indent << ")";
    return os.str();
}

template<typename V>
std::string format_binding(V value)
{
    const auto objects = urd::resolve(value.template get<urd::index<Object>>(), value.context());
    if constexpr (requires { value.get_relation().get_name(); })
    {
        auto result = fmt::format("({}", value.get_relation().get_name());
        for (const auto object : objects)
            fmt::format_to(std::back_inserter(result), " {}", object);
        result.push_back(')');
        return result;
    }
    return fmt::format("({})", fmt::join(ygg::to_strings(objects), " "));
}

}  // namespace

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
static std::string as_text(LiteralView<LiftedTag, StaticTag> value) { return node_format::format(value); }
static std::string as_text(LiteralView<LiftedTag, FluentTag> value) { return node_format::format(value); }
static std::string as_text(AtomView<GroundTag, StaticTag> value) { return node_format::format(value); }
static std::string as_text(AtomView<GroundTag, FluentTag> value) { return node_format::format(value); }
static std::string as_text(LiteralView<GroundTag, StaticTag> value) { return node_format::format(value); }
static std::string as_text(LiteralView<GroundTag, FluentTag> value) { return node_format::format(value); }
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
static std::string as_text(NumericEffectView<GroundTag, FluentTag> value) { return node_format::format(value); }

static std::string as_text(VariableView value) { return fmt::format("{}", value.get_name()); }
static std::string as_text(ObjectView value) { return fmt::format("{}", value.get_name()); }
std::string to_string(TermView value) { return fmt::format("{}", value); }
static std::string as_text(PredicateView<StaticTag> value) { return fmt::format("{}/{}", value.get_name(), value.get_arity()); }
static std::string as_text(PredicateView<FluentTag> value) { return fmt::format("{}/{}", value.get_name(), value.get_arity()); }
static std::string as_text(FunctionView<StaticTag> value) { return fmt::format("{}/{}", value.get_name(), value.get_arity()); }
static std::string as_text(FunctionView<FluentTag> value) { return fmt::format("{}/{}", value.get_name(), value.get_arity()); }
static std::string as_text(FunctionView<AuxiliaryTag> value) { return fmt::format("{}/{}", value.get_name(), value.get_arity()); }
static std::string as_text(PredicateRowView<StaticTag> value) { return format_binding(value); }
static std::string as_text(PredicateRowView<FluentTag> value) { return format_binding(value); }
static std::string as_text(FunctionRowView<StaticTag> value) { return format_binding(value); }
static std::string as_text(FunctionRowView<FluentTag> value) { return format_binding(value); }
static std::string as_text(FunctionRowView<AuxiliaryTag> value) { return format_binding(value); }
static std::string as_text(RuleRowView<PredicateTag> value) { return format_binding(value); }
static std::string as_text(RuleRowView<FunctionTag> value) { return format_binding(value); }

std::string to_string(const urd::data<ygg::Row<Rule<LiftedTag, PredicateTag>>>& value)
{ return fmt::format("{} {}", value.relation, fmt::join(ygg::to_strings(value.values), " ")); }

std::string to_string(const urd::data<ygg::Row<Rule<LiftedTag, FunctionTag>>>& value)
{ return fmt::format("{} {}", value.relation, fmt::join(ygg::to_strings(value.values), " ")); }

std::string to_string(const urd::data<ConjunctiveCondition<LiftedTag>>& value)
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

static std::string as_text(ConjunctiveConditionView<LiftedTag> value)
{
    auto os = std::stringstream {};
    os << "ConjunctiveCondition(\n";
    {
        ygg::IndentScope scope(os);
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "index = ", value.index());
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

std::string to_string(const urd::data<ConjunctiveEffect<LiftedTag>>& value)
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

static std::string as_text(ConjunctiveEffectView<LiftedTag> value)
{
    auto os = std::stringstream {};
    os << "ConjunctiveEffect(\n";
    {
        ygg::IndentScope scope(os);
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "index = ", value.index());
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "numeric effects = ", value.get_numeric_effects());
    }
    os << ygg::print_indent << ")";
    return os.str();
}

std::string to_string(const urd::data<Rule<LiftedTag, PredicateTag>>& value) { return format_data(value); }
std::string to_string(const urd::data<Rule<LiftedTag, FunctionTag>>& value) { return format_data(value); }
static std::string as_text(RuleView<LiftedTag, PredicateTag> value) { return format_view(value); }
static std::string as_text(RuleView<LiftedTag, FunctionTag> value) { return format_view(value); }

std::string to_string(const urd::data<ConjunctiveCondition<GroundTag>>& value)
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

static std::string as_text(ConjunctiveConditionView<GroundTag> value)
{
    auto os = std::stringstream {};
    os << "GroundConjunctiveCondition(\n";
    {
        ygg::IndentScope scope(os);
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "index = ", value.index());
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

std::string to_string(const urd::data<ConjunctiveEffect<GroundTag>>& value)
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

static std::string as_text(ConjunctiveEffectView<GroundTag> value)
{
    auto os = std::stringstream {};
    os << "GroundConjunctiveEffect(\n";
    {
        ygg::IndentScope scope(os);
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "index = ", value.index());
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "numeric effects = ", value.get_numeric_effects());
    }
    os << ygg::print_indent << ")";
    return os.str();
}

std::string to_string(const urd::data<Rule<GroundTag, PredicateTag>>& value) { return format_data(value); }
std::string to_string(const urd::data<Rule<GroundTag, FunctionTag>>& value) { return format_data(value); }
static std::string as_text(RuleView<GroundTag, PredicateTag> value) { return format_view(value); }
static std::string as_text(RuleView<GroundTag, FunctionTag> value) { return format_view(value); }

std::string to_string(const urd::data<Program<LiftedTag>>& value)
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

static std::string as_text(ProgramView<LiftedTag> value)
{
    auto os = std::stringstream {};
    os << "Program(\n";
    {
        ygg::IndentScope scope(os);
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "index = ", value.index());
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

std::string to_string(const urd::data<Program<GroundTag>>& value)
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

static std::string as_text(ProgramView<GroundTag> value)
{
    auto os = std::stringstream {};
    os << "GroundProgram(\n";
    {
        ygg::IndentScope scope(os);
        os << ygg::print_indent;
        fmt::print(os, "{}{}\n", "index = ", value.index());
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

template<typename T>
    requires boost::mp11::mp_contains<SymbolRepositoryTypes, T>::value
std::string to_string(urd::index_view<T, Repository> value)
{ return as_text(value); }

template<typename R>
    requires boost::mp11::mp_contains<RelationRepositoryTypes, R>::value
std::string to_string(ygg::RowView<R, Repository> value)
{ return as_text(value); }

TYR_VIEW_TO_STRINGS(44, 7)

}  // namespace tyr::formalism::datalog
