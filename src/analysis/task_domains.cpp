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

#include "tyr/analysis/domains.hpp"
#include "tyr/formalism/planning/datas.hpp"
#include "tyr/formalism/planning/formatter.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/planning/views.hpp"

#include <algorithm>
#include <assert.h>
#include <gtl/phmap.hpp>
#include <stddef.h>
#include <type_traits>
#include <utility>
#include <vector>
#include <yggdrasil/containers/unordered_set.hpp>
#include <yggdrasil/containers/variant.hpp>
#include <yggdrasil/containers/vector.hpp>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/core/types.hpp>
#include <yggdrasil/formatting/formatter.hpp>
#include <yggdrasil/ids/index_mixins.hpp>

namespace f = tyr::formalism;
namespace fp = tyr::formalism::planning;

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
    ygg::UnorderedSet<ygg::Index<::tyr::formalism::Object>> objects;
};

using TmpVariableDomainList = std::vector<TmpVariableDomain>;

template<typename Element>
using TmpSimpleScopedDomain = TmpScoped<Element, TmpVariableDomainList>;

template<typename Element>
using TmpSimpleScopedDomainMap = ygg::UnorderedMap<ygg::Index<Element>, TmpVariableDomainList>;

template<::tyr::formalism::FactKind T>
using TmpPredicateDomainMap = TmpSimpleScopedDomainMap<::tyr::formalism::Predicate<T>>;

template<::tyr::formalism::FactKind T>
using TmpFunctionDomainMap = TmpSimpleScopedDomainMap<::tyr::formalism::Function<T>>;

using TmpAxiomDomainMap = TmpSimpleScopedDomainMap<::tyr::formalism::planning::Axiom>;

using TmpConjunctiveConditionDomain = TmpSimpleScopedDomain<::tyr::formalism::planning::ConjunctiveCondition>;
using TmpConjunctiveEffectDomain = TmpSimpleScopedDomain<::tyr::formalism::planning::ConjunctiveEffect>;

struct TmpConditionalEffectDomain
{
    TmpConjunctiveConditionDomain condition_domain;
    TmpConjunctiveEffectDomain effect_domain;
};

using TmpConditionalEffectDomainMap = ygg::UnorderedMap<ygg::Index<::tyr::formalism::planning::ConditionalEffect>, TmpConditionalEffectDomain>;

struct TmpActionDomain
{
    TmpConjunctiveConditionDomain precondition_domain;
    TmpConditionalEffectDomainMap effect_domains;
};

using TmpActionDomainMap = ygg::UnorderedMap<ygg::Index<::tyr::formalism::planning::Action>, TmpActionDomain>;

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

AxiomDomainMap to_axiom_domain_map(const TmpAxiomDomainMap& domains)
{
    auto result = AxiomDomainMap {};
    result.reserve(domains.size());

    for (const auto& [axiom, variable_domains] : domains)
    {
        result.emplace(axiom,
                       SimpleScopedDomain<::tyr::formalism::planning::Axiom> {
                           axiom,
                           to_variable_domain_list(variable_domains),
                       });
    }

    return result;
}

ActionDomainMap to_action_domain_map(const TmpActionDomainMap& domains)
{
    auto result = ActionDomainMap {};
    result.reserve(domains.size());

    for (const auto& [action, action_domain] : domains)
    {
        auto effect_domains = ConditionalEffectDomainMap {};
        effect_domains.reserve(action_domain.effect_domains.size());

        for (const auto& [c_effect, c_effect_domain] : action_domain.effect_domains)
        {
            effect_domains.emplace(c_effect,
                                   ConditionalEffectDomain {
                                       c_effect,
                                       ConditionalEffectDomainData {
                                           ConjunctiveConditionDomain {
                                               c_effect_domain.condition_domain.element,
                                               to_variable_domain_list(c_effect_domain.condition_domain.payload),
                                           },
                                           ConjunctiveEffectDomain {
                                               c_effect_domain.effect_domain.element,
                                               to_variable_domain_list(c_effect_domain.effect_domain.payload),
                                           },
                                       },
                                   });
        }

        result.emplace(action,
                       ActionDomain {
                           action,
                           ActionDomainData {
                               ConjunctiveConditionDomain {
                                   action_domain.precondition_domain.element,
                                   to_variable_domain_list(action_domain.precondition_domain.payload),
                               },
                               std::move(effect_domains),
                           },
                       });
    }

    return result;
}

