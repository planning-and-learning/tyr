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

#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/planning/planning_domain.hpp"
#include "tyr/formalism/planning/planning_task.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <concepts>
#include <cstddef>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <string_view>
#include <urd/format.hpp>
#include <utility>
#include <variant>
#include <vector>
#include <yggdrasil/formalism/formatter.hpp>
#include <yggdrasil/formatting/cista_formatters.hpp>

// What each record of the planning formalism is in PDDL. A list is written as its elements, each after a space, so that
// an empty one leaves its parentheses closed: (and), (p). A lifted atom or function term writes a parameter as the
// variable its parameters name at that index.

namespace tf = tyr::formalism;
using tyr::GroundTag;
using tyr::LiftedTag;

fmt::format_context::iterator fmt::formatter<urd::index_view<tf::Object, tf::Repository>>::format(const urd::index_view<tf::Object, tf::Repository>& value,
                                                                                                  fmt::format_context& context) const
{ return fmt::format_to(context.out(), "{}", value.get_name()); }

fmt::format_context::iterator fmt::formatter<urd::index_view<tf::Variable, tf::Repository>>::format(const urd::index_view<tf::Variable, tf::Repository>& value,
                                                                                                    fmt::format_context& context) const
{ return fmt::format_to(context.out(), "{}", value.get_name()); }

template<tf::FactKind F>
fmt::format_context::iterator
fmt::formatter<urd::index_view<tf::Predicate<F>, tf::Repository>>::format(const urd::index_view<tf::Predicate<F>, tf::Repository>& value,
                                                                          fmt::format_context& context) const
{
    auto out = fmt::format_to(context.out(), "({}", value.get_name());
    for (std::size_t position = 0; position < value.get_arity(); ++position)
        out = fmt::format_to(out, " ?x{}", position);
    return fmt::format_to(out, ")");
}

template<tf::FactKind F>
fmt::format_context::iterator
fmt::formatter<urd::index_view<tf::Function<F>, tf::Repository>>::format(const urd::index_view<tf::Function<F>, tf::Repository>& value,
                                                                         fmt::format_context& context) const
{
    auto out = fmt::format_to(context.out(), "({}", value.get_name());
    for (std::size_t position = 0; position < value.get_arity(); ++position)
        out = fmt::format_to(out, " ?x{}", position);
    return fmt::format_to(out, ")");
}

template<tyr::TaskKind T, tf::FactKind F>
fmt::format_context::iterator
fmt::formatter<urd::index_view<tf::Atom<T, F>, tf::Repository>>::format(const urd::index_view<tf::Atom<T, F>, tf::Repository>& value,
                                                                        fmt::format_context& context) const
{
    if constexpr (std::same_as<T, GroundTag>)
        return fmt::format_to(context.out(), "{}", value.get_row());
    else
    {
        // A parameter is written as the variable its parameters name at its index.
        const auto variables = value.get_parameters().get_variables();
        auto out = fmt::format_to(context.out(), "({}", value.get_predicate().get_name());
        for (auto term : value.get_terms())
        {
            if (const auto* parameter = std::get_if<tf::ParameterIndex>(&term))
                out = fmt::format_to(out, " {}", variables[parameter->value]);
            else
                out = fmt::format_to(out, " {}", term);
        }
        return fmt::format_to(out, ")");
    }
}

template<tyr::TaskKind T, tf::FactKind F>
fmt::format_context::iterator
fmt::formatter<urd::index_view<tf::Literal<T, F>, tf::Repository>>::format(const urd::index_view<tf::Literal<T, F>, tf::Repository>& value,
                                                                           fmt::format_context& context) const
{
    if (value.get_polarity())
        return fmt::format_to(context.out(), "{}", value.get_atom());
    return fmt::format_to(context.out(), "(not {})", value.get_atom());
}

template<tyr::TaskKind T, tf::FactKind F>
fmt::format_context::iterator
fmt::formatter<urd::index_view<tf::FunctionTerm<T, F>, tf::Repository>>::format(const urd::index_view<tf::FunctionTerm<T, F>, tf::Repository>& value,
                                                                                fmt::format_context& context) const
{
    if constexpr (std::same_as<T, GroundTag>)
        return fmt::format_to(context.out(), "{}", value.get_row());
    else
    {
        // A parameter is written as the variable its parameters name at its index.
        const auto variables = value.get_parameters().get_variables();
        auto out = fmt::format_to(context.out(), "({}", value.get_function().get_name());
        for (auto term : value.get_terms())
        {
            if (const auto* parameter = std::get_if<tf::ParameterIndex>(&term))
                out = fmt::format_to(out, " {}", variables[parameter->value]);
            else
                out = fmt::format_to(out, " {}", term);
        }
        return fmt::format_to(out, ")");
    }
}

