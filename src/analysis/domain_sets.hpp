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

#ifndef TYR_SRC_ANALYSIS_DOMAIN_SETS_HPP_
#define TYR_SRC_ANALYSIS_DOMAIN_SETS_HPP_

#include "tyr/analysis/declarations.hpp"
#include "tyr/analysis/variable_domain.hpp"
#include "tyr/declarations.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/enums.hpp"
#include "tyr/formalism/parameter_index.hpp"
#include "tyr/formalism/repository.hpp"

#include <algorithm>
#include <cstddef>
#include <urd/elements/index.hpp>
#include <urd/kinds.hpp>
#include <urd/repr/maps/unordered_flat_map.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/repr/values/unordered_flat_set.hpp>
#include <urd/store.hpp>
#include <utility>
#include <yggdrasil/core/config.hpp>

// The domains of a program's and of a task's variables while they are computed (program_domains.cpp,
// task_domains.cpp): the objects each parameter may take, as sets, and their conversion to the public, sorted
// representation (tyr/analysis/variable_domain.hpp).

namespace tyr::analysis
{

template<typename Element, typename Payload>
struct TmpScoped
{
    urd::index<Element> element;
    Payload payload;
};

struct TmpVariableDomain
{
    urd::Store<urd::set<urd::index<formalism::Object>>> objects;
};

using TmpVariableDomainList = urd::Store<urd::sequence<TmpVariableDomain>>;

template<typename Element>
using TmpSimpleScopedDomain = TmpScoped<Element, TmpVariableDomainList>;

template<typename Element>
using TmpSimpleScopedDomainMap = urd::Store<urd::map<urd::index<Element>, TmpVariableDomainList>>;

template<formalism::FactKind T>
using TmpPredicateDomainMap = TmpSimpleScopedDomainMap<formalism::Predicate<T>>;

template<formalism::FactKind T>
using TmpFunctionDomainMap = TmpSimpleScopedDomainMap<formalism::Function<T>>;

inline VariableDomain to_variable_domain(const TmpVariableDomain& domain)
{
    auto objects = urd::Store<urd::sequence<urd::index<formalism::Object>>>(domain.objects.begin(), domain.objects.end());
    std::sort(objects.begin(), objects.end());
    return VariableDomain { std::move(objects) };
}

inline VariableDomainList to_variable_domain_list(const TmpVariableDomainList& domains)
{
    auto result = VariableDomainList {};
    result.reserve(domains.size());

    for (const auto& domain : domains)
        result.push_back(to_variable_domain(domain));

    return result;
}

inline TmpVariableDomainList to_tmp_variable_domain_list(const VariableDomainList& domains)
{
    auto result = TmpVariableDomainList(domains.size());
    for (size_t parameter = 0; parameter < domains.size(); ++parameter)
        result[parameter].objects.insert(domains[parameter].objects.begin(), domains[parameter].objects.end());
    return result;
}

template<formalism::FactKind T>
PredicateDomainMap<T> to_predicate_domain_map(const TmpPredicateDomainMap<T>& domains)
{
    auto result = PredicateDomainMap<T> {};
    result.reserve(domains.size());

    for (const auto& [predicate, variable_domains] : domains)
        result.emplace(predicate, to_variable_domain_list(variable_domains));

    return result;
}

template<formalism::FactKind T>
FunctionDomainMap<T> to_function_domain_map(const TmpFunctionDomainMap<T>& domains)
{
    auto result = FunctionDomainMap<T> {};
    result.reserve(domains.size());

    for (const auto& [function, variable_domains] : domains)
        result.emplace(function, to_variable_domain_list(variable_domains));

    return result;
}

/// The steps both domain computations (a task's and a program's) take over the records they share.
namespace domain_steps
{

/**
 * Initialization of temporary maps.
 */

template<formalism::FactKind T>
TmpPredicateDomainMap<T> initialize_predicate_domain_sets(formalism::PredicateListView<T> predicates)
{
    auto predicate_domain_sets = TmpPredicateDomainMap<T> {};
    predicate_domain_sets.reserve(predicates.size());

    for (const auto predicate : predicates)
        predicate_domain_sets.emplace(predicate.index(), TmpVariableDomainList(predicate.get_arity()));

    return predicate_domain_sets;
}

template<formalism::FactKind T>
void insert_into_predicate_domain_sets(formalism::AtomListView<GroundTag, T> atoms, TmpPredicateDomainMap<T>& predicate_domain_sets)
{
    for (const auto atom : atoms)
    {
        const auto predicate = atom.get_predicate();
        auto& variable_domains = predicate_domain_sets.at(predicate.index());

        auto pos = size_t { 0 };
        for (const auto object : atom.get_row().template resolve<formalism::Object>())
            variable_domains[pos++].objects.insert(object.index());
    }
}

template<formalism::FactKind T>
TmpFunctionDomainMap<T> initialize_function_domain_sets(formalism::FunctionListView<T> functions)
{
    auto function_domain_sets = TmpFunctionDomainMap<T> {};
    function_domain_sets.reserve(functions.size());

    for (const auto function : functions)
        function_domain_sets.emplace(function.index(), TmpVariableDomainList(function.get_arity()));

    return function_domain_sets;
}

template<formalism::FactKind T>
void insert_into_function_domain_sets(formalism::FunctionTermValueListView<GroundTag, T> fterm_values, TmpFunctionDomainMap<T>& function_domain_sets)
{
    for (const auto term_value : fterm_values)
    {
        const auto fterm = term_value.get_fterm();
        const auto function = fterm.get_function();
        auto& variable_domains = function_domain_sets.at(function.index());

        auto pos = size_t { 0 };
        for (const auto object : fterm.get_row().template resolve<formalism::Object>())
            variable_domains[pos++].objects.insert(object.index());
    }
}

template<typename Fn>
void for_each_term_with_position(Fn fn, formalism::TermListView terms)
{
    size_t pos = 0;
    for (const auto term : terms)
    {
        visit([&](auto&& arg) { fn(pos, arg); }, term);
        ++pos;
    }
}

/**
 * Policies
 */

struct InsertConstantPolicy
{
    TmpPredicateDomainMap<formalism::StaticTag>* static_predicate_domain_sets;
    TmpPredicateDomainMap<formalism::FluentTag>* fluent_predicate_domain_sets;
    TmpPredicateDomainMap<formalism::DerivedTag>* derived_predicate_domain_sets;
    TmpFunctionDomainMap<formalism::StaticTag>* static_function_domain_sets;
    TmpFunctionDomainMap<formalism::FluentTag>* fluent_function_domain_sets;

