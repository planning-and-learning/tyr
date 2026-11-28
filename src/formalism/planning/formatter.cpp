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
#include "tyr/formalism/planning/planning_task.hpp"
#include "tyr/formalism/repository.hpp"

#include <concepts>
#include <cstddef>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <string>
#include <string_view>
#include <urd/format.hpp>
#include <utility>
#include <variant>
#include <vector>
#include <yggdrasil/formalism/formatter.hpp>
#include <yggdrasil/formatting/cista_formatters.hpp>

// What each record of the planning formalism is in PDDL (the shared records' in src/formalism/formatter.cpp). A list is
// written as its elements, each after a space, so that an empty one leaves its parentheses closed: (and), (p).

namespace tf = tyr::formalism;
using tyr::GroundTag;
using tyr::LiftedTag;

template<tyr::TaskKind T>
fmt::format_context::iterator
fmt::formatter<urd::index_view<tf::ConjunctiveEffect<T>, tf::Repository>>::format(const urd::index_view<tf::ConjunctiveEffect<T>, tf::Repository>& value,
                                                                                  fmt::format_context& context) const
{
    auto out = fmt::format_to(context.out(), "(and");
    if constexpr (std::same_as<T, LiftedTag>)
    {
        for (auto literal : value.literals())
            out = fmt::format_to(out, " {}", literal);
    }
    else
    {
        for (auto fact : value.add_facts())
            out = fmt::format_to(out, " {}", fact);
        for (auto fact : value.del_facts())
        {
            if (const auto atom = fact.atom(); atom.has_value())
                out = fmt::format_to(out, " (not {})", atom.value());
        }
    }
    for (auto effect : value.numeric_effects())
        out = fmt::format_to(out, " {}", effect);
    if (const auto effect = value.auxiliary_numeric_effect(); effect.has_value())
        out = fmt::format_to(out, " {}", effect.value());
    return fmt::format_to(out, ")");
}

template<tyr::TaskKind T>
fmt::format_context::iterator
fmt::formatter<urd::index_view<tf::Outcome<T>, tf::Repository>>::format(const urd::index_view<tf::Outcome<T>, tf::Repository>& value,
                                                                        fmt::format_context& context) const
{
    auto out = fmt::format_to(context.out(), "(and");
    for (const auto effect : value.effects())
        out = fmt::format_to(out, " {}", effect);
    for (const auto choice : value.choices())
        out = fmt::format_to(out, " {}", choice);
    return fmt::format_to(out, ")");
}

template<tyr::TaskKind T>
fmt::format_context::iterator
fmt::formatter<urd::index_view<tf::ChoiceEffect<T>, tf::Repository>>::format(const urd::index_view<tf::ChoiceEffect<T>, tf::Repository>& value,
                                                                             fmt::format_context& context) const
{
    const auto outcomes = value.outcomes();
    const auto probabilities = value.probabilities();
    auto choice = fmt::format("({}", probabilities.empty() ? "oneof" : "probabilistic");
    for (std::size_t i = 0; i < outcomes.size(); ++i)
    {
        if (!probabilities.empty())
            choice += fmt::format(" {}", probabilities[static_cast<std::ptrdiff_t>(i)]);
        choice += fmt::format(" {}", outcomes[static_cast<std::ptrdiff_t>(i)]);
    }
    choice += ")";

    // Without variables of its own, a choice needs no forall; without a condition, no when.
    const auto condition = value.condition();
    auto unconditional = condition.static_literals().empty() && condition.derived_literals().empty() && condition.numeric_constraints().empty();
    if constexpr (std::same_as<T, LiftedTag>)
        unconditional = unconditional && condition.fluent_literals().empty();
    else
        unconditional = unconditional && condition.positive_facts().empty() && condition.negative_facts().empty();

    auto out = context.out();
    auto quantified = false;
    if constexpr (std::same_as<T, LiftedTag>)
    {
        quantified = !value.variables().empty();
        if (quantified)
            out = fmt::format_to(out, "(forall ({}) ", fmt::join(value.variables(), " "));
    }
    if (unconditional)
        out = fmt::format_to(out, "{}", choice);
    else
        out = fmt::format_to(out, "(when {} {})", condition, choice);
    if (quantified)
        out = fmt::format_to(out, ")");
    return out;
}

