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

#include "static_literal_compatibility.hpp"
#include "tyr/analysis/domains.hpp"
#include "tyr/formalism/datalog/datas.hpp"
#include "tyr/formalism/datalog/formatter.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/datalog/views.hpp"

#include <algorithm>
#include <assert.h>
#include <gtl/phmap.hpp>
#include <stddef.h>
#include <type_traits>
#include <yggdrasil/containers/unordered_set.hpp>
#include <yggdrasil/containers/variant.hpp>
#include <yggdrasil/containers/vector.hpp>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/core/types.hpp>
#include <yggdrasil/formatting/formatter.hpp>
#include <yggdrasil/ids/index_mixins.hpp>

namespace f = tyr::formalism;
namespace fd = tyr::formalism::datalog;

namespace tyr::analysis
{
namespace
{

/**
 * Temporary internal representation during computation.
 */

template<typename Element, typename Payload>
struct TmpScoped
{
    ygg::Index<Element> element;
    Payload payload;
};

struct TmpVariableDomain
{
    ygg::UnorderedSet<ygg::Index<formalism::Object>> objects;
};

using TmpVariableDomainList = std::vector<TmpVariableDomain>;

template<typename Element>
using TmpSimpleScopedDomain = TmpScoped<Element, TmpVariableDomainList>;

template<typename Element>
using TmpSimpleScopedDomainMap = ygg::UnorderedMap<ygg::Index<Element>, TmpVariableDomainList>;

template<formalism::FactKind T>
using TmpPredicateDomainMap = TmpSimpleScopedDomainMap<formalism::Predicate<T>>;

template<formalism::FactKind T>
using TmpFunctionDomainMap = TmpSimpleScopedDomainMap<formalism::Function<T>>;

template<formalism::RelationKind R>
using TmpRuleDomainMap = TmpSimpleScopedDomainMap<formalism::datalog::Rule<LiftedTag, R>>;

/**
 * Conversion helpers to public representation.
 */

VariableDomain to_variable_domain(const TmpVariableDomain& domain)
{
    auto objects = std::vector<ygg::Index<f::Object>>(domain.objects.begin(), domain.objects.end());
    std::sort(objects.begin(), objects.end());
    return VariableDomain { std::move(objects) };
}

VariableDomainList to_variable_domain_list(const TmpVariableDomainList& domains)
{
    auto result = VariableDomainList {};
    result.reserve(domains.size());

    for (const auto& domain : domains)
        result.push_back(to_variable_domain(domain));

    return result;
}

TmpVariableDomainList to_tmp_variable_domain_list(const VariableDomainList& domains)
{
    auto result = TmpVariableDomainList(domains.size());
    for (size_t parameter = 0; parameter < domains.size(); ++parameter)
        result[parameter].objects.insert(domains[parameter].objects.begin(), domains[parameter].objects.end());
    return result;
}

template<f::FactKind T>
PredicateDomainMap<T> to_predicate_domain_map(const TmpPredicateDomainMap<T>& domains)
{
    auto result = PredicateDomainMap<T> {};
    result.reserve(domains.size());

    for (const auto& [predicate, variable_domains] : domains)
        result.emplace(predicate, to_variable_domain_list(variable_domains));

    return result;
}

template<f::FactKind T>
FunctionDomainMap<T> to_function_domain_map(const TmpFunctionDomainMap<T>& domains)
{
    auto result = FunctionDomainMap<T> {};
    result.reserve(domains.size());

    for (const auto& [function, variable_domains] : domains)
        result.emplace(function, to_variable_domain_list(variable_domains));

    return result;
}

template<f::RelationKind R>
RuleDomainMap<R> to_rule_domain_map(const TmpRuleDomainMap<R>& domains)
{
    auto result = RuleDomainMap<R> {};
    result.reserve(domains.size());

    for (const auto& [rule, variable_domains] : domains)
    {
        result.emplace(rule,
                       SimpleScopedDomain<formalism::datalog::Rule<LiftedTag, R>> {
                           rule,
                           to_variable_domain_list(variable_domains),
                       });
    }

    return result;
}

/**
 * Initialization of temporary maps.
 */

template<f::FactKind T>
TmpPredicateDomainMap<T> initialize_predicate_domain_sets(fd::PredicateListView<T> predicates)
{
    auto predicate_domain_sets = TmpPredicateDomainMap<T> {};
    predicate_domain_sets.reserve(predicates.size());

    for (const auto predicate : predicates)
        predicate_domain_sets.emplace(predicate.get_index(), TmpVariableDomainList(predicate.get_arity()));

    return predicate_domain_sets;
}

template<f::FactKind T>
void insert_into_predicate_domain_sets(fd::AtomListView<GroundTag, T> atoms, TmpPredicateDomainMap<T>& predicate_domain_sets)
{
    for (const auto atom : atoms)
    {
        const auto predicate = atom.get_predicate();
        auto& variable_domains = predicate_domain_sets.at(predicate.get_index());

        auto pos = size_t { 0 };
        for (const auto object : atom.get_objects())
            variable_domains[pos++].objects.insert(object.get_index());
    }
}

template<f::FactKind T>
TmpFunctionDomainMap<T> initialize_function_domain_sets(fd::FunctionListView<T> functions)
{
    auto function_domain_sets = TmpFunctionDomainMap<T> {};
    function_domain_sets.reserve(functions.size());

    for (const auto function : functions)
        function_domain_sets.emplace(function.get_index(), TmpVariableDomainList(function.get_arity()));

    return function_domain_sets;
}

template<f::FactKind T>
void insert_into_function_domain_sets(fd::FunctionTermValueListView<GroundTag, T> fterm_values, TmpFunctionDomainMap<T>& function_domain_sets)
{
    for (const auto term_value : fterm_values)
    {
        const auto fterm = term_value.get_fterm();
        const auto function = fterm.get_function();
        auto& variable_domains = function_domain_sets.at(function.get_index());

        auto pos = size_t { 0 };
        for (const auto object : fterm.get_objects())
            variable_domains[pos++].objects.insert(object.get_index());
    }
}

template<typename Fn>
void for_each_term_with_position(Fn&& fn, fd::TermListView terms)
{
    size_t pos = 0;
    for (const auto term : terms)
    {
        visit([&](auto&& arg) { fn(pos, arg); }, term.get_variant());
        ++pos;
    }
}

/**
 * Policies
 */

struct InsertConstantPolicy
{
    TmpPredicateDomainMap<f::StaticTag>& static_predicate_domain_sets;
    TmpPredicateDomainMap<f::FluentTag>& fluent_predicate_domain_sets;
    TmpFunctionDomainMap<f::StaticTag>& static_function_domain_sets;
    TmpFunctionDomainMap<f::FluentTag>& fluent_function_domain_sets;