    template<typename Element>
    [[nodiscard]] bool should_skip(Element) const
    { return false; }

    [[nodiscard]] bool should_skip(formalism::FunctionView<formalism::FluentTag>) const { return true; }

    [[nodiscard]] auto& get_domains(formalism::PredicateView<formalism::StaticTag>) const { return *static_predicate_domain_sets; }

    [[nodiscard]] auto& get_domains(formalism::PredicateView<formalism::FluentTag>) const { return *fluent_predicate_domain_sets; }

    [[nodiscard]] auto& get_domains(formalism::PredicateView<formalism::DerivedTag>) const { return *derived_predicate_domain_sets; }

    [[nodiscard]] auto& get_domains(formalism::FunctionView<formalism::StaticTag>) const { return *static_function_domain_sets; }

    [[nodiscard]] auto& get_domains(formalism::FunctionView<formalism::FluentTag>) const { return *fluent_function_domain_sets; }

    template<typename Symbol>
    void on_object(size_t pos, formalism::ObjectView object, Symbol symbol)
    {
        auto& domain = get_domains(symbol).at(symbol.index())[pos];
        domain.objects.insert(object.index());
    }

    template<typename Symbol>
    void on_parameter(size_t, formalism::ParameterIndex, Symbol)
    {
    }
};

struct RestrictPolicy
{
    const TmpPredicateDomainMap<formalism::StaticTag>* static_predicate_domain_sets;
    const TmpPredicateDomainMap<formalism::FluentTag>* fluent_predicate_domain_sets;
    const TmpPredicateDomainMap<formalism::DerivedTag>* derived_predicate_domain_sets;
    const TmpFunctionDomainMap<formalism::StaticTag>* static_function_domain_sets;
    const TmpFunctionDomainMap<formalism::FluentTag>* fluent_function_domain_sets;
    TmpVariableDomainList* parameter_domains;

    template<typename Element>
    [[nodiscard]] bool should_skip(Element) const
    { return false; }

    template<formalism::FactKind T>
    [[nodiscard]] bool should_skip(formalism::LiteralView<LiftedTag, T> literal) const
    { return !literal.get_polarity(); }

    [[nodiscard]] bool should_skip(formalism::FunctionView<formalism::FluentTag>) const { return true; }

    [[nodiscard]] const auto& get_domains(formalism::PredicateView<formalism::StaticTag>) const { return *static_predicate_domain_sets; }

    [[nodiscard]] const auto& get_domains(formalism::PredicateView<formalism::FluentTag>) const { return *fluent_predicate_domain_sets; }

    [[nodiscard]] const auto& get_domains(formalism::PredicateView<formalism::DerivedTag>) const { return *derived_predicate_domain_sets; }

    [[nodiscard]] const auto& get_domains(formalism::FunctionView<formalism::StaticTag>) const { return *static_function_domain_sets; }

    [[nodiscard]] const auto& get_domains(formalism::FunctionView<formalism::FluentTag>) const { return *fluent_function_domain_sets; }

    template<typename Symbol>
    void on_object(size_t, formalism::ObjectView, Symbol)
    {
    }

    template<typename Symbol>
    void on_parameter(size_t pos, formalism::ParameterIndex param, Symbol symbol)
    {
        auto& parameter_domain = (*parameter_domains)[param.value];
        const auto& symbol_domain = get_domains(symbol).at(symbol.index())[pos];
        erase_if(parameter_domain.objects, [&](const auto& object) { return !symbol_domain.objects.contains(object); });
    }
};

struct LiftPolicy
{
    TmpPredicateDomainMap<formalism::StaticTag>* static_predicate_domain_sets;
    TmpPredicateDomainMap<formalism::FluentTag>* fluent_predicate_domain_sets;
    TmpPredicateDomainMap<formalism::DerivedTag>* derived_predicate_domain_sets;
    TmpFunctionDomainMap<formalism::StaticTag>* static_function_domain_sets;
    TmpFunctionDomainMap<formalism::FluentTag>* fluent_function_domain_sets;
    const TmpVariableDomainList* parameter_domains;