template<tyr::TaskKind T, tf::FactKind F>
fmt::format_context::iterator
fmt::formatter<urd::index_view<tf::FunctionTermValue<T, F>, tf::Repository>>::format(const urd::index_view<tf::FunctionTermValue<T, F>, tf::Repository>& value,
                                                                                     fmt::format_context& context) const
{ return fmt::format_to(context.out(), "(= {} {})", value.get_fterm(), value.get_value()); }

template<tyr::TaskKind T, tf::OperatorKind O>
fmt::format_context::iterator
fmt::formatter<urd::index_view<tf::UnaryOperator<T, O>, tf::Repository>>::format(const urd::index_view<tf::UnaryOperator<T, O>, tf::Repository>& value,
                                                                                 fmt::format_context& context) const
{ return fmt::format_to(context.out(), "({} {})", value.get_operation(), value.get_arg()); }

template<tyr::TaskKind T, tf::OperatorKind O>
fmt::format_context::iterator
fmt::formatter<urd::index_view<tf::BinaryOperator<T, O>, tf::Repository>>::format(const urd::index_view<tf::BinaryOperator<T, O>, tf::Repository>& value,
                                                                                  fmt::format_context& context) const
{
    // PDDL compares with =, <, <=, > and >=: a != b is (not (= a b)).
    if constexpr (std::same_as<O, tf::BooleanOperatorKind>)
    {
        if (value.get_operation() == tf::BooleanOperatorKind::Ne)
            return fmt::format_to(context.out(), "(not (= {} {}))", value.get_lhs(), value.get_rhs());
    }
    return fmt::format_to(context.out(), "({} {} {})", value.get_operation(), value.get_lhs(), value.get_rhs());
}

template<tyr::TaskKind T, tf::OperatorKind O>
fmt::format_context::iterator
fmt::formatter<urd::index_view<tf::MultiOperator<T, O>, tf::Repository>>::format(const urd::index_view<tf::MultiOperator<T, O>, tf::Repository>& value,
                                                                                 fmt::format_context& context) const
{
    auto out = fmt::format_to(context.out(), "({}", value.get_operation());
    for (auto argument : value.get_args())
        out = fmt::format_to(out, " {}", argument);
    return fmt::format_to(out, ")");
}

template<tyr::TaskKind T, tf::FactKind F>
fmt::format_context::iterator
fmt::formatter<urd::index_view<tf::NumericEffect<T, F>, tf::Repository>>::format(const urd::index_view<tf::NumericEffect<T, F>, tf::Repository>& value,
                                                                                 fmt::format_context& context) const
{ return fmt::format_to(context.out(), "({} {} {})", value.get_operation(), value.get_fterm(), value.get_fexpr()); }

template<tyr::TaskKind T>
fmt::format_context::iterator
fmt::formatter<urd::index_view<tf::ConjunctiveCondition<T>, tf::Repository>>::format(const urd::index_view<tf::ConjunctiveCondition<T>, tf::Repository>& value,
                                                                                     fmt::format_context& context) const
{
    auto out = fmt::format_to(context.out(), "(and");
    if constexpr (std::same_as<T, LiftedTag>)
    {
        for (auto literal : value.get_static_literals())
            out = fmt::format_to(out, " {}", literal);
        for (auto literal : value.get_fluent_literals())
            out = fmt::format_to(out, " {}", literal);
        for (auto literal : value.get_derived_literals())
            out = fmt::format_to(out, " {}", literal);
        for (auto constraint : value.get_numeric_constraints())
            out = fmt::format_to(out, " {}", constraint);
    }
    else
    {
        for (auto literal : value.get_static_literals())
            out = fmt::format_to(out, " {}", literal);
        for (auto literal : value.get_derived_literals())
            out = fmt::format_to(out, " {}", literal);
        for (auto fact : value.get_positive_facts())
            out = fmt::format_to(out, " {}", fact);
        // A fact that must not hold: its atom false, or, where its variable holds none of its atoms, one of them true.
        for (auto fact : value.get_negative_facts())
        {
            if (const auto atom = fact.get_atom(); atom.has_value())
                out = fmt::format_to(out, " (not {})", atom.value());
            else
                out = fmt::format_to(out, " (or {})", fmt::join(fact.get_variable().get_atoms(), " "));
        }
        for (auto constraint : value.get_numeric_constraints())
            out = fmt::format_to(out, " {}", constraint);
    }
    return fmt::format_to(out, ")");
}