    template<typename Element>
    bool should_skip(Element) const
    {
        return false;
    }

    bool should_skip(fd::FunctionView<f::FluentTag>) const { return true; }

    auto& get_domains(fd::PredicateView<f::StaticTag>) { return static_predicate_domain_sets; }
    auto& get_domains(fd::PredicateView<f::FluentTag>) { return fluent_predicate_domain_sets; }

    auto& get_domains(fd::FunctionView<f::StaticTag>) { return static_function_domain_sets; }
    auto& get_domains(fd::FunctionView<f::FluentTag>) { return fluent_function_domain_sets; }

    template<typename Symbol>
    void on_object(size_t pos, fd::ObjectView object, Symbol symbol)
    {
        auto& domain = get_domains(symbol).at(symbol.get_index())[pos];
        domain.objects.insert(object.get_index());
    }

    template<typename Symbol>
    void on_parameter(size_t, f::ParameterIndex, Symbol)
    {
    }
};

struct RestrictPolicy
{
    const TmpPredicateDomainMap<f::StaticTag>& static_predicate_domain_sets;
    const TmpPredicateDomainMap<f::FluentTag>& fluent_predicate_domain_sets;
    const TmpFunctionDomainMap<f::StaticTag>& static_function_domain_sets;
    const TmpFunctionDomainMap<f::FluentTag>& fluent_function_domain_sets;
    TmpVariableDomainList& parameter_domains;

    template<typename Element>
    bool should_skip(Element) const
    {
        return false;
    }

    template<f::FactKind T>
    bool should_skip(fd::LiteralView<LiftedTag, T> literal) const
    {
        return !literal.get_polarity();
    }

    bool should_skip(fd::FunctionView<f::FluentTag>) const { return true; }

