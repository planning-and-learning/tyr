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

#ifndef TYR_ANALYSIS_FORMATTER_HPP_
#define TYR_ANALYSIS_FORMATTER_HPP_

#include "tyr/analysis/declarations.hpp"
#include "tyr/formalism/datalog/formatter.hpp"
#include "tyr/formalism/planning/formatter.hpp"

#include <algorithm>
#include <fmt/core.h>
#include <fmt/ostream.h>
#include <fmt/ranges.h>
#include <ostream>
#include <sstream>
#include <vector>
#include <yggdrasil/formatting/cista_formatters.hpp>
#include <yggdrasil/io/iostream.hpp>

namespace fmt
{

template<typename C, typename Char>
struct range_format_kind<tyr::analysis::VariableDomainView<C>, Char, void> : std::false_type
{
};

template<>
struct formatter<tyr::analysis::VariableDomain, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::analysis::VariableDomain& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}", value.objects);
    }
};

template<typename Element, typename Payload>
struct formatter<tyr::analysis::Scoped<Element, Payload>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::analysis::Scoped<Element, Payload>& value, FormatContext& ctx) const
    {
        auto os = std::stringstream {};
        os << "ElementDomain(\n";
        {
            ygg::IndentScope scope(os);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "element = ", value.element);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "payload = ", value.payload);
        }
        os << ygg::print_indent << ")";
        return fmt::format_to(ctx.out(), "{}", os.str());
    }
};

template<typename Payload>
struct formatter<tyr::analysis::Scoped<tyr::formalism::planning::Axiom, Payload>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::analysis::Scoped<tyr::formalism::planning::Axiom, Payload>& value, FormatContext& ctx) const
    {
        auto os = std::stringstream {};
        os << "AxiomDomain(\n";
        {
            ygg::IndentScope scope(os);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "element = ", value.element);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "payload = ", value.payload);
        }
        os << ygg::print_indent << ")";
        return fmt::format_to(ctx.out(), "{}", os.str());
    }
};

template<tyr::formalism::RelationKind R, typename Payload>
struct formatter<tyr::analysis::Scoped<tyr::formalism::datalog::Rule<R>, Payload>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::analysis::Scoped<tyr::formalism::datalog::Rule<R>, Payload>& value, FormatContext& ctx) const
    {
        auto os = std::stringstream {};
        os << "RuleDomain(\n";
        {
            ygg::IndentScope scope(os);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "element = ", value.element);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "payload = ", value.payload);
        }
        os << ygg::print_indent << ")";
        return fmt::format_to(ctx.out(), "{}", os.str());
    }
};

template<>
struct formatter<tyr::analysis::ConditionalEffectDomain, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::analysis::ConditionalEffectDomain& value, FormatContext& ctx) const
    {
        auto os = std::stringstream {};
        os << "ConditionalEffectDomain(\n";
        {
            ygg::IndentScope scope(os);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "element = ", value.element);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "condition domain = ", value.payload.condition_domain);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "effect domain = ", value.payload.effect_domain);
        }
        os << ygg::print_indent << ")";
        return fmt::format_to(ctx.out(), "{}", os.str());
    }
};

template<>
struct formatter<tyr::analysis::ActionDomain, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::analysis::ActionDomain& value, FormatContext& ctx) const
    {
        auto os = std::stringstream {};
        os << "ActionDomain(\n";
        {
            ygg::IndentScope scope(os);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "element = ", value.element);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "precondition domain = ", value.payload.precondition_domain);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "effect domains = ", value.payload.effect_domains);
        }
        os << ygg::print_indent << ")";
        return fmt::format_to(ctx.out(), "{}", os.str());
    }
};

template<>
struct formatter<tyr::analysis::ConditionalEffectDomainData, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::analysis::ConditionalEffectDomainData& value, FormatContext& ctx) const
    {
        auto os = std::stringstream {};
        os << "ConditionalEffectDomainData(\n";
        {
            ygg::IndentScope scope(os);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "condition domain = ", value.condition_domain);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "effect domain = ", value.effect_domain);
        }
        os << ygg::print_indent << ")";
        return fmt::format_to(ctx.out(), "{}", os.str());
    }
};

template<>
struct formatter<tyr::analysis::ActionDomainData, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::analysis::ActionDomainData& value, FormatContext& ctx) const
    {
        auto os = std::stringstream {};
        os << "ActionDomainData(\n";
        {
            ygg::IndentScope scope(os);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "precondition domain = ", value.precondition_domain);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "effect domains = ", value.effect_domains);
        }
        os << ygg::print_indent << ")";
        return fmt::format_to(ctx.out(), "{}", os.str());
    }
};

