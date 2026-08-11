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

#ifndef TYR_DATALOG_POLICIES_TERMINATION_HPP_
#define TYR_DATALOG_POLICIES_TERMINATION_HPP_

#include "tyr/datalog/declarations.hpp"
#include "tyr/datalog/fact_sets.hpp"
#include "tyr/datalog/policies/aggregation.hpp"
#include "tyr/datalog/policies/numeric_support.hpp"
#include "tyr/datalog/policies/termination_concept.hpp"

#include <optional>
#include <yggdrasil/core/config.hpp>

namespace tyr::datalog
{

class NoTerminationPolicy
{
public:
    NoTerminationPolicy() = default;

    void set_goals(::tyr::formalism::datalog::GroundConjunctiveConditionView) {}
    bool check(const FactSets&) const noexcept { return false; }
    bool should_terminate(const FactSets&) const noexcept { return false; }
    Cost get_total_cost(const FactSets&, const PredicateAnnotations<>&, const FunctionAnnotations<>&, const NumericSupportSelector&) const noexcept
    {
        return Cost(0);
    }
    void reset() noexcept {}
    void clear() noexcept {}
};

template<typename AggregationFunction>
class TerminationPolicy
{
public:
    TerminationPolicy() = default;

    void set_goals(::tyr::formalism::datalog::GroundConjunctiveConditionView goals_);

    bool check(const FactSets& fact_sets) const noexcept;
    bool should_terminate(const FactSets& fact_sets) const noexcept;

    void set_early_termination(bool value) noexcept { early_termination = value; }

    Cost get_total_cost(const FactSets& fact_sets,
                        const PredicateAnnotations<>& annotations,
                        const FunctionAnnotations<>&,
                        const NumericSupportSelector& numeric_support_selector) const noexcept;

    const auto& get_goal() const noexcept { return goals; }

    void reset() noexcept;

    void clear() noexcept;

private:
    std::optional<::tyr::formalism::datalog::GroundConjunctiveConditionView> goals;
    mutable NumericSupportSelectorWorkspace numeric_support_selector_workspace;
    AggregationFunction agg;
    bool early_termination { true };
};

}

#endif