/**
 * Initialization of temporary maps.
 */

template<f::FactKind T>
TmpPredicateDomainMap<T> initialize_predicate_domain_sets(fp::PredicateListView<T> predicates)
{
    auto predicate_domain_sets = TmpPredicateDomainMap<T> {};
    predicate_domain_sets.reserve(predicates.size());

    for (const auto predicate : predicates)
        predicate_domain_sets.emplace(predicate.get_index(), TmpVariableDomainList(predicate.get_arity()));

    return predicate_domain_sets;
}

template<f::FactKind T>
void insert_into_predicate_domain_sets(fp::GroundAtomListView<T> atoms, TmpPredicateDomainMap<T>& predicate_domain_sets)
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
TmpFunctionDomainMap<T> initialize_function_domain_sets(fp::FunctionListView<T> functions)
{
    auto function_domain_sets = TmpFunctionDomainMap<T> {};
    function_domain_sets.reserve(functions.size());

    for (const auto function : functions)
        function_domain_sets.emplace(function.get_index(), TmpVariableDomainList(function.get_arity()));

    return function_domain_sets;
}

template<f::FactKind T>
void insert_into_function_domain_sets(fp::GroundFunctionTermValueListView<T> fterm_values, TmpFunctionDomainMap<T>& function_domain_sets)
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
void for_each_term_with_position(Fn&& fn, fp::TermListView terms)
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
    TmpPredicateDomainMap<f::DerivedTag>& derived_predicate_domain_sets;
    TmpFunctionDomainMap<f::StaticTag>& static_function_domain_sets;
    TmpFunctionDomainMap<f::FluentTag>& fluent_function_domain_sets;

    template<typename Element>
    bool should_skip(Element) const
    {
        return false;
    }

    bool should_skip(fp::FunctionView<f::FluentTag>) const { return true; }

    auto& get_domains(fp::PredicateView<f::StaticTag>) { return static_predicate_domain_sets; }
    auto& get_domains(fp::PredicateView<f::FluentTag>) { return fluent_predicate_domain_sets; }
    auto& get_domains(fp::PredicateView<f::DerivedTag>) { return derived_predicate_domain_sets; }
    auto& get_domains(fp::FunctionView<f::StaticTag>) { return static_function_domain_sets; }
    auto& get_domains(fp::FunctionView<f::FluentTag>) { return fluent_function_domain_sets; }

    template<typename Symbol>
    void on_object(size_t pos, fp::ObjectView object, Symbol symbol)
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
    const TmpPredicateDomainMap<f::DerivedTag>& derived_predicate_domain_sets;
    const TmpFunctionDomainMap<f::StaticTag>& static_function_domain_sets;
    const TmpFunctionDomainMap<f::FluentTag>& fluent_function_domain_sets;
    TmpVariableDomainList& parameter_domains;

    template<typename Element>
    bool should_skip(Element) const
    {
        return false;
    }

    template<f::FactKind T>
    bool should_skip(fp::LiteralView<T> literal) const
    {
        return !literal.get_polarity();
    }

    bool should_skip(fp::FunctionView<f::FluentTag>) const { return true; }

    const auto& get_domains(fp::PredicateView<f::StaticTag>) const { return static_predicate_domain_sets; }
    const auto& get_domains(fp::PredicateView<f::FluentTag>) const { return fluent_predicate_domain_sets; }
    const auto& get_domains(fp::PredicateView<f::DerivedTag>) const { return derived_predicate_domain_sets; }
    const auto& get_domains(fp::FunctionView<f::StaticTag>) const { return static_function_domain_sets; }
    const auto& get_domains(fp::FunctionView<f::FluentTag>) const { return fluent_function_domain_sets; }

    template<typename Symbol>
    void on_object(size_t, fp::ObjectView, Symbol)
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
    TmpPredicateDomainMap<f::DerivedTag>& derived_predicate_domain_sets;
    TmpFunctionDomainMap<f::StaticTag>& static_function_domain_sets;
    TmpFunctionDomainMap<f::FluentTag>& fluent_function_domain_sets;
    const TmpVariableDomainList& parameter_domains;

    template<typename Element>
    bool should_skip(Element) const
    {
        return false;
    }

    bool should_skip(fp::FunctionView<f::StaticTag>) const { return true; }

    auto& get_domains(fp::PredicateView<f::StaticTag>) { return static_predicate_domain_sets; }
    auto& get_domains(fp::PredicateView<f::FluentTag>) { return fluent_predicate_domain_sets; }
    auto& get_domains(fp::PredicateView<f::DerivedTag>) { return derived_predicate_domain_sets; }
    auto& get_domains(fp::FunctionView<f::StaticTag>) { return static_function_domain_sets; }
    auto& get_domains(fp::FunctionView<f::FluentTag>) { return fluent_function_domain_sets; }

    template<typename Symbol>
    void on_object(size_t pos, fp::ObjectView object, Symbol symbol)
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
void apply_policy(fp::FunctionExpressionView element, Policy& policy);