    template<typename Element>
    [[nodiscard]] bool should_skip(Element) const
    { return false; }

    [[nodiscard]] bool should_skip(formalism::FunctionView<formalism::StaticTag>) const { return true; }

    [[nodiscard]] auto& get_domains(formalism::PredicateView<formalism::StaticTag>) const { return *static_predicate_domain_sets; }

    [[nodiscard]] auto& get_domains(formalism::PredicateView<formalism::FluentTag>) const { return *fluent_predicate_domain_sets; }

    [[nodiscard]] auto& get_domains(formalism::PredicateView<formalism::DerivedTag>) const { return *derived_predicate_domain_sets; }

    [[nodiscard]] auto& get_domains(formalism::FunctionView<formalism::StaticTag>) const { return *static_function_domain_sets; }

    [[nodiscard]] auto& get_domains(formalism::FunctionView<formalism::FluentTag>) const { return *fluent_function_domain_sets; }

    template<typename Symbol>
    void on_object(size_t pos, formalism::ObjectView object, Symbol symbol)
    {
        auto& domain = get_domains(symbol).at(symbol.index())[pos];
        domain.objects.insert(object.index());
    }

    template<typename Symbol>
    void on_parameter(size_t pos, formalism::ParameterIndex param, Symbol symbol)
    {
        auto& domain = get_domains(symbol).at(symbol.index())[pos];
        {
            const auto& source = (*parameter_domains)[param.value].objects;
            domain.objects.insert(source.begin(), source.end());
        }
    }
};

/**
 * Policy traversal
 */

template<typename Policy>
void apply_policy(formalism::FunctionExpressionView<LiftedTag> element, Policy& policy);

template<typename Policy>
void apply_policy(ygg::float_t, Policy&)
{
}

template<typename Policy>
void apply_policy(formalism::UnaryOperatorView<LiftedTag, ::tyr::formalism::ArithmeticOperatorKind> element, Policy& policy)
{ apply_policy(element.get_arg(), policy); }

template<formalism::OperatorKind O, typename Policy>
void apply_policy(formalism::BinaryOperatorView<LiftedTag, O> element, Policy& policy)
{
    apply_policy(element.get_lhs(), policy);
    apply_policy(element.get_rhs(), policy);
}

template<typename Policy>
void apply_policy(formalism::MultiOperatorView<LiftedTag, ::tyr::formalism::ArithmeticOperatorKind> element, Policy& policy)
{
    for (const auto arg : element.get_args())
        apply_policy(arg, policy);
}

template<formalism::FactKind T, typename Policy>
void apply_policy(formalism::AtomView<LiftedTag, T> element, Policy& policy)
{
    const auto predicate = element.get_predicate();

    if (policy.should_skip(predicate))
        return;

    for_each_term_with_position(
        [&]<typename Alternative>(size_t pos, const Alternative& arg)
        {
            if constexpr (std::is_same_v<Alternative, formalism::ObjectView>)
            {
                policy.on_object(pos, arg, predicate);
            }
            else if constexpr (std::is_same_v<Alternative, formalism::ParameterIndex>)
            {
                policy.on_parameter(pos, arg, predicate);
            }
            else
            {
                static_assert(false, "Missing case");
            }
        },
        element.get_terms());
}

template<formalism::FactKind T, typename Policy>
void apply_policy(formalism::LiteralView<LiftedTag, T> element, Policy& policy)
{
    if (policy.should_skip(element))
        return;

    apply_policy(element.get_atom(), policy);
}

template<formalism::FactKind T, typename Policy>
void apply_policy(formalism::FunctionTermView<LiftedTag, T> element, Policy& policy)
{
    const auto function = element.get_function();

    if (policy.should_skip(function))
        return;

    for_each_term_with_position(
        [&]<typename Alternative>(size_t pos, const Alternative& arg)
        {
            if constexpr (std::is_same_v<Alternative, formalism::ObjectView>)
            {
                policy.on_object(pos, arg, function);
            }
            else if constexpr (std::is_same_v<Alternative, formalism::ParameterIndex>)
            {
                policy.on_parameter(pos, arg, function);
            }
            else
            {
                static_assert(false, "Missing case");
            }
        },
        element.get_terms());
}

template<formalism::FactKind T, typename Policy>
void apply_policy(formalism::NumericEffectView<LiftedTag, T> element, Policy& policy)
{
    apply_policy(element.get_fterm(), policy);
    apply_policy(element.get_fexpr(), policy);
}

template<typename Policy>
void apply_policy(formalism::FunctionExpressionView<LiftedTag> element, Policy& policy)
{
    visit([&](auto&& arg) { apply_policy(arg, policy); }, element);
}

}  // namespace domain_steps

}  // namespace tyr::analysis

#endif
