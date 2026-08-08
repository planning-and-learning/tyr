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

#ifndef TYR_PLANNING_AXIOM_EVALUATOR_HPP_
#define TYR_PLANNING_AXIOM_EVALUATOR_HPP_

#include "tyr/planning/declarations.hpp"
#include "tyr/planning/state_builder.hpp"

#include <concepts>
#include <yggdrasil/core/concepts.hpp>
#include <yggdrasil/core/config.hpp>

namespace tyr::planning
{

template<TaskKind Kind>
class AxiomEvaluator;

template<typename T, typename Kind>
concept AxiomEvaluatorConcept = requires(T& r, const T& const_r, ygg::Builder<State<Kind>>& state_builder, ygg::ExecutionContextPtr execution_context) {
    requires TaskKind<Kind>;
    { r.compute_extended_state(state_builder) } -> std::same_as<void>;
    { const_r.make_worker(execution_context) } -> std::same_as<AxiomEvaluatorPtr<Kind>>;
    { const_r.get_task() } -> std::same_as<const TaskPtr<Kind>&>;
    { const_r.get_execution_context() } -> std::same_as<const ygg::ExecutionContextPtr&>;
    { r.get_index() } -> std::same_as<ygg::uint_t>;
};

}

#endif