template<>
struct formatter<tyr::analysis::ProgramVariableDomains, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::analysis::ProgramVariableDomains& value, FormatContext& ctx) const
    {
        auto os = std::stringstream {};
        os << "ProgramVariableDomains(\n";
        {
            ygg::IndentScope scope(os);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "static predicate domains = ", value.static_predicate_domains);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "fluent predicate domains = ", value.fluent_predicate_domains);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "static function domains = ", value.static_function_domains);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "fluent function domains = ", value.fluent_function_domains);

            const auto format_rules = [&]<tyr::formalism::RelationKind RK>()
            {
                for (const auto& [rule, domain] : value.template get_rule_domains<RK>())
                {
                    os << ygg::print_indent;
                    fmt::print(os, "rule {} domain = {}\n", rule, domain);
                }
            };
            format_rules.template operator()<tyr::formalism::PredicateTag>();
            format_rules.template operator()<tyr::formalism::FunctionTag>();
        }
        os << ygg::print_indent << ")";
        return fmt::format_to(ctx.out(), "{}", os.str());
    }
};

template<>
struct formatter<tyr::analysis::TaskVariableDomains, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::analysis::TaskVariableDomains& value, FormatContext& ctx) const
    {
        auto os = std::stringstream {};
        os << "TaskVariableDomains(\n";
        {
            ygg::IndentScope scope(os);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "static predicate domains = ", value.static_predicate_domains);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "fluent predicate domains = ", value.fluent_predicate_domains);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "derived predicate domains = ", value.derived_predicate_domains);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "static function domains = ", value.static_function_domains);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "fluent function domains = ", value.fluent_function_domains);

            for (const auto& [action, domain] : value.action_domains)
            {
                os << ygg::print_indent;
                fmt::print(os, "action {} domain = {}\n", action, domain);
            }

            for (const auto& [axiom, domain] : value.axiom_domains)
            {
                os << ygg::print_indent;
                fmt::print(os, "axiom {} domain = {}\n", axiom, domain);
            }
        }
        os << ygg::print_indent << ")";
        return fmt::format_to(ctx.out(), "{}", os.str());
    }
};

template<typename C>
struct formatter<tyr::analysis::ScopedView<tyr::formalism::planning::ConditionalEffect, tyr::analysis::ConditionalEffectDomainViewData<C>, C>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::analysis::ScopedView<tyr::formalism::planning::ConditionalEffect, tyr::analysis::ConditionalEffectDomainViewData<C>, C>& value,
                FormatContext& ctx) const
    {
        auto os = std::stringstream {};
        os << "ConditionalEffectDomain(\n";
        {
            ygg::IndentScope scope(os);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "element = ", value.element.get_index());
            os << ygg::print_indent;
            fmt::print(os, "{}\n", value.payload.condition_domain);
            os << ygg::print_indent;
            fmt::print(os, "{}\n", value.payload.effect_domain);
        }
        os << ygg::print_indent << ")";
        return fmt::format_to(ctx.out(), "{}", os.str());
    }
};

template<typename C>
struct formatter<tyr::analysis::ScopedView<tyr::formalism::planning::Action, tyr::analysis::ActionDomainViewData<C>, C>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::analysis::ScopedView<tyr::formalism::planning::Action, tyr::analysis::ActionDomainViewData<C>, C>& value, FormatContext& ctx) const
    {
        auto os = std::stringstream {};
        os << "ActionDomain(\n";
        {
            ygg::IndentScope scope(os);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "element = ", value.element.get_index());
            os << ygg::print_indent;
            fmt::print(os, "{}\n", value.payload.precondition_domain);

            for (const auto& [conditional_effect, domain] : value.payload.effect_domains)
            {
                os << ygg::print_indent;
                fmt::print(os, "{}\n", domain);
            }
        }
        os << ygg::print_indent << ")";
        return fmt::format_to(ctx.out(), "{}", os.str());
    }
};

template<typename Element, typename Payload, typename C>
struct formatter<tyr::analysis::ScopedView<Element, Payload, C>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::analysis::ScopedView<Element, Payload, C>& value, FormatContext& ctx) const
    {
        auto os = std::stringstream {};
        os << "ElementDomain(\n";
        {
            ygg::IndentScope scope(os);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "element = ", value.element.get_index());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "payload = ", value.payload);
        }
        os << ygg::print_indent << ")";
        return fmt::format_to(ctx.out(), "{}", os.str());
    }
};

