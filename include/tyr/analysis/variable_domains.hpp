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

#ifndef TYR_ANALYSIS_VARIABLE_DOMAINS_HPP_
#define TYR_ANALYSIS_VARIABLE_DOMAINS_HPP_

#include "tyr/analysis/declarations.hpp"

namespace tyr::analysis
{

struct ConditionalEffectDomainData
{
    ConjunctiveConditionDomain condition_domain;
    ConjunctiveEffectDomain effect_domain;
};

struct ActionDomainData
{
    ConjunctiveConditionDomain precondition_domain;
    ConditionalEffectDomainMap effect_domains;
};

struct ProgramVariableDomains
{
    PredicateDomainMap<formalism::StaticTag> static_predicate_domains;
    PredicateDomainMap<formalism::FluentTag> fluent_predicate_domains;
    FunctionDomainMap<formalism::StaticTag> static_function_domains;
    FunctionDomainMap<formalism::FluentTag> fluent_function_domains;
    RuleDomainMap rule_domains;
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

}

#endif
