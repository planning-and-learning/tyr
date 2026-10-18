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

#ifndef TYR_PLANNING_STATE_REPOSITORY_CONCEPTS_HPP_
#define TYR_PLANNING_STATE_REPOSITORY_CONCEPTS_HPP_

#include "tyr/declarations.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/planning/declarations.hpp"

#include <concepts>
#include <utility>
#include <vector>
#include <yggdrasil/containers/shared_object_pool.hpp>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/core/types.hpp>

namespace tyr::planning
{

template<typename T, typename Kind>
concept StateRepositoryConcept =
    requires(T& r,
             const T& const_r,
             ygg::Index<State<Kind>> index,
             ygg::SharedObjectPoolPtr<ygg::Builder<State<Kind>>, true> state_builder,
             const BuilderStateView<Kind>& borrowed_state,
             const StateView<Kind>& parent_state,
             const StateDelta& state_delta,
             const std::vector<ygg::Data<formalism::planning::FDRFact<formalism::FluentTag>>>& fluent_facts,
             const std::vector<std::pair<ygg::Index<formalism::planning::FunctionTerm<GroundTag, formalism::FluentTag>>, ygg::float_t>>& fterm_values,
             const std::vector<formalism::planning::FDRFactView<formalism::FluentTag>>& fluent_fact_views,
             const std::vector<formalism::planning::FunctionTermViewValuePair<GroundTag, formalism::FluentTag>>& fterm_value_views,
             AxiomEvaluator<Kind>& axiom_evaluator) {
        requires TaskKind<Kind>;
        { r.get_initial_state(axiom_evaluator) } -> std::same_as<StateView<Kind>>;
        { r.get_registered_state(index) } -> std::same_as<StateView<Kind>>;
        { r.create_state(axiom_evaluator, fluent_facts, fterm_values) } -> std::same_as<StateView<Kind>>;
        { r.create_state(axiom_evaluator, fluent_fact_views, fterm_value_views) } -> std::same_as<StateView<Kind>>;
        { r.get_state_builder() } -> std::same_as<ygg::SharedObjectPoolPtr<ygg::Builder<State<Kind>>, true>>;
        { r.register_state(axiom_evaluator, state_builder) } -> std::same_as<StateView<Kind>>;
        { r.register_state(axiom_evaluator, state_builder, parent_state, state_delta) } -> std::same_as<StateView<Kind>>;
        { r.register_extended_state(state_builder) } -> std::same_as<StateView<Kind>>;
        { r.register_extended_state(borrowed_state) } -> std::same_as<StateView<Kind>>;
        { const_r.make_worker() } -> std::same_as<StateRepositoryPtr<Kind>>;
        { const_r.get_task() } -> std::same_as<const TaskPtr<Kind>&>;
        { const_r.shares_storage_with(const_r) } -> std::same_as<bool>;
        { const_r.is_concurrent() } -> std::same_as<bool>;
        { const_r.get_worker_index() } -> std::same_as<ygg::uint_t>;
        { const_r.get_index() } -> std::same_as<ygg::uint_t>;
    };

}

#endif