template<tyr::TaskKind T>
fmt::format_context::iterator
fmt::formatter<urd::index_view<tf::ConjunctiveEffect<T>, tf::Repository>>::format(const urd::index_view<tf::ConjunctiveEffect<T>, tf::Repository>& value,
                                                                                  fmt::format_context& context) const
{
    auto out = fmt::format_to(context.out(), "(and");
    if constexpr (std::same_as<T, LiftedTag>)
    {
        for (auto literal : value.get_literals())
            out = fmt::format_to(out, " {}", literal);
    }
    else
    {
        for (auto fact : value.get_add_facts())
            out = fmt::format_to(out, " {}", fact);
        for (auto fact : value.get_del_facts())
        {
            if (const auto atom = fact.get_atom(); atom.has_value())
                out = fmt::format_to(out, " (not {})", atom.value());
        }
    }
    for (auto effect : value.get_numeric_effects())
        out = fmt::format_to(out, " {}", effect);
    if (const auto effect = value.get_auxiliary_numeric_effect(); effect.has_value())
        out = fmt::format_to(out, " {}", effect.value());
    return fmt::format_to(out, ")");
}

template<tyr::TaskKind T>
fmt::format_context::iterator
fmt::formatter<urd::index_view<tf::ConditionalEffect<T>, tf::Repository>>::format(const urd::index_view<tf::ConditionalEffect<T>, tf::Repository>& value,
                                                                                  fmt::format_context& context) const
{
    // Without variables of its own, a conditional effect needs no forall; without a condition, no when.
    const auto condition = value.get_condition();
    auto unconditional = condition.get_static_literals().empty() && condition.get_derived_literals().empty() && condition.get_numeric_constraints().empty();
    if constexpr (std::same_as<T, LiftedTag>)
        unconditional = unconditional && condition.get_fluent_literals().empty();
    else
        unconditional = unconditional && condition.get_positive_facts().empty() && condition.get_negative_facts().empty();

    auto out = context.out();
    if constexpr (std::same_as<T, LiftedTag>)
    {
        if (!value.get_variables().empty())
            out = fmt::format_to(out, "(forall ({}) ", fmt::join(value.get_variables(), " "));
        if (unconditional)
            out = fmt::format_to(out, "{}", value.get_effect());
        else
            out = fmt::format_to(out, "(when {} {})", condition, value.get_effect());
        if (!value.get_variables().empty())
            out = fmt::format_to(out, ")");
    }
    else
    {
        if (unconditional)
            out = fmt::format_to(out, "{}", value.get_effect());
        else
            out = fmt::format_to(out, "(when {} {})", condition, value.get_effect());
    }
    return out;
}

template<tyr::TaskKind T>
fmt::format_context::iterator
fmt::formatter<urd::index_view<tf::Action<T>, tf::Repository>>::format(const urd::index_view<tf::Action<T>, tf::Repository>& value,
                                                                       fmt::format_context& context) const
{
    auto out = context.out();
    if constexpr (std::same_as<T, LiftedTag>)
    {
        out = fmt::format_to(out,
                             "(:action {} :parameters ({}) :precondition {} :effect ",
                             value.get_name(),
                             fmt::join(value.get_variables(), " "),
                             value.get_condition());
        if (value.get_effects().size() == 1)
            out = fmt::format_to(out, "{}", value.get_effects()[0]);
        else
            out = fmt::format_to(out, "(and {})", fmt::join(value.get_effects(), " "));
    }
    else
    {
        // A ground action is named after its binding: its action's name and its objects, joined by -.
        const auto row = value.get_row();
        out = fmt::format_to(out, "(:action {}", row.get_relation().get_name());
        for (auto object : row.template resolve<tf::Object>())
            out = fmt::format_to(out, "-{}", object.get_name());
        out = fmt::format_to(out, " :parameters () :precondition {} :effect ", value.get_condition());
        if (value.get_effects().size() == 1)
            out = fmt::format_to(out, "{}", value.get_effects()[0]);
        else
            out = fmt::format_to(out, "(and {})", fmt::join(value.get_effects(), " "));
    }
    return fmt::format_to(out, ")");
}

