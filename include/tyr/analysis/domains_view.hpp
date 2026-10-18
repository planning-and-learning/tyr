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

#ifndef TYR_ANALYSIS_DOMAINS_VIEW_HPP_
#define TYR_ANALYSIS_DOMAINS_VIEW_HPP_

#include "tyr/analysis/declarations.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <concepts>
#include <yggdrasil/core/dependent_false.hpp>

namespace tyr::analysis
{

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

}

#endif