template<tyr::TaskKind T>
fmt::format_context::iterator
fmt::formatter<urd::index_view<tf::ConditionalEffect<T>, tf::Repository>>::format(const urd::index_view<tf::ConditionalEffect<T>, tf::Repository>& value,
                                                                                  fmt::format_context& context) const
{
    // Without variables of its own, a conditional effect needs no forall; without a condition, no when.
    const auto condition = value.condition();
    auto unconditional = condition.static_literals().empty() && condition.derived_literals().empty() && condition.numeric_constraints().empty();
    if constexpr (std::same_as<T, LiftedTag>)
        unconditional = unconditional && condition.fluent_literals().empty();
    else
        unconditional = unconditional && condition.positive_facts().empty() && condition.negative_facts().empty();

    auto out = context.out();
    if constexpr (std::same_as<T, LiftedTag>)
    {
        if (!value.variables().empty())
            out = fmt::format_to(out, "(forall ({}) ", fmt::join(value.variables(), " "));
        if (unconditional)
            out = fmt::format_to(out, "{}", value.effect());
        else
            out = fmt::format_to(out, "(when {} {})", condition, value.effect());
        if (!value.variables().empty())
            out = fmt::format_to(out, ")");
    }
    else
    {
        if (unconditional)
            out = fmt::format_to(out, "{}", value.effect());
        else
            out = fmt::format_to(out, "(when {} {})", condition, value.effect());
    }
    return out;
}

namespace
{
/// An action's effect: its effects and its choices, in an and unless there is exactly one.
template<tyr::TaskKind T>
fmt::format_context::iterator format_action_effect(fmt::format_context::iterator out, const urd::index_view<tf::Action<T>, tf::Repository>& value)
{
    const auto effects = value.effects();
    const auto choices = value.choices();
    if (effects.size() + choices.size() == 1)
    {
        if (effects.empty())
            return fmt::format_to(out, "{}", choices[0]);
        return fmt::format_to(out, "{}", effects[0]);
    }
    out = fmt::format_to(out, "(and");
    for (const auto effect : effects)
        out = fmt::format_to(out, " {}", effect);
    for (const auto choice : choices)
        out = fmt::format_to(out, " {}", choice);
    return fmt::format_to(out, ")");
}
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
                             value.name(),
                             fmt::join(value.variables(), " "),
                             value.condition());
        out = format_action_effect(out, value);
    }
    else
    {
        // A ground action is named after its binding: its action's name and its objects, joined by -.
        const auto row = value.row();
        out = fmt::format_to(out, "(:action {}", row.relation().name());
        for (auto object : row.template resolve<tf::Object>())
            out = fmt::format_to(out, "-{}", object.name());
        out = fmt::format_to(out, " :parameters () :precondition {} :effect ", value.condition());
        out = format_action_effect(out, value);
    }
    // A durative action's duration, over-all condition and end (its condition and effects above are those at its start).
    if (const auto temporal_part = value.temporal_part(); temporal_part.has_value())
    {
        out = fmt::format_to(out, " :duration (and");
        for (const auto bound : temporal_part->min_durations())
            out = fmt::format_to(out, " (>= ?duration {})", bound);
        for (const auto bound : temporal_part->max_durations())
            out = fmt::format_to(out, " (<= ?duration {})", bound);
        for (const auto bound : temporal_part->end_min_durations())
            out = fmt::format_to(out, " (at end (>= ?duration {}))", bound);
        for (const auto bound : temporal_part->end_max_durations())
            out = fmt::format_to(out, " (at end (<= ?duration {}))", bound);
        out = fmt::format_to(out, ") :over-all {} :end-condition {} :end-effect (and", temporal_part->over_all(), temporal_part->end_condition());
        for (const auto effect : temporal_part->end_effects())
            out = fmt::format_to(out, " {}", effect);
        for (const auto choice : temporal_part->end_choices())
            out = fmt::format_to(out, " {}", choice);
        out = fmt::format_to(out, ")");
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
        auto in_head = std::vector<bool>(value.variables().size(), false);
        for (auto term : value.head().terms())
        {
            if (const auto* parameter = std::get_if<tf::ParameterIndex>(&term))
                in_head[parameter->value] = true;
        }
        auto out = fmt::format_to(context.out(), "(:derived {} ", value.head());
        auto existential = false;
        for (std::size_t position = 0; position < in_head.size(); ++position)
        {
            if (in_head[position])
                continue;
            out = fmt::format_to(out, "{}{}", existential ? " " : "(exists (", value.variables()[position]);
            existential = true;
        }
        if (existential)
            return fmt::format_to(out, ") {}))", value.body());
        return fmt::format_to(out, "{})", value.body());
    }
    else
        return fmt::format_to(context.out(), "(:derived {} {})", value.head(), value.body());
}