template<tyr::TaskKind T>
fmt::format_context::iterator fmt::formatter<urd::index_view<tf::Axiom<T>, tf::Repository>>::format(const urd::index_view<tf::Axiom<T>, tf::Repository>& value,
                                                                                                    fmt::format_context& context) const
{
    if constexpr (std::same_as<T, LiftedTag>)
    {
        // The head binds the variables it names; the others are bound by an exists around the body.
        auto in_head = std::vector<bool>(value.get_variables().size(), false);
        for (auto term : value.get_head().get_terms())
        {
            if (const auto* parameter = std::get_if<tf::ParameterIndex>(&term))
                in_head[parameter->value] = true;
        }
        auto out = fmt::format_to(context.out(), "(:derived {} ", value.get_head());
        auto existential = false;
        for (std::size_t position = 0; position < in_head.size(); ++position)
        {
            if (in_head[position])
                continue;
            out = fmt::format_to(out, "{}{}", existential ? " " : "(exists (", value.get_variables()[position]);
            existential = true;
        }
        if (existential)
            return fmt::format_to(out, ") {}))", value.get_body());
        return fmt::format_to(out, "{})", value.get_body());
    }
    else
        return fmt::format_to(context.out(), "(:derived {} {})", value.get_head(), value.get_body());
}

fmt::format_context::iterator fmt::formatter<urd::index_view<tf::Metric, tf::Repository>>::format(const urd::index_view<tf::Metric, tf::Repository>& value,
                                                                                                  fmt::format_context& context) const
{ return fmt::format_to(context.out(), "(:metric {} {})", value.get_optimization_direction(), value.get_fexpr()); }

// A domain: its sections each on a line of their own, one level deeper, its predicates and functions one per line
// another level deeper (as loki writes a domain). Its requirements are those a normalized domain may use, and those of
// the predicates and functions it has. Equality, which tyr keeps as the static predicate =, is PDDL's own: required
// (:equality), not declared.
fmt::format_context::iterator fmt::formatter<urd::index_view<tf::Domain, tf::Repository>>::format(const urd::index_view<tf::Domain, tf::Repository>& value,
                                                                                                  fmt::format_context& context) const
{
    auto out = fmt::format_to(context.out(), "(define (domain {})", value.get_name());
    {
        const auto section = urd::scoped_indent();
        out = fmt::format_to(out, "{}(:requirements :strips :negative-preconditions :conditional-effects :universal-preconditions", urd::new_line());
        for (auto predicate : value.get_static_predicates())
        {
            if (std::string_view(predicate.get_name()) == "=")
                out = fmt::format_to(out, " :equality");
        }
        if (!value.get_derived_predicates().empty())
            out = fmt::format_to(out, " :derived-predicates");
        if (!value.get_static_functions().empty() || !value.get_fluent_functions().empty())
            out = fmt::format_to(out, " :numeric-fluents");
        if (value.get_auxiliary_function().has_value())
            out = fmt::format_to(out, " :action-costs");
        out = fmt::format_to(out, ")");
        if (!value.get_constants().empty())
            out = fmt::format_to(out, "{}(:constants {})", urd::new_line(), fmt::join(value.get_constants(), " "));
        out = fmt::format_to(out, "{}(:predicates", urd::new_line());
        {
            const auto predicates = urd::scoped_indent();
            for (auto predicate : value.get_static_predicates())
            {
                if (std::string_view(predicate.get_name()) != "=")
                    out = fmt::format_to(out, "{}{}", urd::new_line(), predicate);
            }
            for (auto predicate : value.get_fluent_predicates())
                out = fmt::format_to(out, "{}{}", urd::new_line(), predicate);
            for (auto predicate : value.get_derived_predicates())
                out = fmt::format_to(out, "{}{}", urd::new_line(), predicate);
        }
        out = fmt::format_to(out, ")");
        if (!value.get_static_functions().empty() || !value.get_fluent_functions().empty() || value.get_auxiliary_function().has_value())
        {
            out = fmt::format_to(out, "{}(:functions", urd::new_line());
            {
                const auto functions = urd::scoped_indent();
                for (auto function : value.get_static_functions())
                    out = fmt::format_to(out, "{}{}", urd::new_line(), function);
                for (auto function : value.get_fluent_functions())
                    out = fmt::format_to(out, "{}{}", urd::new_line(), function);
                if (const auto function = value.get_auxiliary_function(); function.has_value())
                    out = fmt::format_to(out, "{}{}", urd::new_line(), function.value());
            }
            out = fmt::format_to(out, ")");
        }
        for (auto axiom : value.get_axioms())
            out = fmt::format_to(out, "{}{}", urd::new_line(), axiom);
        for (auto action : value.get_actions())
            out = fmt::format_to(out, "{}{}", urd::new_line(), action);
    }
    return fmt::format_to(out, "{})", urd::new_line());
}

