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
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/repository.hpp"

#include <string_view>
#include <urd/elements/concepts.hpp>
#include <urd/repr/maps/unordered_flat_map.hpp>

namespace tyr::analysis
{

struct TaskVariableDomainsView
{
    using C = formalism::Repository;

    PredicateDomainViewMap<formalism::StaticTag, C> static_predicate_domains;
    PredicateDomainViewMap<formalism::FluentTag, C> fluent_predicate_domains;
    PredicateDomainViewMap<formalism::DerivedTag, C> derived_predicate_domains;
    FunctionDomainViewMap<formalism::StaticTag, C> static_function_domains;
    FunctionDomainViewMap<formalism::FluentTag, C> fluent_function_domains;
    ActionDomainViewMap<C> action_domains;
    AxiomDomainViewMap<C> axiom_domains;

    static constexpr std::string_view name = "TaskVariableDomainsView";

    static constexpr auto fields() noexcept
    {
        return urd::Fields<urd::field<"static_predicate_domains", &TaskVariableDomainsView::static_predicate_domains>,
                           urd::field<"fluent_predicate_domains", &TaskVariableDomainsView::fluent_predicate_domains>,
                           urd::field<"derived_predicate_domains", &TaskVariableDomainsView::derived_predicate_domains>,
                           urd::field<"static_function_domains", &TaskVariableDomainsView::static_function_domains>,
                           urd::field<"fluent_function_domains", &TaskVariableDomainsView::fluent_function_domains>,
                           urd::field<"action_domains", &TaskVariableDomainsView::action_domains>,
                           urd::field<"axiom_domains", &TaskVariableDomainsView::axiom_domains>> {};
    }
};

}

#endif
