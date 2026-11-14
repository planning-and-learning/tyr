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

#include <algorithm>
#include <cstddef>
#include <urd/repr/maps/unordered_flat_map.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/repr/values/unordered_flat_set.hpp>
#include <urd/store.hpp>
#include <utility>

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

}  // namespace tyr::analysis

#endif
