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
#include "tyr/formalism/function_index.hpp"
#include "tyr/formalism/predicate_index.hpp"

#include <vector>
#include <yggdrasil/containers/associative_containers.hpp>
#include <yggdrasil/core/types.hpp>
#include <yggdrasil/semantics/equal_to.hpp>
#include <yggdrasil/semantics/hash.hpp>

namespace tyr::analysis
{

template<::tyr::formalism::RelationKind R>
using RuleIndexSet = ygg::UnorderedSet<ygg::Index<::tyr::formalism::datalog::Rule<::tyr::LiftedTag, R>>>;

template<::tyr::formalism::RelationKind R>
struct TypedListenerStratum
{
    ygg::UnorderedMap<ygg::Index<::tyr::formalism::Predicate<::tyr::formalism::FluentTag>>, RuleIndexSet<R>> predicates;
    ygg::UnorderedMap<ygg::Index<::tyr::formalism::Function<::tyr::formalism::FluentTag>>, RuleIndexSet<R>> functions;
};

struct ListenerStratum
{
    TypedListenerStratum<::tyr::formalism::PredicateTag> predicate_rules;
    TypedListenerStratum<::tyr::formalism::FunctionTag> function_rules;

    auto& get(::tyr::formalism::PredicateTag) noexcept { return predicate_rules; }
    auto& get(::tyr::formalism::FunctionTag) noexcept { return function_rules; }
    const auto& get(::tyr::formalism::PredicateTag) const noexcept { return predicate_rules; }
    const auto& get(::tyr::formalism::FunctionTag) const noexcept { return function_rules; }

    template<::tyr::formalism::RelationKind R>
    auto& get() noexcept
    {
        return get(R {});
    }

    template<::tyr::formalism::RelationKind R>
    const auto& get() const noexcept
    {
        return get(R {});
    }
};

struct ListenerStrata
{
    std::vector<ListenerStratum> data;
};

ListenerStrata compute_listeners(const RuleStrata& strata, const ::tyr::formalism::datalog::Repository& context);
}

#endif
