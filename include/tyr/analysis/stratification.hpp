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

#ifndef TYR_ANALYSIS_STRATIFICATION_HPP_
#define TYR_ANALYSIS_STRATIFICATION_HPP_

#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/function_index.hpp"
#include "tyr/formalism/predicate_index.hpp"
// for Program (ptr only), Rule

#include <vector>
#include <yggdrasil/core/types.hpp>
#include <yggdrasil/formatting/cista_formatters.hpp>

namespace tyr::analysis
{

template<::tyr::formalism::RelationKind R>
using TypedRuleStratum = ygg::IndexList<::tyr::formalism::datalog::Rule<R>>;

struct RuleStratum
{
    TypedRuleStratum<::tyr::formalism::PredicateTag> predicate_rules;
    TypedRuleStratum<::tyr::formalism::FunctionTag> function_rules;

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

struct RuleStrata
{
    std::vector<RuleStratum> data;
};

RuleStrata compute_rule_stratification(::tyr::formalism::datalog::ProgramView<LiftedTag> program);
}

#endif