    const auto& get_domains(fd::PredicateView<f::StaticTag>) const { return static_predicate_domain_sets; }
    const auto& get_domains(fd::PredicateView<f::FluentTag>) const { return fluent_predicate_domain_sets; }

    const auto& get_domains(fd::FunctionView<f::StaticTag>) const { return static_function_domain_sets; }
    const auto& get_domains(fd::FunctionView<f::FluentTag>) const { return fluent_function_domain_sets; }

    template<typename Symbol>
    void on_object(size_t, fd::ObjectView, Symbol)
    {
    }

    template<typename Symbol>
    void on_parameter(size_t pos, f::ParameterIndex param, Symbol symbol)
    {
        auto& parameter_domain = parameter_domains[ygg::uint_t(param)];
        const auto& symbol_domain = get_domains(symbol).at(symbol.get_index())[pos];
        ygg::intersect_inplace(parameter_domain.objects, symbol_domain.objects);
    }
};

struct LiftPolicy
{
    TmpPredicateDomainMap<f::StaticTag>& static_predicate_domain_sets;
    TmpPredicateDomainMap<f::FluentTag>& fluent_predicate_domain_sets;
    TmpFunctionDomainMap<f::StaticTag>& static_function_domain_sets;
    TmpFunctionDomainMap<f::FluentTag>& fluent_function_domain_sets;
    const TmpVariableDomainList& parameter_domains;

    template<typename Element>
    bool should_skip(Element) const
    {
        return false;
    }

    bool should_skip(fd::FunctionView<f::StaticTag>) const { return true; }

    auto& get_domains(fd::PredicateView<f::StaticTag>) { return static_predicate_domain_sets; }

    auto& get_domains(fd::PredicateView<f::FluentTag>) { return fluent_predicate_domain_sets; }

    auto& get_domains(fd::FunctionView<f::StaticTag>) { return static_function_domain_sets; }

    auto& get_domains(fd::FunctionView<f::FluentTag>) { return fluent_function_domain_sets; }

    template<typename Symbol>
    void on_object(size_t pos, fd::ObjectView object, Symbol symbol)
    {
        auto& domain = get_domains(symbol).at(symbol.get_index())[pos];
        domain.objects.insert(object.get_index());
    }

    template<typename Symbol>
    void on_parameter(size_t pos, f::ParameterIndex param, Symbol symbol)
    {
        auto& domain = get_domains(symbol).at(symbol.get_index())[pos];
        ygg::union_inplace(domain.objects, parameter_domains[ygg::uint_t(param)].objects);
    }
};

/**
 * Policy traversal
 */

template<typename Policy>
void apply_policy(fd::FunctionExpressionView<LiftedTag> element, Policy& policy);

template<typename Policy>
void apply_policy(ygg::float_t, Policy&)
{
}

template<typename Policy>
void apply_policy(fd::UnaryOperatorView<LiftedTag> element, Policy& policy)
{
    apply_policy(element.get_arg(), policy);
}

template<f::BinaryOperatorKind O, typename Policy>
void apply_policy(fd::BinaryOperatorView<LiftedTag, O> element, Policy& policy)
{
    apply_policy(element.get_lhs(), policy);
    apply_policy(element.get_rhs(), policy);
}

template<typename Policy>
void apply_policy(fd::MultiOperatorView<LiftedTag> element, Policy& policy)
{
    for (const auto arg : element.get_args())
        apply_policy(arg, policy);
}

template<f::FactKind T, typename Policy>
void apply_policy(fd::AtomView<LiftedTag, T> element, Policy& policy)
{
    const auto predicate = element.get_predicate();

    if (policy.should_skip(predicate))
        return;

    for_each_term_with_position(
        [&](size_t pos, auto&& arg)
        {
            using Alternative = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<Alternative, fd::ObjectView>)
            {
                policy.on_object(pos, arg, predicate);
            }
            else if constexpr (std::is_same_v<Alternative, f::ParameterIndex>)
            {
                policy.on_parameter(pos, arg, predicate);
            }
            else
            {
                static_assert(ygg::dependent_false<Alternative>::value, "Missing case");
            }
        },
        element.get_terms());
}

template<f::FactKind T, typename Policy>
void apply_policy(fd::LiteralView<LiftedTag, T> element, Policy& policy)
{
    if (policy.should_skip(element))
        return;

    apply_policy(element.get_atom(), policy);
}

template<f::FactKind T, typename Policy>
void apply_policy(fd::FunctionTermView<LiftedTag, T> element, Policy& policy)
{
    const auto function = element.get_function();

    if (policy.should_skip(function))
        return;

    for_each_term_with_position(
        [&](size_t pos, auto&& arg)
        {
            using Alternative = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<Alternative, fd::ObjectView>)
            {
                policy.on_object(pos, arg, function);
            }
            else if constexpr (std::is_same_v<Alternative, f::ParameterIndex>)
            {
                policy.on_parameter(pos, arg, function);
            }
            else
            {
                static_assert(ygg::dependent_false<Alternative>::value, "Missing case");
            }
        },
        element.get_terms());
}

