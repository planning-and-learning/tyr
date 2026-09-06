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

#include "tyr/algorithms/kckp/kckp.hpp"
#include "tyr/analysis/variable_domain.hpp"
#include "tyr/formalism/datalog/indices.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/object_index.hpp"
#include "tyr/formalism/planning/indices.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <vector>
#include <yggdrasil/containers/associative_containers.hpp>
#include <yggdrasil/containers/optional.hpp>
#include <yggdrasil/containers/variant.hpp>
#include <yggdrasil/containers/vector.hpp>
#include <yggdrasil/core/types.hpp>
#include <yggdrasil/core/types_utils.hpp>
#include <yggdrasil/semantics/equal_to.hpp>
#include <yggdrasil/semantics/hash.hpp>

namespace tyr::analysis
{

/**
 * ygg::Index based internal representation of variable domains.
 */

template<typename Element, typename Payload>
struct Scoped
{
    ygg::Index<Element> element;
    Payload payload;
};

template<typename Element>
using SimpleScopedDomain = Scoped<Element, VariableDomainList>;

template<typename Element>
using SimpleScopedDomainMap = ygg::UnorderedMap<ygg::Index<Element>, VariableDomainList>;

template<typename Element>
using ScopedDomainMap = ygg::UnorderedMap<ygg::Index<Element>, SimpleScopedDomain<Element>>;

template<formalism::FactKind T>
using PredicateDomainMap = SimpleScopedDomainMap<formalism::Predicate<T>>;

template<formalism::FactKind T>
using FunctionDomainMap = SimpleScopedDomainMap<formalism::Function<T>>;

template<formalism::RelationKind R>
using RuleDomainMap = ScopedDomainMap<formalism::datalog::Rule<LiftedTag, R>>;

using AxiomDomainMap = ScopedDomainMap<formalism::planning::Axiom<LiftedTag>>;

using ConjunctiveConditionDomain = SimpleScopedDomain<formalism::planning::ConjunctiveCondition<LiftedTag>>;

using ConjunctiveEffectDomain = SimpleScopedDomain<formalism::planning::ConjunctiveEffect<LiftedTag>>;

struct ConditionalEffectDomainData
{
    ConjunctiveConditionDomain condition_domain;
    ConjunctiveEffectDomain effect_domain;
    kckp::Graph compatibility_graph;
    std::vector<std::vector<kckp::Vertex>> object_to_vertex;
};

using ConditionalEffectDomain = Scoped<formalism::planning::ConditionalEffect<LiftedTag>, ConditionalEffectDomainData>;

using ConditionalEffectDomainMap = ygg::UnorderedMap<ygg::Index<formalism::planning::ConditionalEffect<LiftedTag>>, ConditionalEffectDomain>;

struct CompatibilityWorkspace
{
    kckp::Workspace kckp;
    std::vector<kckp::Vertex> vertex_prefix;
};

struct ActionDomainData
{
    ConjunctiveConditionDomain precondition_domain;
    ConditionalEffectDomainMap effect_domains;
};

using ActionDomain = Scoped<formalism::planning::Action<LiftedTag>, ActionDomainData>;

using ActionDomainMap = ygg::UnorderedMap<ygg::Index<formalism::planning::Action<LiftedTag>>, ActionDomain>;

struct ProgramVariableDomains
{
    PredicateDomainMap<formalism::StaticTag> static_predicate_domains;
    PredicateDomainMap<formalism::FluentTag> fluent_predicate_domains;
    FunctionDomainMap<formalism::StaticTag> static_function_domains;
    FunctionDomainMap<formalism::FluentTag> fluent_function_domains;
    RuleDomainMap<formalism::PredicateTag> predicate_rule_domains;
    RuleDomainMap<formalism::FunctionTag> function_rule_domains;

    template<formalism::RelationKind R>
    auto& get_rule_domains() noexcept
    {
        if constexpr (std::same_as<R, formalism::PredicateTag>)
            return predicate_rule_domains;
        else if constexpr (std::same_as<R, formalism::FunctionTag>)
            return function_rule_domains;
        else
            static_assert(ygg::dependent_false<R>::value, "Missing case");
    }

