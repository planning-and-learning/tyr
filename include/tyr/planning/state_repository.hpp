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
#include "tyr/planning/state_view.hpp"
#include "tyr/planning/task.hpp"

#include <atomic>
#include <concepts>
#include <memory>
#include <span>
#include <utility>
#include <vector>
#include <yggdrasil/containers/shared_object_pool.hpp>

namespace tyr::planning
{

template<TaskKind Kind>
class StateRepository : public std::enable_shared_from_this<StateRepository<Kind>>
{
    friend class StateRepositoryFactory<Kind>;
    friend class SuccessorGenerator<Kind>;
    friend struct ::ygg::View<ygg::Index<State<Kind>>, StateRepositoryPtr<Kind>>;

private:
    struct Impl;

    StateRepository(ygg::uint_t index, TaskPtr<Kind> task, AxiomEvaluatorPtr<Kind> axiom_evaluator, std::shared_ptr<std::atomic<ygg::uint_t>> next_index);
    explicit StateRepository(std::unique_ptr<Impl> impl);
    [[nodiscard]] std::vector<StateRepositoryPtr<Kind>> make_shared_workers(std::span<const ygg::ExecutionContextPtr> execution_contexts) const;
    ygg::uint_t get_storage_identity() const noexcept;
    void compute_extended_state(ygg::Builder<State<Kind>>& state);
    StateView<Kind> register_extended_state(ygg::SharedObjectPoolPtr<ygg::Builder<State<Kind>>, true> state);

public:
    ~StateRepository();

    StateView<Kind> get_initial_state();
    StateView<Kind> get_registered_state(ygg::Index<State<Kind>> state_index);

    StateView<Kind> create_state(
        const std::vector<ygg::Data<::tyr::formalism::planning::FDRFact<::tyr::formalism::FluentTag>>>& fluent_facts,
        const std::vector<std::pair<ygg::Index<::tyr::formalism::planning::GroundFunctionTerm<::tyr::formalism::FluentTag>>, ygg::float_t>>& fterm_values);
    StateView<Kind> create_state(const std::vector<::tyr::formalism::planning::FDRFactView<::tyr::formalism::FluentTag>>& fluent_facts,
                                 const std::vector<::tyr::formalism::planning::GroundFunctionTermViewValuePair<::tyr::formalism::FluentTag>>& fterm_values);

    ygg::SharedObjectPoolPtr<ygg::Builder<State<Kind>>, true> get_state_builder();

    /// The builder must come from this repository and have no retained mutable aliases.
    StateView<Kind> register_state(ygg::SharedObjectPoolPtr<ygg::Builder<State<Kind>>, true> state);
    [[nodiscard]] StateRepositoryPtr<Kind> make_worker(ygg::ExecutionContextPtr execution_context) const;

    /// All repositories sharing this storage must be quiescent while inspecting memory usage.
    size_t memory_usage() const noexcept;

    const TaskPtr<Kind>& get_task() const noexcept;
    const AxiomEvaluatorPtr<Kind>& get_axiom_evaluator() const noexcept;
    ygg::uint_t get_index() const noexcept;
    size_t num_states() const noexcept;

private:
    std::unique_ptr<Impl> m_impl;
};

extern template class StateRepository<GroundTag>;
extern template class StateRepository<LiftedTag>;

template<typename T, typename Kind>
concept StateRepositoryConcept =
    requires(T& r,
             const T& const_r,
             ygg::Index<State<Kind>> index,
             ygg::SharedObjectPoolPtr<ygg::Builder<State<Kind>>, true> state_builder,
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
        { r.get_state_builder() } -> std::same_as<ygg::SharedObjectPoolPtr<ygg::Builder<State<Kind>>, true>>;
        { r.register_state(state_builder) } -> std::same_as<StateView<Kind>>;
        { const_r.make_worker(execution_context) } -> std::same_as<StateRepositoryPtr<Kind>>;
        { r.get_task() } -> std::same_as<const TaskPtr<Kind>&>;
        { r.get_index() } -> std::same_as<ygg::uint_t>;
    };

template<TaskKind Kind>
StateView<Kind> materialize_state(const StateView<Kind>& source, StateRepository<Kind>& target)
{
    if (source.get_state_repository().get() == &target)
        return source;

    auto builder = target.get_state_builder();
    builder->assign_unextended_part(source.get_state_builder());
    return target.register_state(std::move(builder));
}

extern template StateView<GroundTag> materialize_state(const StateView<GroundTag>& source, StateRepository<GroundTag>& target);
extern template StateView<LiftedTag> materialize_state(const StateView<LiftedTag>& source, StateRepository<LiftedTag>& target);
}

#endif
