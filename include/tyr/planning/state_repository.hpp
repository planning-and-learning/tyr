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
#include "tyr/planning/state_repository_concepts.hpp"
#include "tyr/planning/state_view.hpp"
#include "tyr/planning/task.hpp"

#include <atomic>
#include <concepts>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>
#include <yggdrasil/containers/shared_object_pool.hpp>

namespace tyr::planning
{

template<TaskKind Kind>
class StateRepository : public std::enable_shared_from_this<StateRepository<Kind>>
{
    friend class StateRepositoryFactory<Kind>;

private:
    struct Impl;

    StateRepository(ygg::uint_t index, TaskPtr<Kind> task, bool concurrent, std::shared_ptr<std::atomic<ygg::uint_t>> next_index);
    explicit StateRepository(std::unique_ptr<Impl> impl);

public:
    ~StateRepository();

    StateView<Kind> get_initial_state(AxiomEvaluator<Kind>& axiom_evaluator);
    StateView<Kind> get_registered_state(ygg::Index<State<Kind>> state_index);

    StateView<Kind>
    create_state(AxiomEvaluator<Kind>& axiom_evaluator,
                 const std::vector<ygg::Data<formalism::planning::FDRFact<formalism::FluentTag>>>& fluent_facts,
                 const std::vector<std::pair<ygg::Index<formalism::planning::FunctionTerm<GroundTag, formalism::FluentTag>>, ygg::float_t>>& fterm_values);
    StateView<Kind> create_state(AxiomEvaluator<Kind>& axiom_evaluator,
                                 const std::vector<formalism::planning::FDRFactView<formalism::FluentTag>>& fluent_facts,
                                 const std::vector<formalism::planning::FunctionTermViewValuePair<GroundTag, formalism::FluentTag>>& fterm_values);

    ygg::SharedObjectPoolPtr<ygg::Builder<State<Kind>>, true> get_state_builder();

    /// The builder must come from this repository and have no retained mutable aliases.
    StateView<Kind> register_state(AxiomEvaluator<Kind>& axiom_evaluator, ygg::SharedObjectPoolPtr<ygg::Builder<State<Kind>>, true> state);
    /// Like register_state, but `delta` must be the exact changes from `parent` to `state`, as produced when generating a successor.
    /// If both share storage, storage backends may insert `state` as `delta` to `parent`.
    StateView<Kind> register_state(AxiomEvaluator<Kind>& axiom_evaluator,
                                   ygg::SharedObjectPoolPtr<ygg::Builder<State<Kind>>, true> state,
                                   const StateView<Kind>& parent,
                                   const StateDelta& delta);
    /// The builder must come from this repository, already contain its axiom closure, and have no retained mutable aliases.
    StateView<Kind> register_extended_state(ygg::SharedObjectPoolPtr<ygg::Builder<State<Kind>>, true> state);
    /// Copies an already extended state of this task into repository-owned storage; source storage remains borrowed and unchanged.
    StateView<Kind> register_extended_state(const BuilderStateView<Kind>& source);
    [[nodiscard]] StateRepositoryPtr<Kind> make_worker() const;

    /// All repositories sharing this storage must be quiescent while inspecting memory usage.
    [[nodiscard]] size_t memory_usage() const noexcept;

    const TaskPtr<Kind>& get_task() const noexcept;
    bool shares_storage_with(const StateRepository& other) const noexcept;
    [[nodiscard]] bool is_concurrent() const noexcept;
    /// The index of this worker among the repositories sharing its storage.
    [[nodiscard]] ygg::uint_t get_worker_index() const noexcept;
    /// The identity of the storage that owns the states; shared by all workers of a cohort.
    [[nodiscard]] ygg::uint_t get_index() const noexcept;
    [[nodiscard]] size_t num_states() const noexcept;

    /// A repository owns the states it registers.
    friend const StateRepository& get_repository(const StateRepository& repository) noexcept { return repository; }

private:
    std::unique_ptr<Impl> m_impl;
};

extern template class StateRepository<GroundTag>;
extern template class StateRepository<LiftedTag>;

template<TaskKind Kind>
StateView<Kind> materialize_state(const StateView<Kind>& source, StateRepository<Kind>& target, AxiomEvaluator<Kind>& axiom_evaluator)
{
    if (source.get_state_repository()->get_task() != target.get_task())
        throw std::invalid_argument("materialize_state(...): source state belongs to a different task.");
    if (axiom_evaluator.get_task() != target.get_task())
        throw std::invalid_argument("materialize_state(...): axiom evaluator belongs to a different task.");
    if (source.get_state_repository().get() == &target)
        return source;
    if (source.get_state_repository()->shares_storage_with(target))
        return target.get_registered_state(source.get_index());

    auto builder = target.get_state_builder();
    builder->assign_unextended_part(source.get_state_builder());
    return target.register_state(axiom_evaluator, std::move(builder));
}

extern template StateView<GroundTag>
materialize_state(const StateView<GroundTag>& source, StateRepository<GroundTag>& target, AxiomEvaluator<GroundTag>& axiom_evaluator);
extern template StateView<LiftedTag>
materialize_state(const StateView<LiftedTag>& source, StateRepository<LiftedTag>& target, AxiomEvaluator<LiftedTag>& axiom_evaluator);
}

#endif
