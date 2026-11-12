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
#include "tyr/datalog/policies/annotation_types.hpp"
#include "tyr/datalog/policies/termination_concepts.hpp"

#include <optional>
#include <yggdrasil/core/config.hpp>

namespace tyr::datalog
{

class NoTerminationPolicy
{
public:
    NoTerminationPolicy() = default;

    void set_goals(formalism::datalog::ConjunctiveConditionView<GroundTag>) {}
    [[nodiscard]] bool check(const FactSets&) const noexcept { return false; }
    [[nodiscard]] bool should_terminate(const FactSets&) const noexcept { return false; }
    [[nodiscard]] Cost get_total_cost(const FactSets&, const PredicateAnnotations<>&) const noexcept { return static_cast<Cost>(0); }
    void reset() noexcept {}
    void clear() noexcept {}
};

template<typename AggregationFunction>
class TerminationPolicy
{
public:
    TerminationPolicy() = default;

    void set_goals(formalism::datalog::ConjunctiveConditionView<GroundTag> goals_);

    [[nodiscard]] bool check(const FactSets& fact_sets) const;
    [[nodiscard]] bool should_terminate(const FactSets& fact_sets) const;

    void set_early_termination(bool value) noexcept { early_termination = value; }

    [[nodiscard]] Cost get_total_cost(const FactSets& fact_sets, const PredicateAnnotations<>& annotations) const;

    [[nodiscard]] const auto& get_goal() const noexcept { return goals; }

    void reset() noexcept;

    void clear() noexcept;

private:
    std::optional<formalism::datalog::ConjunctiveConditionView<GroundTag>> goals;
    AggregationFunction agg;
    bool early_termination { true };
};

}

#endif