template<typename Payload, typename C>
struct formatter<tyr::analysis::ScopedView<tyr::formalism::planning::ConjunctiveCondition, Payload, C>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::analysis::ScopedView<tyr::formalism::planning::ConjunctiveCondition, Payload, C>& value, FormatContext& ctx) const
    {
        auto os = std::stringstream {};
        os << "ConjunctiveConditionDomain(\n";
        {
            ygg::IndentScope scope(os);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "element = ", value.element.get_index());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "payload = ", value.payload);
        }
        os << ygg::print_indent << ")";
        return fmt::format_to(ctx.out(), "{}", os.str());
    }
};

template<typename Payload, typename C>
struct formatter<tyr::analysis::ScopedView<tyr::formalism::planning::ConjunctiveEffect, Payload, C>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::analysis::ScopedView<tyr::formalism::planning::ConjunctiveEffect, Payload, C>& value, FormatContext& ctx) const
    {
        auto os = std::stringstream {};
        os << "ConjunctiveEffectDomain(\n";
        {
            ygg::IndentScope scope(os);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "element = ", value.element.get_index());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "payload = ", value.payload);
        }
        os << ygg::print_indent << ")";
        return fmt::format_to(ctx.out(), "{}", os.str());
    }
};

template<typename Payload, typename C>
struct formatter<tyr::analysis::ScopedView<tyr::formalism::planning::Axiom, Payload, C>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::analysis::ScopedView<tyr::formalism::planning::Axiom, Payload, C>& value, FormatContext& ctx) const
    {
        auto os = std::stringstream {};
        os << "AxiomDomain(\n";
        {
            ygg::IndentScope scope(os);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "element = ", value.element.get_index());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "payload = ", value.payload);
        }
        os << ygg::print_indent << ")";
        return fmt::format_to(ctx.out(), "{}", os.str());
    }
};

template<tyr::formalism::RelationKind R, typename Payload, typename C>
struct formatter<tyr::analysis::ScopedView<tyr::formalism::datalog::Rule<R>, Payload, C>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::analysis::ScopedView<tyr::formalism::datalog::Rule<R>, Payload, C>& value, FormatContext& ctx) const
    {
        auto os = std::stringstream {};
        os << "RuleDomain(\n";
        {
            ygg::IndentScope scope(os);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "element = ", value.element.get_index());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "payload = ", value.payload);
        }
        os << ygg::print_indent << ")";
        return fmt::format_to(ctx.out(), "{}", os.str());
    }
};

template<typename C>
struct formatter<tyr::analysis::VariableDomainView<C>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::analysis::VariableDomainView<C>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}", value.objects);
    }
};

template<>
struct formatter<tyr::analysis::ProgramVariableDomainsView, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::analysis::ProgramVariableDomainsView& value, FormatContext& ctx) const
    {
        auto os = std::stringstream {};
        os << "ProgramVariableDomains(\n";
        {
            ygg::IndentScope scope(os);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "static predicate domains = ", value.static_predicate_domains);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "fluent predicate domains = ", value.fluent_predicate_domains);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "static function domains = ", value.static_function_domains);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "fluent function domains = ", value.fluent_function_domains);

            const auto format_rules = [&]<tyr::formalism::RelationKind RK>()
            {
                for (const auto& [rule, domain] : value.template get_rule_domains<RK>())
                {
                    os << ygg::print_indent;
                    fmt::print(os, "{}\n", domain);
                }
            };
            format_rules.template operator()<tyr::formalism::PredicateTag>();
            format_rules.template operator()<tyr::formalism::FunctionTag>();
        }
        os << ygg::print_indent << ")";
        return fmt::format_to(ctx.out(), "{}", os.str());
    }
};

template<>
struct formatter<tyr::analysis::TaskVariableDomainsView, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::analysis::TaskVariableDomainsView& value, FormatContext& ctx) const
    {
        auto os = std::stringstream {};
        os << "TaskVariableDomains(\n";
        {
            ygg::IndentScope scope(os);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "static predicate domains = ", value.static_predicate_domains);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "fluent predicate domains = ", value.fluent_predicate_domains);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "derived predicate domains = ", value.derived_predicate_domains);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "static function domains = ", value.static_function_domains);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "fluent function domains = ", value.fluent_function_domains);

            for (const auto& [action, domain] : value.action_domains)
            {
                os << ygg::print_indent;
                fmt::print(os, "{}\n", domain);
            }

            for (const auto& [axiom, domain] : value.axiom_domains)
            {
                os << ygg::print_indent;
                fmt::print(os, "{}\n", domain);
            }
        }
        os << ygg::print_indent << ")";
        return fmt::format_to(ctx.out(), "{}", os.str());
    }
};

}  // namespace fmt

#endif