// A task as the PDDL problem it is: its sections each on a line of their own, one level deeper (as loki writes a task).
// The predicates and axioms a task derives itself are written as loki's task-level sections; the atoms of equality, which
// PDDL knows itself, are not written.
template<tyr::TaskKind T>
fmt::format_context::iterator fmt::formatter<urd::index_view<tf::Task<T>, tf::Repository>>::format(const urd::index_view<tf::Task<T>, tf::Repository>& value,
                                                                                                   fmt::format_context& context) const
{
    auto out = fmt::format_to(context.out(), "(define (problem {})", value.get_name());
    {
        const auto section = urd::scoped_indent();
        out = fmt::format_to(out, "{}(:domain {})", urd::new_line(), value.get_domain().get_name());
        if (!value.get_derived_predicates().empty())
        {
            out = fmt::format_to(out, "{}(:predicates", urd::new_line());
            {
                const auto predicates = urd::scoped_indent();
                for (auto predicate : value.get_derived_predicates())
                    out = fmt::format_to(out, "{}{}", urd::new_line(), predicate);
            }
            out = fmt::format_to(out, ")");
        }
        if (!value.get_objects().empty())
            out = fmt::format_to(out, "{}(:objects {})", urd::new_line(), fmt::join(value.get_objects(), " "));
        out = fmt::format_to(out, "{}(:init", urd::new_line());
        for (auto atom : value.get_static_atoms())
        {
            if (std::string_view(atom.get_row().get_relation().get_name()) != "=")
                out = fmt::format_to(out, " {}", atom);
        }
        for (auto atom : value.get_fluent_atoms())
            out = fmt::format_to(out, " {}", atom);
        for (auto fterm_value : value.get_static_fterm_values())
            out = fmt::format_to(out, " {}", fterm_value);
        for (auto fterm_value : value.get_fluent_fterm_values())
            out = fmt::format_to(out, " {}", fterm_value);
        if (const auto fterm_value = value.get_auxiliary_fterm_value(); fterm_value.has_value())
            out = fmt::format_to(out, " {}", fterm_value.value());
        out = fmt::format_to(out, ")");
        out = fmt::format_to(out, "{}(:goal {})", urd::new_line(), value.get_goal());
        if (const auto metric = value.get_metric(); metric.has_value())
            out = fmt::format_to(out, "{}{}", urd::new_line(), metric.value());
        for (auto axiom : value.get_axioms())
            out = fmt::format_to(out, "{}{}", urd::new_line(), axiom);
    }
    return fmt::format_to(out, "{})", urd::new_line());
}

fmt::format_context::iterator fmt::formatter<tf::FDRFactView<tf::FluentTag>>::format(const tf::FDRFactView<tf::FluentTag>& value,
                                                                                     fmt::format_context& context) const
{
    if (const auto atom = value.get_atom(); atom.has_value())
        return fmt::format_to(context.out(), "{}", atom.value());
    auto out = fmt::format_to(context.out(), "(and");
    for (auto atom : value.get_variable().get_atoms())
        out = fmt::format_to(out, " (not {})", atom);
    return fmt::format_to(out, ")");
}

fmt::format_context::iterator
fmt::formatter<urd::owning_view<tf::FDRFact<tf::FluentTag>, tf::Repository>>::format(const urd::owning_view<tf::FDRFact<tf::FluentTag>, tf::Repository>& value,
                                                                                     fmt::format_context& context) const
{
    if (const auto atom = value.get_atom(); atom.has_value())
        return fmt::format_to(context.out(), "{}", atom.value());
    auto out = fmt::format_to(context.out(), "(and");
    for (auto atom : value.get_variable().get_atoms())
        out = fmt::format_to(out, " (not {})", atom);
    return fmt::format_to(out, ")");
}

