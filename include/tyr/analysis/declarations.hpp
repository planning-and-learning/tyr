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

#ifndef TYR_ANALYSIS_DECLARATIONS_HPP_
#define TYR_ANALYSIS_DECLARATIONS_HPP_

#include "tyr/analysis/variable_domain.hpp"
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/planning/declarations.hpp"

#include <urd/elements/equal_to.hpp>
#include <urd/elements/hash.hpp>
#include <urd/utils/family.hpp>
#include <urd/repr/maps/std_map.hpp>
#include <urd/repr/maps/unordered_flat_map.hpp>
#include <urd/repr/values/unordered_flat_set.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <vector>
#include <urd/elements/flat.hpp>

namespace tyr::analysis
{

/**
 * urd::index based internal representation of variable domains.
 */

template<typename Element, typename Payload>
struct Scoped
{
    urd::index<Element> element;
    Payload payload;
};

template<typename Element>
using SimpleScopedDomain = Scoped<Element, VariableDomainList>;

template<typename Element>
using SimpleScopedDomainMap = urd::Store<urd::map<urd::index<Element>, VariableDomainList>>;

template<typename Element>
using ScopedDomainMap = urd::Store<urd::map<urd::index<Element>, SimpleScopedDomain<Element>>>;

template<formalism::FactKind T>
using PredicateDomainMap = SimpleScopedDomainMap<formalism::Predicate<T>>;

template<formalism::FactKind T>
using FunctionDomainMap = SimpleScopedDomainMap<formalism::Function<T>>;

template<typename R>
using RuleDomainMap = ScopedDomainMap<formalism::datalog::Rule<LiftedTag, R>>;

using AxiomDomainMap = ScopedDomainMap<formalism::planning::Axiom<LiftedTag>>;

using ConjunctiveConditionDomain = SimpleScopedDomain<formalism::planning::ConjunctiveCondition<LiftedTag>>;

using ConjunctiveEffectDomain = SimpleScopedDomain<formalism::planning::ConjunctiveEffect<LiftedTag>>;

struct ConditionalEffectDomainData
{
    ConjunctiveConditionDomain condition_domain;
    ConjunctiveEffectDomain effect_domain;
};

using ConditionalEffectDomain = Scoped<formalism::planning::ConditionalEffect<LiftedTag>, ConditionalEffectDomainData>;

using ConditionalEffectDomainMap = urd::Store<urd::map<urd::index<formalism::planning::ConditionalEffect<LiftedTag>>, ConditionalEffectDomain>>;

struct ActionDomainData
{
    ConjunctiveConditionDomain precondition_domain;
    ConditionalEffectDomainMap effect_domains;
};

using ActionDomain = Scoped<formalism::planning::Action<LiftedTag>, ActionDomainData>;

using ActionDomainMap = urd::Store<urd::map<urd::index<formalism::planning::Action<LiftedTag>>, ActionDomain>>;

struct ProgramVariableDomains
{
    PredicateDomainMap<formalism::StaticTag> static_predicate_domains;
    PredicateDomainMap<formalism::FluentTag> fluent_predicate_domains;
    FunctionDomainMap<formalism::StaticTag> static_function_domains;
    FunctionDomainMap<formalism::FluentTag> fluent_function_domains;
    urd::utils::Family<RuleDomainMap, formalism::PredicateTag, formalism::FunctionTag> rule_domains;
};

struct TaskVariableDomains
{
    PredicateDomainMap<formalism::StaticTag> static_predicate_domains;
    PredicateDomainMap<formalism::FluentTag> fluent_predicate_domains;
    PredicateDomainMap<formalism::DerivedTag> derived_predicate_domains;
    FunctionDomainMap<formalism::StaticTag> static_function_domains;
    FunctionDomainMap<formalism::FluentTag> fluent_function_domains;
    ActionDomainMap action_domains;
    AxiomDomainMap axiom_domains;
};

/**
 * View based external representation of variable domains.
 */

template<typename Element, typename Payload, typename C>
struct ScopedView
{
    urd::index_view<Element, C> element;
    Payload payload;
};

template<typename C>
struct VariableDomainView
{
    urd::Store<urd::sequence<urd::index_view<formalism::Object, C>>> objects;

    auto begin() noexcept { return objects.begin(); }
    auto end() noexcept { return objects.end(); }
    [[nodiscard]] auto begin() const noexcept { return objects.begin(); }
    [[nodiscard]] auto end() const noexcept { return objects.end(); }

    [[nodiscard]] auto size() const noexcept { return objects.size(); }
    [[nodiscard]] bool empty() const noexcept { return objects.empty(); }

    auto& operator[](std::size_t i) noexcept { return objects[i]; }
    const auto& operator[](std::size_t i) const noexcept { return objects[i]; }
};

template<typename C>
using VariableDomainViewList = urd::Store<urd::sequence<VariableDomainView<C>>>;

template<typename Element, typename C>
using SimpleScopedDomainView = ScopedView<Element, VariableDomainViewList<C>, C>;

template<typename Element, typename C>
using SimpleScopedDomainViewMap = urd::Store<urd::map<urd::index_view<Element, C>, VariableDomainViewList<C>>>;

template<typename Element, typename C>
using ScopedDomainViewMap = urd::Store<urd::map<urd::index_view<Element, C>, SimpleScopedDomainView<Element, C>>>;

template<formalism::FactKind T, typename C>
using PredicateDomainViewMap = SimpleScopedDomainViewMap<formalism::Predicate<T>, C>;

template<formalism::FactKind T, typename C>
using FunctionDomainViewMap = SimpleScopedDomainViewMap<formalism::Function<T>, C>;

template<typename C>
using AxiomDomainViewMap = ScopedDomainViewMap<formalism::planning::Axiom<LiftedTag>, C>;

template<typename C>
using ConjunctiveConditionDomainView = SimpleScopedDomainView<formalism::planning::ConjunctiveCondition<LiftedTag>, C>;

template<typename C>
using ConjunctiveEffectDomainView = SimpleScopedDomainView<formalism::planning::ConjunctiveEffect<LiftedTag>, C>;

template<typename C>
struct ConditionalEffectDomainViewData
{
    ConjunctiveConditionDomainView<C> condition_domain;
    ConjunctiveEffectDomainView<C> effect_domain;
};

template<typename C>
using ConditionalEffectDomainView = ScopedView<formalism::planning::ConditionalEffect<LiftedTag>, ConditionalEffectDomainViewData<C>, C>;

template<typename C>
using ConditionalEffectDomainViewMap =
    urd::Store<urd::map<urd::index_view<formalism::planning::ConditionalEffect<LiftedTag>, C>, ConditionalEffectDomainView<C>>>;

template<typename C>
struct ActionDomainViewData
{
    ConjunctiveConditionDomainView<C> precondition_domain;
    ConditionalEffectDomainViewMap<C> effect_domains;
};

template<typename C>
using ActionDomainView = ScopedView<formalism::planning::Action<LiftedTag>, ActionDomainViewData<C>, C>;

template<typename C>
using ActionDomainViewMap = urd::Store<urd::map<urd::index_view<formalism::planning::Action<LiftedTag>, C>, ActionDomainView<C>>>;

/**
 * Stratification
 */

struct RuleStrata;
}

#endif
