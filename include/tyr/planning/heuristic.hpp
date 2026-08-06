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

#ifndef TYR_PLANNING_HEURISTIC_HPP_
#define TYR_PLANNING_HEURISTIC_HPP_

#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/planning/declarations.hpp"

#include <cstddef>
#include <yggdrasil/containers/associative_containers.hpp>
#include <yggdrasil/core/config.hpp>

namespace tyr::planning
{

/**
 * A heuristic instance is a mutable evaluator owned by one search worker. Evaluation may reuse scratch storage, update statistics, and replace preferred
 * actions, so evaluate() is intentionally non-const and must not be called concurrently on the same instance. Preferred actions describe the latest
 * evaluation and must not be retained across another evaluation or reconfiguration.
 *
 * Heavy built-in heuristics hide a task-derived definition and a worker-local evaluator behind their private implementation. Definitions are frozen before
 * being shared between workers, while evaluators own all mutable workspaces. This avoids repeating task translation without exposing those implementation
 * details through the search API. set_goal() is configuration and must run before workers are materialized, or after they have been discarded. Custom
 * heuristics need not use the same internal representation, but must return an independently mutable evaluator from make_worker(). Each worker instance is
 * called serially, but may run on different OS threads because remote states execute as work of their logical owner. Evaluation must not re-enter the
 * search or wait for work from the same logical worker.
 */
template<TaskKind Kind>
class Heuristic
{
public:
    virtual ~Heuristic() = default;

    virtual void set_goal(::tyr::formalism::planning::GroundConjunctiveConditionView goal) = 0;

    virtual ygg::float_t evaluate(const StateView<Kind>& state) = 0;

    [[nodiscard]] virtual HeuristicPtr<Kind> make_worker(ygg::ExecutionContextPtr execution_context) const = 0;

    virtual const ygg::UnorderedSet<::tyr::formalism::planning::ActionBindingView>& get_preferred_actions();

    virtual void print_summary([[maybe_unused]] size_t verbosity) const {}
};

}

#endif