template<typename R>
fmt::format_context::iterator fmt::formatter<ygg::RowView<R, tf::Repository>>::format(const ygg::RowView<R, tf::Repository>& value,
                                                                                      fmt::format_context& context) const
{
    // An axiom or a rule has no name: its binding is its values.
    if constexpr (std::same_as<R, tf::Axiom<LiftedTag>> || std::same_as<R, tyr::formalism::Rule<LiftedTag>>)
        return fmt::format_to(context.out(), "{}", ygg::values_text<R>(value));
    else
    {
        if (value.get_values().empty())
            return fmt::format_to(context.out(), "({})", value.get_relation().get_name());
        return fmt::format_to(context.out(), "({} {})", value.get_relation().get_name(), ygg::values_text<R>(value));
    }
}

fmt::format_context::iterator fmt::formatter<tf::ActionRowBuilderView>::format(const tf::ActionRowBuilderView& value, fmt::format_context& context) const
{
    // The relation in whatever form the binding holds it (interned, as a staged binding's is).
    const auto name = urd::visit_view(value.get_relation(), [](const auto& action) { return std::string(action.get_name()); });
    if (value.get_values().empty())
        return fmt::format_to(context.out(), "({})", name);
    return fmt::format_to(context.out(), "({} {})", name, ygg::values_text<tf::Action<LiftedTag>>(value));
}

fmt::format_context::iterator fmt::formatter<tf::PlanningDomain>::format(const tf::PlanningDomain& value, fmt::format_context& context) const
{ return fmt::format_to(context.out(), "{}", value.get_domain()); }

template<tyr::TaskKind T>
fmt::format_context::iterator fmt::formatter<tf::PlanningTask<T>>::format(const tf::PlanningTask<T>& value, fmt::format_context& context) const
{ return fmt::format_to(context.out(), "{}\n{}", value.get_task().get_domain(), value.get_task()); }

fmt::format_context::iterator
fmt::formatter<std::pair<tf::ActionRowView, tf::PlanFormatting>>::format(const std::pair<tf::ActionRowView, tf::PlanFormatting>& value,
                                                                         fmt::format_context& context) const
{
    auto out = fmt::format_to(context.out(), "({}", value.first.get_relation().get_original_name());
    for (const auto object : value.first.template resolve<tf::Object>(0, value.first.get_relation().get_original_arity()))
        out = fmt::format_to(out, " {}", object.get_name());
    return fmt::format_to(out, ")");
}

fmt::format_context::iterator
fmt::formatter<std::pair<tf::ActionView<GroundTag>, tf::PlanFormatting>>::format(const std::pair<tf::ActionView<GroundTag>, tf::PlanFormatting>& value,
                                                                                 fmt::format_context& context) const
{ return fmt::format_to(context.out(), "{}", std::make_pair(value.first.get_row(), value.second)); }