template<typename Policy>
void apply_policy(ygg::float_t, Policy&)
{
}

template<typename Policy>
void apply_policy(fp::LiftedUnaryOperatorView element, Policy& policy)
{
    apply_policy(element.get_arg(), policy);
}

template<f::BinaryOperatorKind O, typename Policy>
void apply_policy(fp::LiftedBinaryOperatorView<O> element, Policy& policy)
{
    apply_policy(element.get_lhs(), policy);
    apply_policy(element.get_rhs(), policy);
}

template<typename Policy>
void apply_policy(fp::LiftedMultiOperatorView element, Policy& policy)
{
    for (const auto arg : element.get_args())
        apply_policy(arg, policy);
}

template<f::FactKind T, typename Policy>
void apply_policy(fp::AtomView<T> element, Policy& policy)
{
    const auto predicate = element.get_predicate();

    if (policy.should_skip(predicate))
        return;

    for_each_term_with_position(
        [&](size_t pos, auto&& arg)
        {
            using Alternative = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<Alternative, fp::ObjectView>)
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
void apply_policy(fp::LiteralView<T> element, Policy& policy)
{
    if (policy.should_skip(element))
        return;

    apply_policy(element.get_atom(), policy);
}

template<f::FactKind T, typename Policy>
void apply_policy(fp::FunctionTermView<T> element, Policy& policy)
{
    const auto function = element.get_function();

    if (policy.should_skip(function))
        return;

    for_each_term_with_position(
        [&](size_t pos, auto&& arg)
        {
            using Alternative = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<Alternative, fp::ObjectView>)
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
void apply_policy(fp::NumericEffectView<T> element, Policy& policy)
{
    apply_policy(element.get_fterm(), policy);
    apply_policy(element.get_fexpr(), policy);
}

template<typename Policy>
void apply_policy(fp::LiftedArithmeticOperatorView element, Policy& policy)
{
    visit([&](auto&& arg) { apply_policy(arg, policy); }, element.get_variant());
}

template<typename Policy>
void apply_policy(fp::FunctionExpressionView element, Policy& policy)
{
    visit([&](auto&& arg) { apply_policy(arg, policy); }, element.get_variant());
}

template<typename Policy>
void apply_policy(fp::LiftedBooleanOperatorView element, Policy& policy)
{
    visit([&](auto&& arg) { apply_policy(arg, policy); }, element.get_variant());
}

template<f::FactKind T, typename Policy>
void apply_policy(fp::NumericEffectOperatorView<T> element, Policy& policy)
{
    visit([&](auto&& arg) { apply_policy(arg, policy); }, element.get_variant());
}

}  // namespace

TaskVariableDomains compute_variable_domains(fp::TaskView task)
{
    auto universe = ygg::UnorderedSet<ygg::Index<f::Object>> {};
    for (const auto object : task.get_domain().get_constants())
        universe.insert(object.get_index());
    for (const auto object : task.get_objects())
        universe.insert(object.get_index());

    ///--- Step 1: Initialize static, fluent, and derived predicate parameter domains

    auto static_predicate_domain_sets = initialize_predicate_domain_sets(task.get_domain().get_predicates<f::StaticTag>());
    auto fluent_predicate_domain_sets = initialize_predicate_domain_sets(task.get_domain().get_predicates<f::FluentTag>());

    auto derived_predicate_indices = ygg::IndexList<f::Predicate<f::DerivedTag>> {};
    for (const auto predicate : task.get_domain().get_predicates<f::DerivedTag>())
        derived_predicate_indices.push_back(predicate.get_index());
    for (const auto predicate : task.get_derived_predicates())
        derived_predicate_indices.push_back(predicate.get_index());

    const auto derived_predicates = ygg::make_view(derived_predicate_indices, task.get_context());
    auto derived_predicate_domain_sets = initialize_predicate_domain_sets(derived_predicates);

    insert_into_predicate_domain_sets(task.get_atoms<f::StaticTag>(), static_predicate_domain_sets);
    insert_into_predicate_domain_sets(task.get_atoms<f::FluentTag>(), fluent_predicate_domain_sets);

    ///--- Step 2: Initialize static and fluent function parameter domains

    auto static_function_domain_sets = initialize_function_domain_sets(task.get_domain().get_functions<f::StaticTag>());
    auto fluent_function_domain_sets = initialize_function_domain_sets(task.get_domain().get_functions<f::FluentTag>());

    insert_into_function_domain_sets(task.get_fterm_values<f::StaticTag>(), static_function_domain_sets);
    insert_into_function_domain_sets(task.get_fterm_values<f::FluentTag>(), fluent_function_domain_sets);

    ///--- Step 2.5: Important not to forget constants in schemas

    {
        auto insert_policy = InsertConstantPolicy {
            static_predicate_domain_sets, fluent_predicate_domain_sets, derived_predicate_domain_sets, static_function_domain_sets, fluent_function_domain_sets,
        };

        for (const auto action : task.get_domain().get_actions())
        {
            for (const auto literal : action.get_condition().get_literals<f::StaticTag>())
                apply_policy(literal.get_atom(), insert_policy);

            for (const auto op : action.get_condition().get_numeric_constraints())
                apply_policy(op, insert_policy);

            for (const auto c_effect : action.get_effects())
            {
                for (const auto literal : c_effect.get_condition().get_literals<f::StaticTag>())
                    apply_policy(literal.get_atom(), insert_policy);

                for (const auto literal : c_effect.get_condition().get_literals<f::FluentTag>())
                    apply_policy(literal.get_atom(), insert_policy);

                for (const auto op : c_effect.get_condition().get_numeric_constraints())
                    apply_policy(op, insert_policy);
            }
        }

        for (const auto axiom : task.get_domain().get_axioms())
        {
            for (const auto literal : axiom.get_body().get_literals<f::StaticTag>())
                apply_policy(literal.get_atom(), insert_policy);

            for (const auto op : axiom.get_body().get_numeric_constraints())
                apply_policy(op, insert_policy);
        }

        for (const auto axiom : task.get_axioms())
        {
            for (const auto literal : axiom.get_body().get_literals<f::StaticTag>())
                apply_policy(literal.get_atom(), insert_policy);

            for (const auto op : axiom.get_body().get_numeric_constraints())
                apply_policy(op, insert_policy);
        }
    }

    ///--- Step 3: Compute action and axiom parameter domains from static information.

    auto action_domain_sets = TmpActionDomainMap {};
    action_domain_sets.reserve(task.get_domain().get_actions().size());

    for (const auto action : task.get_domain().get_actions())
    {
        auto parameter_domains = TmpVariableDomainList(action.get_variables().size());
        for (auto& domain : parameter_domains)
            domain.objects = universe;

        auto restrict_policy = RestrictPolicy {
            static_predicate_domain_sets, fluent_predicate_domain_sets, derived_predicate_domain_sets,
            static_function_domain_sets,  fluent_function_domain_sets,  parameter_domains,
        };

        for (const auto literal : action.get_condition().get_literals<f::StaticTag>())
            apply_policy(literal, restrict_policy);

        for (const auto op : action.get_condition().get_numeric_constraints())
            apply_policy(op, restrict_policy);

        auto effect_domains = TmpConditionalEffectDomainMap {};
        effect_domains.reserve(action.get_effects().size());

        for (const auto c_effect : action.get_effects())
        {
            auto c_parameter_domains = parameter_domains;
            c_parameter_domains.resize(action.get_variables().size() + c_effect.get_variables().size());

            for (size_t i = parameter_domains.size(); i < c_parameter_domains.size(); ++i)
                c_parameter_domains[i].objects = universe;

            auto c_restrict_policy = RestrictPolicy {
                static_predicate_domain_sets, fluent_predicate_domain_sets, derived_predicate_domain_sets,
                static_function_domain_sets,  fluent_function_domain_sets,  c_parameter_domains,
            };

            for (const auto literal : c_effect.get_condition().get_literals<f::StaticTag>())
                apply_policy(literal, c_restrict_policy);

            for (const auto op : c_effect.get_condition().get_numeric_constraints())
                apply_policy(op, c_restrict_policy);

            effect_domains.emplace(c_effect.get_index(),
                                   TmpConditionalEffectDomain {
                                       TmpConjunctiveConditionDomain { c_effect.get_condition().get_index(), c_parameter_domains },
                                       TmpConjunctiveEffectDomain { c_effect.get_effect().get_index(), c_parameter_domains },
                                   });
        }

        action_domain_sets.emplace(action.get_index(),
                                   TmpActionDomain {
                                       TmpConjunctiveConditionDomain { action.get_condition().get_index(), parameter_domains },
                                       std::move(effect_domains),
                                   });
    }

    auto axiom_domain_sets = TmpAxiomDomainMap {};
    axiom_domain_sets.reserve(task.get_domain().get_axioms().size() + task.get_axioms().size());

    for (const auto axiom : task.get_domain().get_axioms())
    {
        auto parameter_domains = TmpVariableDomainList(axiom.get_body().get_variables().size());
        for (auto& domain : parameter_domains)
            domain.objects = universe;

        auto restrict_policy = RestrictPolicy {
            static_predicate_domain_sets, fluent_predicate_domain_sets, derived_predicate_domain_sets,
            static_function_domain_sets,  fluent_function_domain_sets,  parameter_domains,
        };

        for (const auto literal : axiom.get_body().get_literals<f::StaticTag>())
            apply_policy(literal, restrict_policy);

        for (const auto op : axiom.get_body().get_numeric_constraints())
            apply_policy(op, restrict_policy);

        axiom_domain_sets.emplace(axiom.get_index(), std::move(parameter_domains));
    }

    for (const auto axiom : task.get_axioms())
    {
        auto parameter_domains = TmpVariableDomainList(axiom.get_body().get_variables().size());
        for (auto& domain : parameter_domains)
            domain.objects = universe;

        auto restrict_policy = RestrictPolicy {
            static_predicate_domain_sets, fluent_predicate_domain_sets, derived_predicate_domain_sets,
            static_function_domain_sets,  fluent_function_domain_sets,  parameter_domains,
        };

        for (const auto literal : axiom.get_body().get_literals<f::StaticTag>())
            apply_policy(literal, restrict_policy);

        for (const auto op : axiom.get_body().get_numeric_constraints())
            apply_policy(op, restrict_policy);

        axiom_domain_sets.emplace(axiom.get_index(), std::move(parameter_domains));
    }

    ///--- Step 4: Lift predicate/function domains given the variable relationships in actions and axioms.

    for (const auto action : task.get_domain().get_actions())
    {
        auto& action_domain = action_domain_sets.at(action.get_index());
        auto& parameter_domains = action_domain.precondition_domain.payload;

        auto lift_policy = LiftPolicy {
            static_predicate_domain_sets, fluent_predicate_domain_sets, derived_predicate_domain_sets,
            static_function_domain_sets,  fluent_function_domain_sets,  parameter_domains,
        };

        for (const auto literal : action.get_condition().get_literals<f::StaticTag>())
            apply_policy(literal, lift_policy);

        for (const auto literal : action.get_condition().get_literals<f::FluentTag>())
            apply_policy(literal, lift_policy);

        for (const auto literal : action.get_condition().get_literals<f::DerivedTag>())
            apply_policy(literal, lift_policy);

        for (const auto op : action.get_condition().get_numeric_constraints())
            apply_policy(op, lift_policy);

        for (const auto c_effect : action.get_effects())
        {
            auto& c_effect_domain = action_domain.effect_domains.at(c_effect.get_index());
            auto& c_parameter_domains = c_effect_domain.condition_domain.payload;

            auto c_lift_policy = LiftPolicy {
                static_predicate_domain_sets, fluent_predicate_domain_sets, derived_predicate_domain_sets,
                static_function_domain_sets,  fluent_function_domain_sets,  c_parameter_domains,
            };

            for (const auto literal : c_effect.get_condition().get_literals<f::StaticTag>())
                apply_policy(literal, c_lift_policy);

            for (const auto literal : c_effect.get_condition().get_literals<f::FluentTag>())
                apply_policy(literal, c_lift_policy);

            for (const auto op : c_effect.get_condition().get_numeric_constraints())
                apply_policy(op, c_lift_policy);

            for (const auto literal : c_effect.get_effect().get_literals())
                apply_policy(literal, c_lift_policy);

            for (const auto op : c_effect.get_effect().get_numeric_effects())
                apply_policy(op, c_lift_policy);

            // Keep effect payload aligned with condition payload, as in the original approximation.
            c_effect_domain.effect_domain.payload = c_effect_domain.condition_domain.payload;
        }
    }

    for (const auto axiom : task.get_domain().get_axioms())
    {
        auto& parameter_domains = axiom_domain_sets.at(axiom.get_index());

        auto lift_policy = LiftPolicy {
            static_predicate_domain_sets, fluent_predicate_domain_sets, derived_predicate_domain_sets,
            static_function_domain_sets,  fluent_function_domain_sets,  parameter_domains,
        };

        for (const auto literal : axiom.get_body().get_literals<f::StaticTag>())
            apply_policy(literal, lift_policy);

        for (const auto literal : axiom.get_body().get_literals<f::FluentTag>())
            apply_policy(literal, lift_policy);

        for (const auto literal : axiom.get_body().get_literals<f::DerivedTag>())
            apply_policy(literal, lift_policy);

        for (const auto op : axiom.get_body().get_numeric_constraints())
            apply_policy(op, lift_policy);

        apply_policy(axiom.get_head(), lift_policy);
    }

    for (const auto axiom : task.get_axioms())
    {
        auto& parameter_domains = axiom_domain_sets.at(axiom.get_index());

        auto lift_policy = LiftPolicy {
            static_predicate_domain_sets, fluent_predicate_domain_sets, derived_predicate_domain_sets,
            static_function_domain_sets,  fluent_function_domain_sets,  parameter_domains,
        };

        for (const auto literal : axiom.get_body().get_literals<f::StaticTag>())
            apply_policy(literal, lift_policy);

        for (const auto literal : axiom.get_body().get_literals<f::FluentTag>())
            apply_policy(literal, lift_policy);

        for (const auto literal : axiom.get_body().get_literals<f::DerivedTag>())
            apply_policy(literal, lift_policy);

        for (const auto op : axiom.get_body().get_numeric_constraints())
            apply_policy(op, lift_policy);

        apply_policy(axiom.get_head(), lift_policy);
    }

    ///--- Step 5: Convert internal sets to public domain wrapper types.

    auto static_predicate_domains = to_predicate_domain_map(static_predicate_domain_sets);
    auto fluent_predicate_domains = to_predicate_domain_map(fluent_predicate_domain_sets);
    auto derived_predicate_domains = to_predicate_domain_map(derived_predicate_domain_sets);
    auto static_function_domains = to_function_domain_map(static_function_domain_sets);
    auto fluent_function_domains = to_function_domain_map(fluent_function_domain_sets);
    auto action_domains = to_action_domain_map(action_domain_sets);
    auto axiom_domains = to_axiom_domain_map(axiom_domain_sets);

    return TaskVariableDomains {
        std::move(static_predicate_domains),
        std::move(fluent_predicate_domains),
        std::move(derived_predicate_domains),
        std::move(static_function_domains),
        std::move(fluent_function_domains),
        std::move(action_domains),
        std::move(axiom_domains),
    };
}

}  // namespace tyr::analysis