    template<formalism::RelationKind R>
    const auto& get_rule_domains() const noexcept
    {
        if constexpr (std::same_as<R, formalism::PredicateTag>)
            return predicate_rule_domains;
        else if constexpr (std::same_as<R, formalism::FunctionTag>)
            return function_rule_domains;
        else
            static_assert(ygg::dependent_false<R>::value, "Missing case");
    }
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
 * ygg::View based external representation of variable domains.
 */

template<typename Element, typename Payload, typename C>
struct ScopedView
{
    ygg::View<ygg::Index<Element>, C> element;
    Payload payload;
};

template<typename C>
struct VariableDomainView
{
    std::vector<ygg::View<ygg::Index<formalism::Object>, C>> objects;

    auto begin() noexcept { return objects.begin(); }
    auto end() noexcept { return objects.end(); }
    auto begin() const noexcept { return objects.begin(); }
    auto end() const noexcept { return objects.end(); }

    auto size() const noexcept { return objects.size(); }
    bool empty() const noexcept { return objects.empty(); }

    auto& operator[](std::size_t i) noexcept { return objects[i]; }
    const auto& operator[](std::size_t i) const noexcept { return objects[i]; }
};

template<typename C>
using VariableDomainViewList = std::vector<VariableDomainView<C>>;

template<typename Element, typename C>
using SimpleScopedDomainView = ScopedView<Element, VariableDomainViewList<C>, C>;

template<typename Element, typename C>
using SimpleScopedDomainViewMap = ygg::UnorderedMap<ygg::View<ygg::Index<Element>, C>, VariableDomainViewList<C>>;

template<typename Element, typename C>
using ScopedDomainViewMap = ygg::UnorderedMap<ygg::View<ygg::Index<Element>, C>, SimpleScopedDomainView<Element, C>>;

template<formalism::FactKind T, typename C>
using PredicateDomainViewMap = SimpleScopedDomainViewMap<formalism::Predicate<T>, C>;

template<formalism::FactKind T, typename C>
using FunctionDomainViewMap = SimpleScopedDomainViewMap<formalism::Function<T>, C>;

template<formalism::RelationKind R, typename C>
using RuleDomainViewMap = ScopedDomainViewMap<formalism::datalog::Rule<LiftedTag, R>, C>;

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
    ygg::UnorderedMap<ygg::View<ygg::Index<formalism::planning::ConditionalEffect<LiftedTag>>, C>, ConditionalEffectDomainView<C>>;

template<typename C>
struct ActionDomainViewData
{
    ConjunctiveConditionDomainView<C> precondition_domain;
    ConditionalEffectDomainViewMap<C> effect_domains;
};

template<typename C>
using ActionDomainView = ScopedView<formalism::planning::Action<LiftedTag>, ActionDomainViewData<C>, C>;

template<typename C>
using ActionDomainViewMap = ygg::UnorderedMap<ygg::View<ygg::Index<formalism::planning::Action<LiftedTag>>, C>, ActionDomainView<C>>;

struct ProgramVariableDomainsView
{
    using C = formalism::datalog::Repository;

    PredicateDomainViewMap<formalism::StaticTag, C> static_predicate_domains;
    PredicateDomainViewMap<formalism::FluentTag, C> fluent_predicate_domains;
    FunctionDomainViewMap<formalism::StaticTag, C> static_function_domains;
    FunctionDomainViewMap<formalism::FluentTag, C> fluent_function_domains;
    RuleDomainViewMap<formalism::PredicateTag, C> predicate_rule_domains;
    RuleDomainViewMap<formalism::FunctionTag, C> function_rule_domains;

    template<formalism::RelationKind R>
    const auto& get_rule_domains() const noexcept
    {
        if constexpr (std::same_as<R, formalism::PredicateTag>)
            return predicate_rule_domains;
        else if constexpr (std::same_as<R, formalism::FunctionTag>)
            return function_rule_domains;
        else
            static_assert(ygg::dependent_false<R>::value, "Missing case");
    }
};

struct TaskVariableDomainsView
{
    using C = formalism::planning::Repository;

    PredicateDomainViewMap<formalism::StaticTag, C> static_predicate_domains;
    PredicateDomainViewMap<formalism::FluentTag, C> fluent_predicate_domains;
    PredicateDomainViewMap<formalism::DerivedTag, C> derived_predicate_domains;
    FunctionDomainViewMap<formalism::StaticTag, C> static_function_domains;
    FunctionDomainViewMap<formalism::FluentTag, C> fluent_function_domains;
    ActionDomainViewMap<C> action_domains;
    AxiomDomainViewMap<C> axiom_domains;
};

/**
 * Stratification
 */

struct RuleStrata;
}

#endif