// Each family's formatter, once for each record of the family the repository stores.
template struct fmt::formatter<urd::index_view<tf::Predicate<tf::StaticTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::Predicate<tf::FluentTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::Predicate<tf::DerivedTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::Function<tf::StaticTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::Function<tf::FluentTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::Function<tf::AuxiliaryTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::Atom<LiftedTag, tf::StaticTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::Atom<LiftedTag, tf::FluentTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::Atom<LiftedTag, tf::DerivedTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::Atom<GroundTag, tf::StaticTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::Atom<GroundTag, tf::FluentTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::Atom<GroundTag, tf::DerivedTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::Literal<LiftedTag, tf::StaticTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::Literal<LiftedTag, tf::FluentTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::Literal<LiftedTag, tf::DerivedTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::Literal<GroundTag, tf::StaticTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::Literal<GroundTag, tf::FluentTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::Literal<GroundTag, tf::DerivedTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::FunctionTerm<LiftedTag, tf::StaticTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::FunctionTerm<LiftedTag, tf::FluentTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::FunctionTerm<LiftedTag, tf::AuxiliaryTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::FunctionTerm<GroundTag, tf::StaticTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::FunctionTerm<GroundTag, tf::FluentTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::FunctionTerm<GroundTag, tf::AuxiliaryTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::FunctionTermValue<GroundTag, tf::StaticTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::FunctionTermValue<GroundTag, tf::FluentTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::FunctionTermValue<GroundTag, tf::AuxiliaryTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::UnaryOperator<LiftedTag, tf::ArithmeticOperatorKind>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::UnaryOperator<GroundTag, tf::ArithmeticOperatorKind>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::BinaryOperator<LiftedTag, tf::ArithmeticOperatorKind>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::BinaryOperator<LiftedTag, tf::BooleanOperatorKind>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::BinaryOperator<GroundTag, tf::ArithmeticOperatorKind>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::BinaryOperator<GroundTag, tf::BooleanOperatorKind>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::MultiOperator<LiftedTag, tf::ArithmeticOperatorKind>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::MultiOperator<GroundTag, tf::ArithmeticOperatorKind>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::NumericEffect<LiftedTag, tf::FluentTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::NumericEffect<LiftedTag, tf::AuxiliaryTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::NumericEffect<GroundTag, tf::FluentTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::NumericEffect<GroundTag, tf::AuxiliaryTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::ConjunctiveCondition<LiftedTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::ConjunctiveCondition<GroundTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::ConjunctiveEffect<LiftedTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::ConjunctiveEffect<GroundTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::ConditionalEffect<LiftedTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::ConditionalEffect<GroundTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::Action<LiftedTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::Action<GroundTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::Axiom<LiftedTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::Axiom<GroundTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::Task<LiftedTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::Task<GroundTag>, tf::Repository>>;
template struct fmt::formatter<ygg::RowView<tf::Predicate<tf::StaticTag>, tf::Repository>>;
template struct fmt::formatter<ygg::RowView<tf::Predicate<tf::FluentTag>, tf::Repository>>;
template struct fmt::formatter<ygg::RowView<tf::Predicate<tf::DerivedTag>, tf::Repository>>;
template struct fmt::formatter<ygg::RowView<tf::Function<tf::StaticTag>, tf::Repository>>;
template struct fmt::formatter<ygg::RowView<tf::Function<tf::FluentTag>, tf::Repository>>;
template struct fmt::formatter<ygg::RowView<tf::Function<tf::AuxiliaryTag>, tf::Repository>>;
template struct fmt::formatter<ygg::RowView<tf::Action<LiftedTag>, tf::Repository>>;
template struct fmt::formatter<ygg::RowView<tf::Axiom<LiftedTag>, tf::Repository>>;
template struct fmt::formatter<ygg::RowView<tyr::formalism::Rule<LiftedTag>, tf::Repository>>;
template struct fmt::formatter<tf::PlanningTask<LiftedTag>>;
template struct fmt::formatter<tf::PlanningTask<GroundTag>>;

fmt::format_context::iterator fmt::formatter<tyr::formalism::FDRValue>::format(const tyr::formalism::FDRValue& value, fmt::format_context& ctx) const
{ return fmt::format_to(ctx.out(), "{}", value.value); }

fmt::format_context::iterator fmt::formatter<tyr::formalism::OptimizationDirection>::format(tyr::formalism::OptimizationDirection value,
                                                                                            fmt::format_context& ctx) const
{ return fmt::format_to(ctx.out(), "{}", tyr::formalism::to_string(value)); }

template<typename T>
fmt::format_context::iterator
fmt::formatter<tyr::formalism::unification::SubstitutionFunction<T>>::format(const tyr::formalism::unification::SubstitutionFunction<T>& value,
                                                                             fmt::format_context& ctx) const
{
    auto out = fmt::format_to(ctx.out(), "SubstitutionFunction(");
    auto separator = "";
    for (const auto parameter : value.parameters())
    {
        if (const auto& bound = *value.try_get(parameter); bound.has_value())
            out = fmt::format_to(out, "{}{}={}", separator, parameter, *bound);
        else
            out = fmt::format_to(out, "{}{}=_", separator, parameter);
        separator = ", ";
    }
    return fmt::format_to(out, ")");
}

template struct fmt::formatter<tyr::formalism::unification::SubstitutionFunction<tyr::formalism::Term>>;
template struct fmt::formatter<tyr::formalism::unification::SubstitutionFunction<urd::index<tyr::formalism::Object>>>;
