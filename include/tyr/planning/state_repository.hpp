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

#ifndef TYR_PLANNING_STATE_REPOSITORY_HPP_
#define TYR_PLANNING_STATE_REPOSITORY_HPP_

#include "tyr/planning/declarations.hpp"
#include "tyr/planning/state_builder.hpp"
#include "tyr/planning/state_index.hpp"
#include "tyr/planning/state_repository.hpp"
#include "tyr/planning/state_view.hpp"
#include "tyr/planning/task.hpp"

#include <concepts>
#include <yggdrasil/containers/shared_object_pool.hpp>

namespace tyr::planning
{

template<TaskKind Kind>
class StateRepository;

template<typename T, typename Kind>
concept StateRepositoryConcept =
    requires(T& r,
             const T& const_r,
             ygg::Index<State<Kind>> index,
             ygg::SharedObjectPoolPtr<ygg::Builder<State<Kind>>> state_builder,
             const std::vector<ygg::Data<::tyr::formalism::planning::FDRFact<::tyr::formalism::FluentTag>>>& fluent_facts,
             const std::vector<std::pair<ygg::Index<::tyr::formalism::planning::GroundFunctionTerm<::tyr::formalism::FluentTag>>, ygg::float_t>>& fterm_values,
             const std::vector<::tyr::formalism::planning::FDRFactView<::tyr::formalism::FluentTag>>& fluent_fact_views,
             const std::vector<::tyr::formalism::planning::GroundFunctionTermViewValuePair<::tyr::formalism::FluentTag>>& fterm_value_views,
             ygg::ExecutionContextPtr execution_context) {
        requires TaskKind<Kind>;
        { r.get_initial_state() } -> std::same_as<StateView<Kind>>;
        { r.get_registered_state(index) } -> std::same_as<StateView<Kind>>;
        { r.create_state(fluent_facts, fterm_values) } -> std::same_as<StateView<Kind>>;
        { r.create_state(fluent_fact_views, fterm_value_views) } -> std::same_as<StateView<Kind>>;
        { r.get_state_builder() } -> std::same_as<ygg::SharedObjectPoolPtr<ygg::Builder<State<Kind>>>>;
        { r.register_state(state_builder) } -> std::same_as<StateView<Kind>>;
        { const_r.make_worker(execution_context) } -> std::same_as<StateRepositoryPtr<Kind>>;
        { r.get_task() } -> std::same_as<const TaskPtr<Kind>&>;
        { r.get_index() } -> std::same_as<ygg::uint_t>;
    };
}

#endif