fmt::format_context::iterator fmt::formatter<urd::index_view<tf::Metric, tf::Repository>>::format(const urd::index_view<tf::Metric, tf::Repository>& value,
                                                                                                  fmt::format_context& context) const
{ return fmt::format_to(context.out(), "(:metric {} {})", value.optimization_direction(), value.fexpr()); }

// A domain: its sections each on a line of their own, one level deeper, its predicates and functions one per line
// another level deeper (as loki writes a domain). Its requirements are those a normalized domain may use, and those of
// the predicates and functions it has. Equality, which tyr keeps as the static predicate =, is PDDL's own: required
// (:equality), not declared.
fmt::format_context::iterator fmt::formatter<urd::index_view<tf::Domain, tf::Repository>>::format(const urd::index_view<tf::Domain, tf::Repository>& value,
                                                                                                  fmt::format_context& context) const
{
    auto out = fmt::format_to(context.out(), "(define (domain {})", value.name());
    {
        const auto section = urd::scoped_indent();
        out = fmt::format_to(out, "{}(:requirements :strips :negative-preconditions :conditional-effects :universal-preconditions", urd::new_line());
        for (auto predicate : value.static_predicates())
        {
            if (std::string_view(predicate.name()) == "=")
                out = fmt::format_to(out, " :equality");
        }
        if (!value.derived_predicates().empty())
            out = fmt::format_to(out, " :derived-predicates");
        if (!value.static_functions().empty() || !value.fluent_functions().empty())
            out = fmt::format_to(out, " :numeric-fluents");
        if (value.auxiliary_function().has_value())
            out = fmt::format_to(out, " :action-costs");
        out = fmt::format_to(out, ")");
        if (!value.constants().empty())
            out = fmt::format_to(out, "{}(:constants {})", urd::new_line(), fmt::join(value.constants(), " "));
        out = fmt::format_to(out, "{}(:predicates", urd::new_line());
        {
            const auto predicates = urd::scoped_indent();
            for (auto predicate : value.static_predicates())
            {
                if (std::string_view(predicate.name()) != "=")
                    out = fmt::format_to(out, "{}{}", urd::new_line(), predicate);
            }
            for (auto predicate : value.fluent_predicates())
                out = fmt::format_to(out, "{}{}", urd::new_line(), predicate);
            for (auto predicate : value.derived_predicates())
                out = fmt::format_to(out, "{}{}", urd::new_line(), predicate);
        }
        out = fmt::format_to(out, ")");
        if (!value.static_functions().empty() || !value.fluent_functions().empty() || value.auxiliary_function().has_value())
        {
            out = fmt::format_to(out, "{}(:functions", urd::new_line());
            {
                const auto functions = urd::scoped_indent();
                for (auto function : value.static_functions())
                    out = fmt::format_to(out, "{}{}", urd::new_line(), function);
                for (auto function : value.fluent_functions())
                    out = fmt::format_to(out, "{}{}", urd::new_line(), function);
                if (const auto function = value.auxiliary_function(); function.has_value())
                    out = fmt::format_to(out, "{}{}", urd::new_line(), function.value());
            }
            out = fmt::format_to(out, ")");
        }
        for (auto axiom : value.axioms())
            out = fmt::format_to(out, "{}{}", urd::new_line(), axiom);
        for (auto action : value.actions())
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
    auto out = fmt::format_to(context.out(), "(define (problem {})", value.name());
    {
        const auto section = urd::scoped_indent();
        out = fmt::format_to(out, "{}(:domain {})", urd::new_line(), value.domain().name());
        if (!value.derived_predicates().empty())
        {
            out = fmt::format_to(out, "{}(:predicates", urd::new_line());
            {
                const auto predicates = urd::scoped_indent();
                for (auto predicate : value.derived_predicates())
                    out = fmt::format_to(out, "{}{}", urd::new_line(), predicate);
            }
            out = fmt::format_to(out, ")");
        }
        if (!value.objects().empty())
            out = fmt::format_to(out, "{}(:objects {})", urd::new_line(), fmt::join(value.objects(), " "));
        out = fmt::format_to(out, "{}(:init", urd::new_line());
        for (auto atom : value.static_atoms())
        {
            if (std::string_view(atom.row().relation().name()) != "=")
                out = fmt::format_to(out, " {}", atom);
        }
        for (auto atom : value.fluent_atoms())
            out = fmt::format_to(out, " {}", atom);
        for (auto fterm_value : value.static_fterm_values())
            out = fmt::format_to(out, " {}", fterm_value);
        for (auto fterm_value : value.fluent_fterm_values())
            out = fmt::format_to(out, " {}", fterm_value);
        if (const auto fterm_value = value.auxiliary_fterm_value(); fterm_value.has_value())
            out = fmt::format_to(out, " {}", fterm_value.value());
        out = fmt::format_to(out, ")");
        out = fmt::format_to(out, "{}(:goal {})", urd::new_line(), value.goal());
        if (const auto metric = value.metric(); metric.has_value())
            out = fmt::format_to(out, "{}{}", urd::new_line(), metric.value());
        for (auto axiom : value.axioms())
            out = fmt::format_to(out, "{}{}", urd::new_line(), axiom);
    }
    return fmt::format_to(out, "{})", urd::new_line());
}

fmt::format_context::iterator fmt::formatter<tf::FDRFactView<tf::FluentTag>>::format(const tf::FDRFactView<tf::FluentTag>& value,
                                                                                     fmt::format_context& context) const
{
    if (const auto atom = value.atom(); atom.has_value())
        return fmt::format_to(context.out(), "{}", atom.value());
    auto out = fmt::format_to(context.out(), "(and");
    for (auto atom : value.variable().atoms())
        out = fmt::format_to(out, " (not {})", atom);
    return fmt::format_to(out, ")");
}

fmt::format_context::iterator
fmt::formatter<urd::owning_view<tf::FDRFact<tf::FluentTag>, tf::Repository>>::format(const urd::owning_view<tf::FDRFact<tf::FluentTag>, tf::Repository>& value,
                                                                                     fmt::format_context& context) const
{
    if (const auto atom = value.atom(); atom.has_value())
        return fmt::format_to(context.out(), "{}", atom.value());
    auto out = fmt::format_to(context.out(), "(and");
    for (auto atom : value.variable().atoms())
        out = fmt::format_to(out, " (not {})", atom);
    return fmt::format_to(out, ")");
}

fmt::format_context::iterator fmt::formatter<tf::ActionRowBuilderView>::format(const tf::ActionRowBuilderView& value, fmt::format_context& context) const
{
    // The relation in whatever form the binding holds it (interned, as a staged binding's is); its values as a stored
    // binding's are written (ygg::values_text).
    const auto name = urd::visit_view(value.relation(), [](const auto& action) { return std::string(action.name()); });
    if (value.values().empty())
        return fmt::format_to(context.out(), "({})", name);
    return fmt::format_to(context.out(), "({} {})", name, ygg::values_text(value));
}

fmt::format_context::iterator fmt::formatter<tf::PlanningDomain>::format(const tf::PlanningDomain& value, fmt::format_context& context) const
{ return fmt::format_to(context.out(), "{}", value.get_domain()); }

template<tyr::TaskKind T>
fmt::format_context::iterator fmt::formatter<tf::PlanningTask<T>>::format(const tf::PlanningTask<T>& value, fmt::format_context& context) const
{ return fmt::format_to(context.out(), "{}\n{}", value.get_task().domain(), value.get_task()); }

fmt::format_context::iterator
fmt::formatter<std::pair<tf::ActionView<GroundTag>, tf::PlanFormatting>>::format(const std::pair<tf::ActionView<GroundTag>, tf::PlanFormatting>& value,
                                                                                 fmt::format_context& context) const
{ return fmt::format_to(context.out(), "{}", std::make_pair(value.first.row(), value.second)); }

// Each family's formatter, once for each record of the family the repository stores.
template struct fmt::formatter<urd::index_view<tf::ConjunctiveEffect<LiftedTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::ConjunctiveEffect<GroundTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::Outcome<LiftedTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::Outcome<GroundTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::ChoiceEffect<LiftedTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::ChoiceEffect<GroundTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::ConditionalEffect<LiftedTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::ConditionalEffect<GroundTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::Action<LiftedTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::Action<GroundTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::Axiom<LiftedTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::Axiom<GroundTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::Task<LiftedTag>, tf::Repository>>;
template struct fmt::formatter<urd::index_view<tf::Task<GroundTag>, tf::Repository>>;
template struct fmt::formatter<tf::PlanningTask<LiftedTag>>;
template struct fmt::formatter<tf::PlanningTask<GroundTag>>;