template<f::FactKind T, typename Policy>
void apply_policy(fd::NumericEffectView<LiftedTag, T> element, Policy& policy)
{
    apply_policy(element.get_fterm(), policy);
    apply_policy(element.get_fexpr(), policy);
}

template<typename Policy>
void apply_policy(fd::ArithmeticOperatorView<LiftedTag> element, Policy& policy)
{
    visit([&](auto&& arg) { apply_policy(arg, policy); }, element.get_variant());
}

template<typename Policy>
void apply_policy(fd::FunctionExpressionView<LiftedTag> element, Policy& policy)
{
    visit([&](auto&& arg) { apply_policy(arg, policy); }, element.get_variant());
}

template<typename Policy>
void apply_policy(fd::BooleanOperatorView<LiftedTag> element, Policy& policy)
{
    visit([&](auto&& arg) { apply_policy(arg, policy); }, element.get_variant());
}

template<f::FactKind T, typename Policy>
void apply_policy(fd::NumericEffectOperatorView<LiftedTag, T> element, Policy& policy)
{
    visit([&](auto&& arg) { apply_policy(arg, policy); }, element.get_variant());
}

}  // namespace

ProgramAnalysis analyze_program(fd::ProgramView<LiftedTag> program)
{
    auto universe = ygg::UnorderedSet<ygg::Index<f::Object>> {};
    for (const auto object : program.get_objects())
        universe.insert(object.get_index());

    ///--- Step 1: Initialize static and fluent predicate parameter domains

    auto static_predicate_domain_sets = initialize_predicate_domain_sets(program.get_predicates<f::StaticTag>());
    auto fluent_predicate_domain_sets = initialize_predicate_domain_sets(program.get_predicates<f::FluentTag>());
    insert_into_predicate_domain_sets(program.get_atoms<f::StaticTag>(), static_predicate_domain_sets);
    insert_into_predicate_domain_sets(program.get_atoms<f::FluentTag>(), fluent_predicate_domain_sets);

    ///--- Step 2: Initialize static and fluent function parameter domains

    auto static_function_domain_sets = initialize_function_domain_sets(program.get_functions<f::StaticTag>());
    auto fluent_function_domain_sets = initialize_function_domain_sets(program.get_functions<f::FluentTag>());
    insert_into_function_domain_sets(program.get_fterm_values<f::StaticTag>(), static_function_domain_sets);
    insert_into_function_domain_sets(program.get_fterm_values<f::FluentTag>(), fluent_function_domain_sets);

    ///--- Step 2.5: Important not to forget constants in schemas

    {
        auto insert_policy = InsertConstantPolicy {
            static_predicate_domain_sets,
            fluent_predicate_domain_sets,
            static_function_domain_sets,
            fluent_function_domain_sets,
        };

        const auto insert_rule_constants = [&](auto kind)
        {
            using R = decltype(kind);

            for (const auto rule : program.get_rules<R>())
            {
                for (const auto literal : rule.get_body().template get_literals<f::StaticTag>())
                    apply_policy(literal.get_atom(), insert_policy);

                for (const auto op : rule.get_body().get_numeric_constraints())
                    apply_policy(op, insert_policy);

                apply_policy(rule.get_head(), insert_policy);
            }
        };

        insert_rule_constants(f::PredicateTag {});
        insert_rule_constants(f::FunctionTag {});
    }

    ///--- Step 3: Compute rule parameter domains as tightest bound from the previously computed domains of the static predicates.

    auto predicate_rule_domain_sets = TmpRuleDomainMap<f::PredicateTag> {};
    auto function_rule_domain_sets = TmpRuleDomainMap<f::FunctionTag> {};
    auto compatibility_graphs = ProgramCompatibilityGraphs {};

    const auto restrict_rule_domains = [&](auto kind, auto& domain_sets)
    {
        using R = decltype(kind);

        domain_sets.reserve(program.get_rules<R>().size());
        auto& graphs = compatibility_graphs.get_rules<R>();
        graphs.reserve(program.get_rules<R>().size());

        for (const auto rule : program.get_rules<R>())
        {
            auto variables = rule.get_body().get_variables();
            auto parameter_domains = TmpVariableDomainList(variables.size());

            for (auto& domain : parameter_domains)
                domain.objects = universe;

            auto restrict_policy = RestrictPolicy {
                static_predicate_domain_sets, fluent_predicate_domain_sets, static_function_domain_sets, fluent_function_domain_sets, parameter_domains,
            };

            for (const auto literal : rule.get_body().template get_literals<f::StaticTag>())
                apply_policy(literal, restrict_policy);

            for (const auto op : rule.get_body().get_numeric_constraints())
                apply_policy(op, restrict_policy);

            const auto domains = to_variable_domain_list(parameter_domains);
            const auto static_compatibility = detail::StaticLiteralCompatibility(rule.get_body().template get_literals<f::StaticTag>(),
                                                                                 program.get_atoms<f::StaticTag>(),
                                                                                 domains,
                                                                                 program.get_objects().size());
            auto compatibility = detail::create_pairwise_compatibility_graph(domains, program.get_objects().size(), { &static_compatibility });
            parameter_domains = to_tmp_variable_domain_list(compatibility.domains);
            graphs.emplace(rule.get_index(), std::move(compatibility.graph));

            domain_sets.emplace(rule.get_index(), std::move(parameter_domains));
        }
    };

    restrict_rule_domains(f::PredicateTag {}, predicate_rule_domain_sets);
    restrict_rule_domains(f::FunctionTag {}, function_rule_domain_sets);

    ///--- Step 4: Lift the fluent predicate domains given the variable relationships in the rules.

    const auto lift_rule_domains = [&](auto kind, const auto& domain_sets)
    {
        using R = decltype(kind);

        for (const auto rule : program.get_rules<R>())
        {
            const auto& parameter_domains = domain_sets.at(rule.get_index());

            auto lift_policy = LiftPolicy {
                static_predicate_domain_sets, fluent_predicate_domain_sets, static_function_domain_sets, fluent_function_domain_sets, parameter_domains,
            };

            for (const auto literal : rule.get_body().template get_literals<f::StaticTag>())
                apply_policy(literal, lift_policy);

            for (const auto literal : rule.get_body().template get_literals<f::FluentTag>())
                apply_policy(literal, lift_policy);

            for (const auto op : rule.get_body().get_numeric_constraints())
                apply_policy(op, lift_policy);

            apply_policy(rule.get_head(), lift_policy);
        }
    };

    lift_rule_domains(f::PredicateTag {}, predicate_rule_domain_sets);
    lift_rule_domains(f::FunctionTag {}, function_rule_domain_sets);

    ///--- Step 5: Convert internal sets to public domain wrapper types.

    auto static_predicate_domains = to_predicate_domain_map(static_predicate_domain_sets);
    auto fluent_predicate_domains = to_predicate_domain_map(fluent_predicate_domain_sets);
    auto static_function_domains = to_function_domain_map(static_function_domain_sets);
    auto fluent_function_domains = to_function_domain_map(fluent_function_domain_sets);
    auto predicate_rule_domains = to_rule_domain_map(predicate_rule_domain_sets);
    auto function_rule_domains = to_rule_domain_map(function_rule_domain_sets);

    return ProgramAnalysis {
        ProgramVariableDomains {
            std::move(static_predicate_domains),
            std::move(fluent_predicate_domains),
            std::move(static_function_domains),
            std::move(fluent_function_domains),
            std::move(predicate_rule_domains),
            std::move(function_rule_domains),
        },
        std::move(compatibility_graphs),
    };
}

ProgramVariableDomains compute_variable_domains(fd::ProgramView<LiftedTag> program) { return std::move(analyze_program(program).domains); }

}  // namespace tyr::analysis
