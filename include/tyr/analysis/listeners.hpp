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

#ifndef TYR_ANALYSIS_LISTENERS_HPP_
#define TYR_ANALYSIS_LISTENERS_HPP_

#include "tyr/analysis/declarations.hpp"
#include "tyr/analysis/stratification.hpp"
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/declarations.hpp"

#include <concepts>
#include <urd/elements/equal_to.hpp>
#include <urd/elements/flat.hpp>
#include <urd/elements/hash.hpp>
#include <urd/repr/maps/std_map.hpp>
#include <urd/repr/maps/unordered_flat_map.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/repr/values/std_set.hpp>
#include <urd/repr/values/unordered_flat_set.hpp>
#include <urd/store.hpp>
#include <vector>
#include <yggdrasil/core/family.hpp>

namespace tyr::analysis
{

using RuleIndexSet = urd::Store<urd::set<urd::index<formalism::Rule<LiftedTag>>>>;

/// The rules of a stratum that read the predicates of kind F, by predicate.
template<formalism::FactKind F>
using PredicateListeners = urd::Store<urd::map<urd::index<formalism::Predicate<F>>, RuleIndexSet>>;

/// The rules of a stratum that read each fluent and derived predicate (a positive literal) and each fluent function.
struct TypedListenerStratum
{
    PredicateListeners<formalism::FluentTag> fluent_predicates;
    PredicateListeners<formalism::DerivedTag> derived_predicates;
    urd::Store<urd::map<urd::index<formalism::Function<formalism::FluentTag>>, RuleIndexSet>> functions;

    /// The listeners of the predicates of kind F, fluent or derived.
    template<formalism::FactKind F>
    [[nodiscard]] const PredicateListeners<F>& get_predicates() const noexcept
    {
        if constexpr (std::same_as<F, formalism::FluentTag>)
            return fluent_predicates;
        else
            return derived_predicates;
    }

    template<formalism::FactKind F>
    PredicateListeners<F>& get_predicates() noexcept
    {
        if constexpr (std::same_as<F, formalism::FluentTag>)
            return fluent_predicates;
        else
            return derived_predicates;
    }
};

using ListenerStratum = TypedListenerStratum;

struct ListenerStrata
{
    urd::Store<urd::sequence<ListenerStratum>> data;
};

ListenerStrata compute_listeners(const RuleStrata& strata, const formalism::Repository& context);
}

#endif
